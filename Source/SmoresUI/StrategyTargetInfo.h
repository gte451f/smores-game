// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresRefusalReason.h"
#include "StrategyTargetInfo.generated.h"

/**
 *  One button on the target panel's action row.
 *
 *  **A disabled action is still an action.** "Talk" greyed out because the person is hostile
 *  teaches the player the rule; a button that simply isn't there teaches nothing, and leaves them
 *  wondering whether talking is possible at all. So an action the controller *could* offer on
 *  this kind of target is always in the list, with bEnabled saying whether it can be used right
 *  now and DisabledReason saying why not.
 *
 *  DisabledReason is an ESmoresRefusalReason rather than a sentence for the same reason every
 *  other refusal in this project is: URefusalWidget::GetRefusalText is the only place a refusal
 *  is worded, so a reason shown here and the same reason shown as a refusal line can never
 *  disagree. None means "no reason to give" - either the action is enabled, or it is off for a
 *  reason that isn't a refusal (attacking someone already fighting you).
 */
USTRUCT(BlueprintType)
struct SMORESUI_API FTargetAction
{
	GENERATED_BODY()

	/** Which action this is. Sent straight back to IStrategyHUDCommands::RequestTargetAction. */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FName Id;

	/** The button's label ("Open", "Talk", "Attack") */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText Label;

	/** The key that does the same thing ("O", "T", "H"), or empty if none does */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText KeyHint;

	/** True if the player can use this right now */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	bool bEnabled = false;

	/** Why it's disabled, in the shared refusal vocabulary. None when enabled. */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	ESmoresRefusalReason DisabledReason = ESmoresRefusalReason::None;
};

/**
 *  Everything the target panel draws about whatever the player last clicked: the one struct that
 *  replaced GetSelectionTargetLabel's single line of text.
 *
 *  It is rebuilt from scratch every frame by AStrategyPlayerController::BuildTargetInfo and
 *  pushed through AStrategyHUD::DrawHUD, the same way gold and the selection count already were.
 *  Nothing here is cached, and deliberately: distance and reach change as the squad walks, so a
 *  panel that remembered its action row would offer "Open" on a chest the squad had wandered away
 *  from.
 */
USTRUCT(BlueprintType)
struct SMORESUI_API FStrategyTargetInfo
{
	GENERATED_BODY()

	/**
	 *  True when there is something to describe.
	 *
	 *  A flag of its own rather than "is the name empty", because a target's name is authored
	 *  data: an actor nobody got round to naming would otherwise make the whole panel vanish,
	 *  and the bug would read as the panel being broken rather than as a blank field.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	bool bHasTarget = false;

	/** The target's name, blank if whoever placed it never gave it one */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText DisplayName;

	/** What it is and how it feels about the player ("CONTAINER", "PERSON - HOSTILE", "BODY") */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText Classification;

	/** Metres from the nearest selected unit, or negative when nothing is selected to measure from */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float DistanceMeters = -1.0f;

	/** True if this target has health worth drawing a bar for - a container doesn't */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	bool bHasHealth = false;

	/** Current health as a 0-1 fraction of maximum. Only meaningful when bHasHealth. */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float HealthFraction = 0.0f;

	/** What the player may do to this target right now, enabled and disabled alike. Empty when
	 *  there is genuinely nothing this kind of target offers - one of your own squad, say. */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	TArray<FTargetAction> Actions;

	/** True when there is something to show. The panel hides itself when this is false. */
	bool HasTarget() const { return bHasTarget; }
};

/**
 *  The action ids, in one place so the panel that offers an action and the controller that runs
 *  it cannot drift apart over a typo'd string.
 *
 *  Functions rather than header constants because an FName built during static initialisation
 *  runs before the name pool is guaranteed to exist; a function-local static is built on first
 *  use, which is always late enough.
 */
namespace StrategyTargetAction
{
	/** Open a container's grid. The `O` key's action. */
	SMORESUI_API FName Open();

	/** Loot a downed or dead body. Also the `O` key - same window, same rules. */
	SMORESUI_API FName Loot();

	/** Talk to / trade with someone on their feet. The `T` key's action. */
	SMORESUI_API FName Talk();

	/** Send the squad to attack. The `H` key's action. */
	SMORESUI_API FName Attack();
}
