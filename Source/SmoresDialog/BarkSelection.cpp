// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "BarkSelection.h"
#include "DialogLibrary.h"
#include "DialogFacts.h"

void FBarkMemory::Record(const FBarkLine& Line, FObjectKey Speaker, double Now)
{
	LastSaidByLine.Add(Line.Id, Now);
	LastSaidBySpeaker.Add(TPair<FObjectKey, FName>(Speaker, Line.Id), Now);
	LastSpokeBySpeaker.Add(Speaker, Now);
}

bool FBarkMemory::IsOnCooldown(const FBarkLine& Line, FObjectKey Speaker, double Now) const
{
	if (Line.CooldownSeconds <= 0.0f)
	{
		return false;
	}

	const double* LastSaid = LastSaidBySpeaker.Find(TPair<FObjectKey, FName>(Speaker, Line.Id));

	return LastSaid && Now - *LastSaid < Line.CooldownSeconds;
}

double FBarkMemory::GetLastSaid(FName LineId) const
{
	const double* LastSaid = LastSaidByLine.Find(LineId);

	return LastSaid ? *LastSaid : TNumericLimits<double>::Lowest();
}

double FBarkMemory::GetLastSpoke(FObjectKey Speaker) const
{
	const double* LastSpoke = LastSpokeBySpeaker.Find(Speaker);

	return LastSpoke ? *LastSpoke : TNumericLimits<double>::Lowest();
}

void FBarkMemory::Reset()
{
	LastSaidByLine.Reset();
	LastSaidBySpeaker.Reset();
	LastSpokeBySpeaker.Reset();
}

TArray<FString> FBarkSelection::Explain() const
{
	TArray<FString> Lines;

	auto Describe = [](const FBarkCandidate& Candidate) -> FString
	{
		const FBarkLine& Line = *Candidate.Line;
		const FString Where = FString::Printf(TEXT("%s (%s:%d, %d clauses)"), *Line.Id.ToString(), *Line.File, Line.Line, Line.Condition.GetSpecificity());

		switch (Candidate.Outcome)
		{
		case EBarkCandidateOutcome::Won:
			return FString::Printf(TEXT("WON     %s \"%s\""), *Where, *Line.SourceText);

		case EBarkCandidateOutcome::ConditionFailed:
			return FString::Printf(TEXT("failed  %s - '%s' didn't hold"), *Where,
				Line.Condition.Clauses.IsValidIndex(Candidate.FailedClause) ? *Line.Condition.Clauses[Candidate.FailedClause].Text : TEXT("?"));

		case EBarkCandidateOutcome::OnCooldown:
			return FString::Printf(TEXT("waiting %s - this speaker said it within its %.0fs cooldown"), *Where, Line.CooldownSeconds);

		case EBarkCandidateOutcome::LessSpecific:
			return FString::Printf(TEXT("matched %s - but a line with more clauses also matched"), *Where);

		case EBarkCandidateOutcome::SaidMoreRecently:
			return FString::Printf(TEXT("matched %s - equally specific, but said more recently"), *Where);

		case EBarkCandidateOutcome::LostWeightedPick:
		default:
			return FString::Printf(TEXT("matched %s - tied, and the weighted pick (weight %d) went elsewhere"), *Where, Line.Weight);
		}
	};

	for (const FBarkCandidate& Candidate : Candidates)
	{
		if (Candidate.Outcome == EBarkCandidateOutcome::Won)
		{
			Lines.Add(Describe(Candidate));
		}
	}

	for (const FBarkCandidate& Candidate : Candidates)
	{
		if (Candidate.Outcome != EBarkCandidateOutcome::Won)
		{
			Lines.Add(Describe(Candidate));
		}
	}

	return Lines;
}

namespace SmoresDialog
{
	const FBarkLine* SelectBark(
		const TArray<const FBarkLine*>& Lines,
		const FDialogContext& Context,
		const FDialogFactRegistry& Facts,
		const FBarkMemory& Memory,
		FObjectKey Speaker,
		double Now,
		FRandomStream& Stream,
		FBarkSelection* OutSelection)
	{
		TArray<FBarkCandidate> Candidates;
		Candidates.Reserve(Lines.Num());

		int32 BestSpecificity = -1;

		for (const FBarkLine* Line : Lines)
		{
			if (!Line)
			{
				continue;
			}

			FBarkCandidate Candidate;
			Candidate.Line = Line;

			if (!Line->Condition.Evaluate(Context, Facts, &Candidate.FailedClause))
			{
				Candidate.Outcome = EBarkCandidateOutcome::ConditionFailed;
			}
			else if (Memory.IsOnCooldown(*Line, Speaker, Now))
			{
				Candidate.Outcome = EBarkCandidateOutcome::OnCooldown;
			}
			else
			{
				// provisionally in the running; narrowed below
				Candidate.Outcome = EBarkCandidateOutcome::LostWeightedPick;
				BestSpecificity = FMath::Max(BestSpecificity, Line->Condition.GetSpecificity());
			}

			Candidates.Add(Candidate);
		}

		// most clauses wins
		double OldestSaid = TNumericLimits<double>::Max();

		for (FBarkCandidate& Candidate : Candidates)
		{
			if (Candidate.Outcome != EBarkCandidateOutcome::LostWeightedPick)
			{
				continue;
			}

			if (Candidate.Line->Condition.GetSpecificity() < BestSpecificity)
			{
				Candidate.Outcome = EBarkCandidateOutcome::LessSpecific;
				continue;
			}

			OldestSaid = FMath::Min(OldestSaid, Memory.GetLastSaid(Candidate.Line->Id));
		}

		// then the least recently said, by anyone
		int32 TotalWeight = 0;

		for (FBarkCandidate& Candidate : Candidates)
		{
			if (Candidate.Outcome != EBarkCandidateOutcome::LostWeightedPick)
			{
				continue;
			}

			if (Memory.GetLastSaid(Candidate.Line->Id) > OldestSaid)
			{
				Candidate.Outcome = EBarkCandidateOutcome::SaidMoreRecently;
				continue;
			}

			TotalWeight += FMath::Max(1, Candidate.Line->Weight);
		}

		// then weight - one draw across what's left, walked in load order so the same stream always
		// lands on the same line
		FBarkCandidate* Winner = nullptr;

		if (TotalWeight > 0)
		{
			int32 Roll = Stream.RandRange(0, TotalWeight - 1);

			for (FBarkCandidate& Candidate : Candidates)
			{
				if (Candidate.Outcome != EBarkCandidateOutcome::LostWeightedPick)
				{
					continue;
				}

				Roll -= FMath::Max(1, Candidate.Line->Weight);

				if (Roll < 0)
				{
					Winner = &Candidate;
					break;
				}
			}
		}

		if (Winner)
		{
			Winner->Outcome = EBarkCandidateOutcome::Won;
		}

		const FBarkLine* WinningLine = Winner ? Winner->Line : nullptr;

		if (OutSelection)
		{
			OutSelection->Winner = WinningLine;
			OutSelection->Candidates = MoveTemp(Candidates);
		}

		return WinningLine;
	}
}
