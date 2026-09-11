// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyCameraCommands.h"
#include "StrategyTouchControls.generated.h"

/**
 *  Base class for additional touchscreen controls for a strategy game.
 *  Exposes some game commands to UI
 */
UCLASS(abstract)
class SMORESUI_API UStrategyTouchControls : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Interface to the owning Strategy PC's camera/selection commands */
	TScriptInterface<IStrategyCameraCommands> PlayerController;

public:

	/** Sets the owning Strategy PC interface */
	void SetPlayerController(TScriptInterface<IStrategyCameraCommands> PC);

	/** Syncs the camera zoom percentage with the UI. Called by the owning PC */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta=(DisplayName="Set Zoom Percentage"))
	void BP_SetZoomPercentage(float Percentage);

protected:

	/** Resets the camera zoom level */
	UFUNCTION(BlueprintCallable, Category="UI")
	void ResetZoom();

	/** Toggles between select all units and deselect all units. */
	UFUNCTION(BlueprintCallable, Category="UI")
	void ToggleSelectAllUnits();

	/** Sets the camera zoom percentage level */
	UFUNCTION(BlueprintCallable, Category="UI")
	void SetZoomPercentage(float Percentage);
};
