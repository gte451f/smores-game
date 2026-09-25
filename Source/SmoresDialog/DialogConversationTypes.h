// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogCondition.h"
#include "SmoresRefusalReason.h"
#include "DialogConversationTypes.generated.h"

struct FDialogConversationScript;

/**
 *  The three shapes a conversation comes in - a Yarn node's `kind:` header.
 *
 *  Greeting and Topic play in the conversation window with a squad member talking to an NPC.
 *  Ambient plays with no window at all: the lines go to the feed and float over whoever says them,
 *  which is how the squad banters among itself.
 */
UENUM(BlueprintType)
enum class EConversationKind : uint8
{
	/** What an NPC opens with when a squad member talks to them. The highest-priority eligible one plays. */
	Greeting,

	/** Offered as a choice once a greeting ends - every eligible one, plus Goodbye */
	Topic,

	/** Banter: played without a window, among a player's own squad members */
	Ambient
};

/** Why a conversation stopped - what the client is told, so the window can close and say why */
UENUM()
enum class EConversationEndReason : uint8
{
	/** The script ran out with nothing more to offer */
	Finished,

	/** The player said Goodbye, closed the window or pressed Talk again */
	Goodbye,

	/** The squad member or the NPC walked out of range */
	WalkedAway,

	/** Either side went down */
	Downed,

	/** A fight started - the NPC turned hostile, somebody attacked, somebody got hurt */
	Combat,

	/** The dialog files were reloaded (SmoresReloadDialog) */
	Reloaded,

	/** The player talked to somebody else */
	Replaced,

	/** The script ran OpenTrade, and the trade screen replaced the window */
	OpenedTrade,

	/** The script couldn't go on - a bug in the script or the game, logged on the server */
	Failed
};

namespace SmoresDialog
{
	/**
	 *  The speaker slot for a line with no speaker - narration. Every other slot is a participant:
	 *  in a window conversation 0 is the NPC and 1 the squad member doing the talking; in an
	 *  Ambient one each slot is its `participants:` header's position.
	 */
	inline constexpr uint8 NarrationSlot = 255;

	/** The window conversation's two slots */
	inline constexpr uint8 NpcSlot = 0;
	inline constexpr uint8 SquadMemberSlot = 1;
}

/**
 *  One conversation as loaded: a Yarn node carrying our headers.
 *
 *  A definition in the game-data sense, like a bark - authored, identical on every machine, never
 *  written to in play. Its script is the compiled file the node lives in, shared by every
 *  conversation in that file and by every playthrough of it at once.
 */
struct SMORESDIALOG_API FConversationDefinition
{
	/** "example.Shakedown" - the package id and the node's title. What Seen() and the debug exec name. */
	FName Id;

	/** The node's title as the writer typed it, "Shakedown" */
	FName LocalId;

	FName PackageId;

	EConversationKind Kind = EConversationKind::Greeting;

	/** `attach:` - who it belongs to, asked about the Speaker only. Empty for Ambient. */
	FDialogCondition Attach;

	/** `requires:` - what else must be true right now. Empty means nothing more. */
	FDialogCondition Requires;

	/** `priority:` - the highest eligible greeting wins; topics are listed highest first */
	int32 Priority = 0;

	/** `once:` - never again for a squad that has seen it */
	bool bOnce = false;

	/** `participants:` - Ambient only: the names its lines are said under, each played by a different squad member */
	TArray<FName> Participants;

	/** Topic only: the qualified id of its `label:` - the choice text that offers it */
	FName LabelId;

	/** The compiled file this node is in */
	TSharedPtr<const FDialogConversationScript> Script;

	/** Where it was written - "conversations/shakedown.yarn" and the node's title line */
	FString File;
	int32 Line = 0;

	/** Its place in the library. The last tie-break in selection, so a tie never depends on anything but the files. */
	int32 LoadOrder = 0;
};

/** One line of a running conversation, as it crosses the network: an id, never its text */
USTRUCT()
struct SMORESDIALOG_API FConversationLineView
{
	GENERATED_BODY()

	/** The qualified line id, "example.shakedown_toll" - the client looks the words up in its own language */
	UPROPERTY()
	FName LineId;

	/** Who says it - SmoresDialog::NpcSlot, SquadMemberSlot, or NarrationSlot */
	UPROPERTY()
	uint8 SpeakerSlot = SmoresDialog::NarrationSlot;

	/** Values the line inserts ({0} in its text), already formatted on the server */
	UPROPERTY()
	TArray<FString> Substitutions;
};

/** One choice on offer, as it crosses the network */
USTRUCT()
struct SMORESDIALOG_API FConversationChoiceView
{
	GENERATED_BODY()

	/** The qualified id of the choice's text - a Yarn option's line, or a topic's label. None for Goodbye. */
	UPROPERTY()
	FName TextId;

	UPROPERTY()
	TArray<FString> Substitutions;

	/** False for a choice shown greyed out - its #reason: tag says why */
	UPROPERTY()
	bool bEnabled = true;

	/** Why it is greyed out. None when enabled. */
	UPROPERTY()
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	/** The window's own way out, offered after every greeting and topic */
	UPROPERTY()
	bool bGoodbye = false;
};
