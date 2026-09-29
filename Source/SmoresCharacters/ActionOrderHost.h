// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmoresRefusalReason.h"
#include "ActionOrderHost.generated.h"

class AStrategyUnit;

/**
 *  Why a walk-over-to-act order ended without the action being carried out.
 *
 *  Only the endings the player should hear about. The one silent ending - the player gave the
 *  unit something else to do - never reaches the host at all: they did it, so there is nothing
 *  to explain.
 */
UENUM()
enum class EActionOrderEnd : uint8
{
	/** Couldn't get close enough, after the retries. A CannotReach refusal. */
	CannotReach,

	/** Got there, and the rules said no - the target turned hostile, went down, closed. The reason travels with it. */
	Refused,

	/** Went Downed or Dead on the way */
	ActorDown,

	/** Stopped to fight back against someone who hit them */
	ActorFighting,

	/** The target stopped existing - picked up by someone else, say */
	TargetGone
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UActionOrderHost : public UInterface
{
	GENERATED_BODY()
};

/**
 *  What a unit's UActionOrderComponent needs from whoever gave it the order - implemented by
 *  AStrategyPlayerController, and found through the unit's owning controller.
 *
 *  The IDialogHost pattern (unreal-module-organization.md): nothing depends on the smores module,
 *  so SmoresCharacters declares the narrow interface it needs and the controller implements it.
 *  The rules that decide what may be done to what live in smores, beside the target panel that
 *  shows them, so the component asks through here rather than restating any of them.
 *
 *  Every call arrives on the server.
 */
class SMORESCHARACTERS_API IActionOrderHost
{
	GENERATED_BODY()

public:

	/**
	 *  Is ActionId still available for Actor on Target, right now? Asked on arrival, just before
	 *  acting, through the same rules that offered the action - so a target that turned hostile on
	 *  the way refuses exactly the way the menu would have. False fills OutReason.
	 */
	virtual bool CanPerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId, ESmoresRefusalReason& OutReason) = 0;

	/** Actor is within reach and the action checked out: do it. The order has already ended by the time this runs. */
	virtual void PerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId) = 0;

	/**
	 *  The order ended without acting, for a reason worth telling the player. TargetName is
	 *  carried separately because a TargetGone target may already be null. Reason is the refusal
	 *  for Refused and CannotReach, None otherwise.
	 */
	virtual void HandleActionOrderEnded(AStrategyUnit* Actor, AActor* Target, const FText& TargetName, FName ActionId, EActionOrderEnd Why, ESmoresRefusalReason Reason) = 0;
};
