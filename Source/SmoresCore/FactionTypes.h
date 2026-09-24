// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FactionTypes.generated.h"

/**
 *  How much territory a faction holds - the three kinds of territorial standing from the
 *  game-design skill's factions-and-world-state.md.
 *
 *  **This is a state, not an identity.** A Minor faction that seizes a town becomes Major; a
 *  Major one that loses every town drops to Minor or Nomadic. So the definition asset only holds
 *  the tier a faction *starts* a campaign at, and the current tier lives in its FFactionRecord.
 *  Nothing moves a faction between tiers yet - that is the world-activity roadmap's job.
 */
UENUM(BlueprintType)
enum class EFactionTier : uint8
{
	/** Holds at least one town - the dominant political and military powers */
	Major		UMETA(DisplayName = "Major"),
	/** Holds an outpost or minor settlement but no town - bandit clans, local guilds */
	Minor		UMETA(DisplayName = "Minor"),
	/** Holds no fixed territory at all - trader guilds, criminal enterprises, wanderers */
	Nomadic		UMETA(DisplayName = "Nomadic")
};

/**
 *  Where a faction sits between mono-lineage and cosmopolitan (characters-and-squads.md's
 *  Lineage section). Unlike the tier, this *is* identity - static, authored, and the same every
 *  playthrough, so a player can learn it and exploit it.
 *
 *  Stored and read by nothing yet. What a stance actually does - refusing trade, suspicion,
 *  lineage-gated recruits - is behavior, and arrives with the systems that act on it.
 */
UENUM(BlueprintType)
enum class ELineageStance : uint8
{
	/** Recruits and trades freely across every lineage */
	Cosmopolitan	UMETA(DisplayName = "Cosmopolitan"),
	/** Deals with other lineages, but warily - worse prices, closer watch */
	Guarded			UMETA(DisplayName = "Guarded"),
	/** Distrusts, refuses or persecutes lineages other than its own */
	MonoLineage		UMETA(DisplayName = "Mono-Lineage")
};

/**
 *  The standing scale shared by faction-to-faction and player-to-faction standing, so the two
 *  can never quietly disagree about what "hostile" means numerically.
 *
 *  Whole numbers from -100 to 100 with 0 as neutral. Every write clamps into that range rather
 *  than refusing, so a large penalty on an already-hated faction lands at the floor instead of
 *  being thrown away.
 */
namespace SmoresStanding
{
	constexpr int32 Min = -100;
	constexpr int32 Max = 100;

	/** What any query about a faction nobody has an opinion of yet answers - including an id that doesn't exist */
	constexpr int32 Neutral = 0;

	inline int32 Clamp(int32 Value) { return FMath::Clamp(Value, Min, Max); }
}

/**
 *  One faction's campaign state - the record half of the definition/record/actor model (see
 *  game-systems' game-data.md). The definition says what Ironclan *is*; this says what Ironclan
 *  is *right now*.
 *
 *  Deliberately small: only what can already differ from the definition. It references its
 *  definition by id, never by asset pointer, so a save naming a mod's faction still loads once
 *  the mod is gone.
 */
USTRUCT(BlueprintType)
struct FFactionRecord
{
	GENERATED_BODY()

	/** Which faction this is - the DefinitionId of its UFactionDefinition */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	FName FactionId;

	/** How much territory it holds today. Starts at the definition's StartingTier and drifts from there. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	EFactionTier Tier = EFactionTier::Minor;
};

/**
 *  One cell of the faction-to-faction standing matrix.
 *
 *  **Standing between factions is symmetric**, so a pair is stored once, under its two ids in
 *  lexical order - FirstId always sorts before SecondId. That is what the "ordered id pair" key
 *  means: whichever way round a caller asks, the same cell answers. Lexical order rather than
 *  FName's own comparison because FName compares by table index, which differs between runs and
 *  machines and would put a saved pair under a different key when it loaded.
 */
USTRUCT(BlueprintType)
struct FFactionPairStanding
{
	GENERATED_BODY()

	/** The lexically smaller of the two faction ids */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	FName FirstId;

	/** The lexically larger of the two faction ids */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	FName SecondId;

	/** How the two regard each other, on the SmoresStanding scale */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	int32 Standing = SmoresStanding::Neutral;
};

/** One player's standing with one faction, on the SmoresStanding scale */
USTRUCT(BlueprintType)
struct FPlayerFactionStanding
{
	GENERATED_BODY()

	/** The DefinitionId of the faction this is standing with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	FName FactionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
	int32 Standing = SmoresStanding::Neutral;
};
