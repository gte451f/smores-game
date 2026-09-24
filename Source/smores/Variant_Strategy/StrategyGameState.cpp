// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyGameState.h"
#include "TimePaceComponent.h"
#include "WorldFactionComponent.h"

AStrategyGameState::AStrategyGameState()
{
	TimePace = CreateDefaultSubobject<UTimePaceComponent>(TEXT("TimePace"));
	WorldFactions = CreateDefaultSubobject<UWorldFactionComponent>(TEXT("WorldFactions"));
}
