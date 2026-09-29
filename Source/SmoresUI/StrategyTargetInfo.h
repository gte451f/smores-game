// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresRefusalReason.h"
#include "StrategyTargetInfo.generated.h"

/**
 *  One button on the target panel's action row, and one row of the right-click menu - the two are
 *  views of the same list, built by the same rules (smores' StrategyTargetActions).
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

	/**
	 *  Who would carry it out - the selected squad member nearest the target, "Everyone selected"
	 *  for an attack by several. Empty when nobody would (and then the action is disabled with
	 *  NoOneSelected), and for Examine, which nobody walks anywhere to do.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText ActorName;

	/**
	 *  The line that changes with who would go: a success chance for an action whose outcome
	 *  depends on the squad member attempting it ("62% chance"), and that squad member's name when
	 *  it isn't the one the menu's header names. Empty for most actions.
	 *
	 *  A number is allowed here because it is built from the squad's own numbers, which the player
	 *  already knows - never from a target's, which only ever surface as words. See game-design's
	 *  player-interface.md.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText Detail;
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

	/**
	 *  The thing described. Carried so that a click on one of this struct's actions acts on the
	 *  thing that was drawn, not on whatever the controller happens to have targeted by then - the
	 *  menu and the panel can describe two different things at once.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	TWeakObjectPtr<AActor> Target;

	/** The target's name, blank if whoever placed it never gave it one */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText DisplayName;

	/** What it is and how it feels about the player ("CONTAINER", "PERSON - HOSTILE", "BODY") */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText Classification;

	/** Metres from the nearest selected unit, or negative when nothing is selected to measure from */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	float DistanceMeters = -1.0f;

	/** Who would carry out most of these actions - the menu's "who would go" line. Empty when nobody would. */
	UPROPERTY(BlueprintReadOnly, Category = "Target")
	FText ActorName;

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

	/**
	 *  True if Other would draw identically to this - the panel's and the menu's shared "is there
	 *  anything to redraw?" check.
	 *
	 *  The HUD pushes a freshly built struct every frame, and almost every one of them describes
	 *  the same target in the same state - distance is the only field that moves continuously, and
	 *  it is drawn to the nearest metre. Comparing what would be *drawn*, rather than the raw
	 *  floats, is what keeps a stationary squad from invalidating Slate layout sixty times a
	 *  second. Which actor is described is deliberately not compared: it isn't drawn.
	 */
	bool DrawsIdenticallyTo(const FStrategyTargetInfo& Other) const;
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
	/** Open a shut door. The `O` key's action on a door. (Until the action menu this opened a container - that is Loot now.) */
	SMORESUI_API FName Open();

	/** Shut an open door. Also the `O` key. */
	SMORESUI_API FName Close();

	/** Go through a container, or a downed or dead body. The `O` key - same window, same rules. */
	SMORESUI_API FName Loot();

	/** Talk to someone on their feet: a conversation if one is eligible, else trade if they keep a shop, else a line. The `T` key's action. */
	SMORESUI_API FName Talk();

	/** Open a trader's shop directly, skipping the conversation */
	SMORESUI_API FName Trade();

	/** Pick up a loose item lying on the ground */
	SMORESUI_API FName PickUp();

	/** Look at something. The one action nobody walks anywhere to do. */
	SMORESUI_API FName Examine();

	/** Send the squad to attack. The `H` key's action. */
	SMORESUI_API FName Attack();

	/** Treat someone's wounds. A placeholder - see StrategyTargetActions' IsPlaceholderAction. */
	SMORESUI_API FName Heal();

	/** Carry off someone who is down. A placeholder. */
	SMORESUI_API FName Kidnap();

	/** Lift something from someone's pockets. A placeholder. */
	SMORESUI_API FName Pickpocket();

	/** Knock someone out cold, without killing them. A placeholder. */
	SMORESUI_API FName KnockOut();
}
