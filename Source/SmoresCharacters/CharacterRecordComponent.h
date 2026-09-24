// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterRecord.h"
#include "CharacterRecordComponent.generated.h"

class UCharacterDefinition;

/**
 *  Every character record in the session, owned by the GameState - the registry half of the
 *  definition / record / actor model in game-data.md.
 *
 *  **The soft split.** Every record has exactly one actor standing in for it, and the two are
 *  created together when that actor begins play. Nothing here spawns or despawns anything, and
 *  nothing advances a record that has no actor. The actor's components are the working copy that
 *  play reads every frame; the actor writes its condition back here whenever it changes. The
 *  world-activity roadmap is what will let a record outlive its actor and move without one.
 *
 *  **Server-owned and deliberately not replicated.** Every client already sees each character
 *  through the actor's own replicated components, which *are* the copy of this. Replicating the
 *  records as well would send every character's pack to every player twice - and once the full
 *  split lands, records for characters nowhere near anyone would be the bulk of the traffic. A
 *  client-side view that needs a record (a squad roster, say) wants its own narrower channel.
 *
 *  Every mutator is authority-only and refuses, having done nothing, on a client.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESCHARACTERS_API UCharacterRecordComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCharacterRecordComponent();

	/**
	 *  The record store for this world - the component on its GameState - or null when the world
	 *  has no GameState or the GameState has no store (the main menu, a test world that didn't add
	 *  one). A unit that finds none runs exactly as it did before records existed.
	 */
	static UCharacterRecordComponent* Get(const UObject* WorldContextObject);

	/**
	 *  Creates a record from a definition. **Authority only.** Returns the new record's id, or an
	 *  invalid FGuid when refused:
	 *
	 *    - off-authority;
	 *    - a record with RequestedId already exists (adopt it instead - see FindRecord);
	 *    - the definition is unique and a record of it already exists, alive or dead.
	 *
	 *  An invalid RequestedId mints a fresh one. The name is the definition's DisplayName for a
	 *  unique definition; otherwise NameOverride when it isn't empty (a placed unit the level
	 *  designer named), otherwise a roll from the NamePool - see RollName.
	 *
	 *  A null definition is allowed and makes a record with no DefinitionId, named NameOverride:
	 *  a unit authored before it had a definition still gets a record rather than being left out.
	 *  A DefaultFactionId the world doesn't know is copied anyway, with a warning - an unresolvable
	 *  id is a stripped mod, not an error.
	 *
	 *  Identity only: the condition half (health, carried, equipped, location) is empty until the
	 *  record's actor first writes back, which it does straight after creating it. Placing the
	 *  DefaultLoadout needs a grid, and the actor's grid is the one with the placement rules.
	 */
	FGuid CreateRecord(const UCharacterDefinition* Definition, const FGuid& RequestedId, const FText& NameOverride = FText::GetEmpty());

	/** The record with this id, or null. Works on the server only - clients hold none. */
	const FCharacterRecord* FindRecord(const FGuid& RecordId) const;

	/**
	 *  Writable access to one record. **Authority only** - null on a client, and null for an
	 *  unknown id. Don't hold the pointer across a CreateRecord, which can reallocate the storage.
	 */
	FCharacterRecord* EditRecord(const FGuid& RecordId);

	/** Every record in the session, in creation order */
	const TArray<FCharacterRecord>& GetRecords() const { return Records; }

	/** True if any record - alive or dead - was created from this definition id. What makes a unique definition unique. */
	bool HasRecordOfDefinition(FName DefinitionId) const;

	/**
	 *  Marks Actor as the one standing in for this record. **Authority only.** Refuses an unknown
	 *  record, and refuses a record that already has a *different* live actor - two actors
	 *  puppeting one record would each overwrite the other's write-backs. Re-binding the same
	 *  actor is fine.
	 */
	bool BindActor(const FGuid& RecordId, AActor* Actor);

	/** Clears the binding, but only if Actor is the one bound. The record itself stays - it is the truth, and it outlives its puppet. */
	void UnbindActor(const FGuid& RecordId, const AActor* Actor);

	/** The live actor standing in for this record, or null */
	AActor* GetBoundActor(const FGuid& RecordId) const;

	/**
	 *  The name a non-unique record with this id rolls from the definition's pool. Seeded from the
	 *  id rather than a random stream, so the same individual gets the same name every time it is
	 *  asked - the save-scumming rule in characters-and-squads.md, applied to names. Falls back to
	 *  the definition's DisplayName when the pool is empty or the entry it lands on is blank.
	 */
	static FText RollName(const UCharacterDefinition* Definition, const FGuid& RecordId);

	/**
	 *  True if the world knows this faction id. Asks the UWorldFactionComponent beside this one once
	 *  it has seeded; before that (the two BeginPlay in no guaranteed order) or without one, falls
	 *  back to whether the id resolves to a faction definition at all.
	 */
	bool IsFactionKnown(FName FactionId) const;

protected:

	/** True if this component's owner is the authoritative copy - the one gate on every mutator here */
	bool HasOwnerAuthority() const;

private:

	/** Every record in the session. Server-only; see the class comment for why it isn't replicated. */
	UPROPERTY()
	TArray<FCharacterRecord> Records;

	/**
	 *  Which live actor stands in for which record. Runtime-only bookkeeping, never saved - on
	 *  load, actors bind themselves again as they begin play.
	 */
	TMap<FGuid, TWeakObjectPtr<AActor>> BoundActors;
};
