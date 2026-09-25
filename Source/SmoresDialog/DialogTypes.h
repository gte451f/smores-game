// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "DialogTypes.generated.h"

class AActor;
class APlayerState;

/**
 *  The moments a bark can be said in. Every one of them comes from a signal the game already had
 *  before dialog existed - see game-systems' dialog.md for where each is raised.
 *
 *  A bark file names these by their C++ name ("Hurt", "TradeOpened"), case-insensitively. Adding a
 *  value here is adding a word to the writers' vocabulary, so give it a row in
 *  SmoresDialog::GetEventSubjects in the same change.
 */
UENUM(BlueprintType)
enum class EBarkEvent : uint8
{
	/** The speaker took a hit and is still on their feet */
	Hurt,

	/** The speaker just went down */
	Downed,

	/** Someone on the speaker's side died near them */
	WitnessedDeath,

	/** A squad member just opened the speaker's shop */
	TradeOpened,

	/** A squad member tried to talk to the speaker, who has no shop and nothing else to offer */
	NothingToSay
};

/**
 *  The people a condition can ask about. A fact reads one of these (StandingWithSpeaker reads two),
 *  and each event says which of them it carries - so "Listener.Role" in a Downed row is caught at
 *  load time instead of silently never matching.
 */
enum class EDialogSubject : uint8
{
	None = 0,

	/** Whoever is saying the line */
	Speaker = 1 << 0,

	/** Whoever it is said to - the squad member at the counter, the unit that landed the hit */
	Listener = 1 << 1,

	/** The player whose squad this is about. Standing is per player, so this is whose standing */
	Player = 1 << 2,

	/** WitnessedDeath's payload: the one who died */
	Victim = 1 << 3
};
ENUM_CLASS_FLAGS(EDialogSubject);

/** What kind of answer a fact gives */
enum class EDialogValueType : uint8
{
	Number,
	Name,
	Bool
};

/**
 *  One answer to a fact, or one literal in a condition.
 *
 *  "Unset" is a real state, not a default: a fact asked about somebody the event doesn't have (a
 *  Hurt with no attacker) answers unset, and **every comparison against an unset value is false**.
 *  A missing listener can therefore never accidentally satisfy "Listener.Faction != Raiders".
 */
struct SMORESDIALOG_API FDialogValue
{
	EDialogValueType Type = EDialogValueType::Name;

	bool bIsSet = false;

	double Number = 0.0;

	FName Name;

	bool bBool = false;

	static FDialogValue MakeNumber(double InNumber);
	static FDialogValue MakeName(FName InName);
	static FDialogValue MakeBool(bool bInBool);
	static FDialogValue MakeUnset(EDialogValueType InType);

	/** "12", "Bandit", "true", or "(none)" - for the report and SmoresTestBark's explanation */
	FString ToString() const;
};

/**
 *  Who and what a condition is being asked about, at the moment of asking.
 *
 *  Actors rather than characters, and weak pointers throughout, because a context outlives nothing:
 *  it is built for one selection and thrown away. A fact that needs a unit casts; a fact asked
 *  about a subject that has gone away answers unset.
 */
struct SMORESDIALOG_API FDialogContext
{
	/** The bark event this selection is for */
	EBarkEvent Event = EBarkEvent::Hurt;

	TWeakObjectPtr<const AActor> Speaker;

	TWeakObjectPtr<const AActor> Listener;

	TWeakObjectPtr<const AActor> Victim;

	TWeakObjectPtr<const APlayerState> Player;

	/** The actor standing in for Subject, or null. Player is a player state, not an actor someone can see, so it is never returned here. */
	const AActor* GetCharacter(EDialogSubject Subject) const;
};

/** How bad a load problem is */
enum class EDialogProblemSeverity : uint8
{
	/** Something was skipped - a row, a file, a whole package */
	Error,

	/**
	 *  Loaded, but probably not what the author meant - most often a faction or definition id this
	 *  game doesn't know. Allowed at runtime because a mod may name content from another mod that
	 *  isn't installed; the core content sweep treats every warning in our own files as a failure.
	 */
	Warning
};

/** One thing wrong with loaded dialog, pinned to where it is */
struct SMORESDIALOG_API FDialogProblem
{
	EDialogProblemSeverity Severity = EDialogProblemSeverity::Error;

	/** The package's id, or its folder name if the manifest never got far enough to have an id */
	FString Package;

	/** Path inside the package ("barks/core.csv"), or empty for a problem with the package itself */
	FString File;

	/** 1-based line in File, or 0 when the problem isn't about one line */
	int32 Line = 0;

	FString Message;

	/** "error: core barks/core.csv:12: unknown fact 'Speaker.Factoin'" */
	FString ToString() const;
};

namespace SmoresDialog
{
	/** Every bark event, in declaration order */
	SMORESDIALOG_API const TArray<EBarkEvent>& GetAllBarkEvents();

	/** "Hurt" - the spelling a bark file uses */
	SMORESDIALOG_API FString GetBarkEventName(EBarkEvent Event);

	/** Parses a bark file's event column, case-insensitively. False for anything that isn't one. */
	SMORESDIALOG_API bool ParseBarkEvent(const FString& Text, EBarkEvent& OutEvent);

	/**
	 *  Who each event carries. A condition may only ask about these; asking about anyone else is a
	 *  load-time error, because the answer would always be unset and the rule would never match.
	 *
	 *  Hurt's Listener (the attacker) and Player (the attacker's player) are *possible*, not
	 *  guaranteed - a hit from nobody, or from another NPC, has no player. The facts answer unset
	 *  then, and the clause is false.
	 */
	SMORESDIALOG_API EDialogSubject GetEventSubjects(EBarkEvent Event);

	/** "Speaker", "Listener, Player" - for error messages */
	SMORESDIALOG_API FString DescribeSubjects(EDialogSubject Subjects);
}
