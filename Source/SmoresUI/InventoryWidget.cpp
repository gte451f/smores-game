// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

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
		// display data lives on the shared item definition, not on the carried instance
		const FText SlotContents = (Items.IsValidIndex(SlotIndex) && !Items[SlotIndex].IsEmpty())
			? UInventoryWidget::GetItemLabel(Items[SlotIndex])
			: LOCTEXT("EmptySlot", "(empty)");

		Lines.Add(FString::Printf(TEXT("%d. %s"), SlotIndex + 1, *SlotContents.ToString()));
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UInventoryWidget::GetItemLabel(const FInventoryItem& Item)
{
	if (Item.IsEmpty())
	{
		return FText::GetEmpty();
	}

	// suffix the count only once stacking actually produces one - a lone item reads as just its name
	if (Item.Quantity > 1)
	{
		return FText::Format(LOCTEXT("StackedItemLabel", "{0} x{1}"), Item.GetDisplayName(), FText::AsNumber(Item.Quantity));
	}

	return Item.GetDisplayName();
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
			SlotWidget->SetSlot(BoundInventory.Get(), Index, SlotItem);

			if (GridPanel)
			{
				// UUniformGridSlot defaults to HAlign_Left/VAlign_Top, so an unfilled slot widget
				// shrink-wraps to its own content and sticks in the cell's top-left corner, leaving
				// most of the (much larger) uniform cell empty - stretch it to fill the cell instead.
				if (UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(SlotWidget, Index / SafeGridColumns, Index % SafeGridColumns))
				{
					GridSlot->SetHorizontalAlignment(HAlign_Fill);
					GridSlot->SetVerticalAlignment(VAlign_Fill);
				}
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
