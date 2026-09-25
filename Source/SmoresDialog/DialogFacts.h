// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"

/**
 *  One named question the game can answer about a moment: "Speaker.Faction", "StandingWithSpeaker".
 *
 *  **Only register a fact something can actually answer** - the refusal-reason rule from
 *  refusals-and-feedback.md, applied to dialog. The registered list *is* the writers' and modders'
 *  reference (SmoresDialogReport facts prints it), so a fact nothing can answer yet is a promise
 *  the game doesn't keep. No Speaker.Lineage until lineage exists; no skill facts until skills do.
 */
struct SMORESDIALOG_API FDialogFact
{
	/** What a condition writes - "Speaker.Role". Matched case-insensitively, like every FName. */
	FName Name;

	EDialogValueType Type = EDialogValueType::Name;

	/** Everyone this fact reads. A condition may only use it in an event that carries all of them. */
	EDialogSubject Reads = EDialogSubject::None;

	/** True for the Flag(name)-style call form. No Slice 1 fact takes one; the grammar is ready for those that will. */
	bool bTakesArgument = false;

	/**
	 *  For a Name fact whose answers are game content ids - SmoresDialog::FactionDomain() and
	 *  friends. A literal compared against it that no loaded content has is a *warning*: it may be a
	 *  typo, or it may be another mod's faction that isn't installed. None when anything goes.
	 */
	FName ContentDomain;

	/** For a Name fact with a fixed vocabulary, like LifeState. Anything else is an *error* - no mod can add a life state. */
	TArray<FName> AllowedNames;

	/** One line for the writers' reference */
	FString Description;

	/** Answers the question. Unset when the subject it reads isn't there. */
	TFunction<FDialogValue(const FDialogContext& Context, FName Argument)> Answer;
};

/**
 *  The game content ids a condition's literals can be checked against, by domain.
 *
 *  A domain with no entry here is simply unchecked, which is what a test that builds its own facts
 *  wants. GatherFromGame fills every domain the built-in facts use.
 */
struct SMORESDIALOG_API FDialogKnownIds
{
	TMap<FName, TSet<FName>> IdsByDomain;

	/** False only when Domain is checked and doesn't hold Id. "None" is always known: it is how a condition says "unaffiliated". */
	bool IsKnown(FName Domain, FName Id) const;

	/** Every faction id, character definition id and role id the Asset Manager can find */
	static FDialogKnownIds GatherFromGame();
};

/**
 *  Every fact a condition may use.
 *
 *  A plain value type rather than a global, so a test can build a registry of its own facts
 *  answering from a local map, and the game's is simply MakeBuiltIn() held by the dialog subsystem.
 */
class SMORESDIALOG_API FDialogFactRegistry
{
public:

	/** Adds a fact. False, with nothing added, for a blank name or one already registered. */
	bool Register(FDialogFact Fact);

	/** The fact of this name, or null */
	const FDialogFact* Find(FName Name) const;

	/** Every registered fact, in registration order */
	const TArray<FDialogFact>& GetFacts() const { return Facts; }

	/** Asks a fact. Unset for a name that isn't registered - validation stops that reaching here. */
	FDialogValue Ask(FName Name, const FDialogContext& Context, FName Argument = NAME_None) const;

	/**
	 *  The facts the game answers today: for each of Speaker, Listener and Event.Victim, their
	 *  Definition, Role, Faction, Name, LifeState and Health; plus StandingWithSpeaker. See
	 *  dialog.md for what each one reads.
	 */
	static FDialogFactRegistry MakeBuiltIn();

private:

	TArray<FDialogFact> Facts;

	TMap<FName, int32> IndexByName;
};

namespace SmoresDialog
{
	/** Content domain: faction definition ids ("Raiders") */
	SMORESDIALOG_API FName FactionDomain();

	/** Content domain: character definition ids ("Bandit") */
	SMORESDIALOG_API FName CharacterDomain();

	/** Content domain: role ids authored on character definitions ("trader") */
	SMORESDIALOG_API FName RoleDomain();
}
