// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FactionTypes.h"
#include "WorldFactionComponent.generated.h"

class UFactionDefinition;

/** Broadcast on every machine when any faction record or faction-to-faction standing changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldFactionsChangedDelegate);

/**
 *  The session's faction records and the faction-to-faction standing matrix, owned by the
 *  GameState.
 *
 *  **Session-wide state, so it lives on the GameState** - there is one world, and every player
 *  sees the same Ironclan at war with the same Raiders. A player's *own* standing with a faction
 *  is different state with a different owner; that is UPlayerStandingComponent on the player
 *  state. See multiplayer-discipline.md's "whose state is it" table.
 *
 *  **Storage and queries only.** Nothing in the project reads a standing here to decide who
 *  attacks whom; hostility is still the placeholder in AStrategyUnit. That is deliberate - the
 *  game-data roadmap's Slice 3 stores standing, and deriving behavior from it is an AI change.
 *
 *  Every mutator is authority-only and returns false having done nothing on a client. Both
 *  arrays replicate to every machine, so every query works on a client too.
 *
 *  The records and the matrix are TArrays rather than TMaps because Unreal cannot replicate a
 *  TMap. A handful of factions makes the linear search free; if faction counts reach the
 *  hundreds this is the class that grows an index, and callers ask the same questions either way.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESCORE_API UWorldFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UWorldFactionComponent();

	/** Fired whenever a record or a pair standing changes, on the server and on every client that sees it */
	UPROPERTY(BlueprintAssignable, Category = "Factions")
	FOnWorldFactionsChangedDelegate OnFactionsChanged;

	/**
	 *  Replaces every record and pair standing with a fresh campaign start built from these
	 *  definitions: one record per faction at its StartingTier, and the matrix seeded from each
	 *  one's StartingRelations. **Authority only.**
	 *
	 *  BeginPlay calls this with every faction the Asset Manager knows about. It is public so a
	 *  test can hand it in-memory definitions instead, and so a later "new campaign" flow has a
	 *  front door that isn't BeginPlay.
	 *
	 *  Definitions are seeded in id order, so a relation authored on both sides with different
	 *  numbers resolves the same way every run (the later id wins, with a warning) - though the
	 *  content sweep exists so that never ships. Nulls, blank ids and duplicate ids are skipped
	 *  with a warning. Returns the number of records created, or INDEX_NONE off-authority.
	 */
	int32 InitializeFromDefinitions(const TArray<const UFactionDefinition*>& Definitions);

	/** True if the world has a record for this faction id */
	UFUNCTION(BlueprintPure, Category = "Factions")
	bool IsKnownFaction(FName FactionId) const { return FindRecord(FactionId) != nullptr; }

	/** This faction's record, or null if the world has none - an unknown or stripped faction is a legitimate answer, not an error */
	const FFactionRecord* FindRecord(FName FactionId) const;

	/** Every faction record, in id order */
	const TArray<FFactionRecord>& GetRecords() const { return Records; }

	/** Every authored or changed pair standing. A pair absent from here is neutral. */
	const TArray<FFactionPairStanding>& GetStandingMatrix() const { return PairStandings; }

	/** A faction's current tier. False, and OutTier untouched, if the faction is unknown. */
	UFUNCTION(BlueprintPure, Category = "Factions")
	bool GetFactionTier(FName FactionId, EFactionTier& OutTier) const;

	/**
	 *  How two factions regard each other. Symmetric - (A, B) and (B, A) are the same question.
	 *
	 *  A faction regards itself at SmoresStanding::Max. Anything involving an unknown faction, or
	 *  a pair nobody has an opinion about yet, is SmoresStanding::Neutral - never an error,
	 *  because a save with a stripped mod's faction in it has to keep answering.
	 */
	UFUNCTION(BlueprintPure, Category = "Factions")
	int32 GetStandingBetween(FName FactionA, FName FactionB) const;

	/**
	 *  Sets the standing between two known, different factions, clamped into the SmoresStanding
	 *  range. **Authority only.** False, with nothing changed, off-authority, for an unknown
	 *  faction, or when both ids are the same faction. True otherwise, including when the value
	 *  was already what was asked for.
	 */
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool SetStandingBetween(FName FactionA, FName FactionB, int32 NewStanding);

	/** SetStandingBetween(A, B, current + Delta), clamped. Same refusals, same return. */
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool AdjustStandingBetween(FName FactionA, FName FactionB, int32 Delta);

	/**
	 *  Moves a faction to a different tier. **Authority only**; false for an unknown faction.
	 *  Never touches the definition - its StartingTier stays what was authored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Factions")
	bool SetFactionTier(FName FactionId, EFactionTier NewTier);

	/** The two ids in the order the matrix stores them under - see FFactionPairStanding */
	static void OrderPair(FName& InOutFirst, FName& InOutSecond);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent interface

	/** True if this component's owner is the authoritative copy - the one gate on every mutator here */
	bool HasOwnerAuthority() const;

	/** The stored cell for an already-ordered pair, or null if nobody has set one */
	FFactionPairStanding* FindPair(FName FirstId, FName SecondId);
	const FFactionPairStanding* FindPair(FName FirstId, FName SecondId) const;

	/** Writes a pair without the known-faction checks - the shared tail of seeding and SetStandingBetween */
	bool WritePair(FName FactionA, FName FactionB, int32 NewStanding);

	/** Reacts on non-authority machines to either array changing - authority broadcasts from the mutator */
	UFUNCTION()
	void OnRep_Factions();

private:

	/** One record per faction in the world, in id order. Server-owned; clients hold a replicated copy. */
	UPROPERTY(ReplicatedUsing = OnRep_Factions)
	TArray<FFactionRecord> Records;

	/** The faction-to-faction matrix, one cell per pair that isn't neutral-by-omission */
	UPROPERTY(ReplicatedUsing = OnRep_Factions)
	TArray<FFactionPairStanding> PairStandings;
};
