// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SquadActivityWatcher.generated.h"

class AStrategyUnit;
class USmoresActivityLog;

/**
 *  Turns one unit's health events into activity-feed lines.
 *
 *  **Why this exists at all, rather than four handlers on the player controller.**
 *  UHealthComponent's delegates are dynamic, so a handler has to be a UFUNCTION on a UObject -
 *  no lambdas, no payload binding - and three of the four (OnDowned, OnRecovered, OnDied) carry
 *  no parameters whatsoever. A single handler on the controller would therefore be told that
 *  *somebody* went down and have no way to find out who. One watcher per unit is what supplies
 *  the missing parameter: the watcher knows its unit because that is the only thing it is for.
 *
 *  It reports both directions of a fight: what happened to one of the player's own squad, and
 *  what the player's squad did to somebody else. Which of those a given unit produces is decided
 *  once, at Watch() time, by whether the unit is a player pawn.
 *
 *  **Local, and knowingly so.** These delegates fire on whichever machine ran the damage, which
 *  today is the server (UHealthComponent::TakeDamage is authority-gated). So this reports
 *  correctly in a standalone session and on a listen server's own screen, and a remote client
 *  would see nothing until health events are routed to owning clients. That is a real gap rather
 *  than a hidden one - the alternative, replicating a feed nobody can see yet, is the speculative
 *  machinery multiplayer-discipline.md says not to build.
 */
UCLASS()
class USquadActivityWatcher : public UObject
{
	GENERATED_BODY()

public:

	/**
	 *  Binds this watcher to a unit's health component and to the feed to report into.
	 *
	 *  Safe to call on a unit with no health component - it simply reports nothing, which is what
	 *  a unit that cannot be hurt should do.
	 */
	void Watch(AStrategyUnit* InUnit, USmoresActivityLog* InLog, bool bInPlayerSquad);

	/** Unbinds from the unit's delegates. Called before a watcher is dropped. */
	void Unwatch();

	/** The unit being watched, or null if it has gone away */
	AStrategyUnit* GetUnit() const { return Unit.Get(); }

protected:

	/** The unit this watcher speaks for */
	TWeakObjectPtr<AStrategyUnit> Unit;

	/** Where the lines go */
	TWeakObjectPtr<USmoresActivityLog> Log;

	/** True if this unit is one of the player's own, which is what decides how its news is worded */
	bool bPlayerSquad = false;

	/**
	 *  Set once one of the player's own squad has hurt this unit.
	 *
	 *  It is what keeps a fight the player had nothing to do with out of their feed. Going down
	 *  and dying carry no instigator - both delegates are parameterless - so without this the
	 *  only honest options would be reporting every NPC that falls over anywhere in the world, or
	 *  reporting none of them. Damage *does* carry an instigator, and nothing dies here without
	 *  being damaged first, so remembering that one bit answers the other two events.
	 */
	bool bHurtByPlayerSquad = false;

	/** The watched unit's display name, or a fallback if it was never named */
	FText GetUnitName() const;

	UFUNCTION()
	void HandleDamaged(AActor* DamageInstigator);

	UFUNCTION()
	void HandleDowned();

	UFUNCTION()
	void HandleRecovered();

	UFUNCTION()
	void HandleDied();
};
