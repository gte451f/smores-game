// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresStrategyTestActors.h"
#include "StrategyTargetActions.h"

ATestStrategyNPC::ATestStrategyNPC()
{
	// a test world has no navmesh and wants no AI controller; none of the rules under test
	// involve either, and spawning one is cost and noise for nothing
	AutoPossessAI = EAutoPossessAI::Disabled;
}

ATestStrategyPlayerUnit::ATestStrategyPlayerUnit()
{
	AutoPossessAI = EAutoPossessAI::Disabled;
}

bool UTestActionOrderHost::CanPerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId, ESmoresRefusalReason& OutReason)
{
	++CanPerformCount;

	TArray<AStrategyUnit*> SquadUnits;

	for (const TWeakObjectPtr<AStrategyUnit>& Member : Squad)
	{
		if (AStrategyUnit* Unit = Member.Get())
		{
			SquadUnits.Add(Unit);
		}
	}

	// the controller's arrival check, verbatim - see AStrategyPlayerController::CanPerformAction
	FTargetAction Entry;

	if (!FStrategyTargetActions::FindActionFor(Target, Actor, ActionId, SquadUnits, Entry))
	{
		OutReason = ESmoresRefusalReason::NotInteractable;
		return false;
	}

	OutReason = Entry.DisabledReason;

	return Entry.bEnabled;
}

void UTestActionOrderHost::PerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId)
{
	++PerformCount;
	LastPerformedAction = ActionId;
}

void UTestActionOrderHost::HandleActionOrderEnded(AStrategyUnit* Actor, AActor* Target, const FText& TargetName, FName ActionId, EActionOrderEnd Why, ESmoresRefusalReason Reason)
{
	++EndedCount;
	LastEnd = Why;
	LastReason = Reason;
}
