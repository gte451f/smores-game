// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ActionOrderComponent.h"
#include "ActionOrderHost.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "SmoresInteractable.h"
#include "SmoresCharacters.h"

UActionOrderComponent::UActionOrderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AStrategyUnit* UActionOrderComponent::GetUnit() const
{
	return Cast<AStrategyUnit>(GetOwner());
}

IActionOrderHost* UActionOrderComponent::GetHost() const
{
	if (HostOverride)
	{
		return Cast<IActionOrderHost>(HostOverride);
	}

	// only a player's own pawn has someone to answer to today - an NPC given an order would walk
	// over and then find nobody to act for it, which is fine until AI orders exist
	const AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(GetOwner());

	return PlayerUnit ? Cast<IActionOrderHost>(PlayerUnit->GetOwningController()) : nullptr;
}

bool UActionOrderComponent::IsTargetInReach() const
{
	const AActor* CurrentTarget = Target.Get();
	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(CurrentTarget);

	// the target's own reach, not a figure of this component's - the same gate every transfer and
	// every panel entry has always used, so "close enough to act" can't mean two things
	return Interactable && Interactable->IsInRangeOf(GetOwner());
}

bool UActionOrderComponent::IssueOrder(AActor* InTarget, FName InActionId)
{
	AStrategyUnit* Unit = GetUnit();

	if (!Unit || !Unit->HasAuthority() || Unit->IsIncapacitated())
	{
		return false;
	}

	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(InTarget);

	if (!IsValid(InTarget) || !Interactable || InActionId.IsNone())
	{
		return false;
	}

	// whatever came before is replaced without a word - this is the player giving a new order
	CancelOrder();

	// ...and so is whatever else the unit was doing: a pending move query, a walk, a fight. Done
	// before the order's own state is set, because stopping a walk reports its end synchronously
	// and nothing should read that as this order's walk arriving.
	Unit->TakeOverForActionOrder();

	Target = InTarget;
	ActionId = InActionId;
	TargetName = Interactable->GetInteractionDisplayName();
	RetryCount = 0;
	bHasOrder = true;

	UE_LOG(LogSmoresCharacters, Log, TEXT("[Orders] %s: %s %s"), *Unit->GetName(), *ActionId.ToString(), *GetNameSafe(InTarget));

	if (IsTargetInReach())
	{
		Act();
	}
	else
	{
		StartApproach();
	}

	return true;
}

void UActionOrderComponent::CancelOrder()
{
	if (!bHasOrder)
	{
		return;
	}

	ClearOrder();

	// stops the walk if this order's walk is the one running. Anything that cancels also starts
	// something of its own, but an attack in range starts nothing that would stop it.
	if (AStrategyUnit* Unit = GetUnit())
	{
		Unit->StopActionApproach();
	}
}

void UActionOrderComponent::AbandonOrder(EActionOrderEnd Why)
{
	if (!bHasOrder)
	{
		return;
	}

	AStrategyUnit* Unit = GetUnit();
	AActor* OrderTarget = Target.Get();
	const FName OrderAction = ActionId;
	const FText OrderTargetName = TargetName;

	ClearOrder();

	if (Unit)
	{
		Unit->StopActionApproach();
	}

	UE_LOG(LogSmoresCharacters, Log, TEXT("[Orders] %s: %s %s abandoned (%s)"),
		*GetNameSafe(Unit), *OrderAction.ToString(), *OrderTargetName.ToString(), *UEnum::GetValueAsString(Why));

	if (IActionOrderHost* Host = GetHost())
	{
		const ESmoresRefusalReason Reason = (Why == EActionOrderEnd::CannotReach) ? ESmoresRefusalReason::CannotReach : ESmoresRefusalReason::None;

		Host->HandleActionOrderEnded(Unit, OrderTarget, OrderTargetName, OrderAction, Why, Reason);
	}
}

void UActionOrderComponent::HandleApproachFinished()
{
	if (!bHasOrder)
	{
		return;
	}

	ArriveOrRetry();
}

void UActionOrderComponent::StartApproach()
{
	AStrategyUnit* Unit = GetUnit();
	AActor* CurrentTarget = Target.Get();

	if (!Unit || !CurrentTarget)
	{
		AbandonOrder(EActionOrderEnd::TargetGone);
		return;
	}

	const EActionApproachResult Result = ApproachOverride
		? ApproachOverride(CurrentTarget)
		: Unit->MoveToActor(CurrentTarget, ApproachAcceptanceRadius);

	switch (Result)
	{
	case EActionApproachResult::Walking:
		// the unit reports back through HandleApproachFinished
		break;

	case EActionApproachResult::AlreadyThere:
		// no walk means no report - trap 3 in the action-menu roadmap. Without this, a unit
		// standing as close as it can get waits forever.
		ArriveOrRetry();
		break;

	case EActionApproachResult::Failed:
	default:
		AbandonOrder(EActionOrderEnd::CannotReach);
		break;
	}
}

void UActionOrderComponent::ArriveOrRetry()
{
	if (!Target.IsValid())
	{
		AbandonOrder(EActionOrderEnd::TargetGone);
		return;
	}

	if (IsTargetInReach())
	{
		Act();
		return;
	}

	// out of reach: a partial path, or a target that walked off while the path was being followed.
	// A fresh walk from wherever the unit ended up is what gets round both - but only a few times,
	// or a shut door would have the unit trying forever.
	if (RetryCount < MaxRetries)
	{
		++RetryCount;

		StartApproach();
		return;
	}

	AbandonOrder(EActionOrderEnd::CannotReach);
}

void UActionOrderComponent::Act()
{
	AStrategyUnit* Unit = GetUnit();
	AActor* OrderTarget = Target.Get();
	const FName OrderAction = ActionId;
	const FText OrderTargetName = TargetName;

	if (!OrderTarget)
	{
		AbandonOrder(EActionOrderEnd::TargetGone);
		return;
	}

	// over before the host hears about it, so anything the action starts - a conversation, a new
	// move - can't find this order still pending and end it a second time
	ClearOrder();

	IActionOrderHost* Host = GetHost();

	if (!Unit || !Host)
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("[Orders] %s reached %s with nobody to act for - the order is dropped."),
			*GetNameSafe(Unit), *GetNameSafe(OrderTarget));
		return;
	}

	// turn to face what is being acted on - somebody talking to your back reads as a glitch.
	// FaceToward skips the unit itself (treating its own wounds) and anyone down.
	Unit->FaceToward(OrderTarget);

	// asked again here, through the same rules that offered the action - the target may have
	// turned hostile, gone down or closed on the way over
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	if (!Host->CanPerformAction(Unit, OrderTarget, OrderAction, Reason))
	{
		Host->HandleActionOrderEnded(Unit, OrderTarget, OrderTargetName, OrderAction, EActionOrderEnd::Refused, Reason);
		return;
	}

	Host->PerformAction(Unit, OrderTarget, OrderAction);
}

void UActionOrderComponent::ClearOrder()
{
	bHasOrder = false;
	Target = nullptr;
	ActionId = NAME_None;
	TargetName = FText::GetEmpty();
	RetryCount = 0;
}
