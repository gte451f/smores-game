// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  The one shape both conversation players are driven through - THROWAWAY, part of the spike.
 *
 *  The tests and the PIE commands only ever see this, so they run the Ink scene and the Yarn scene
 *  identically and any difference in what comes out is a difference between the languages.
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

	/** Who says it, from the "Name: text" convention both languages' writers use. Empty for narration. */
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
	 *  False for a choice shown but not selectable - Yarn's `-> text <<if condition>>`.
	 *  Ink has no such thing: a choice whose condition fails is simply not in the list.
	 */
	bool bAvailable = true;
};

/** What a conversation asks the game, and tells it to do. The same two hooks for both languages. */
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

	/** "Yarn" or "Ink" */
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

/** A loaded Ink script - the story, converted to inkcpp's form in memory */
struct FSpikeInkScript;

namespace SmoresDialogSpike
{
	/** Where the spike's scene lives: the example mod's conversations/ folder, as a player's mod would sit */
	FString GetSceneDirectory();

	/** The scene's file name without its extension, in both languages */
	inline const TCHAR* GetSceneName() { return TEXT("shakedown"); }

	/** Reads <Name>.yarnc and <Name>-Lines.csv from Directory - the files ysc writes */
	TSharedPtr<FSpikeYarnScript> LoadYarnScript(const FString& Directory, const FString& Name, FString& OutError);

	/** Reads <Name>.ink.json from Directory - the file Inky exports - and converts it in memory */
	TSharedPtr<FSpikeInkScript> LoadInkScript(const FString& Directory, const FString& Name, FString& OutError);

	/** A new conversation over a loaded script, starting at its first node ("Shakedown" for the scene) */
	TUniquePtr<ISpikeConversation> MakeYarnConversation(const TSharedRef<FSpikeYarnScript>& Script, const FString& StartNode, FSpikeGameHooks Hooks);

	TUniquePtr<ISpikeConversation> MakeInkConversation(const TSharedRef<FSpikeInkScript>& Script, FSpikeGameHooks Hooks);

	/** Splits "Bandit: Toll road." into the speaker and the words. Shared by both players. */
	void SplitSpeaker(const FString& Raw, FString& OutSpeaker, FString& OutText);
}
