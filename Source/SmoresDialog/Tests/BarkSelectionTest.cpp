// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BarkSelection.h"
#include "DialogLibrary.h"
#include "DialogLoader.h"
#include "Tests/SmoresTestWorld.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  Bark selection: most clauses matched wins; a tie goes to the line said least recently, then to
 *  weight; cooldowns are per speaker. Each of those is a rule a player would only ever notice as
 *  "the guard keeps saying the same thing", which is exactly why it is pinned here rather than
 *  judged in PIE.
 *
 *  Lines are loaded from CSV text through the real loader, so a test reads like the file a writer
 *  would write. Facts answer from a map; see SmoresDialogTestFactory.h.
 */

/** Loads Rows as core's barks and hands back the Hurt lines, which is the event every test here uses */
static FDialogLibrary SmoresBarkSelectionTest_Load(FAutomationTestBase& Test, const FDialogFactRegistry& Facts, const TArray<FString>& Rows)
{
	FDialogLibrary Library = SmoresDialog::LoadPackages({ MakeTestCorePackage(MakeTestBarkCsv(Rows)) }, Facts, nullptr);

	Test.TestEqual(FString::Printf(TEXT("The test lines load cleanly:\n%s"), *DescribeDialogProblems(Library)), Library.Problems.Num(), 0);

	return Library;
}

