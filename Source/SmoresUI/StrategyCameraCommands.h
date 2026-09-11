// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategyCameraCommands.generated.h"

UINTERFACE(MinimalAPI)
class UStrategyCameraCommands : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by whichever PlayerController owns the strategy camera, so touch UI can issue camera/selection commands. */
class SMORESUI_API IStrategyCameraCommands
{
	GENERATED_BODY()

public:

	/** Returns the default camera zoom percentage value */
	virtual float GetDefaultZoomPercentage() const = 0;

	/** Resets the camera zoom to default */
	virtual void DoCameraResetZoomCommand() = 0;

	/** Toggles between selecting all units on screen and deselecting units */
	virtual void DoToggleSelectAllUnitsCommand() = 0;

	/** Sets the camera zoom to a percentage between min and max zoom */
	virtual void DoCameraSetZoomPercentageCommand(float Percentage) = 0;
};
