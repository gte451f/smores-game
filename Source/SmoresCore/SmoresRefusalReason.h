// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresRefusalReason.generated.h"

/**
 *  Why something the player asked for was refused.
 *
 *  This is a *code*, never a sentence. Whoever refuses names the reason; the UI decides what
 *  words to put on screen (URefusalWidget::GetRefusalText is the only place the wording lives).
 *  That keeps one refusal from being phrased three different ways, keeps every phrase
 *  translatable in one pass, and lets the same reason drive different presentations depending
 *  on where it happened - red cells under a drag, a line of text at the cursor, a sound.
 *
 *  It lives in SmoresCore because refusing is not an inventory idea: items, equipment and trade
 *  all refuse things today, and orders and combat will. All of them have to name the reason in
 *  a vocabulary SmoresUI can read, and SmoresCore is the one module every one of them can see.
 *
 *  **Prefer preventing a refusal to explaining one.** The drop preview turning cells red is a
 *  better answer than this enum, because the player never completes the gesture; a reason
 *  reaches here only when it could not be shown in advance. See inventory.md's "Refusals".
 *
 *  Adding a reason is one line here and one line in GetRefusalText. Only add one something
 *  actually raises - a value nothing ever sends is an empty hook the player never sees.
 */
UENUM(BlueprintType)
enum class ESmoresRefusalReason : uint8
{
	/** Not a refusal. Either it worked, or it failed in a way not worth interrupting the player over. */
	None            UMETA(DisplayName = "None"),

	/** Nobody is close enough to reach it. */
	TooFar          UMETA(DisplayName = "Too Far"),

	/** It wouldn't fit: no free cells of the right shape, and no matching stack with space left. */
	NoRoom          UMETA(DisplayName = "No Room"),

	/** The price is more than the wallet holds. */
	CannotAfford    UMETA(DisplayName = "Cannot Afford"),

	/** The item isn't worn in that slot, or isn't wearable at all. */
	WrongSlot       UMETA(DisplayName = "Wrong Slot"),

	/** They won't deal with you - currently hostile, or not on their feet. */
	NotInteractable UMETA(DisplayName = "Not Interactable")
};
