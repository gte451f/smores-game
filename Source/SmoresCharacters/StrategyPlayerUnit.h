// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "InventoryComponent.h"
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
class SMORESCHARACTERS_API AStrategyPlayerUnit : public AStrategyUnit
{
	GENERATED_BODY()

public:

	/** Returns whichever PlayerController currently owns/commands this unit, or nullptr if unclaimed. See ClaimForController. */
	APlayerController* GetOwningController() const { return OwningController; }

	/** Claims this unit for the given controller. Authority-only; a no-op if already claimed by anyone (including the same controller).
	 *  Placeholder squad-assignment policy for the current single-tested-player prototype - see
	 *  AStrategyPlayerController::BeginPlay, which calls this for every not-yet-claimed unit at startup. Session
	 *  management (assigning specific squads to specific connecting players) isn't designed yet - see the
	 *  game-design skill's multiplayer-and-content.md - so this exists to unblock per-player squad scoping
	 *  (RefreshPlayerPawns, DoSelectAllUnitsOnScreenCommand, drag-select, click-select) without guessing at
	 *  that design. A real login/spawn flow can call this (or set OwningController directly) instead. */
	void ClaimForController(APlayerController* NewOwningController);

protected:

	/** Items every player unit starts with, authored per-Blueprint as references to UItemDefinition assets */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryItem> StartingItems;

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

private:

	/** Which PlayerController currently owns/commands this unit. Replicated so the owning client can filter
	 *  its own squad locally (selection/commands are client-side input handling). Set once via ClaimForController. */
	UPROPERTY(Replicated)
	TObjectPtr<APlayerController> OwningController;
};
