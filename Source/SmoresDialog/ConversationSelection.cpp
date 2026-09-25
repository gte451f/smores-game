// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ConversationSelection.h"
#include "DialogCondition.h"
#include "DialogFacts.h"
#include "DialogMemoryComponent.h"

namespace
{
	/** Highest priority first, then load order - the order the topic list reads in */
	bool IsOfferedBefore(const FConversationDefinition& A, const FConversationDefinition& B)
	{
		return A.Priority != B.Priority ? A.Priority > B.Priority : A.LoadOrder < B.LoadOrder;
	}
}

namespace SmoresDialog
{
	bool IsConversationEligible(const FConversationDefinition& Conversation, const FDialogContext& Context, const FDialogFactRegistry& Facts, const FDialogMemoryRecord& Memory, FString* OutWhyNot)
	{
		int32 FailedClause = INDEX_NONE;

		if (!Conversation.Attach.Evaluate(Context, Facts, &FailedClause))
		{
			if (OutWhyNot)
			{
				*OutWhyNot = FString::Printf(TEXT("attach: '%s' doesn't hold"), *Conversation.Attach.Clauses[FailedClause].Text);
			}

			return false;
		}

		if (!Conversation.Requires.Evaluate(Context, Facts, &FailedClause))
		{
			if (OutWhyNot)
			{
				*OutWhyNot = FString::Printf(TEXT("requires: '%s' doesn't hold"), *Conversation.Requires.Clauses[FailedClause].Text);
			}

			return false;
		}

		if (Conversation.bOnce && Memory.FindSeen(Conversation.Id))
		{
			if (OutWhyNot)
			{
				*OutWhyNot = TEXT("once: this squad has seen it");
			}

			return false;
		}

		return true;
	}

	TArray<const FConversationDefinition*> GetEligibleConversations(const TArray<const FConversationDefinition*>& Candidates, const FDialogContext& Context, const FDialogFactRegistry& Facts, const FDialogMemoryRecord& Memory)
	{
		TArray<const FConversationDefinition*> Eligible;

		for (const FConversationDefinition* Candidate : Candidates)
		{
			if (Candidate && IsConversationEligible(*Candidate, Context, Facts, Memory))
			{
				Eligible.Add(Candidate);
			}
		}

		Eligible.Sort([](const FConversationDefinition& A, const FConversationDefinition& B)
		{
			return IsOfferedBefore(A, B);
		});

		return Eligible;
	}

	const FConversationDefinition* SelectConversation(const TArray<const FConversationDefinition*>& Candidates, const FDialogContext& Context, const FDialogFactRegistry& Facts, const FDialogMemoryRecord& Memory, TArray<FString>* OutExplanation)
	{
		const FConversationDefinition* Best = nullptr;
		int32 BestSeenOrder = 0;

		for (const FConversationDefinition* Candidate : Candidates)
		{
			if (!Candidate)
			{
				continue;
			}

			FString WhyNot;

			if (!IsConversationEligible(*Candidate, Context, Facts, Memory, &WhyNot))
			{
				if (OutExplanation)
				{
					OutExplanation->Add(FString::Printf(TEXT("  %s: not eligible - %s"), *Candidate->Id.ToString(), *WhyNot));
				}

				continue;
			}

			// never seen is order 0, older than anything seen
			const FDialogSeenEntry* Seen = Memory.FindSeen(Candidate->Id);
			const int32 SeenOrder = Seen ? Seen->LastSeenOrder : 0;

			if (OutExplanation)
			{
				OutExplanation->Add(FString::Printf(TEXT("  %s: eligible, priority %d, %s"), *Candidate->Id.ToString(), Candidate->Priority,
					Seen ? *FString::Printf(TEXT("seen %d time%s"), Seen->TimesSeen, Seen->TimesSeen == 1 ? TEXT("") : TEXT("s")) : TEXT("never seen")));
			}

			const bool bBetter = !Best
				|| Candidate->Priority > Best->Priority
				|| (Candidate->Priority == Best->Priority && SeenOrder < BestSeenOrder)
				|| (Candidate->Priority == Best->Priority && SeenOrder == BestSeenOrder && Candidate->LoadOrder < Best->LoadOrder);

			if (bBetter)
			{
				Best = Candidate;
				BestSeenOrder = SeenOrder;
			}
		}

		if (OutExplanation)
		{
			OutExplanation->Add(Best ? FString::Printf(TEXT("  => %s"), *Best->Id.ToString()) : FString(TEXT("  => none")));
		}

		return Best;
	}

	bool GetConversationInterruption(const FConversationWatch& Watch, EConversationEndReason& OutReason)
	{
		if (Watch.bSomeoneGone)
		{
			OutReason = EConversationEndReason::WalkedAway;
			return true;
		}

		if (Watch.bNpcDown || Watch.bSquadMemberDown)
		{
			OutReason = EConversationEndReason::Downed;
			return true;
		}

		if (Watch.bNpcHostile || Watch.bEitherAttacking || Watch.bEitherHurt)
		{
			OutReason = EConversationEndReason::Combat;
			return true;
		}

		if (Watch.bCheckRange && Watch.Distance > Watch.BreakOffRange)
		{
			OutReason = EConversationEndReason::WalkedAway;
			return true;
		}

		return false;
	}
}
