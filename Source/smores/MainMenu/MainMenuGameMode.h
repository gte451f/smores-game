// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 *  Game mode for the main menu level. No pawn/player-start needed - the level is UI only.
 *  Check the Blueprint derived class for the HUD class assignment.
 */
UCLASS(abstract)
class AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
};
