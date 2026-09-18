// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "SquadBarWidget.generated.h"

class AStrategyUnit;
class UPanelWidget;
class USquadPortraitWidget;
class UTextBlock;

/**
 *  The bottom-left squad bar: one portrait per member of the player's own squad, plus the
 *  "N selected" readout that used to sit loose on the HUD canvas.
 *
 *  **It subsumes that readout rather than sitting next to it.** The count was a bare UBorder
 *  dropped on HUDCanvas, which meant it leaked right-clicks straight through to the world - the
 *  exact bug hud-and-panels.md's "anything that reads as a panel must be a UHUDRegionWidget" rule
 *  exists for. Folding it into this region fixes that by construction, and the count belongs next
 *  to the portraits anyway: it is a fact about the same thing they draw.
 *
 *  Order is whatever the controller's roster gives, which is the same deterministic order the Tab
 *  cycle walks. That matters more than it sounds: a bar that re-sorted itself as units moved or
 *  died would move the portrait out from under a player's finger mid-click.
 *
 *  Like every region, it is rebuilt from a push rather than from a subscription (see
 *  AStrategyHUD::DrawHUD), and like every region it compares what it was handed against what it
 *  is already drawing before touching Slate.
 */
UCLASS(abstract)
class SMORESUI_API USquadBarWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** Holds the portraits, in roster order. Name it "PortraitBox" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> PortraitBox;

	/** The "3 selected" line. Name it "SelectionCountText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectionCountText;

	/** Widget class used for each portrait */
	UPROPERTY(EditAnywhere, Category = "Squad Bar")
	TSubclassOf<USquadPortraitWidget> PortraitWidgetClass;

	/** The portraits currently in PortraitBox, in roster order. Resized rather than rebuilt - a
	 *  squad changes size far less often than it changes state. */
	UPROPERTY()
	TArray<TObjectPtr<USquadPortraitWidget>> PortraitWidgets;

	/** The roster as last pushed */
	TArray<TWeakObjectPtr<AStrategyUnit>> Roster;

	/** How many units are selected, for the readout */
	int32 SelectedCount = 0;

public:

	/**
	 *  Sets the squad and which of them are selected. Pushed every frame.
	 *
	 *  Selection is passed as the selected units rather than as a count plus a lookup, because a
	 *  portrait needs to know whether *it* is selected, and a count can't answer that.
	 */
	void SetSquad(const TArray<AStrategyUnit*>& InRoster, const TArray<AStrategyUnit*>& SelectedUnits);

	/** Blueprint handler for anything beyond the bound portraits and count */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Squad Bar"))
	void BP_UpdateSquadBar();

protected:

	/** The selection readout's line, e.g. "2 selected" */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetSelectionCountLine() const;

	/** Runs a portrait's click. Straight to the controller, which owns what selecting means. */
	void HandlePortraitClicked(AStrategyUnit* Unit, bool bFocusCamera);

	/** Grows or shrinks PortraitWidgets to match the roster size */
	void ResizePortraitRow(int32 UnitCount);

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
