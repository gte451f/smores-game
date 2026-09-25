// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogFacts.h"
#include "DialogLibrary.h"
#include "DialogLoader.h"

/**
 *  The dialog content sweep - the dialog equivalent of Smores.Content.Definitions.EveryAssetIsWellFormed.
 *
 *  Like the definition sweeps, this deliberately reads the real files (Content/Dialog/core, and the
 *  example mod under Mods/), unlike every other dialog test. And it holds our own files to a stricter
 *  rule than the game holds a mod: **a warning is a failure here.** At runtime an unknown faction id
 *  is only a warning, because a mod may name another mod's content; in the base game's own files it
 *  can only be a typo.
 *
 *  Count the number, not the colour: every check first asserts it found something to check.
 */

/** Loads core alone - or core plus Mods/ - the way the game does, with the game's facts and known ids */
static FDialogLibrary SmoresDialogContentTest_Load(bool bIncludeMods)
{
	TArray<FDialogPackageSource> Sources;
	TArray<FDialogProblem> DiskProblems;

	SmoresDialog::GatherPackagesFromDisk(SmoresDialog::GetCoreDirectory(), bIncludeMods ? SmoresDialog::GetModsDirectory() : FString(), Sources, DiskProblems);

	const FDialogKnownIds KnownIds = FDialogKnownIds::GatherFromGame();

	FDialogLibrary Library = SmoresDialog::LoadPackages(Sources, FDialogFactRegistry::MakeBuiltIn(), &KnownIds);
	Library.Problems.Insert(DiskProblems, 0);

	return Library;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogCoreContentTest,
	"Smores.Content.Dialog.CoreLoadsWithZeroProblems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogCoreContentTest::RunTest(const FString& Parameters)
{
	const FDialogLibrary Library = SmoresDialogContentTest_Load(/*bIncludeMods*/ false);

	const FDialogPackageInfo* Core = Library.FindPackage(SmoresDialog::CorePackageId());

	if (!TestTrue(TEXT("The base game's dialog package loaded"), Core && Core->bLoaded))
	{
		for (const FDialogProblem& Problem : Library.Problems)
		{
			AddError(Problem.ToString());
		}

		return true;
	}

	// every problem, warning or error, fails the sweep - naming exactly where it is
	for (const FDialogProblem& Problem : Library.Problems)
	{
		AddError(Problem.ToString());
	}

	if (!TestTrue(TEXT("The base game has barks at all"), Library.Barks.Num() > 0))
	{
		return true;
	}

	// every event has something to say, and something to say whatever the circumstances - a generic
	// line with no conditions, so a bark never falls silent just because nothing specific applied
	for (const EBarkEvent Event : SmoresDialog::GetAllBarkEvents())
	{
		const TArray<const FBarkLine*> Lines = Library.GetBarksForEvent(Event);
		const FString EventName = SmoresDialog::GetBarkEventName(Event);

		TestTrue(FString::Printf(TEXT("%s has at least one bark"), *EventName), Lines.Num() > 0);

		TestTrue(FString::Printf(TEXT("%s has a generic line (no conditions) to fall back on"), *EventName), Lines.ContainsByPredicate([](const FBarkLine* Line)
		{
			return Line->Condition.GetSpecificity() == 0;
		}));

		TestTrue(FString::Printf(TEXT("%s has at least one more specific line"), *EventName), Lines.ContainsByPredicate([](const FBarkLine* Line)
		{
			return Line->Condition.GetSpecificity() > 0;
		}));
	}

	// a file saved in the wrong encoding decodes to replacement characters rather than failing
	for (const FBarkLine& Line : Library.Barks)
	{
		TestFalse(FString::Printf(TEXT("%s (%s:%d) decoded cleanly - save the file as UTF-8"), *Line.Id.ToString(), *Line.File, Line.Line), Line.SourceText.Contains(TEXT("\uFFFD")));
	}

	for (const FDialogTranslation& Translation : Library.Translations)
	{
		TestFalse(FString::Printf(TEXT("The %s translation of %s decoded cleanly - save the file as UTF-8"), *Translation.Culture, *Translation.LineId.ToString()), Translation.Text.Contains(TEXT("\uFFFD")));
	}

	TestTrue(TEXT("The base game ships at least one translation - the localization proof's content"), Library.Translations.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogExampleModTest,
	"Smores.Content.Dialog.ExampleModLoadsCleanly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogExampleModTest::RunTest(const FString& Parameters)
{
	const FDialogLibrary Library = SmoresDialogContentTest_Load(/*bIncludeMods*/ true);
	const FName ExampleId(TEXT("example"));

	const FDialogPackageInfo* Example = Library.FindPackage(ExampleId);

	if (!TestTrue(TEXT("The example mod (Mods/example) loaded"), Example && Example->bLoaded))
	{
		return true;
	}

	// only the example's own problems - a developer's other local mods are none of this test's business
	for (const FDialogProblem& Problem : Library.Problems)
	{
		if (FName(*Problem.Package) == ExampleId)
		{
			AddError(Problem.ToString());
		}
	}

	TestTrue(TEXT("It adds at least one bark"), Example->NumBarks > 0);

	// its whole point: every bark it adds outranks every base-game line for the same event, so it wins
	// in play without touching core's files
	for (const FBarkLine& Line : Library.Barks)
	{
		if (Line.PackageId != ExampleId)
		{
			continue;
		}

		for (const FBarkLine* CoreLine : Library.GetBarksForEvent(Line.Event))
		{
			if (CoreLine->PackageId == SmoresDialog::CorePackageId())
			{
				TestTrue(FString::Printf(TEXT("%s is more specific than %s"), *Line.Id.ToString(), *CoreLine->Id.ToString()),
					Line.Condition.GetSpecificity() > CoreLine->Condition.GetSpecificity());
			}
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
