// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WorldSeedComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UWorldSeedComponent::UWorldSeedComponent()
{
	// a number that changes when a campaign starts, and never otherwise
	PrimaryComponentTick.bCanEverTick = false;

	// server-only; see the class comment for why clients get no copy
	SetIsReplicatedByDefault(false);
}

UWorldSeedComponent* UWorldSeedComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	return GameState ? GameState->FindComponentByClass<UWorldSeedComponent>() : nullptr;
}

int32 UWorldSeedComponent::GetWorldSeedFor(const UObject* WorldContextObject)
{
	const UWorldSeedComponent* SeedComponent = Get(WorldContextObject);

	return SeedComponent ? SeedComponent->GetWorldSeed() : 0;
}

void UWorldSeedComponent::SetWorldSeed(int32 NewSeed)
{
	// campaign state - only the server may change it
	const AActor* Owner = GetOwner();

	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	WorldSeed = NewSeed;
}
