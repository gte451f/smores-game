// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActivityEntry.generated.h"

/**
 *  Which tab of the activity feed an entry belongs under.
 *
 *  There is deliberately no "All" value. The feed's LOG tab shows everything, and it does that by
 *  *not filtering*, not by every entry carrying a second category nobody posts to. A value that
 *  only the reader ever uses would be one more thing every producer has to get right.
 */
UENUM(BlueprintType)
enum class EActivityCategory : uint8
{
	/** Things that happened to or because of the player's own squad - damage, downs, loot, refusals */
	Squad,

	/** Talking, trading, and anything else said by someone who isn't in the squad */
	Comms,

	/** Objectives. Nothing posts here yet - see quests-and-objectives.md, which has no authored content. */
	Quests
};

/**
 *  How loud an entry is. Drives colour and nothing else.
 *
 *  Kept separate from the category on purpose: "you took damage" and "you dealt damage" are the
 *  same category and read completely differently, and a player scanning a fight afterwards is
 *  looking for the red lines, not for a tab.
 */
UENUM(BlueprintType)
enum class EActivitySeverity : uint8
{
	/** Ordinary record - picked something up, sold something */
	Normal,

	/** Went well for the player - a kill, a good trade */
	Good,

	/** Worth noticing - a refusal, a near miss */
	Warning,

	/** Went badly - squad damage, a squad member down */
	Bad
};

/**
 *  One line in the activity feed.
 *
 *  **Already-worded text, not a code.** This is the opposite of ESmoresRefusalReason, and
 *  deliberately: a refusal is one of six fixed sentences and wants a single place to word them,
 *  whereas a feed line names an item, a quantity, a person and a price, so there is no finite
 *  vocabulary to centralise. What does travel through the refusal vocabulary still gets worded
 *  by URefusalWidget::GetRefusalText before it is posted here, so the line in the feed and the
 *  line at the cursor read identically.
 *
 *  Timestamp is FPlatformTime::Seconds(), not world time, and that matters: the feed fades on
 *  wall-clock seconds, so an entry posted just before the player pauses doesn't sit there
 *  forever, and one posted at 8x doesn't vanish eight times too fast.
 */
USTRUCT(BlueprintType)
struct SMORESCORE_API FActivityEntry
{
	GENERATED_BODY()

	/** Which tab this shows under */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	EActivityCategory Category = EActivityCategory::Squad;

	/** How loud it is */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	EActivitySeverity Severity = EActivitySeverity::Normal;

	/** The line itself, already worded */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	FText Text;

	/** Who or what it came from ("Pawn 1", "Marla"), or empty if nobody in particular */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	FText Source;

	/** FPlatformTime::Seconds() when it was posted - see the struct comment for why not world time */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	double Timestamp = 0.0;

	/**
	 *  Monotonically increasing, assigned by the log. Two entries with identical words are still
	 *  different entries, which is exactly what the feed widget needs in order to tell "nothing
	 *  happened" from "that happened again" without comparing FTexts.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	int32 Id = 0;
};
