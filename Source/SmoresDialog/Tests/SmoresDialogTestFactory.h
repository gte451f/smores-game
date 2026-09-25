// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogFacts.h"
#include "DialogLibrary.h"
#include "DialogLoader.h"
#include "DialogCondition.h"
#include "DialogConversationTypes.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 *  Shared builders for the dialog tests. In a header rather than an anonymous namespace per file for
 *  the unity-build reason in testing.md.
 *
 *  The dialog tests build everything from strings - packages, manifests, bark rows - and answer
 *  facts from a map the test fills in. **None of them read Content/Dialog**, except the core content
 *  sweep, which exists to read it. So a writer retuning a bark can't break a selection test, and a
 *  test's inputs sit next to its assertions.
 */

/** What the test facts answer, keyed by fact name ("Speaker.Role"), or "Flag:met_kess" for the call form. Unset for anything absent. */
using FDialogTestAnswers = TMap<FName, FDialogValue>;

/**
 *  A registry of stand-in facts that answer from Answers at the moment of asking - change the map
 *  between evaluations to change the moment. Mirrors the real facts' shapes (a name with a content
 *  domain, a number, a fixed vocabulary, a yes/no, a Listener fact, a two-subject fact, a call form)
 *  without needing a unit, a world or a player to answer them.
 */
inline FDialogFactRegistry MakeTestDialogFacts(const TSharedRef<FDialogTestAnswers>& Answers)
{
	FDialogFactRegistry Registry;

	auto Add = [&Registry, Answers](const TCHAR* Name, EDialogValueType Type, EDialogSubject Reads, bool bTakesArgument, FName Domain, const TArray<FName>& Allowed)
	{
		FDialogFact Fact;
		Fact.Name = FName(Name);
		Fact.Type = Type;
		Fact.Reads = Reads;
		Fact.bTakesArgument = bTakesArgument;
		Fact.ContentDomain = Domain;
		Fact.AllowedNames = Allowed;

		const FName FactName = Fact.Name;

		Fact.Answer = [Answers, FactName, Type](const FDialogContext&, FName Argument)
		{
			const FName Key = Argument.IsNone() ? FactName : FName(*FString::Printf(TEXT("%s:%s"), *FactName.ToString(), *Argument.ToString()));
			const FDialogValue* Value = Answers->Find(Key);

			return Value ? *Value : FDialogValue::MakeUnset(Type);
		};

		Registry.Register(MoveTemp(Fact));
	};

	Add(TEXT("Speaker.Role"), EDialogValueType::Name, EDialogSubject::Speaker, false, SmoresDialog::RoleDomain(), {});
	Add(TEXT("Speaker.Faction"), EDialogValueType::Name, EDialogSubject::Speaker, false, SmoresDialog::FactionDomain(), {});
	Add(TEXT("Speaker.Health"), EDialogValueType::Number, EDialogSubject::Speaker, false, NAME_None, {});
	Add(TEXT("Speaker.LifeState"), EDialogValueType::Name, EDialogSubject::Speaker, false, NAME_None, { FName(TEXT("Alive")), FName(TEXT("Downed")), FName(TEXT("Dead")) });
	Add(TEXT("Speaker.IsArmed"), EDialogValueType::Bool, EDialogSubject::Speaker, false, NAME_None, {});
	Add(TEXT("Listener.Role"), EDialogValueType::Name, EDialogSubject::Listener, false, SmoresDialog::RoleDomain(), {});
	Add(TEXT("Listener.Faction"), EDialogValueType::Name, EDialogSubject::Listener, false, SmoresDialog::FactionDomain(), {});
	Add(TEXT("StandingWithSpeaker"), EDialogValueType::Number, EDialogSubject::Speaker | EDialogSubject::Player, false, NAME_None, {});
	Add(TEXT("Flag"), EDialogValueType::Bool, EDialogSubject::Player, true, NAME_None, {});
	Add(TEXT("Gold"), EDialogValueType::Number, EDialogSubject::Player, false, NAME_None, {});

	return Registry;
}

/** Everything the test facts could ask about - for compiling a condition outside any particular event */
inline EDialogSubject AllTestDialogSubjects()
{
	return EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player | EDialogSubject::Victim;
}

/** A bark file: the standard header, then one row per entry */
inline FString MakeTestBarkCsv(const TArray<FString>& Rows)
{
	return TEXT("Id,Event,Conditions,Text,Weight,Cooldown\n") + FString::Join(Rows, TEXT("\n")) + TEXT("\n");
}

/** One package folder with a manifest and, optionally, one bark file */
inline FDialogPackageSource MakeTestDialogPackage(const TCHAR* FolderName, const TCHAR* Id, const TArray<FString>& Requires = TArray<FString>(), const FString& BarksCsv = FString(), bool bIsCore = false)
{
	FDialogPackageSource Source;
	Source.FolderName = FolderName;
	Source.bIsCore = bIsCore;

	TArray<FString> QuotedRequires;

	for (const FString& Required : Requires)
	{
		QuotedRequires.Add(FString::Printf(TEXT("\"%s\""), *Required));
	}

	Source.Files.Add({ TEXT("mod.json"), FString::Printf(TEXT("{ \"Id\": \"%s\", \"Version\": \"1.0\", \"Requires\": [%s] }"), Id, *FString::Join(QuotedRequires, TEXT(", "))) });

	if (!BarksCsv.IsEmpty())
	{
		Source.Files.Add({ TEXT("barks/test.csv"), BarksCsv });
	}

	return Source;
}

