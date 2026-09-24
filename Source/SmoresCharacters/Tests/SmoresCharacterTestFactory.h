// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Shared builders for the character record tests - here and in the smores module's record-sync
 *  tests, which need a concrete unit and so can't live in this module.
 *
 *  Same rule as the item factory: **tests never load a UCharacterDefinition out of Content/**, so
 *  retuning DA_Character_Bandit can't break a record test. And same reason these live in a header
 *  rather than an anonymous namespace per file: unity builds concatenate test files.
 */

/** A character definition with exactly what the assertion cares about. Kept alive by the test world. */
inline UCharacterDefinition* MakeTestCharacterDefinition(
	FSmoresTestWorld& TestWorld,
	const TCHAR* DefinitionId,
	bool bUnique = false,
	const TArray<FText>& NamePool = TArray<FText>(),
	FName DefaultFactionId = NAME_None)
{
	UCharacterDefinition* Definition = TestWorld.NewKeptObject<UCharacterDefinition>();

	if (!Definition)
	{
		return nullptr;
	}

	Definition->DefinitionId = FName(DefinitionId);
	Definition->DisplayName = FText::FromString(DefinitionId);
	Definition->bUnique = bUnique;
	Definition->NamePool = NamePool;
	Definition->DefaultFactionId = DefaultFactionId;

	return Definition;
}

/**
 *  The record store on the test world's GameState - where UCharacterRecordComponent::Get looks, so
 *  a unit spawned afterwards finds it exactly as it would in LVL_Strategy. Reuses the store the
 *  GameState already has, adds one if it has none, and spawns a bare AGameStateBase if the world
 *  has no GameState at all. Call after TestWorld.BeginPlay(), which
 *  can itself bring a game state up.
 */
inline UCharacterRecordComponent* MakeTestRecordStore(FSmoresTestWorld& TestWorld)
{
	UWorld* World = TestWorld.GetWorld();

	if (!World)
	{
		return nullptr;
	}

	AGameStateBase* GameState = World->GetGameState();

	if (!GameState)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GameState = World->SpawnActor<AGameStateBase>(AGameStateBase::StaticClass(), FTransform::Identity, SpawnParameters);

		if (GameState && World->GetGameState() != GameState)
		{
			World->SetGameState(GameState);
		}
	}

	if (!GameState)
	{
		return nullptr;
	}

	// BeginPlay brings the project's game mode up, and its BP_StrategyGameState already carries a
	// store as a default subobject. Adding a second would leave units registering in the first
	// (Get() finds that one) while the test reads the second - so reuse whatever is there.
	if (UCharacterRecordComponent* Existing = GameState->FindComponentByClass<UCharacterRecordComponent>())
	{
		return Existing;
	}

	return TestWorld.AddComponent<UCharacterRecordComponent>(GameState);
}

#endif // WITH_DEV_AUTOMATION_TESTS
