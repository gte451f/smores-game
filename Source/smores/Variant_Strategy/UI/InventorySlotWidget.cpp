// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventorySlotWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "InventorySlotWidget"

void UInventorySlotWidget::SetSlot(int32 InSlotIndex, const FInventoryItem& InItem)
{
	SlotIndex = InSlotIndex;
	Item = InItem;

	if (SlotText)
	{
		SlotText->SetText(IsSlotEmpty() ? LOCTEXT("EmptySlot", "(empty)") : Item.DisplayName);
	}
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

#undef LOCTEXT_NAMESPACE
