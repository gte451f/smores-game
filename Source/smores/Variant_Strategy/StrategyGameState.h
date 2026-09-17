// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "StrategyGameState.generated.h"

class UTimePaceComponent;

/**
 *  Per-session state for the strategy variant. Like AStrategyPlayerState it owns no gameplay
 *  state of its own - it hosts the components that do.
 *
 *  The GameState is Unreal's composition root for state that is shared by everyone in a session
 *  and replicated to every client, which is exactly the shape of the simulation's pace. World
 *  clock, weather and faction standing will each want a component here for the same reason, and
 *  each belongs in the module that owns it rather than as a field on this class - see
 *  unreal-module-organization.md's "Framework Classes vs. Feature Modules".
 */
UCLASS(abstract)
class AStrategyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AStrategyGameState();

	/** The session's simulation speed. Never null - it's a default subobject. */
	UTimePaceComponent* GetTimePace() const { return TimePace; }

private:

	/** How fast the simulation is running, in SmoresCore. See the class comment for why it isn't
	 *  a float here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTimePaceComponent> TimePace;
};
