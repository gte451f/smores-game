// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyTouchControls.h"

void UStrategyTouchControls::SetPlayerController(TScriptInterface<IStrategyCameraCommands> PC)
{
	PlayerController = PC;
}

void UStrategyTouchControls::ResetZoom()
{
	if (PlayerController)
	{
		PlayerController->DoCameraResetZoomCommand();

		BP_SetZoomPercentage(PlayerController->GetDefaultZoomPercentage());
	}
}

void UStrategyTouchControls::ToggleSelectAllUnits()
{
	if (PlayerController)
	{
		PlayerController->DoToggleSelectAllUnitsCommand();
	}
}

void UStrategyTouchControls::SetZoomPercentage(float Percentage)
{
	if (PlayerController)
	{
		PlayerController->DoCameraSetZoomPercentageCommand(Percentage);
	}
}