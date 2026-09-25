// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"
#include "DialogConversationTypes.h"

class FDialogFactRegistry;
struct FDialogMemoryRecord;

/** What a running window conversation's check looks at, every quarter second */
struct SMORESDIALOG_API FConversationWatch
{
	/** Either side no longer exists */
	bool bSomeoneGone = false;

	bool bNpcDown = false;

	bool bSquadMemberDown = false;

	/** The NPC has turned hostile */
	bool bNpcHostile = false;

	/** Either side has somebody to attack */
	bool bEitherAttacking = false;

	/** Either side has lost health since the conversation began */
	bool bEitherHurt = false;

	/** How far apart they are, and how far is too far */
	float Distance = 0.0f;
	float BreakOffRange = 0.0f;

	/** False for a conversation the debug exec opened across the map */
	bool bCheckRange = true;
};

/**
 *  Which conversation happens - chosen the way Hades chooses, by rules, never by a trigger wired to
 *  one scene. Pure functions over loaded definitions, a moment and one squad's memory: nothing here
 *  touches a world, so a test builds all three by hand.
 */
namespace SmoresDialog
{
	/**
	 *  True if Conversation can be had right now: its attach: and requires: both hold, and it isn't
	 *  a once: conversation this squad has already seen. OutWhyNot, when given, says which failed.
	 */
	SMORESDIALOG_API bool IsConversationEligible(
		const FConversationDefinition& Conversation,
		const FDialogContext& Context,
		const FDialogFactRegistry& Facts,
		const FDialogMemoryRecord& Memory,
		FString* OutWhyNot = nullptr);

	/**
	 *  Every eligible candidate, in the order they are offered: highest priority first, then the
	 *  order they were loaded. The topic list after a greeting is exactly this over the topics.
	 */
	SMORESDIALOG_API TArray<const FConversationDefinition*> GetEligibleConversations(
		const TArray<const FConversationDefinition*>& Candidates,
		const FDialogContext& Context,
		const FDialogFactRegistry& Facts,
		const FDialogMemoryRecord& Memory);

	/**
	 *  The one that plays, of the eligible candidates: the highest priority. A tie goes to the one
	 *  this squad saw least recently - one never seen beats one that was - and a tie after that to
	 *  the one loaded first, so the choice never depends on anything but the files and the memory.
	 *  Null when none is eligible. OutExplanation, when given, gets one line per candidate.
	 */
	SMORESDIALOG_API const FConversationDefinition* SelectConversation(
		const TArray<const FConversationDefinition*>& Candidates,
		const FDialogContext& Context,
		const FDialogFactRegistry& Facts,
		const FDialogMemoryRecord& Memory,
		TArray<FString>* OutExplanation = nullptr);

	/**
	 *  Whether a window conversation has to end, and why: someone gone or out of range is
	 *  WalkedAway, someone down is Downed, and a hostile NPC, an attack or a wound is Combat. False
	 *  while it can go on.
	 */
	SMORESDIALOG_API bool GetConversationInterruption(const FConversationWatch& Watch, EConversationEndReason& OutReason);
}
