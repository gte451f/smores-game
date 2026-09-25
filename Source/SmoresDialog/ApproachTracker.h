// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"

/** Someone who might be approached - one per unit in the world, NPCs and squad members alike */
struct SMORESDIALOG_API FApproachSpeaker
{
	/** Who this is. Only ever compared, never resolved. */
	FObjectKey Key;

	FVector Location = FVector::ZeroVector;

	/** On their feet. A Downed or Dead NPC never greets anyone, and doesn't when they get up either. */
	bool bCanSpeak = true;

	/** One of a player's squad. Squad members never raise Approached - not at each other, not at anyone. */
	bool bIsSquadMember = false;
};

/** One player's squad: whose it is, and where each member on their feet is standing */
struct SMORESDIALOG_API FApproachSquad
{
	/** The player - a player state in play. Approaches are tracked per player, so one squad's doesn't use up another's. */
	FObjectKey Player;

	TArray<FVector> MemberLocations;
};

/** An approach that should raise the event: indices into the arrays FApproachTracker::Update was given */
struct SMORESDIALOG_API FApproach
{
	int32 SpeakerIndex = INDEX_NONE;

	int32 SquadIndex = INDEX_NONE;

	/** The squad member who came near - the nearest one inside range. They are the event's Listener. */
	int32 MemberIndex = INDEX_NONE;
};

/**
 *  Decides when a squad coming near an NPC counts as an approach - the edge-trigger behind the
 *  Approached bark, as a plain value type so the rules are testable without a world.
 *
 *  For each NPC and each player it remembers whether that player's squad is inside range, and when
 *  the NPC last had an approach from them. An approach is:
 *
 *  - **the way in**: the squad was outside range (or has never been seen) and is now inside it.
 *    Standing inside is not an approach, however long it lasts;
 *  - **by a player, to an NPC on their feet**: a squad member is never approached, and an NPC who
 *    is down when the squad arrives doesn't greet them on getting up - the squad has to leave and
 *    come back;
 *  - **not again for ApproachCooldownSeconds** after the last one from that player, *and* only once
 *    the squad has left and come back. Both, so a squad that arrived during the cooldown and is
 *    still standing there when it runs out isn't greeted mid-conversation.
 *
 *  An approach counts from the moment it is raised, whether or not the NPC then said anything - the
 *  director's quiet time and each line's cooldown still apply on top, like every other bark.
 *
 *  **Transient**, like the rest of the bark director's memory: never saved, and nothing but the
 *  director reads it. It keeps only pairs that differ from "outside and free to fire", so it holds
 *  roughly the NPCs someone is standing near, and forgets anyone missing from an update.
 *
 *  Straight-line distance, so it notices through walls exactly as HearingRange does. This is the
 *  seam real perception replaces (ai-and-behavior.md's Awareness): whatever decides "the squad is
 *  inside" can change without the rules here changing.
 */
class SMORESDIALOG_API FApproachTracker
{
public:

	/**
	 *  One check. Speakers and Squads are everyone there is right now; Range is ApproachRange in cm,
	 *  and Now and CooldownSeconds are world seconds. Returns the approaches to raise, in speaker
	 *  order - and records them, so the same arrival never comes back from a later call.
	 */
	TArray<FApproach> Update(const TArray<FApproachSpeaker>& Speakers, const TArray<FApproachSquad>& Squads, float Range, double Now, double CooldownSeconds);

	/** Forgets everything - every squad counts as outside, and nobody is on cooldown */
	void Reset();

	/** How many NPC-and-player pairs are remembered. For tests: memory stays bounded by who is near whom. */
	int32 GetTrackedCount() const { return Pairs.Num(); }

private:

	struct FPairState
	{
		/** The squad was inside range at the last check */
		bool bInside = false;

		/** An approach has been raised for this pair, at LastRaisedTime */
		bool bHasRaised = false;

		double LastRaisedTime = 0.0;

		/** The update this pair was last seen in - anything not seen in the latest one is forgotten */
		uint32 LastSeenUpdate = 0;
	};

	/** Keyed by (speaker, player) */
	TMap<TPair<FObjectKey, FObjectKey>, FPairState> Pairs;

	uint32 UpdateCount = 0;
};
