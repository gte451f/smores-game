// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventorySlotWidget.h"
#include "InventoryDragDropOperation.h"
#include "Components/TextBlock.h"

void UInventorySlotWidget::SetSlot(UInventoryComponent* InOwningInventory, int32 InSlotIndex, const FInventoryItem& InItem)
{
	OwningInventory = InOwningInventory;
	SlotIndex = InSlotIndex;
	Item = InItem;

	if (SlotText)
	{
		SlotText->SetText(IsSlotEmpty() ? FText::GetEmpty() : Item.DisplayName);
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// only a non-empty slot has anything to drag; an empty slot's mouse-down falls through
	// to the enclosing window (which swallows it so it can't reach world/selection input)
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsSlotEmpty())
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		BP_SlotClicked();

		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UInventoryDragDropOperation* DragOperation = NewObject<UInventoryDragDropOperation>(this);
	DragOperation->SourceInventory = OwningInventory;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->Pivot = EDragPivot::MouseDown;

	// floating drag visual: another instance of this same slot's class, showing the same item -
	// reuses whatever "text in a box" look the WBP already gives a slot, no new content asset needed
	if (UInventorySlotWidget* DragVisual = CreateWidget<UInventorySlotWidget>(this, GetClass()))
	{
		DragVisual->SetSlot(OwningInventory.Get(), SlotIndex, Item);
		DragOperation->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOperation;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	if (!DragOperation || !OwningInventory.IsValid())
	{
		return false;
	}

	return UInventoryComponent::MoveItem(DragOperation->SourceInventory.Get(), DragOperation->SourceSlotIndex, OwningInventory.Get(), SlotIndex);
}
