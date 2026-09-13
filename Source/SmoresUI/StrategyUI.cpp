// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyUI.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "StrategyUI"

void UStrategyUI::SetSelectedUnitsCount(int32 Count)
{
	// is this a different count?
	bool bChanged = SelectedUnitCount != Count;

	// update the counter
	SelectedUnitCount = Count;

	// if the count changed, call the BP handler
	if (bChanged)
	{
		BP_UpdateUnitsCount();
	}
}

void UStrategyUI::SetSelectionTargetLabel(const FText& Label)
{
	SelectionTargetLabel = Label;
}

void UStrategyUI::SetGold(int32 NewGold)
{
	// the HUD pushes this every frame, so only touch the readout when the balance actually moved
	if (Gold == NewGold)
	{
		return;
	}

	Gold = NewGold;

	RefreshGoldDisplay();
}

FText UStrategyUI::GetGoldLabel() const
{
	return FText::Format(LOCTEXT("GoldReadout", "Gold: {0}"), FText::AsNumber(Gold));
}

void UStrategyUI::NativeConstruct()
{
	Super::NativeConstruct();

	// the first balance push may well have happened before this widget existed
	RefreshGoldDisplay();
}

void UStrategyUI::RefreshGoldDisplay()
{
	if (GoldText)
	{
		GoldText->SetText(GetGoldLabel());
	}

	BP_UpdateGold();
}

#undef LOCTEXT_NAMESPACE
