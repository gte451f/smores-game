// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyContainer.h"
#include "StrategyChest.generated.h"

/**
 *  A chest: the first concrete AStrategyContainer type.
 *  Adds no behavior of its own yet - it exists as a distinct type so future chest-specific
 *  behavior (locks, keys, etc.) has a home without touching AStrategyContainer or other
 *  container types (barrels, bags, ...). Its contents are authored as StartingItems on the
 *  Blueprint subclass, pointing at UItemDefinition assets.
 */
UCLASS(abstract)
class SMORESITEMS_API AStrategyChest : public AStrategyContainer
{
	GENERATED_BODY()
};
