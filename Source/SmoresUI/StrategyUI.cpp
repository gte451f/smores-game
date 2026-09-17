// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyUI.h"
#include "NavRailWidget.h"
#include "ResourceStripWidget.h"

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
	// the readout lives in the resource strip; this is only the route the HUD's per-frame push
	// already takes. The strip itself is what decides whether the balance actually moved.
	if (ResourceStrip)
	{
		ResourceStrip->SetGold(NewGold);
	}
}

void UStrategyUI::RefreshNavRail()
{
	if (NavRail)
	{
		NavRail->RefreshPanelStates();
	}
}
