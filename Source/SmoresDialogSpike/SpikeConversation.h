// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  The shape the spike's conversation player is driven through - THROWAWAY, part of the spike.
 *
 *  The tests and the PIE commands only ever see this. It began as the common shape for an Ink and a
 *  Yarn player run side by side; Yarn won (2026-09-25) and the Ink half is in git history. It is
 *  roughly what Slice 3's server-side conversation needs to hand its clients: a line as an id plus
 *  its speaker, or the choices as ids with an availability flag.
 *
 *  A *script* is a loaded file, shared. A *conversation* is one playthrough of it with its own
 *  position and its own memory - several squads talking to the same NPC at once are several
 *  conversations over one script.
 */

/** A line the conversation has reached */
struct FSpikeLine
{
	/** The line's id - what would cross the network, and what a translation keys on */
	FString Id;

	/** Who says it, from Yarn's "Name: text" convention. Empty for narration. */
	FString Speaker;

	/** The words, without the speaker's name */
	FString Text;
};

/** One of the choices on offer */
struct FSpikeChoice
{
	FString Id;
	FString Text;

	/**
	 *  False for a choice shown but not selectable - Yarn's `-> text <<if condition>>`. Yarn never
	 *  hides a choice whose condition fails; it offers it marked unavailable, and the screen decides
	 *  whether to grey it out or leave it off.
	 */
	bool bAvailable = true;
};

/** What a conversation asks the game, and tells it to do */
struct FSpikeGameHooks
{
	/** Answers the script's gold() */
	TFunction<int32()> GetGold;

	/** Carries out an action the script names - TakeMoney, ChangeStanding - with its arguments as written */
	TFunction<void(const FString& Name, const TArray<FString>& Arguments)> RunCommand;
};

enum class ESpikeState : uint8
{
	/** Showing a line; Advance() moves on */
	Line,

	/** Waiting on a pick; Choose() takes one */
	Choices,

	Ended,

	/** Something went wrong mid-conversation; GetError() says what */
	Failed
};

class ISpikeConversation
{
public:

	virtual ~ISpikeConversation() = default;

	/** "Yarn" */
	virtual FString GetLanguage() const = 0;

	/** Runs from the beginning to the first line or choice. False (and GetError) if it couldn't. */
	virtual bool Start() = 0;

	virtual ESpikeState GetState() const = 0;

	/** Valid while GetState() is Line */
	virtual const FSpikeLine& GetLine() const = 0;

	/** Valid while GetState() is Choices */
	virtual const TArray<FSpikeChoice>& GetChoices() const = 0;

	/** Past the current line, to the next line or choice */
	virtual void Advance() = 0;

	/** Picks a choice by its position in GetChoices(). False if it isn't one, or isn't available. */
	virtual bool Choose(int32 Index) = 0;

	virtual FString GetError() const = 0;
};

/** A loaded Yarn script - the compiled program and its table of line text */
struct FSpikeYarnScript;

namespace SmoresDialogSpike
{
	/** Where the spike's scene lives: the example mod's conversations/ folder, as a player's mod would sit */
	FString GetSceneDirectory();

	/** The scene's file name without its extension */
	inline const TCHAR* GetSceneName() { return TEXT("shakedown"); }

	/** Reads <Name>.yarnc and <Name>-Lines.csv from Directory - the files ysc writes */
	TSharedPtr<FSpikeYarnScript> LoadYarnScript(const FString& Directory, const FString& Name, FString& OutError);

	/** A new conversation over a loaded script, starting at its first node ("Shakedown" for the scene) */
	TUniquePtr<ISpikeConversation> MakeYarnConversation(const TSharedRef<FSpikeYarnScript>& Script, const FString& StartNode, FSpikeGameHooks Hooks);

	/** Splits "Bandit: Toll road." into the speaker and the words */
	void SplitSpeaker(const FString& Raw, FString& OutSpeaker, FString& OutText);
}
