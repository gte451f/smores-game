// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyUI.h"

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
