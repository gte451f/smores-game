// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ApproachTracker.h"

TArray<FApproach> FApproachTracker::Update(const TArray<FApproachSpeaker>& Speakers, const TArray<FApproachSquad>& Squads, float Range, double Now, double CooldownSeconds)
{
	TArray<FApproach> Approaches;

	++UpdateCount;

	const double RangeSquared = FMath::Square(static_cast<double>(Range));

	for (int32 SpeakerIndex = 0; SpeakerIndex < Speakers.Num(); ++SpeakerIndex)
	{
		const FApproachSpeaker& Speaker = Speakers[SpeakerIndex];

		// squad members are never approached - not by their own squad, not by anyone else's
		if (Speaker.bIsSquadMember)
		{
			continue;
		}

		for (int32 SquadIndex = 0; SquadIndex < Squads.Num(); ++SquadIndex)
		{
			const FApproachSquad& Squad = Squads[SquadIndex];

			// the nearest member inside range, who becomes the listener if this is an approach
			int32 NearestMember = INDEX_NONE;
			double NearestDistanceSquared = RangeSquared;

			for (int32 MemberIndex = 0; MemberIndex < Squad.MemberLocations.Num(); ++MemberIndex)
			{
				const double DistanceSquared = FVector::DistSquared(Squad.MemberLocations[MemberIndex], Speaker.Location);

				if (DistanceSquared <= NearestDistanceSquared)
				{
					NearestMember = MemberIndex;
					NearestDistanceSquared = DistanceSquared;
				}
			}

			const bool bInside = NearestMember != INDEX_NONE;
			const TPair<FObjectKey, FObjectKey> PairKey(Speaker.Key, Squad.Player);

			FPairState* State = Pairs.Find(PairKey);

			// a pair with no entry is outside and free to fire, and staying outside keeps it that way
			if (!State)
			{
				if (!bInside)
				{
					continue;
				}

				State = &Pairs.Add(PairKey);
			}

			const bool bWasInside = State->bInside;

			State->bInside = bInside;
			State->LastSeenUpdate = UpdateCount;

			// the way in only - and to someone on their feet, and not inside the cooldown. An arrival
			// that fails either test is still an arrival: the squad is inside now, so nothing fires
			// until they leave and come back.
			const bool bArrived = bInside && !bWasInside;
			const bool bOffCooldown = !State->bHasRaised || Now - State->LastRaisedTime >= CooldownSeconds;

			if (bArrived && Speaker.bCanSpeak && bOffCooldown)
			{
				State->bHasRaised = true;
				State->LastRaisedTime = Now;

				FApproach& Approach = Approaches.AddDefaulted_GetRef();
				Approach.SpeakerIndex = SpeakerIndex;
				Approach.SquadIndex = SquadIndex;
				Approach.MemberIndex = NearestMember;
			}
		}
	}

	// keep only what differs from having no entry at all: forget anyone not in this update (a
	// destroyed NPC, a player who left), and any pair that is outside with its cooldown run out
	for (auto It = Pairs.CreateIterator(); It; ++It)
	{
		const FPairState& State = It.Value();
		const bool bUnseen = State.LastSeenUpdate != UpdateCount;
		const bool bSettled = !State.bInside && (!State.bHasRaised || Now - State.LastRaisedTime >= CooldownSeconds);

		if (bUnseen || bSettled)
		{
			It.RemoveCurrent();
		}
	}

	return Approaches;
}

void FApproachTracker::Reset()
{
	Pairs.Reset();
}
