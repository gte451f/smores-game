// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "InventoryWidget"

void UInventoryWidget::SetInventory(UInventoryComponent* InInventory)
{
	// drop any previous binding
	ClearInventory();

	BoundInventory = InInventory;

	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	RefreshDisplay();
}

void UInventoryWidget::ClearInventory()
{
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	BoundInventory.Reset();
}

int32 UInventoryWidget::GetNumSlots() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetNumSlots() : 0;
}

TArray<FInventoryItem> UInventoryWidget::GetItems() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetItems() : TArray<FInventoryItem>();
}

FText UInventoryWidget::GetSlotSummary() const
{
	if (!BoundInventory.IsValid())
	{
		return FText::GetEmpty();
	}

	const TArray<FInventoryItem>& Items = BoundInventory->GetItems();
	const int32 SlotCount = BoundInventory->GetNumSlots();

	TArray<FString> Lines;
	Lines.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const FText SlotContents = Items.IsValidIndex(SlotIndex)
			? Items[SlotIndex].DisplayName
			: LOCTEXT("EmptySlot", "(empty)");

		Lines.Add(FString::Printf(TEXT("%d. %s"), SlotIndex + 1, *SlotContents.ToString()));
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void UInventoryWidget::HandleInventoryChanged()
{
	RefreshDisplay();
}

void UInventoryWidget::RefreshDisplay()
{
	if (SlotListText)
	{
		SlotListText->SetText(GetSlotSummary());
	}

	BP_InventoryUpdated();
}

void UInventoryWidget::NativeDestruct()
{
	ClearInventory();

	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
