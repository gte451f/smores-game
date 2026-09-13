// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "EquipmentSlotWidget.h"
#include "InventoryDragDropOperation.h"
#include "InventoryMoveHost.h"
#include "InventoryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UEquipmentSlotWidget::SetSlot(UEquipmentComponent* InEquipment, EEquipSlot InSlot)
{
	BoundEquipment = InEquipment;
	EquipSlot = InSlot;

	// this widget is a drop target and a right-click target, so it has to be hit-testable
	// regardless of what the WBP left its root visibility at
	SetVisibility(ESlateVisibility::Visible);

	RefreshVisuals();
}

FInventoryItem UEquipmentSlotWidget::GetEquippedItem() const
{
	return BoundEquipment.IsValid() ? BoundEquipment->GetEquippedItem(EquipSlot) : FInventoryItem();
}

void UEquipmentSlotWidget::RefreshVisuals()
{
	const FInventoryItem Worn = GetEquippedItem();

	if (SlotLabel)
	{
		// the enum's own display name, so there's no second list of slot names to keep in step
		SlotLabel->SetText(UEquipmentComponent::GetSlotDisplayName(EquipSlot));
	}

	if (ItemLabel)
	{
		// the same labelling the grid uses, so a worn item reads identically to a carried one
		ItemLabel->SetText(UInventoryWidget::GetItemLabel(Worn));
	}

	if (SlotBorder)
	{
		FLinearColor Tint = Worn.IsEmpty() ? EmptySlotColor : FilledSlotColor;

		switch (Highlight)
		{
		case EInventoryCellHighlight::Valid:
			Tint = ValidHighlightColor;
			break;

		case EInventoryCellHighlight::Invalid:
			Tint = InvalidHighlightColor;
			break;

		default:
			break;
		}

		SlotBorder->SetBrushColor(Tint);
	}

	BP_SlotUpdated();
}

void UEquipmentSlotWidget::SetHighlight(EInventoryCellHighlight InHighlight)
{
	if (Highlight == InHighlight)
	{
		return;
	}

	Highlight = InHighlight;

	RefreshVisuals();
}

bool UEquipmentSlotWidget::WouldAcceptDrop(const UInventoryDragDropOperation* DragOperation) const
{
	if (!DragOperation || !BoundEquipment.IsValid() || !DragOperation->SourceInventory.IsValid())
	{
		return false;
	}

	// slot-type matching is the whole gate - training never blocks an equip
	if (!BoundEquipment->CanEquipItem(DragOperation->DraggedItem, EquipSlot))
	{
		return false;
	}

	const FInventoryItem Displaced = BoundEquipment->GetEquippedItem(EquipSlot);

	if (Displaced.IsEmpty())
	{
		return true;
	}

	// a swap only lands if the item it displaces has somewhere to go. Mirrors the same test
	// UEquipmentComponent::Equip runs server-side, including which entry's cells may be reused,
	// so the highlight can't promise a swap the server will reject
	FIntPoint DisplacedCell;
	bool bDisplacedRotated = false;

	const int32 IgnoreEntryId = (DragOperation->DraggedItem.Quantity <= 1) ? DragOperation->SourceEntryId : INDEX_NONE;

	return DragOperation->SourceInventory->FindFreePlacement(Displaced, DisplacedCell, bDisplacedRotated, IgnoreEntryId);
}

void UEquipmentSlotWidget::TryUnequip()
{
	if (!BoundEquipment.IsValid() || !BoundEquipment->IsSlotOccupied(EquipSlot))
	{
		return;
	}

	// a worn item comes off into its own pawn's pack, never into whatever window happens to be
	// open next to it
	UInventoryComponent* DestInventory = BoundEquipment->GetOwnerInventory();

	if (!DestInventory)
	{
		return;
	}

	// equipment is shared world state, so this can't be applied locally - the same
	// client->server hop every inventory mutation takes. A rejected unequip (no room in the
	// grid) changes nothing, and the panel redraws unchanged replicated state.
	if (IInventoryMoveHost* MoveHost = Cast<IInventoryMoveHost>(GetOwningPlayer()))
	{
		MoveHost->Server_UnequipItem(BoundEquipment.Get(), EquipSlot, DestInventory);
	}
}

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshVisuals();
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// right-click takes it off, the mirror of right-click-to-equip in the grid. Handled on press
	// rather than release, matching the one-shot convention the keybinds use.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TryUnequip();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEquipmentSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// see UWindowWidget::NativeOnMouseButtonDoubleClick - a fast second click is a different Slate
	// event, and an unclaimed one reaches the world
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UEquipmentSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (const UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation))
	{
		SetHighlight(WouldAcceptDrop(DragOperation) ? EInventoryCellHighlight::Valid : EInventoryCellHighlight::Invalid);
	}
}

void UEquipmentSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	SetHighlight(EInventoryCellHighlight::None);
}

bool UEquipmentSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (const UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation))
	{
		// re-tested every frame rather than cached from drag-enter, since the rotate key can
		// change nothing here but the source grid's free space can change under a long drag
		SetHighlight(WouldAcceptDrop(DragOperation) ? EInventoryCellHighlight::Valid : EInventoryCellHighlight::Invalid);

		return true;
	}

	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	SetHighlight(EInventoryCellHighlight::None);

	if (!DragOperation || !BoundEquipment.IsValid() || !DragOperation->SourceInventory.IsValid())
	{
		return false;
	}

	if (IInventoryMoveHost* MoveHost = Cast<IInventoryMoveHost>(GetOwningPlayer()))
	{
		// this slot names itself rather than passing None: a drop is a deliberate placement, and
		// the server still rejects it if the item doesn't belong here
		MoveHost->Server_EquipItem(DragOperation->SourceInventory.Get(), DragOperation->SourceEntryId, BoundEquipment.Get(), EquipSlot);

		return true;
	}

	return false;
}
