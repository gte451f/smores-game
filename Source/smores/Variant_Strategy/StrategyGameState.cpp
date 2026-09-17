// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyGameState.h"
#include "TimePaceComponent.h"

AStrategyGameState::AStrategyGameState()
{
	TimePace = CreateDefaultSubobject<UTimePaceComponent>(TEXT("TimePace"));
}
