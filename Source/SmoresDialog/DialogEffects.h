// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresRefusalReason.h"

class AActor;
class APlayerState;
class IDialogHost;
class UWorld;
struct FDialogKnownIds;

/** Who and what an effect acts on - the conversation it was run from */
struct SMORESDIALOG_API FDialogEffectContext
{
	UWorld* World = nullptr;

	/** The NPC (or, in banter, the first participant) */
	AActor* Speaker = nullptr;

	/** The squad member talking (or, in banter, the second participant) */
	AActor* Listener = nullptr;

	/** Whose squad this is - whose flags, standing and gold an effect changes */
	APlayerState* Player = nullptr;

	/** That player's controller, for what only it can do (OpenTrade). May be null in a test. */
	IDialogHost* Host = nullptr;
};

/** What running one effect did */
struct SMORESDIALOG_API FDialogEffectOutcome
{
	/** True if it did what it says. False changed nothing. */
	bool bDone = false;

	/** Why not, when the player should hear about it - None otherwise */
	ESmoresRefusalReason Refusal = ESmoresRefusalReason::None;

	/** A line for the player's feed - "Paid 20 gold." Worded on the server, where the numbers are. */
	FText FeedLine;

	/** True if the conversation should end here - OpenTrade, whose screen replaces the window */
	bool bEndsConversation = false;
};

/**
 *  One named action a conversation can take - a Yarn command, `<<TakeMoney 20>>`.
 *
 *  **Every effect is C++, authority-only, and checked at load time**, like a fact: its name and its
 *  number of arguments are checked against every command a script runs, and Check looks at the
 *  arguments themselves where they are written out (not a {substitution}). A script can only
 *  change the world through this list (`multiplayer-and-content.md`: mods are data, not code).
 */
struct SMORESDIALOG_API FDialogEffect
{
	/** What a script writes - "TakeMoney". Matched case-insensitively. */
	FName Name;

	int32 MinArguments = 0;

	int32 MaxArguments = 0;

	/** "TakeMoney <amount>" - for error messages and the writers' reference */
	FString Usage;

	/** One line for the writers' reference */
	FString Description;

	/** True for an effect that needs the conversation window (OpenTrade): an Ambient script may not use it */
	bool bNeedsWindow = false;

	/** Checks one written use's arguments at load time. Errors reject the conversation; warnings (an unknown faction id) don't. */
	TFunction<void(const TArray<FString>& Arguments, const FDialogKnownIds* KnownIds, TArray<FString>& OutErrors, TArray<FString>& OutWarnings)> Check;

	/** Does it. Only ever called through FDialogEffectRegistry::Run, which checks authority first. */
	TFunction<FDialogEffectOutcome(const FDialogEffectContext& Context, const TArray<FString>& Arguments)> Run;
};

/** Every effect a script may use. A plain value type, like FDialogFactRegistry. */
class SMORESDIALOG_API FDialogEffectRegistry
{
public:

	/** False, with nothing added, for a blank name or one already registered */
	bool Register(FDialogEffect Effect);

	const FDialogEffect* Find(FName Name) const;

	const TArray<FDialogEffect>& GetEffects() const { return Effects; }

	/**
	 *  Runs one effect. **Authority only**: with no player, or a player this machine doesn't have
	 *  authority over, it refuses with nothing changed - the effects themselves then never need to
	 *  ask. An unknown name refuses too; the loader stops that reaching here.
	 */
	FDialogEffectOutcome Run(FName Name, const FDialogEffectContext& Context, const TArray<FString>& Arguments) const;

	/**
	 *  The game's effects: SetFlag, ChangeStanding, TakeMoney, GiveMoney and OpenTrade. dialog.md
	 *  lists what each does.
	 */
	static FDialogEffectRegistry MakeBuiltIn();

private:

	TArray<FDialogEffect> Effects;

	TMap<FName, int32> IndexByName;
};

namespace SmoresDialog
{
	/**
	 *  The reason a #reason:<key> tag names - `#reason:not_enough_money` is CannotAfford. A choice
	 *  tagged with one shows greyed, with the refusal line's own words for it
	 *  (URefusalWidget::GetRefusalText), when its condition fails. False for a key nobody defined.
	 */
	SMORESDIALOG_API bool FindChoiceReason(FName Key, ESmoresRefusalReason& OutReason);

	/** Every #reason: key, for error messages and the writers' reference */
	SMORESDIALOG_API TArray<FName> GetChoiceReasonKeys();

	/** A flag's name: letters, digits and _ */
	SMORESDIALOG_API bool IsValidFlagName(const FString& Name);
}
