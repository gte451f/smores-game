// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/Function.h"
#include "ActionOrderHost.h"
#include "ActionOrderComponent.generated.h"

class AStrategyUnit;
class IActionOrderHost;

/** How a request to walk over to a target went, at the moment it was asked */
enum class EActionApproachResult : uint8
{
	/** Setting off - the unit reports back when the walk ends */
	Walking,

	/** Already as close as the walk would get it. No walk, and no report: the order checks reach at once. */
	AlreadyThere,

	/** No walk could start at all - no controller, no navigation, no path */
	Failed
};

/**
 *  One squad member's "walk over there and do this" order - the order behind every
 *  right-click-menu pick, the target panel's buttons, the `T` and `O` keys and the double-click.
 *
 *  **The lifecycle.** Issue: already within reach acts at once, otherwise the unit walks toward
 *  the target, following it if it moves. Arrive: within reach acts; still out of reach (a partial
 *  path, a target that moved) walks again, up to MaxRetries times, then fails CannotReach. Act:
 *  the host re-checks the action through the same rules that offered it, then does it.
 *
 *  **Cancellation is the part to get right.**
 *
 *  - The player's own new order ends it silently - a move, an attack, another action order. The
 *    player did it; there is nothing to explain. CancelOrder.
 *  - Anything else that pulls the unit away ends it and says so - going Downed or Dead, swinging
 *    back at someone who hit it, the target ceasing to exist. A blocked job is visible, never
 *    silent (game-design's orders-and-jobs.md). AbandonOrder.
 *  - AStrategyUnit::MoveToLocation is the catch-all: every caller of it means "this unit is now
 *    doing something else", so it cancels whatever order is pending, silently.
 *
 *  **Authority only.** Nothing about an order needs replicating: what the player sees is the
 *  unit walking, which replicates as movement already. A default subobject of every
 *  AStrategyUnit, NPCs included, harmlessly - future AI orders will want the same thing, and a
 *  job queue (orders-and-jobs.md) is the obvious thing for this to grow into.
 */
UCLASS(ClassGroup = (Characters))
class SMORESCHARACTERS_API UActionOrderComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UActionOrderComponent();

	/**
	 *  Sets this unit off to do ActionId to Target, replacing any order it already had - silently,
	 *  since a new order from the player is exactly the "the player did it" case.
	 *
	 *  Server only. False, with nothing changed, if the order can't even start: no authority, the
	 *  unit is Downed or Dead, or Target isn't something that can be acted on. Whether the action
	 *  is *allowed* is the caller's check - see AStrategyPlayerController::Server_RequestActionOrder -
	 *  and is asked again on arrival.
	 */
	bool IssueOrder(AActor* Target, FName ActionId);

	/** The unit was given something else to do: the order ends without a word. Harmless with no order. */
	void CancelOrder();

	/** Something the player didn't ask for pulled the unit away: the order ends, and the host is told why. Harmless with no order. */
	void AbandonOrder(EActionOrderEnd Why);

	/**
	 *  The walk this order started has ended - arrived, blocked, or cut short. Called by the owning
	 *  unit, which filters out every other move's endings (see AStrategyUnit::OnMoveFinished).
	 *  Public so a test can stand in for a walk that can't happen without navigation.
	 */
	void HandleApproachFinished();

	/** True while an order is pending - walking over, or about to act */
	bool HasOrder() const { return bHasOrder; }

	/** The pending order's target, or null */
	AActor* GetOrderTarget() const { return Target.Get(); }

	/** The pending order's action id, or None */
	FName GetOrderActionId() const { return bHasOrder ? ActionId : NAME_None; }

	/** How many times the pending order has re-walked after arriving out of reach */
	int32 GetRetryCount() const { return RetryCount; }

	/** Test hook: answer to this object instead of the owning unit's controller. Must implement IActionOrderHost. */
	void SetHostForTest(UObject* InHost) { HostOverride = InHost; }

	/** Test hook: stand in for the walk, which needs navigation a test world doesn't have. Empty restores the real walk. */
	void SetApproachForTest(TFunction<EActionApproachResult(AActor*)> InApproach) { ApproachOverride = MoveTemp(InApproach); }

protected:

	/**
	 *  How many times an arrival out of reach walks again before giving up with CannotReach.
	 *  Each retry is a fresh path from wherever the unit ended up, which is what gets it round a
	 *  target that stepped away mid-walk.
	 */
	UPROPERTY(EditAnywhere, Category = "Action Order", meta = (ClampMin = 0, ClampMax = 10))
	int32 MaxRetries = 2;

	/**
	 *  How close the walk tries to get: the gap between this unit's edge and the target's centre.
	 *  Has to stay inside every target's reach - a unit's is 250 cm centre to centre, so 100 here
	 *  plus a capsule radius of 42 leaves room. The target's own size is deliberately left out of
	 *  the sum, because an actor's bounds can include its reach sphere and would make the walk stop
	 *  short of it.
	 */
	UPROPERTY(EditAnywhere, Category = "Action Order", meta = (ClampMin = 0, Units = "cm"))
	float ApproachAcceptanceRadius = 100.0f;

	/** The unit this is part of */
	AStrategyUnit* GetUnit() const;

	/** The test's host if one was set, else the owning unit's controller if it implements IActionOrderHost */
	IActionOrderHost* GetHost() const;

	/** True if the target is within its own reach of this unit */
	bool IsTargetInReach() const;

	/** Asks the unit to walk over (or the test's stand-in), and deals with a walk that couldn't start */
	void StartApproach();

	/** At the end of a walk: act if within reach, otherwise walk again or give up */
	void ArriveOrRetry();

	/** The order's last step: ask the host whether the action still stands, then have it done */
	void Act();

	/** Forgets the pending order. Touches nothing else - the callers decide about movement and the host. */
	void ClearOrder();

	/** What the order is acting on */
	TWeakObjectPtr<AActor> Target;

	/** Which action, in StrategyTargetAction's vocabulary - the component never interprets it */
	FName ActionId;

	/** The target's name when the order was issued, for a TargetGone line about a target that no longer exists */
	FText TargetName;

	/** Walks re-issued so far */
	int32 RetryCount = 0;

	/** True while an order is pending */
	bool bHasOrder = false;

	/** Set only by tests */
	UPROPERTY(Transient)
	TObjectPtr<UObject> HostOverride;

	/** Set only by tests */
	TFunction<EActionApproachResult(AActor*)> ApproachOverride;
};
