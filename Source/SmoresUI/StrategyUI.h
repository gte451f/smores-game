// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePace.h"
#include "StrategyUI.generated.h"

class UNavRailWidget;
class UResourceStripWidget;
class UTimePaceWidget;
class UTargetPanelWidget;
class USquadBarWidget;
class UActivityFeedWidget;
class UBarkBubbleLayerWidget;
class AStrategyUnit;
struct FStrategyTargetInfo;

/**
 *  The always-on HUD root, spawned by AStrategyHUD::BeginPlay at Z-order 0 (below every floating
 *  window - see the comment block in RefusalWidget.h for why that ordering matters).
 *
 *  This is the **root that hosts the HUD's regions**, not a peer of them: its WBP is a
 *  full-screen canvas with an anchored slot per region of the wireframe, and each region is its
 *  own widget class bound by name here. Readouts belong in a region, not on this class - gold
 *  moved out to UResourceStripWidget for exactly that reason. What stays here is the plumbing
 *  that AStrategyHUD::DrawHUD pushes into every frame, forwarded on to whichever region wants it.
 *
 *  Every region is optional. A WBP that doesn't provide one simply doesn't show it, which is what
 *  let the six regions arrive a few at a time without the HUD breaking in between - and is still
 *  what lets a seventh be added without touching the six.
 */
UCLASS(abstract)
class SMORESUI_API UStrategyUI : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Number of units currently selected */
	int32 SelectedUnitCount = 0;

	/** The left-hand panel rail. Name it "NavRail" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UNavRailWidget> NavRail;

	/** The top-right at-a-glance figures (gold today). Name it "ResourceStrip" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UResourceStripWidget> ResourceStrip;

	/** The top-centre simulation-speed strip. Name it "TimePaceRegion" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTimePaceWidget> TimePaceRegion;

	/** The top-right "what did I just click" panel. Name it "TargetPanelRegion" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTargetPanelWidget> TargetPanelRegion;

	/** The bottom-left squad portraits and selection count. Name it "SquadBarRegion" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USquadBarWidget> SquadBarRegion;

	/** The bottom-right record of what just happened. Name it "ActivityFeedRegion" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UActivityFeedWidget> ActivityFeedRegion;

	/**
	 *  Barks floating over whoever said them. Not a region: a full-screen, click-through layer that
	 *  sits beneath the regions in the WBP (so first in its canvas). Name it "BarkBubbleLayer" to
	 *  auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBarkBubbleLayerWidget> BarkBubbleLayer;

public:

	/** Sets the number of units selected */
	void SetSelectedUnitsCount(int32 Count);

	/** Sets the player's squad and which of them are selected. Forwarded to the squad bar. */
	void SetSquad(const TArray<AStrategyUnit*>& Roster, const TArray<AStrategyUnit*>& SelectedUnits);

	/** Sets everything the target panel draws. Forwarded to that region, which owns the display. */
	void SetTargetInfo(const FStrategyTargetInfo& TargetInfo);

	/** Sets the simulation's current speed. Forwarded to the pace strip, which owns the readout. */
	void SetPace(EGamePace Pace);

	/** Sets the owning player's gold balance. Forwarded to the resource strip, which owns the readout. */
	void SetGold(int32 NewGold);

	/** Repaints the nav rail's "this panel is open" state. Pushed every frame by the HUD. */
	void RefreshNavRail();

	/** Re-fades the activity feed and rebuilds it if anything was posted. Pushed every frame by the HUD. */
	void RefreshActivityFeed();

	/** Expands or collapses the activity feed. The `L` key's route in, via AStrategyHUD. */
	void ToggleActivityFeed();

	/** Floats Line over Speaker, replacing any bark they are already showing. Forwarded to the bubble layer. */
	void ShowBarkBubble(const AActor* Speaker, const FText& Line);

	/** Moves, fades and retires the bark bubbles. Pushed every frame by the HUD. */
	void RefreshBarkBubbles();

	/** Blueprint handler to update unit count sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta = (DisplayName="Update Units Count"))
	void BP_UpdateUnitsCount();

protected:

	/** Returns the number of units selected */
	UFUNCTION(BlueprintPure, Category="UI")
	int32 GetSelectedUnitsCount() { return SelectedUnitCount; }
};
