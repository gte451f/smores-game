// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "Inventory/InventoryComponent.h"
#include "StrategyPlayerUnit.generated.h"

/**
 *  A player-controlled strategy unit.
 *
 *  Functionally identical to AStrategyUnit today, but is a distinct type so the Strategy
 *  Player Controller can enumerate and Tab-cycle between player pawns without affecting
 *  the generic NPC units. This is also the intended hook for future player-unit state
 *  (team ID, hover highlight, etc.).
 */
UCLASS(abstract)
class AStrategyPlayerUnit : public AStrategyUnit
{
	GENERATED_BODY()

public:

	/** Constructor */
	AStrategyPlayerUnit();

protected:

	/** Items every player unit starts with. Populated with defaults in the constructor; overridable per-Blueprint. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryItem> StartingItems;

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface
};
