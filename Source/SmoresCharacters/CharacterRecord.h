// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HealthComponent.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "CharacterRecord.generated.h"

/**
 *  The seven attributes from the game-design skill's characters-and-squads.md. Settled design,
 *  which is why they are stored now; the skill roster is explicitly *not* settled, which is why
 *  there is no skill map beside them.
 *
 *  **Nothing reads these yet.** No roll, no resolution, no drift. The scale is a placeholder too:
 *  10 is an arbitrary "ordinary person" baseline picked so an unfilled asset holds something
 *  sensible, and whoever writes the first resolution math owns the real range. Floats because the
 *  design has them drifting slowly through sustained behaviour, which wants fractions.
 */
USTRUCT(BlueprintType)
struct FCharacterAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Strength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Endurance = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Agility = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Perception = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Intelligence = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Willpower = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = 0.0))
	float Charisma = 10.0f;

	bool operator==(const FCharacterAttributes& Other) const
	{
		return Strength == Other.Strength && Endurance == Other.Endurance && Agility == Other.Agility
			&& Perception == Other.Perception && Intelligence == Other.Intelligence
			&& Willpower == Other.Willpower && Charisma == Other.Charisma;
	}

	bool operator!=(const FCharacterAttributes& Other) const { return !(*this == Other); }

	/** The lowest of the seven - what a content sweep checks, since a negative attribute is never authored on purpose */
	float GetLowest() const
	{
		return FMath::Min(FMath::Min(FMath::Min(Strength, Endurance), FMath::Min(Agility, Perception)),
			FMath::Min(FMath::Min(Intelligence, Willpower), Charisma));
	}
};

/**
 *  One particular character - this bandit, with this name, this health, this pack - for as long
 *  as the campaign lasts. The **record** layer of the definition / record / actor model in
 *  game-data.md, and the truth about the character: the AStrategyUnit standing in the level is a
 *  working copy of this, not the other way round.
 *
 *  Two halves, written by different people:
 *
 *    Identity  - RecordId, DefinitionId, Name, FactionId, Attributes. Set when the record is
 *                created and copied *onto* the actor; nothing on the actor ever writes them back.
 *    Condition - Health, LifeState, LastKnownLocation, Carried, Equipped. The actor's components
 *                play these out, and the actor writes them back whenever one of them changes.
 *
 *  A plain reflected struct holding its definition by id, so whatever the save system decides to
 *  do can serialize it unchanged. **Known exception:** Carried and Equipped hold FInventoryItem,
 *  which still points at its item definition by TObjectPtr rather than by id - see game-data.md's
 *  Known Gaps; that has to change before saving is real.
 *
 *  Server-owned and not replicated - see UCharacterRecordComponent.
 */
USTRUCT(BlueprintType)
struct FCharacterRecord
{
	GENERATED_BODY()

	/** This individual, forever. For a unit placed in a level it is the actor's authored PlacedRecordId. */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Identity")
	FGuid RecordId;

	/** What kind of character this is - a UCharacterDefinition's DefinitionId. None for a unit authored with no definition. */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Identity")
	FName DefinitionId;

	/** Authored for a unique character, rolled from the definition's NamePool (or given by the placed actor) for everyone else */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Identity")
	FText Name;

	/** Which faction this character belongs to, or None. Nothing reads it for behaviour yet - see factions.md. */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Identity")
	FName FactionId;

	/** Current attributes. Starts as the definition's BaseAttributes; will drift from them once anything drives drift. */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Identity")
	FCharacterAttributes Attributes;

	/** Current health, as last written back by the actor */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Condition")
	float Health = 0.0f;

	/** Alive / Downed / Dead - the same state machine as UHealthComponent, reused rather than mirrored */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Condition")
	EHealthState LifeState = EHealthState::Alive;

	/** Where the character was when the actor last wrote back - on arrival from a move, and on every other write-back */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Condition")
	FVector LastKnownLocation = FVector::ZeroVector;

	/** The carried grid's placed entries, ids and anchors included */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Condition")
	TArray<FInventoryEntry> Carried;

	/** The worn slots */
	UPROPERTY(BlueprintReadOnly, Category = "Character|Condition")
	TArray<FEquippedItem> Equipped;

	/** True if this is a real record rather than a "not found" result */
	bool IsValid() const { return RecordId.IsValid(); }
};
