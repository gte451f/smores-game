// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SquadPortraitWidget.generated.h"

class AStrategyUnit;
class UButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UWidget;

/** Broadcast when the player clicks a portrait. Carries the unit, and whether the camera should cut to it. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSquadPortraitClicked, AStrategyUnit*, bool);

/**
 *  One circular portrait in the squad bar: a face (or initials), a name, a health ring, and a
 *  selection ring.
 *
 *  Deliberately **not** a UHUDRegionWidget - it lives inside one, exactly like
 *  UTargetActionWidget. Its UButton consumes its own press, and anything that lands in the gaps
 *  between portraits is caught by the bar's own shield.
 *
 *  **Click selects; a fast second click also cuts the camera.** That second gesture is not a
 *  Slate double-click event: SButton turns the second click of a rapid pair into another ordinary
 *  press (see input-and-keybinds.md), so what arrives here is two OnClicked calls, and the timing
 *  between them is the whole signal. Doing it this way rather than by overriding
 *  NativeOnMouseButtonDoubleClick on a non-button root is what keeps the click off the world
 *  behind the HUD without a second click shield of our own.
 *
 *  The consequence worth knowing: the first click of a double-click *does* select, immediately.
 *  That is wanted - a portrait click should never feel like it's waiting to see what you do next -
 *  and it means the camera cut is purely additive to a selection that already happened.
 */
UCLASS(abstract)
class SMORESUI_API USquadPortraitWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The clickable disc. Name it "PortraitButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PortraitButton;

	/** The unit's face. Name it "PortraitImage" to auto-bind. Hidden when the unit has no texture. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;

	/** Stand-in initials, shown only when there is no portrait texture. Name it "InitialsText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitialsText;

	/** The unit's name under the disc. Name it "NameText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** Health, 0-1. Name it "HealthBar" to auto-bind. The wireframe's ring; a bar until the styling pass. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** Shown only while this unit is in the selection. Name it "SelectionRing" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectionRing;

	/**
	 *  How long after a click a second click still counts as "and focus the camera".
	 *
	 *  0.5s matches IA_Strategy_SelectAllDoubleClick's RepeatDelay and Windows' own system-wide
	 *  double-click speed, so the gesture that opens a chest in the world and the gesture that
	 *  snaps the camera to a portrait want the same speed from the player's hand. Measured
	 *  release-to-release, which is what OnClicked gives us.
	 */
	UPROPERTY(EditAnywhere, Category = "Squad Portrait", meta = (ClampMin = 0, Units = "s"))
	float DoubleClickSeconds = 0.5f;

	/** The unit this portrait stands for */
	TWeakObjectPtr<AStrategyUnit> Unit;

	/** True while this unit is in the current selection */
	bool bSelected = false;

	/** Health as drawn, 0-1 */
	float HealthFraction = 1.0f;

	/** True if the unit has health at all. A unit with none draws no ring rather than an empty one. */
	bool bHasHealth = false;

	/** FPlatformTime::Seconds() of the last click, for the focus gesture above. Negative until the first. */
	double LastClickTime = -1.0;

public:

	/** Fired when the player clicks this portrait. The bool is true for the camera-focus gesture. */
	FOnSquadPortraitClicked OnPortraitClicked;

	/** Points this portrait at a unit and its current state. Called every time the bar refreshes. */
	void SetUnit(AStrategyUnit* InUnit, bool bInSelected);

	/** The unit this portrait currently stands for, or null */
	AStrategyUnit* GetUnit() const { return Unit.Get(); }

	/** Blueprint handler for anything beyond the bound face, name, ring and selection state */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Portrait"))
	void BP_UpdatePortrait();

protected:

	/** Up to two initials from the unit's name, for a unit with no portrait texture */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetInitials() const;

	/** This unit's name, or empty if nobody named it */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetUnitName() const;

	/** True while this unit is in the current selection */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsUnitSelected() const { return bSelected; }

	UFUNCTION()
	void HandleClicked();

	/** Pushes the current unit and state into every bound widget and the BP hook */
	void RefreshPortraitDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
