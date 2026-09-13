// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategyResourceHost.generated.h"

UINTERFACE(MinimalAPI)
class UStrategyResourceHost : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implemented by whichever PlayerController can report the local player's at-a-glance
 *  resources to the HUD.
 *
 *  The balance itself lives on that player's PlayerState, which is a gameplay class in the
 *  primary module - SmoresUI can't see it, and shouldn't have to. Same shape as
 *  IStrategySelectionHost/IStrategyCameraCommands: one narrow read, implemented by
 *  AStrategyPlayerController, keyed off the *owning* controller rather than any global.
 */
class SMORESUI_API IStrategyResourceHost
{
	GENERATED_BODY()

public:

	/** This controller's player's current gold balance, or 0 if there's no player state yet */
	virtual int32 GetPlayerGold() const = 0;
};
