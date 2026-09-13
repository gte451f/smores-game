// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "InventoryItemWidget.h"
#include "InventoryDragDropOperation.h"
#include "InventoryMoveHost.h"
#include "InventoryWidget.h"
#include "EquipmentComponent.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void UInventoryItemWidget::SetEntry(UInventoryComponent* InOwningInventory, const FInventoryEntry& InEntry)
{
	OwningInventory = InOwningInventory;
	Entry = InEntry;
	bDrawRotated = InEntry.bRotated;

	// a grid instance is stretched to its footprint by the grid slot's row/column spans, so any
	// size the WBP happens to author would fight that - only a decorator wants an explicit size
	if (ItemSizeBox)
	{
		ItemSizeBox->ClearWidthOverride();
		ItemSizeBox->ClearHeightOverride();
	}

	// this widget is the drag source, so it has to be hit-testable regardless of what the WBP
	// left its root visibility at
	SetVisibility(ESlateVisibility::Visible);

	RefreshVisuals();
}

void UInventoryItemWidget::SetPreviewOrientation(bool bInRotated, const FVector2D& CellSize)
{
	bDrawRotated = bInRotated;

	// a decorator floats free of any panel, so nothing stretches it to the footprint - it has
	// to size itself, or the ghost won't cover the cells the drop is about to claim
	if (ItemSizeBox)
	{
		const FIntPoint Footprint = GetDrawnFootprint();

		ItemSizeBox->SetWidthOverride(CellSize.X * FMath::Max(1, Footprint.X));
		ItemSizeBox->SetHeightOverride(CellSize.Y * FMath::Max(1, Footprint.Y));
	}

	RefreshVisuals();
}

void UInventoryItemWidget::RefreshVisuals()
{
	if (ItemLabel)
	{
		// name/quantity come from the shared definition, not the carried instance
		ItemLabel->SetText(UInventoryWidget::GetItemLabel(Entry.Item));

		// with no item icons authored, the label *is* the item, and a horizontal name in a
		// one-cell-wide column is unreadable - turn it down the footprint's long axis instead.
		// Keyed off the drawn shape rather than the rotation flag, so a 1x3 sword reads
		// vertically whichever orientation produced it and a 2x2 rope is left alone
		const FIntPoint Footprint = GetDrawnFootprint();

		ItemLabel->SetRenderTransformAngle(Footprint.Y > Footprint.X ? 90.0f : 0.0f);
	}

	BP_ItemUpdated();
}

void UInventoryItemWidget::TryEquip()
{
	// only a window opened against a pawn carries an equipment target, so right-clicking an item
	// in a chest or a Downed NPC's loot panel does nothing rather than dressing the holder
	const UInventoryWidget* OwnerWidget = GetTypedOuter<UInventoryWidget>();

	if (!OwnerWidget || !OwningInventory.IsValid() || !Entry.IsValidEntry())
	{
		return;
	}

	UEquipmentComponent* Equipment = OwnerWidget->GetEquipmentTarget();

	// no point sending an RPC the server will only reject - the item's own slot is the gate, and
	// the client can read it off the same shared definition
	if (!Equipment || UEquipmentComponent::GetSlotForItem(Entry.Item) == EEquipSlot::None)
	{
		return;
	}

	if (IInventoryMoveHost* MoveHost = Cast<IInventoryMoveHost>(GetOwningPlayer()))
	{
		// EEquipSlot::None means "whichever slot this belongs in" - a right-click never names one
		MoveHost->Server_EquipItem(OwningInventory.Get(), Entry.EntryId, Equipment, EEquipSlot::None);
	}
}

FReply UInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Entry.IsValidEntry())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	// right-click wears it. Handled on press rather than release, matching the one-shot
	// convention the keybinds use - and handled here rather than left to bubble up, since the
	// window above only swallows what nothing inside it claimed.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TryEquip();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventoryItemWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// a fast second click arrives as a double-click event, not a press (see
	// UWindowWidget::NativeOnMouseButtonDoubleClick) - so it has to be claimed here too, or it
	// reaches the world. Treated as a press, which makes a double right-click simply equip twice.
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventoryItemWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		BP_ItemClicked();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!Entry.IsValidEntry() || !OwningInventory.IsValid())
	{
		return;
	}

	const FIntPoint Footprint = GetDrawnFootprint();
	const FVector2D WidgetSize = InGeometry.GetLocalSize();

	// this widget spans exactly its footprint, so its own arranged size divided by that
	// footprint *is* the grid's cell size - no hardcoded pixel constant, and it stays correct
	// when the player resizes the window
	const FVector2D CellSize(
		Footprint.X > 0 ? WidgetSize.X / Footprint.X : WidgetSize.X,
		Footprint.Y > 0 ? WidgetSize.Y / Footprint.Y : WidgetSize.Y);

	// which cell of the footprint the pointer came down on. Carrying this means a 1x3 sword
	// grabbed by its tip lands with its tip where the player dropped it, instead of jumping so
	// its top-left corner sits under the cursor
	const FVector2D LocalGrab = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

	const FIntPoint GrabOffset(
		FMath::Clamp(CellSize.X > 0.0f ? FMath::FloorToInt32(LocalGrab.X / CellSize.X) : 0, 0, FMath::Max(0, Footprint.X - 1)),
		FMath::Clamp(CellSize.Y > 0.0f ? FMath::FloorToInt32(LocalGrab.Y / CellSize.Y) : 0, 0, FMath::Max(0, Footprint.Y - 1)));

	UInventoryDragDropOperation* DragOperation = NewObject<UInventoryDragDropOperation>(this);
	DragOperation->SourceInventory = OwningInventory;
	DragOperation->SourceEntryId = Entry.EntryId;
	DragOperation->DraggedItem = Entry.Item;
	DragOperation->GrabOffset = GrabOffset;
	DragOperation->CellSize = CellSize;

	// the item keeps whatever orientation it's already placed at unless the player rotates it
	DragOperation->bRotated = Entry.bRotated;

	// TopLeft pivot plus a normalised Offset is what keeps the grabbed cell under the cursor.
	// The alternative (EDragPivot::MouseDown) freezes the pointer-to-decorator offset at drag
	// start, so a mid-drag rotate would leave the ghost and the drop disagreeing about where
	// the item lands - see UInventoryDragDropOperation::ApplyPreview
	DragOperation->Pivot = EDragPivot::TopLeft;

	// floating drag visual: another instance of this same class showing the same item, so it
	// reuses whatever look the WBP already gives an item - no second content asset needed
	if (UInventoryItemWidget* DragVisual = CreateWidget<UInventoryItemWidget>(this, GetClass()))
	{
		DragVisual->SetEntry(OwningInventory.Get(), Entry);

		DragOperation->DefaultDragVisual = DragVisual;
	}

	DragOperation->ApplyPreview();

	OutOperation = DragOperation;
}
