// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresRefusalReason.h"
#include "YarnProgram.h"

/**
 *  **SmoresDialog's own header - never include it from another module.** It is the one place a
 *  Yarn Spinner type appears in a header, and YarnSpinner is a private dependency of this module:
 *  confining the plugin here is what lets nothing else in the game link against it. Everything
 *  public refers to a script as the forward-declared FDialogConversationScript.
 */

/** What the loader learned about one line of a compiled script, from its -Lines.csv and -Metadata.csv */
struct FConversationLineInfo
{
	/** "example.shakedown_toll" - Yarn's "line:shakedown_toll", qualified by the package */
	FName QualifiedId;

	/** The "Bandit" of "Bandit: Toll road." - who the writer said says it. Empty for narration. */
	FString SpeakerCue;

	/** The text with the speaker cue taken off - what goes into the package's string table */
	FString SourceText;

	/** True if the program offers this line as a choice (an `->` option) */
	bool bIsOption = false;

	/** True for an option with an `<<if>>` - the only kind that can ever be unavailable */
	bool bHasCondition = false;

	/** The key of its #reason: tag, or None. Only on options: a greyed choice shows this reason; an unavailable one without it is hidden. */
	FName ReasonKey;

	/** ReasonKey, resolved */
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	/** Its line in the .yarn */
	int32 Line = 0;
};

/**
 *  One compiled .yarn file, loaded: the program ysc wrote, and what the loader learned about its
 *  lines. Shared by every conversation in the file and by every playthrough of them at once - each
 *  playthrough gets its own player and variable store, never its own copy of this.
 */
struct FDialogConversationScript
{
	FYarnProgram Program;

	FName PackageId;

	/** "conversations/shakedown.yarn" */
	FString File;

	/** Keyed by Yarn's own id, "line:shakedown_toll" - what the program's instructions name */
	TMap<FString, FConversationLineInfo> Lines;

	const FConversationLineInfo* FindLine(const FString& YarnLineId) const { return Lines.Find(YarnLineId); }
};
