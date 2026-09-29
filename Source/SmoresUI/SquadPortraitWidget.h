// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatComponent.h"
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
 *  One circular portrait in the squad bar: a face (or initials), a name, a health ring, a
 *  selection ring, and the danger flash.
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
 *
 *  **The danger flash** fires when this unit's UCombatComponent signals - entering a hostile
 *  engagement, or it getting worse (game-design's notifications-and-alerts.md). The portrait
 *  subscribes to its own unit, because it is the thing that stands for that unit on screen. It is
 *  brief and marks only the transition; the health bar underneath is the ongoing readout. It is
 *  timed on wall-clock rather than world time, like the bark bubbles, so it lasts as long at 8x as
 *  at 1x - the flash is for the player's eyes, not the simulation.
 *
 *  It must never be colour alone, and never confusable with the selection ring: the C++ half shows
 *  and pulses DangerMarker, which the WBP draws as a badge with a glyph in the corner rather than
 *  anything ring-shaped, and BP_DangerFlash is the cosmetic half for styling and timing curves.
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
	 *  The danger flash's icon, shown and pulsed only while it runs. Name it "DangerMarker" to
	 *  auto-bind. It carries the non-colour half of the alert, so it must be a *shape* - a badge with
	 *  a glyph - and must not read as a second selection ring.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DangerMarker;

	/** How long the danger flash runs, in real seconds. Brief by design - see the class comment. */
	UPROPERTY(EditAnywhere, Category = "Squad Portrait|Danger", meta = (ClampMin = 0.1, Units = "s"))
	float DangerFlashSeconds = 2.0f;

	/**
	 *  How many times a second DangerMarker pulses while the flash runs. Capped below 3 - the
	 *  common photosensitivity guideline is no more than three flashes a second, and this is exactly
	 *  the kind of signal the accessibility baseline exists to catch.
	 */
	UPROPERTY(EditAnywhere, Category = "Squad Portrait|Danger", meta = (ClampMin = 0, ClampMax = 2.9, Units = "Hz"))
	float DangerPulseHz = 2.0f;

	/** DangerMarker's opacity at the bottom of each pulse. 1 turns the pulse off and leaves a steady badge. */
	UPROPERTY(EditAnywhere, Category = "Squad Portrait|Danger", meta = (ClampMin = 0, ClampMax = 1))
	float DangerPulseMinOpacity = 0.3f;

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

	/** The unit's combat component this portrait is listening to, if any */
	TWeakObjectPtr<UCombatComponent> WatchedCombat;

	/** The subscription to WatchedCombat's OnDangerSignal */
	FDelegateHandle DangerSignalHandle;

	/** FPlatformTime::Seconds() the running danger flash started at. Negative while none is running. */
	double DangerFlashStartTime = -1.0;

	/** Why the most recent danger flash fired */
	EDangerSignal LastDangerSignal = EDangerSignal::Entered;

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

	/** Blueprint handler for the cosmetic half of a danger flash starting - fires again if a new signal restarts one */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Danger Flash"))
	void BP_DangerFlash(EDangerSignal Signal);

	/** Blueprint handler for the danger flash running out */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Danger Flash Ended"))
	void BP_DangerFlashEnded();

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

	/** True while a danger flash is running */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsDangerFlashing() const { return DangerFlashStartTime >= 0.0; }

	/** How far through the running danger flash, 0-1, or 0 if none is - the input for a BP timing curve */
	UFUNCTION(BlueprintPure, Category = "UI")
	float GetDangerFlashProgress() const;

	/** Why the most recent danger flash fired */
	UFUNCTION(BlueprintPure, Category = "UI")
	EDangerSignal GetLastDangerSignal() const { return LastDangerSignal; }

	UFUNCTION()
	void HandleClicked();

	/** Pushes the current unit and state into every bound widget and the BP hook */
	void RefreshPortraitDisplay();

	/** Moves the danger subscription to InUnit's combat component, dropping any flash that belonged to the old unit */
	void WatchUnitCombat(AStrategyUnit* InUnit);

	/** Bound to the watched unit's UCombatComponent::OnDangerSignal */
	void HandleDangerSignal(AActor* SignalUnit, EDangerSignal Signal);

	/** Pulses DangerMarker and ends the flash once it has run its course. Driven by SetUnit's per-frame push. */
	void UpdateDangerFlash();

	/** Ends a running danger flash, if there is one */
	void StopDangerFlash();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface
};