/** The core package, which every real load has */
inline FDialogPackageSource MakeTestCorePackage(const FString& BarksCsv = FString())
{
	return MakeTestDialogPackage(TEXT("core"), TEXT("core"), TArray<FString>(), BarksCsv, /*bIsCore*/ true);
}

/**
 *  Where the conversation tests' own scripts live - Source/SmoresDialog/Tests/Conversations, each
 *  .yarn beside the three files ysc wrote from it. A compiled program is bytes nobody writes by hand,
 *  so these are the one thing the dialog tests read from disk; they are the tests' copies, never
 *  the game's content, so a writer can't break a test.
 */
inline FString GetTestConversationDirectory()
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("SmoresDialog"), TEXT("Tests"), TEXT("Conversations"));
}

/**
 *  Adds a test script to Source as conversations/<Name>.yarn with the three files beside it - read
 *  from disk the way GatherPackagesFromDisk reads them, without their timestamps (so a checkout's
 *  file times can't trip the stale-compile check). False if a file is missing.
 */
inline bool AddTestConversationFiles(FDialogPackageSource& Source, const TCHAR* Name)
{
	const FString Directory = GetTestConversationDirectory();
	bool bAllRead = true;

	for (const TCHAR* Suffix : { TEXT(".yarn"), TEXT(".yarnc"), TEXT("-Lines.csv"), TEXT("-Metadata.csv") })
	{
		const FString FileName = FString(Name) + Suffix;
		const FString FullPath = FPaths::Combine(Directory, FileName);

		FDialogSourceFile File;
		File.Path = TEXT("conversations/") + FileName;

		const bool bRead = FString(Suffix) == TEXT(".yarnc") ? FFileHelper::LoadFileToArray(File.Bytes, *FullPath) : FFileHelper::LoadFileToString(File.Contents, *FullPath);

		bAllRead &= bRead;

		if (bRead)
		{
			Source.Files.Add(MoveTemp(File));
		}
	}

	return bAllRead;
}

/** The file at this package path in Source, or null - for a test that tampers with one */
inline FDialogSourceFile* FindTestSourceFile(FDialogPackageSource& Source, const FString& Path)
{
	return Source.Files.FindByPredicate([&Path](const FDialogSourceFile& File) { return File.Path == Path; });
}

/**
 *  A conversation definition built by hand, for the selection tests - no script, since selection
 *  never runs one. Conditions compile against Facts over Speaker, Listener and Player; a condition
 *  that doesn't compile fails the test through the returned definition's empty id.
 */
inline FConversationDefinition MakeTestConversation(const FDialogFactRegistry& Facts, const TCHAR* Id, EConversationKind Kind, const TCHAR* Attach,
	const TCHAR* Requires = TEXT(""), int32 Priority = 0, bool bOnce = false, int32 LoadOrder = 0)
{
	FConversationDefinition Conversation;
	Conversation.Id = FName(Id);
	Conversation.Kind = Kind;
	Conversation.Priority = Priority;
	Conversation.bOnce = bOnce;
	Conversation.LoadOrder = LoadOrder;
	Conversation.LabelId = FName(*(FString(Id) + TEXT("_label")));

	TArray<FString> Errors;
	TArray<FString> Warnings;
	const EDialogSubject Subjects = EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player;

	if (!SmoresDialog::CompileCondition(Attach, Facts, Subjects, nullptr, Conversation.Attach, Errors, Warnings)
		|| !SmoresDialog::CompileCondition(Requires, Facts, Subjects, nullptr, Conversation.Requires, Errors, Warnings))
	{
		Conversation.Id = NAME_None;
	}

	return Conversation;
}

/** Every problem, one per line - what a failed assertion prints so the reader sees why */
inline FString DescribeDialogProblems(const FDialogLibrary& Library)
{
	TArray<FString> Lines;

	for (const FDialogProblem& Problem : Library.Problems)
	{
		Lines.Add(Problem.ToString());
	}

	return Lines.Num() > 0 ? FString::Join(Lines, TEXT("\n")) : FString(TEXT("(no problems)"));
}

/** True if some problem of this severity mentions Fragment - and, when Line is given, is pinned to that line */
inline bool HasDialogProblem(const FDialogLibrary& Library, EDialogProblemSeverity Severity, const TCHAR* Fragment, int32 Line = INDEX_NONE)
{
	return Library.Problems.ContainsByPredicate([Severity, Fragment, Line](const FDialogProblem& Problem)
	{
		return Problem.Severity == Severity && Problem.Message.Contains(Fragment) && (Line == INDEX_NONE || Problem.Line == Line);
	});
}

/** The load order as a string - "core, alpha, bravo" - so an ordering assertion reads at a glance */
inline FString DescribeLoadedPackages(const FDialogLibrary& Library)
{
	TArray<FString> Ids;

	for (const FDialogPackageInfo& Package : Library.Packages)
	{
		if (Package.bLoaded)
		{
			Ids.Add(Package.Id.ToString());
		}
	}

	return FString::Join(Ids, TEXT(", "));
}

#endif // WITH_DEV_AUTOMATION_TESTS
