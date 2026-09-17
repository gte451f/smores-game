// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "GamePace.h"
#include "TimePaceWidget.generated.h"

class UButton;
class UTextBlock;

/**
 *  The top-centre time-pace strip: four buttons that jump to the tiers a player reaches for, and
 *  a readout of where the simulation actually is.
 *
 *  **The readout is the source of truth, not the buttons.** EGamePace carries the whole ladder
 *  (paused, 1/3x, 1/2x, 3/4x, 1x, 2x, 4x, 8x) and `-`/`=` step through all of it, but only four
 *  tiers are worth a button. Land on one of the other four and no button is lit while the readout
 *  says "3/4x" - which is honest and costs nothing, where lighting the nearest button would be a
 *  quiet lie about what the world is doing.
 *
 *  Every button is a real UButton, so it consumes its own click rather than drag-selecting the
 *  world behind the HUD; the strip's background catches the gaps. See UHUDRegionWidget.
 *
 *  The current tier arrives the way gold does - AStrategyHUD resolves the GameState's
 *  UTimePaceComponent and pushes it through UStrategyUI every frame. Nothing here reads the
 *  component directly, so a client whose GameState hasn't replicated in yet simply shows nothing
 *  rather than needing a null check of its own.
 */
UCLASS(abstract)
class SMORESUI_API UTimePaceWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** Freezes the simulation. Name it "PauseButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PauseButton;

	/** Real time. Name it "NormalButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NormalButton;

	/** Twice real time. Name it "DoubleButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DoubleButton;

	/** Four times real time. Name it "QuadrupleButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuadrupleButton;

	/** The tier readout. Name it "PaceText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PaceText;

	/** Tint applied to the button whose tier is currently running */
	UPROPERTY(EditAnywhere, Category = "Time Pace")
	FLinearColor ActiveButtonTint = FLinearColor(1.0f, 0.85f, 0.45f, 1.0f);

	/** Tint applied to every other button */
	UPROPERTY(EditAnywhere, Category = "Time Pace")
	FLinearColor InactiveButtonTint = FLinearColor::White;

	/** The tier the HUD last pushed in */
	EGamePace Pace = EGamePace::Normal;

public:

	/** The tiers this strip has a button for, left to right */
	static const TArray<EGamePace>& GetStripPaces();

	/** Sets the tier currently running, repainting only if it actually changed. Pushed every frame. */
	void SetPace(EGamePace NewPace);

	/** Blueprint handler for anything beyond the bound readout and tints */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Pace"))
	void BP_UpdatePace();

protected:

	/** The tier currently running */
	UFUNCTION(BlueprintPure, Category = "UI")
	EGamePace GetPace() const { return Pace; }

	/** The readout text for the current tier ("Paused", "1x", "4x") */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetPaceLabel() const;

	/** The button for a tier, or null if this WBP doesn't provide one */
	UButton* GetButtonForPace(EGamePace InPace) const;

	/** Asks the controller for a tier, if this widget can reach one */
	void RequestPace(EGamePace NewPace);

	UFUNCTION()
	void HandlePauseClicked();

	UFUNCTION()
	void HandleNormalClicked();

	UFUNCTION()
	void HandleDoubleClicked();

	UFUNCTION()
	void HandleQuadrupleClicked();

	/** Pushes the current tier into the readout, the tints and the BP hook */
	void RefreshPaceDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
