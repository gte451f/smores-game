// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyGameState.h"
#include "TimePaceComponent.h"
#include "WorldFactionComponent.h"
#include "CharacterRecordComponent.h"
#include "WorldSeedComponent.h"

AStrategyGameState::AStrategyGameState()
{
	TimePace = CreateDefaultSubobject<UTimePaceComponent>(TEXT("TimePace"));
	WorldFactions = CreateDefaultSubobject<UWorldFactionComponent>(TEXT("WorldFactions"));
	CharacterRecords = CreateDefaultSubobject<UCharacterRecordComponent>(TEXT("CharacterRecords"));
	WorldSeed = CreateDefaultSubobject<UWorldSeedComponent>(TEXT("WorldSeed"));
}
