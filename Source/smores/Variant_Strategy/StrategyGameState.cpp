// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyGameState.h"
#include "TimePaceComponent.h"
#include "WorldFactionComponent.h"
#include "CharacterRecordComponent.h"
#include "WorldSeedComponent.h"
#include "BarkDirectorComponent.h"
#include "BanterDirectorComponent.h"

AStrategyGameState::AStrategyGameState()
{
	TimePace = CreateDefaultSubobject<UTimePaceComponent>(TEXT("TimePace"));
	WorldFactions = CreateDefaultSubobject<UWorldFactionComponent>(TEXT("WorldFactions"));
	CharacterRecords = CreateDefaultSubobject<UCharacterRecordComponent>(TEXT("CharacterRecords"));
	WorldSeed = CreateDefaultSubobject<UWorldSeedComponent>(TEXT("WorldSeed"));
	BarkDirector = CreateDefaultSubobject<UBarkDirectorComponent>(TEXT("BarkDirector"));
	BanterDirector = CreateDefaultSubobject<UBanterDirectorComponent>(TEXT("BanterDirector"));
}
