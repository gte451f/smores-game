// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"
#include "DialogConversationTypes.h"
#include "SmoresRefusalReason.h"

class FDialogFactRegistry;
class FDialogConversationPlayerImpl;
struct FDialogConversationScript;
struct FDialogFact;

/** Where a running conversation is */
enum class EConversationPlayerState : uint8
{
	/** Showing a line; Advance() moves on */
	Line,

	/** Waiting on a pick; Choose() takes one */
	Choices,

	/** The script ran out */
	Ended,

	/** Something went wrong mid-conversation; GetError() says what */
	Failed
};

/** The line a conversation has reached */
struct SMORESDIALOG_API FConversationPlayerLine
{
	/** The qualified id, "example.shakedown_toll" - what crosses the network */
	FName LineId;

	/** Who says it - see SmoresDialog::GetSpeakerSlot */
	uint8 SpeakerSlot = SmoresDialog::NarrationSlot;

	/** The name the writer put before the line ("Bandit"), or empty */
	FString SpeakerCue;

	/** Values the line inserts, formatted */
	TArray<FString> Substitutions;

	/** The loaded source text, cue removed - for logs and tests. Players see the translated string-table entry instead. */
	FString SourceText;
};

/** One choice of the set on offer - every one the script offered, shown or not */
struct SMORESDIALOG_API FConversationPlayerChoice
{
	FName LineId;

	TArray<FString> Substitutions;

	/** False when its <<if>> failed */
	bool bAvailable = true;

	/** Unavailable and tagged #reason: - it shows greyed out, with this reason */
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	/**
	 *  Whether the window shows it at all: an available choice always, an unavailable one only
	 *  with a #reason: tag. Yarn itself never hides a choice - it offers every one, marked - so
	 *  this is where "grey this one out, leave that one off" is decided.
	 */
	bool bShown = true;

	FString SourceText;
};

/** Everything a conversation needs to run */
struct SMORESDIALOG_API FConversationPlayerSetup
{
	/** The loaded file. Shared - never copied per conversation. */
	TSharedPtr<const FDialogConversationScript> Script;

	/** The node to start at: the conversation's own */
	FString StartNode;

	/** Answers the script's questions - gold(), flag("x"), speaker_faction() */
	const FDialogFactRegistry* Facts = nullptr;

	/** Who the questions are about */
	FDialogContext Context;

	/** Ambient only: the names its lines are said under. Empty for a window conversation. */
	TArray<FName> Participants;

	/**
	 *  Carries out a command the script ran (an effect, `<<TakeMoney 20>>`), with its arguments as
	 *  written. Return false to end the conversation there - OpenTrade does.
	 */
	TFunction<bool(FName Command, const TArray<FString>& Arguments)> RunCommand;

	/** Seeds random() and dice() */
	int32 RandomSeed = 0;
};

/**
 *  One playthrough of a conversation: the Yarn Spinner plugin's player ("virtual machine"),
 *  driven directly, with its own place and its own variable store over a shared, loaded script.
 *
 *  Only the plugin's player is used - none of its dialogue runner, presenters or widgets, which
 *  assume the script runs on the machine that shows it. Ours runs on the server and sends ids.
 *  dialog.md's "How the player is driven" has what that takes: the operators are ours, the VM
 *  pauses after every command as well as every line, and a failed choice is offered, never hidden.
 *
 *  A plain C++ object, and nothing here touches a world: the facts and the effects come in through
 *  the setup, so a test drives it with stand-ins.
 */
class SMORESDIALOG_API FDialogConversationPlayer
{
public:

	explicit FDialogConversationPlayer(FConversationPlayerSetup Setup);
	~FDialogConversationPlayer();

	FDialogConversationPlayer(const FDialogConversationPlayer&) = delete;
	FDialogConversationPlayer& operator=(const FDialogConversationPlayer&) = delete;

	/** Runs from the start node to the first line or choice. False (and GetError) if it couldn't. */
	bool Start();

	EConversationPlayerState GetState() const;

	/** Valid while GetState() is Line */
	const FConversationPlayerLine& GetLine() const;

	/** Valid while GetState() is Choices: every choice offered, in order, shown or not */
	const TArray<FConversationPlayerChoice>& GetChoices() const;

	/** Past the current line, to the next line or choice */
	void Advance();

	/** Picks a choice by its position in GetChoices(). False if it isn't one, or isn't available. */
	bool Choose(int32 Index);

	/** Stops it where it is, running nothing more */
	void Stop();

	const FString& GetError() const;

private:

	TUniquePtr<FDialogConversationPlayerImpl> Impl;
};

namespace SmoresDialog
{
	/**
	 *  The functions a Yarn script may call that aren't facts - its operators ("Number.Add", which
	 *  compiled Yarn calls for `+`) and the built-ins we answer (visited, visited_count, random,
	 *  random_range, dice, round, floor, ceil, int, min, max). OutParameters is how many each takes.
	 */
	SMORESDIALOG_API bool FindYarnBuiltIn(const FString& Name, int32& OutParameters);

	/**
	 *  The name a Yarn script calls a fact by: the fact's name with each "." as "_" -
	 *  "Speaker.Faction" is speaker_faction(), "Gold" is gold(). Yarn reads a dotted name as a
	 *  member of an enum type, so the dotted form can't be called. Matched case-insensitively.
	 */
	SMORESDIALOG_API FString GetYarnFunctionName(FName FactName);

	/** The fact a Yarn function name calls ("gold" is Gold), or null */
	SMORESDIALOG_API const FDialogFact* FindFactForYarnFunction(const FDialogFactRegistry& Facts, const FString& FunctionName);

	/** Splits "Bandit: Toll road." into the speaker cue and the words. A line that merely contains a colon keeps it. */
	SMORESDIALOG_API void SplitSpeaker(const FString& Raw, FString& OutSpeaker, FString& OutText);

	/**
	 *  Who a speaker cue means. With no participants (a window conversation): "You" is the squad
	 *  member doing the talking, any other name is the NPC, and no name is narration. With
	 *  participants (Ambient): the name's position among them, or narration for none of them.
	 */
	SMORESDIALOG_API uint8 GetSpeakerSlot(const FString& Cue, const TArray<FName>& Participants);

	/** "You" - the cue for a line the squad member says */
	SMORESDIALOG_API const TCHAR* GetSquadMemberCue();
}
