// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"
#include "Math/RandomStream.h"
#include "DialogTypes.h"

struct FBarkLine;
class FDialogFactRegistry;

/**
 *  What barks have been said this session: when each line was last said by anyone, and when each
 *  speaker last said each line.
 *
 *  **Transient, never saved** - the roadmap's settled call. A reload that forgot who just spoke
 *  costs one repeated line. Keyed by FObjectKey rather than a pointer, so a speaker who has gone
 *  away is simply never asked about again instead of dangling.
 */
struct SMORESDIALOG_API FBarkMemory
{
	/** Notes that Speaker just said Line */
	void Record(const FBarkLine& Line, FObjectKey Speaker, double Now);

	/** True while Speaker must wait before saying Line again. Per speaker, so two guards can still say the same thing. */
	bool IsOnCooldown(const FBarkLine& Line, FObjectKey Speaker, double Now) const;

	/** When anyone last said this line, or the lowest double if never - never-said lines are the most rested of all */
	double GetLastSaid(FName LineId) const;

	/** When this speaker last said anything, or the lowest double if never */
	double GetLastSpoke(FObjectKey Speaker) const;

	/** Forgets everything - after a reload, since the lines themselves may have changed */
	void Reset();

private:

	TMap<FName, double> LastSaidByLine;

	TMap<TPair<FObjectKey, FName>, double> LastSaidBySpeaker;

	TMap<FObjectKey, double> LastSpokeBySpeaker;
};

/** What happened to one candidate line during a selection - the "why" SmoresTestBark prints */
enum class EBarkCandidateOutcome : uint8
{
	/** A clause didn't hold */
	ConditionFailed,

	/** This speaker said it too recently */
	OnCooldown,

	/** It matched, but another matching line had more clauses */
	LessSpecific,

	/** Equally specific, but said more recently than another */
	SaidMoreRecently,

	/** Tied all the way down, and the weighted pick went elsewhere */
	LostWeightedPick,

	Won
};

struct SMORESDIALOG_API FBarkCandidate
{
	const FBarkLine* Line = nullptr;

	EBarkCandidateOutcome Outcome = EBarkCandidateOutcome::ConditionFailed;

	/** For ConditionFailed, which clause failed */
	int32 FailedClause = INDEX_NONE;
};

/** One selection's full account: every line considered, and what became of it */
struct SMORESDIALOG_API FBarkSelection
{
	const FBarkLine* Winner = nullptr;

	TArray<FBarkCandidate> Candidates;

	/** A readable account, one line per candidate, winner first */
	TArray<FString> Explain() const;
};

namespace SmoresDialog
{
	/**
	 *  Picks the line to say, or null if none may be said.
	 *
	 *  **Most clauses matched wins.** Of the lines whose conditions all hold and which this speaker
	 *  isn't waiting out a cooldown on, the ones with the most clauses are the candidates. A tie
	 *  goes to whichever was said least recently by anyone (never-said first), and a tie after that
	 *  is a weighted pick from Stream - which is the only time Weight matters.
	 *
	 *  Pure: reads Memory, never writes it. The caller records the winner, so a test can select
	 *  twice from the same state and a dry run changes nothing.
	 */
	SMORESDIALOG_API const FBarkLine* SelectBark(
		const TArray<const FBarkLine*>& Lines,
		const FDialogContext& Context,
		const FDialogFactRegistry& Facts,
		const FBarkMemory& Memory,
		FObjectKey Speaker,
		double Now,
		FRandomStream& Stream,
		FBarkSelection* OutSelection = nullptr);
}
