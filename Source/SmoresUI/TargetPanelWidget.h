// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "StrategyTargetInfo.h"
#include "TargetPanelWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UPanelWidget;
class UTargetActionWidget;

/**
 *  The top-right target panel: what the player last clicked, and what they may do to it.
 *
 *  It replaced the one-line "Pawn: Pawn 1" label that used to sit loose on the HUD canvas - and
 *  replaced it rather than joining it, so the name on screen and the actions offered can never
 *  describe two different things.
 *
 *  **The action row is real, not decorative.** Every button is assembled from what the player
 *  controller's own gating helpers would actually permit on that target right now, and clicking
 *  one runs the same code the equivalent key runs. Disabled actions stay on screen with their
 *  reason - see FTargetAction.
 *
 *  The whole panel is rebuilt from an FStrategyTargetInfo the HUD pushes every frame. Nothing is
 *  remembered between frames on purpose: the squad walks, so reach changes, so "Open" has to stop
 *  being offered the moment the chest is out of range. What *is* guarded is redrawing - the push
 *  is compared against what's on screen and does nothing at all unless something visible moved.
 */
UCLASS(abstract)
class SMORESUI_API UTargetPanelWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** The target's name. Name it "NameText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** What it is, how it feels about you, and how far away. Name it "ClassificationText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassificationText;

	/** Health, for a target that has any. Name it "HealthBar" to auto-bind. Hidden for a container. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** Holds the action buttons. Name it "ActionBox" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ActionBox;

	/** Widget class used for each button on the action row */
	UPROPERTY(EditAnywhere, Category = "Target Panel")
	TSubclassOf<UTargetActionWidget> ActionWidgetClass;

	/** What the HUD last pushed in */
	FStrategyTargetInfo Target;

	/** The action buttons currently in ActionBox, in row order. Reused between frames rather than
	 *  rebuilt, since the row usually keeps its shape while its contents change. */
	UPROPERTY()
	TArray<TObjectPtr<UTargetActionWidget>> ActionWidgets;

public:

	/** Sets what the panel describes, redrawing only if something visible actually changed. Pushed every frame. */
	void SetTargetInfo(const FStrategyTargetInfo& NewTarget);

	/** Blueprint handler for anything beyond the bound name, classification, bar and action row */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Target"))
	void BP_UpdateTarget();

protected:

	/** The line under the name, e.g. "PERSON - HOSTILE - 12m" */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetClassificationLine() const;

	/** True while something is targeted */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasTarget() const { return Target.HasTarget(); }

	/** Runs an action the player clicked. Straight to the controller, which re-checks it. */
	void HandleActionClicked(FName ActionId);

	/** Grows or shrinks ActionWidgets to match the current action count */
	void ResizeActionRow(int32 ActionCount);

	/** Pushes the current target into every bound widget and the BP hook */
	void RefreshTargetDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
