// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyContainer.h"
#include "StrategyChest.generated.h"

/**
 *  A chest: the first concrete AStrategyContainer type.
 *  Adds no new behavior yet beyond seeding its own default StartingItems - it exists as a
 *  distinct type so future chest-specific behavior (locks, keys, etc.) has a home without
 *  touching AStrategyContainer or other container types (barrels, bags, ...).
 */
UCLASS(abstract)
class AStrategyChest : public AStrategyContainer
{
	GENERATED_BODY()

public:

	/** Constructor */
	AStrategyChest();
};
