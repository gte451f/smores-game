// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldSeedComponent.generated.h"

/**
 *  The one number every seeded roll in a campaign starts from, owned by the GameState.
 *
 *  The world itself is hand-authored (vision-and-pillars.md), so this seeds *outcomes*, not
 *  geography: a loot container rolls its contents from this plus its own stable id (see
 *  UWeightedTableDefinition::MakeRollStream). Two campaigns with different seeds find different
 *  things in the same chest; one campaign finds the same thing in it every time, which is what
 *  characters-and-squads.md's "a retry under identical conditions produces an identical result"
 *  asks for.
 *
 *  **Part of the campaign, so it will be saved** - a reload that picked a fresh seed would re-roll
 *  every container in the world. Nothing saves yet; for now the seed is authored on the GameState
 *  Blueprint, and SetWorldSeed is the seam a new-campaign screen will call.
 *
 *  **Server-only and deliberately not replicated.** Every roll that reads it is authority-only, and
 *  a client that knew the seed could work out every chest's contents before opening one.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESCORE_API UWorldSeedComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UWorldSeedComponent();

	/** The seed component on this world's GameState, or null (the main menu, a bare test world) */
	static UWorldSeedComponent* Get(const UObject* WorldContextObject);

	/**
	 *  This world's seed, or 0 when it has no seed component. Still deterministic without one -
	 *  every roll in such a world simply shares seed 0 - which is the right fallback for a test
	 *  world or a map that isn't a campaign.
	 */
	static int32 GetWorldSeedFor(const UObject* WorldContextObject);

	/** The campaign's seed. Meaningful on the server only; clients hold whatever was authored. */
	int32 GetWorldSeed() const { return WorldSeed; }

	/**
	 *  Replaces the seed. **Authority only** - a client calling this does nothing. Only affects
	 *  rolls made afterwards: a container that has already rolled keeps what it rolled.
	 */
	void SetWorldSeed(int32 NewSeed);

protected:

	/** The campaign's seed. Any value is fine, zero included - it is mixed, never used raw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
	int32 WorldSeed = 0;
};
