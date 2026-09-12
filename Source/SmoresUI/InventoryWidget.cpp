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

FIntPoint UInventoryWidget::GetGridSize() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetGridSize() : FIntPoint::ZeroValue;
}

TArray<FInventoryEntry> UInventoryWidget::GetEntries() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetEntries() : TArray<FInventoryEntry>();
}

FText UInventoryWidget::GetContentsSummary() const
{
	if (!BoundInventory.IsValid())
	{
		return FText::GetEmpty();
	}

	const TArray<FInventoryEntry>& Entries = BoundInventory->GetEntries();
	const FIntPoint GridSize = BoundInventory->GetGridSize();

	TArray<FString> Lines;
	Lines.Reserve(Entries.Num() + 1);

	Lines.Add(FString::Printf(TEXT("%dx%d grid, %d/%d cells free"),
		GridSize.X, GridSize.Y, BoundInventory->GetFreeCellCount(), GridSize.X * GridSize.Y));

	for (const FInventoryEntry& Entry : Entries)
	{
		// display data lives on the shared item definition, not on the carried instance
		const FIntPoint Footprint = Entry.GetFootprint();

		Lines.Add(FString::Printf(TEXT("- %s @ (%d,%d) %dx%d%s"),
			*UInventoryWidget::GetItemLabel(Entry.Item).ToString(),
			Entry.AnchorCell.X, Entry.AnchorCell.Y,
			Footprint.X, Footprint.Y,
			Entry.bRotated ? TEXT(" rotated") : TEXT("")));
	}

	if (Entries.IsEmpty())
	{
		Lines.Add(LOCTEXT("EmptyInventory", "(empty)").ToString());
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UInventoryWidget::GetItemLabel(const FInventoryItem& Item)
{
	if (Item.IsEmpty())
	{
		return FText::GetEmpty();
	}

	// suffix the count only for an actual stack - a lone item reads as just its name
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
		SlotListText->SetText(GetContentsSummary());
	}

	if (SlotContainer && SlotWidgetClass)
	{
		SlotContainer->ClearChildren();
		SlotWidgets.Reset();

		const FIntPoint GridSize = GetGridSize();
		const TArray<FInventoryEntry> CurrentEntries = GetEntries();

		// one lookup per cell over a handful of entries is cheaper than building a map for a grid
		// this small, and keeps the covering entry (not just the anchor) attached to every cell
		auto FindEntryCovering = [&CurrentEntries](FIntPoint Cell) -> FInventoryEntry
		{
			for (const FInventoryEntry& Entry : CurrentEntries)
			{
				if (Entry.CoversCell(Cell))
				{
					return Entry;
				}
			}

			return FInventoryEntry();
		};

		UUniformGridPanel* GridPanel = Cast<UUniformGridPanel>(SlotContainer.Get());

		SlotWidgets.Reserve(GridSize.X * GridSize.Y);

		for (int32 Row = 0; Row < GridSize.Y; ++Row)
		{
			for (int32 Column = 0; Column < GridSize.X; ++Column)
			{
				UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);

				if (!SlotWidget)
				{
					continue;
				}

				const FIntPoint Cell(Column, Row);
				SlotWidget->SetCell(BoundInventory.Get(), Cell, FindEntryCovering(Cell));

				if (GridPanel)
				{
					// UUniformGridSlot defaults to HAlign_Left/VAlign_Top, so an empty cell widget
					// shrink-wraps to its own content and sticks in the cell's top-left corner, leaving
					// most of the (much larger) uniform cell empty - stretch it to fill the cell instead.
					if (UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(SlotWidget, Row, Column))
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
