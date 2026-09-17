// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyTargetInfo.h"
#include "TargetActionWidget.generated.h"

class UButton;
class UTextBlock;

/** Broadcast when the player clicks an action. Carries the action id, which is all the panel needs. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTargetActionClicked, FName);

/**
 *  One button on the target panel's action row - "Open", "Talk", "Attack".
 *
 *  Deliberately **not** a UHUDRegionWidget: it lives inside one. A real UButton consumes its own
 *  press, and when this action is disabled the button doesn't handle the click at all, so it
 *  bubbles to the panel's own shield rather than reaching the world. Either way nothing falls
 *  through - see hud-and-panels.md's "the clickable-HUD trap".
 *
 *  A disabled action still shows, with its reason beside it. That is the rule FTargetAction's
 *  comment explains: a greyed-out "Talk" teaches why you can't, a missing one teaches nothing.
 */
UCLASS(abstract)
class SMORESUI_API UTargetActionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The clickable part. Name it "ActionButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActionButton;

	/** The action's name and key hint. Name it "LabelText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	/** Why it's unavailable. Name it "ReasonText" in the WBP to auto-bind. Hidden while enabled. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReasonText;

	/** The action this button currently stands for */
	FTargetAction Action;

public:

	/** Fired when the player clicks an enabled action */
	FOnTargetActionClicked OnActionClicked;

	/** Fills this button in from an action. Called every time the panel rebuilds its row. */
	void SetAction(const FTargetAction& InAction);

	/** Blueprint handler for anything beyond the bound label, reason and enabled state */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Action"))
	void BP_UpdateAction();

protected:

	/** The label with its key hint, e.g. "Open [O]" */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetActionLabel() const;

	/** Whether the player can use this action right now */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsActionEnabled() const { return Action.bEnabled; }

	UFUNCTION()
	void HandleClicked();

	/** Pushes the current action into the bound widgets and the BP hook */
	void RefreshActionDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
