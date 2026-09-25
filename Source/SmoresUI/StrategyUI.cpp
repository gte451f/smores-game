// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyUI.h"
#include "NavRailWidget.h"
#include "ResourceStripWidget.h"
#include "TimePaceWidget.h"
#include "TargetPanelWidget.h"
#include "SquadBarWidget.h"
#include "ActivityFeedWidget.h"
#include "BarkBubbleLayerWidget.h"
#include "StrategyTargetInfo.h"

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

void UStrategyUI::SetSquad(const TArray<AStrategyUnit*>& Roster, const TArray<AStrategyUnit*>& SelectedUnits)
{
	// the bar owns the portraits and the "N selected" readout alike - the readout used to sit
	// loose on this canvas, where it leaked right-clicks to the world. See USquadBarWidget.
	if (SquadBarRegion)
	{
		SquadBarRegion->SetSquad(Roster, SelectedUnits);
	}
}

void UStrategyUI::SetTargetInfo(const FStrategyTargetInfo& TargetInfo)
{
	// the panel owns the display, and decides for itself whether anything visible actually moved
	if (TargetPanelRegion)
	{
		TargetPanelRegion->SetTargetInfo(TargetInfo);
	}
}

void UStrategyUI::SetPace(EGamePace Pace)
{
	if (TimePaceRegion)
	{
		TimePaceRegion->SetPace(Pace);
	}
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

void UStrategyUI::RefreshActivityFeed()
{
	if (ActivityFeedRegion)
	{
		ActivityFeedRegion->RefreshFeed();
	}
}

void UStrategyUI::ToggleActivityFeed()
{
	if (ActivityFeedRegion)
	{
		ActivityFeedRegion->ToggleExpanded();
	}
}

void UStrategyUI::ShowBarkBubble(const AActor* Speaker, const FText& Line)
{
	if (BarkBubbleLayer)
	{
		BarkBubbleLayer->ShowBark(Speaker, Line);
	}
}

void UStrategyUI::RefreshBarkBubbles()
{
	if (BarkBubbleLayer)
	{
		BarkBubbleLayer->RefreshBubbles();
	}
}