/** The winner's local id, or "(none)" - so a failed assertion reads as the line that won */
static FString SmoresBarkSelectionTest_Pick(const FDialogLibrary& Library, const FDialogFactRegistry& Facts, const FBarkMemory& Memory,
	const UObject* Speaker, double Now, FRandomStream& Stream, FBarkSelection* OutSelection = nullptr)
{
	const FBarkLine* Winner = SmoresDialog::SelectBark(Library.GetBarksForEvent(EBarkEvent::Hurt), FDialogContext(), Facts, Memory, FObjectKey(Speaker), Now, Stream, OutSelection);

	return Winner ? Winner->LocalId.ToString() : FString(TEXT("(none)"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkMostSpecificWinsTest,
	"Smores.Dialog.Barks.MostSpecificWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkMostSpecificWinsTest::RunTest(const FString& Parameters)
{
	// speakers are actors in play, and an actor is what FObjectKey is taken from here too - a bare
	// NewObject<UObject>() trips an abstract-class ensure that fires once per session (testing.md)
	FSmoresTestWorld TestWorld;

	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FDialogLibrary Library = SmoresBarkSelectionTest_Load(*this, Facts, {
		TEXT("generic,Hurt,,Ow.,1,0"),
		TEXT("guard,Hurt,Speaker.Role == guard,Guard ow.,1,0"),
		TEXT("guard_hated,Hurt,Speaker.Role == guard; StandingWithSpeaker <= -20,Hated guard ow.,1,0"),
		TEXT("guard_armed_hated,Hurt,Speaker.Role == guard; StandingWithSpeaker <= -20; Speaker.IsArmed,Armed hated guard ow.,1,0")
	});

	AActor* Speaker = TestWorld.SpawnOwner();

	if (!TestNotNull(TEXT("The speaker spawned"), Speaker))
	{
		return true;
	}
	FBarkMemory Memory;
	FRandomStream Stream(1);

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("guard")));
	Answers->Add(TEXT("StandingWithSpeaker"), FDialogValue::MakeNumber(-30));
	Answers->Add(TEXT("Speaker.IsArmed"), FDialogValue::MakeBool(true));

	TestEqual(TEXT("Everything matches: the three-clause line wins"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Speaker, 0.0, Stream), FString(TEXT("guard_armed_hated")));

	Answers->Add(TEXT("Speaker.IsArmed"), FDialogValue::MakeBool(false));
	TestEqual(TEXT("Unarmed: the three-clause line fails, so the two-clause one wins"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Speaker, 0.0, Stream), FString(TEXT("guard_hated")));

	Answers->Add(TEXT("StandingWithSpeaker"), FDialogValue::MakeNumber(0));
	TestEqual(TEXT("Not hated: the role line wins"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Speaker, 0.0, Stream), FString(TEXT("guard")));

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("trader")));

	FBarkSelection Selection;
	TestEqual(TEXT("Not a guard: only the generic line holds"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Speaker, 0.0, Stream, &Selection), FString(TEXT("generic")));

	// the explanation SmoresTestBark prints: every line accounted for, with the failed clause named
	TestEqual(TEXT("The selection accounts for every line"), Selection.Candidates.Num(), 4);

	const FBarkCandidate* GuardCandidate = Selection.Candidates.FindByPredicate([](const FBarkCandidate& Candidate)
	{
		return Candidate.Line->LocalId == FName(TEXT("guard"));
	});

	if (TestNotNull(TEXT("The guard line is in the account"), GuardCandidate))
	{
		TestTrue(TEXT("...marked as a failed condition"), GuardCandidate->Outcome == EBarkCandidateOutcome::ConditionFailed);
		TestEqual(TEXT("...on its first clause"), GuardCandidate->FailedClause, 0);
	}

	const TArray<FString> Explanation = Selection.Explain();
	TestTrue(TEXT("The explanation leads with the winner"), Explanation.Num() == 4 && Explanation[0].StartsWith(TEXT("WON")) && Explanation[0].Contains(TEXT("core.generic")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkLeastRecentlySaidTest,
	"Smores.Dialog.Barks.TiesGoToLeastRecentlySaid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkLeastRecentlySaidTest::RunTest(const FString& Parameters)
{
	// speakers are actors in play, and an actor is what FObjectKey is taken from here too - a bare
	// NewObject<UObject>() trips an abstract-class ensure that fires once per session (testing.md)
	FSmoresTestWorld TestWorld;

	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// weights deliberately lopsided: recency must decide before weight is ever consulted
	const FDialogLibrary Library = SmoresBarkSelectionTest_Load(*this, Facts, {
		TEXT("first,Hurt,,One.,100,0"),
		TEXT("second,Hurt,,Two.,1,0"),
		TEXT("third,Hurt,,Three.,1,0")
	});

	AActor* Guard = TestWorld.SpawnOwner();
	AActor* OtherGuard = TestWorld.SpawnOwner();

	if (!TestTrue(TEXT("Two distinct speakers spawned"), Guard && OtherGuard && Guard != OtherGuard))
	{
		return true;
	}
	FBarkMemory Memory;
	FRandomStream Stream(7);

	const FBarkLine* First = Library.FindBark(TEXT("core.first"));
	const FBarkLine* Second = Library.FindBark(TEXT("core.second"));
	const FBarkLine* Third = Library.FindBark(TEXT("core.third"));

	if (!TestTrue(TEXT("All three lines loaded"), First && Second && Third))
	{
		return true;
	}

	Memory.Record(*First, FObjectKey(Guard), 1.0);
	Memory.Record(*Second, FObjectKey(Guard), 2.0);

	TestEqual(TEXT("The line never said beats two that were, whatever their weight"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Guard, 3.0, Stream), FString(TEXT("third")));

	Memory.Record(*Third, FObjectKey(Guard), 3.0);

	TestEqual(TEXT("Once all are said, the one said longest ago wins"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Guard, 4.0, Stream), FString(TEXT("first")));

	// recency is about the line, not the speaker - a second guard also avoids what the first just said
	TestEqual(TEXT("A different speaker is steered the same way"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, OtherGuard, 4.0, Stream), FString(TEXT("first")));

	// selection only reads memory; recording is the caller's job, so asking twice changes nothing
	TestEqual(TEXT("Asking again gives the same answer"), SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Guard, 4.0, Stream), FString(TEXT("first")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkWeightBreaksTiesTest,
	"Smores.Dialog.Barks.WeightOnlyBreaksFullTies",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkWeightBreaksTiesTest::RunTest(const FString& Parameters)
{
	// speakers are actors in play, and an actor is what FObjectKey is taken from here too - a bare
	// NewObject<UObject>() trips an abstract-class ensure that fires once per session (testing.md)
	FSmoresTestWorld TestWorld;

	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FDialogLibrary Library = SmoresBarkSelectionTest_Load(*this, Facts, {
		TEXT("heavy,Hurt,,Heavy.,3,0"),
		TEXT("light,Hurt,,Light.,1,0")
	});

	AActor* Speaker = TestWorld.SpawnOwner();

	if (!TestNotNull(TEXT("The speaker spawned"), Speaker))
	{
		return true;
	}
	const FBarkMemory Empty;

	// one fixed stream, so this is one deterministic outcome rather than a flaky distribution check -
	// the tolerance only has to survive a correct change to how the stream is consumed (testing.md)
	FRandomStream Stream(12345);
	int32 HeavyWins = 0;
	const int32 Rolls = 4000;

	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		if (SmoresBarkSelectionTest_Pick(Library, Facts, Empty, Speaker, 0.0, Stream) == TEXT("heavy"))
		{
			++HeavyWins;
		}
	}

	TestTrue(FString::Printf(TEXT("Weight 3 against 1 wins about three times in four (%d of %d)"), HeavyWins, Rolls), HeavyWins > 2800 && HeavyWins < 3200);

	// ...but weight never outranks specificity
	const FDialogLibrary Specific = SmoresBarkSelectionTest_Load(*this, Facts, {
		TEXT("generic_heavy,Hurt,,Generic.,1000,0"),
		TEXT("guard_light,Hurt,Speaker.Role == guard,Guard.,1,0")
	});

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("guard")));

	int32 GuardWins = 0;

	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		if (SmoresBarkSelectionTest_Pick(Specific, Facts, Empty, Speaker, 0.0, Stream) == TEXT("guard_light"))
		{
			++GuardWins;
		}
	}

	TestEqual(TEXT("The more specific line wins every time, whatever the other's weight"), GuardWins, 200);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkCooldownPerSpeakerTest,
	"Smores.Dialog.Barks.CooldownsArePerSpeaker",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkCooldownPerSpeakerTest::RunTest(const FString& Parameters)
{
	// speakers are actors in play, and an actor is what FObjectKey is taken from here too - a bare
	// NewObject<UObject>() trips an abstract-class ensure that fires once per session (testing.md)
	FSmoresTestWorld TestWorld;

	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FDialogLibrary Library = SmoresBarkSelectionTest_Load(*this, Facts, {
		TEXT("generic,Hurt,,Ow.,1,0"),
		TEXT("guard,Hurt,Speaker.Role == guard,Guard ow.,1,10")
	});

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("guard")));

	AActor* Guard = TestWorld.SpawnOwner();
	AActor* OtherGuard = TestWorld.SpawnOwner();

	if (!TestTrue(TEXT("Two distinct speakers spawned"), Guard && OtherGuard && Guard != OtherGuard))
	{
		return true;
	}
	FBarkMemory Memory;
	FRandomStream Stream(3);

	const FBarkLine* GuardLine = Library.FindBark(TEXT("core.guard"));

	if (!TestNotNull(TEXT("The guard line loaded"), GuardLine))
	{
		return true;
	}

	Memory.Record(*GuardLine, FObjectKey(Guard), 0.0);

	FBarkSelection Selection;
	TestEqual(TEXT("5s later the same guard can't repeat it, and falls back to the generic line"),
		SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Guard, 5.0, Stream, &Selection), FString(TEXT("generic")));

	const FBarkCandidate* Waiting = Selection.Candidates.FindByPredicate([](const FBarkCandidate& Candidate)
	{
		return Candidate.Line->LocalId == FName(TEXT("guard"));
	});

	TestTrue(TEXT("...and the account says it was waiting out its cooldown"), Waiting && Waiting->Outcome == EBarkCandidateOutcome::OnCooldown);

	TestEqual(TEXT("A second guard may say it at the same moment"),
		SmoresBarkSelectionTest_Pick(Library, Facts, Memory, OtherGuard, 5.0, Stream), FString(TEXT("guard")));

	TestEqual(TEXT("The first guard may say it again once the full cooldown has passed"),
		SmoresBarkSelectionTest_Pick(Library, Facts, Memory, Guard, 10.0, Stream), FString(TEXT("guard")));

	// with every line on cooldown, nothing is said at all
	const FBarkLine* GenericLine = Library.FindBark(TEXT("core.generic"));
	const FDialogLibrary OnlyCooled = SmoresBarkSelectionTest_Load(*this, Facts, { TEXT("only,Hurt,,Only.,1,10") });
	const FBarkLine* OnlyLine = OnlyCooled.FindBark(TEXT("core.only"));

	if (TestTrue(TEXT("The single-line library loaded"), GenericLine && OnlyLine))
	{
		Memory.Record(*OnlyLine, FObjectKey(Guard), 20.0);

		const FBarkLine* Winner = SmoresDialog::SelectBark(OnlyCooled.GetBarksForEvent(EBarkEvent::Hurt), FDialogContext(), Facts, Memory, FObjectKey(Guard), 25.0, Stream);
		TestNull(TEXT("A speaker whose every line is cooling says nothing"), Winner);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
