// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventorySlotWidget.h"
#include "InventoryDragDropOperation.h"
#include "Components/TextBlock.h"
#include "InventoryMoveHost.h"
#include "InventoryWidget.h"

void UInventorySlotWidget::SetCell(UInventoryComponent* InOwningInventory, FIntPoint InCellCoord, const FInventoryEntry& InEntry)
{
	OwningInventory = InOwningInventory;
	CellCoord = InCellCoord;
	Entry = InEntry;

	if (SlotText)
	{
		// only the anchor cell labels the item, so a multi-cell footprint doesn't repeat its name
		// once per cell; name/quantity come from the shared definition, not the carried instance
		SlotText->SetText(IsAnchorCell() ? UInventoryWidget::GetItemLabel(Entry.Item) : FText::GetEmpty());
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// only a cell holding something has anything to drag; an empty cell's mouse-down falls through
	// to the enclosing window (which swallows it so it can't reach world/selection input)
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsCellEmpty())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		BP_CellClicked();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (IsCellEmpty())
	{
		return;
	}

	UInventoryDragDropOperation* DragOperation = NewObject<UInventoryDragDropOperation>(this);
	DragOperation->SourceInventory = OwningInventory;
	DragOperation->SourceEntryId = Entry.EntryId;

	// the item keeps whatever orientation it's already placed at unless the player rotates it mid-drag
	DragOperation->bRotated = Entry.bRotated;
	DragOperation->Pivot = EDragPivot::MouseDown;

	// floating drag visual: another instance of this same cell's class, showing the same item -
	// reuses whatever "text in a box" look the WBP already gives a cell, no new content asset needed
	if (UInventorySlotWidget* DragVisual = CreateWidget<UInventorySlotWidget>(this, GetClass()))
	{
		// force the visual onto the entry's anchor so it labels itself regardless of which cell was grabbed
		DragVisual->SetCell(OwningInventory.Get(), Entry.AnchorCell, Entry);
		DragOperation->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOperation;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	if (!DragOperation || !OwningInventory.IsValid() || !DragOperation->SourceInventory.IsValid())
	{
		return false;
	}

	// the actual move is shared-world state, owned by the server - this widget can't mutate
	// inventory contents directly (UInventoryComponent's mutators are authority-only), so dispatch
	// through the owning PlayerController instead. A rejected move simply changes nothing, and the
	// replicated state the UI redraws from is unchanged, so the item visually snaps back.
	if (IInventoryMoveHost* MoveHost = Cast<IInventoryMoveHost>(GetOwningPlayer()))
	{
		// quantity 0 means "the whole stack" - partial-stack drags are a later slice
		MoveHost->Server_MoveInventoryItem(DragOperation->SourceInventory.Get(), DragOperation->SourceEntryId, OwningInventory.Get(), CellCoord, DragOperation->bRotated, 0);

		return true;
	}

	return false;
}
