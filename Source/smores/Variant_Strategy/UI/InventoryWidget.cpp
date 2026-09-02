// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"

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
		const FText SlotContents = (Items.IsValidIndex(SlotIndex) && !Items[SlotIndex].IsEmpty())
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

	if (SlotContainer && SlotWidgetClass)
	{
		SlotContainer->ClearChildren();
		SlotWidgets.Reset();

		const int32 SlotCount = GetNumSlots();
		const TArray<FInventoryItem> CurrentItems = GetItems();
		UUniformGridPanel* GridPanel = Cast<UUniformGridPanel>(SlotContainer.Get());
		const int32 SafeGridColumns = FMath::Max(GridColumns, 1);

		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);

			if (!SlotWidget)
			{
				continue;
			}

			const FInventoryItem SlotItem = CurrentItems.IsValidIndex(Index) ? CurrentItems[Index] : FInventoryItem();
			SlotWidget->SetSlot(Index, SlotItem);

			if (GridPanel)
			{
				GridPanel->AddChildToUniformGrid(SlotWidget, Index / SafeGridColumns, Index % SafeGridColumns);
			}
			else
			{
				SlotContainer->AddChild(SlotWidget);
			}

			SlotWidgets.Add(SlotWidget);
		}
	}

	BP_InventoryUpdated();
}

void UInventoryWidget::NativeDestruct()
{
	ClearInventory();

	Super::NativeDestruct();
}

void UInventoryWidget::RequestClose_Implementation()
{
	ClearInventory();

	if (IsInViewport())
	{
		RemoveFromParent();
	}
}

#undef LOCTEXT_NAMESPACE
