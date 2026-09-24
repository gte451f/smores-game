// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FactionTypes.h"
#include "PlayerStandingComponent.generated.h"

/** Broadcast on the server and the owning client whenever any of this player's standings changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStandingChangedDelegate);

/**
 *  One player's standing with each faction, hosted on AStrategyPlayerState.
 *
 *  **Per-player, never global.** factions-and-world-state.md tracks standing independently per
 *  faction *and* per player - in co-op, one player robbing Ironclan blind doesn't make Ironclan
 *  hate their partner - and multiplayer-discipline.md puts per-player state on the player state.
 *  The world's view of factions (their tiers, how they regard each other) is session-wide and
 *  lives on the GameState instead, in UWorldFactionComponent.
 *
 *  Replicated to its **owner only**: a player's reputation is theirs to read, and nothing yet
 *  needs a co-op partner's. Widening it is one replication condition if a squad screen ever does.
 *
 *  A faction with no entry is neutral, which is also what every player starts at: an
 *  unaffiliated squad starts outside every faction hierarchy. Standing isn't checked against
 *  the world's faction list on write, deliberately - an id is allowed to name a faction that no
 *  longer resolves (a stripped mod), and the entry simply sits there answering its own number.
 *
 *  **Storage and queries only**, like UWorldFactionComponent: nothing reads this to decide
 *  whether a patrol attacks on sight yet.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESCORE_API UPlayerStandingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UPlayerStandingComponent();

	/** Fired whenever a standing changes, on the server and on the owning client */
	UPROPERTY(BlueprintAssignable, Category = "Factions")
	FOnPlayerStandingChangedDelegate OnStandingChanged;

	/** This player's standing with a faction - SmoresStanding::Neutral for one they've never dealt with, or an id that means nothing */
	UFUNCTION(BlueprintPure, Category = "Factions")
	int32 GetStanding(FName FactionId) const;

	/** Every faction this player has a recorded standing with. A faction absent from here is neutral. */
	const TArray<FPlayerFactionStanding>& GetStandings() const { return Standings; }

	/**
	 *  Sets this player's standing with a faction, clamped into the SmoresStanding range.
	 *  **Authority only.** False, with nothing changed, off-authority or for a None id; true
	 *  otherwise, including when the value was already what was asked for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool SetStanding(FName FactionId, int32 NewStanding);

	/** SetStanding(FactionId, current + Delta), clamped. Same refusals, same return. */
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool AdjustStanding(FName FactionId, int32 Delta);

protected:

	//~ Begin UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent interface

	/** True if this component's owner is the authoritative copy - the one gate on every mutator here */
	bool HasOwnerAuthority() const;

	/** Reacts on the owning client to a replicated change - authority broadcasts from the mutator */
	UFUNCTION()
	void OnRep_Standings();

private:

	/** One entry per faction this player has a non-default history with. Server-owned. */
	UPROPERTY(ReplicatedUsing = OnRep_Standings)
	TArray<FPlayerFactionStanding> Standings;
};
