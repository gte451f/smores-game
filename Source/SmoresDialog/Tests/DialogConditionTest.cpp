// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogCondition.h"
#include "DialogFacts.h"
#include "DialogLoader.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  The condition language: one grammar for every piece of selection dialog will ever do, so a bug
 *  here is a bug in every bark and every conversation at once. Checked at load time - these tests
 *  pin that a mistake is an error with a reason, never a rule that silently never matches.
 */

/** Compiles against the test facts with every subject available; the errors joined for a failure message */
static bool SmoresDialogConditionTest_Compile(const FDialogFactRegistry& Facts, const FString& Text, FDialogCondition& OutCondition, FString& OutErrors,
	EDialogSubject Subjects = AllTestDialogSubjects(), const FDialogKnownIds* KnownIds = nullptr, TArray<FString>* OutWarnings = nullptr)
{
	TArray<FString> Errors;
	TArray<FString> Warnings;

	const bool bCompiled = SmoresDialog::CompileCondition(Text, Facts, Subjects, KnownIds, OutCondition, Errors, Warnings);

	OutErrors = FString::Join(Errors, TEXT(" | "));

	if (OutWarnings)
	{
		*OutWarnings = Warnings;
	}

	return bCompiled;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogConditionEvaluatesTest,
	"Smores.Dialog.Condition.ParsesAndEvaluates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogConditionEvaluatesTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);
	const FDialogContext Context;

	FDialogCondition Condition;
	FString Errors;

	if (!TestTrue(TEXT("The roadmap's example compiles"), SmoresDialogConditionTest_Compile(Facts,
		TEXT("Speaker.Role == guard; StandingWithSpeaker <= -20; Speaker.IsArmed"), Condition, Errors)))
	{
		AddInfo(Errors);
		return true;
	}

	TestEqual(TEXT("Three clauses, so three toward specificity"), Condition.GetSpecificity(), 3);

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("Guard")));
	Answers->Add(TEXT("StandingWithSpeaker"), FDialogValue::MakeNumber(-20));
	Answers->Add(TEXT("Speaker.IsArmed"), FDialogValue::MakeBool(true));

	int32 FailedClause = 99;
	TestTrue(TEXT("Holds when every clause holds - names match case-insensitively, <= includes the bound"), Condition.Evaluate(Context, Facts, &FailedClause));
	TestEqual(TEXT("...and reports no failed clause"), FailedClause, (int32)INDEX_NONE);

	Answers->Add(TEXT("StandingWithSpeaker"), FDialogValue::MakeNumber(-19));
	TestFalse(TEXT("Fails when one clause fails"), Condition.Evaluate(Context, Facts, &FailedClause));
	TestEqual(TEXT("...naming the clause that failed"), FailedClause, 1);

	Answers->Add(TEXT("StandingWithSpeaker"), FDialogValue::MakeNumber(-50));
	Answers->Add(TEXT("Speaker.IsArmed"), FDialogValue::MakeBool(false));
	TestFalse(TEXT("A bare yes/no fact means == true"), Condition.Evaluate(Context, Facts));

	// an unset answer - the subject isn't there - makes every comparison false, including !=
	FDialogCondition NotRaiders;

	if (TestTrue(TEXT("A != clause compiles"), SmoresDialogConditionTest_Compile(Facts, TEXT("Listener.Faction != Raiders"), NotRaiders, Errors)))
	{
		TestFalse(TEXT("An absent listener never satisfies != Raiders"), NotRaiders.Evaluate(Context, Facts));

		Answers->Add(TEXT("Listener.Faction"), FDialogValue::MakeName(TEXT("Ironclan")));
		TestTrue(TEXT("...but a listener from another faction does"), NotRaiders.Evaluate(Context, Facts));
	}

	// a fact on both sides
	FDialogCondition SameFaction;

	if (TestTrue(TEXT("Fact-to-fact compiles"), SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Faction == Listener.Faction"), SameFaction, Errors)))
	{
		Answers->Add(TEXT("Speaker.Faction"), FDialogValue::MakeName(TEXT("Ironclan")));
		TestTrue(TEXT("Two facts with the same answer compare equal"), SameFaction.Evaluate(Context, Facts));

		Answers->Add(TEXT("Speaker.Faction"), FDialogValue::MakeName(TEXT("Raiders")));
		TestFalse(TEXT("...and different answers don't"), SameFaction.Evaluate(Context, Facts));
	}

	// numbers, decimals, and every ordering
	FDialogCondition Health;

	if (TestTrue(TEXT("Decimal comparisons compile"), SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Health > 0.25; Speaker.Health < 0.5; Speaker.Health >= 0.3"), Health, Errors)))
	{
		Answers->Add(TEXT("Speaker.Health"), FDialogValue::MakeNumber(0.3));
		TestTrue(TEXT("0.3 is above 0.25, below 0.5 and at least 0.3"), Health.Evaluate(Context, Facts));

		Answers->Add(TEXT("Speaker.Health"), FDialogValue::MakeNumber(0.5));
		TestFalse(TEXT("0.5 is not below 0.5"), Health.Evaluate(Context, Facts));
	}

	// quoted names, a trailing semicolon, and the call form
	FDialogCondition Quoted;

	if (TestTrue(TEXT("Quoted names, trailing ';' and a call compile"), SmoresDialogConditionTest_Compile(Facts, TEXT("  Speaker.Role == \"night watch\" ; Flag(met_kess);  "), Quoted, Errors)))
	{
		TestEqual(TEXT("An empty trailing clause costs nothing"), Quoted.GetSpecificity(), 2);

		Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("Night Watch")));
		Answers->Add(TEXT("Flag:met_kess"), FDialogValue::MakeBool(true));
		TestTrue(TEXT("A quoted name with a space matches, and the call reads its argument"), Quoted.Evaluate(Context, Facts));

		Answers->Remove(TEXT("Flag:met_kess"));
		TestFalse(TEXT("...and an unset flag is false"), Quoted.Evaluate(Context, Facts));
	}

	// the empty condition is a generic line: it always holds and is the least specific there is
	FDialogCondition Always;

	if (TestTrue(TEXT("An empty condition compiles"), SmoresDialogConditionTest_Compile(Facts, TEXT(""), Always, Errors)))
	{
		TestEqual(TEXT("...with no clauses"), Always.GetSpecificity(), 0);
		TestTrue(TEXT("...and always holds"), Always.Evaluate(Context, Facts));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogConditionRejectsTest,
	"Smores.Dialog.Condition.RejectsUnknownFactsAndBadSyntax",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogConditionRejectsTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// each mistake, and a fragment of the reason a writer should be given for it
	const TArray<TPair<FString, FString>> Cases = {
		{ TEXT("Speaker.Roel == guard"),                     TEXT("isn't a fact") },
		{ TEXT("Speaker.Faction == Listener.Factoin"),       TEXT("isn't a fact") },
		{ TEXT("Speaker.Role = guard"),                      TEXT("use '=='") },
		{ TEXT("Speaker.Role == "),                          TEXT("right-hand side") },
		{ TEXT("Speaker.Role == guard guard"),               TEXT("more follows") },
		{ TEXT("20 <= StandingWithSpeaker"),                 TEXT("has to start with a fact") },
		{ TEXT("StandingWithSpeaker <= guard"),              TEXT("is a number") },
		{ TEXT("Speaker.Role < guard"),                      TEXT("only compares with") },
		{ TEXT("Speaker.Role"),                              TEXT("needs comparing") },
		{ TEXT("Speaker.LifeState == Dwned"),                TEXT("is one of") },
		{ TEXT("Speaker.Role == \"guard"),                   TEXT("closing") },
		{ TEXT("Flag == true"),                              TEXT("needs an id in brackets") },
		{ TEXT("Speaker.Role(x) == guard"),                  TEXT("doesn't take") },
		{ TEXT("Flag(met_kess"),                             TEXT("closing ')'") },
		{ TEXT("Speaker.Health > 0.5abc"),                   TEXT("isn't a number") },
		{ TEXT("Speaker.Role == guard & armed"),             TEXT("unexpected '&'") },
		{ TEXT("Speaker.Role == guard; Speaker.Role==guard"), TEXT("written twice") }
	};

	for (const TPair<FString, FString>& Case : Cases)
	{
		FDialogCondition Condition;
		FString Errors;

		const bool bCompiled = SmoresDialogConditionTest_Compile(Facts, Case.Key, Condition, Errors);

		TestFalse(FString::Printf(TEXT("'%s' is rejected"), *Case.Key), bCompiled);
		TestTrue(FString::Printf(TEXT("'%s' says why (wanted '%s', got '%s')"), *Case.Key, *Case.Value, *Errors), Errors.Contains(Case.Value));
		TestEqual(FString::Printf(TEXT("'%s' leaves nothing half-built"), *Case.Key), Condition.GetSpecificity(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogConditionSubjectsTest,
	"Smores.Dialog.Condition.RejectsSubjectsTheEventLacks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogConditionSubjectsTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FDialogCondition Condition;
	FString Errors;

	// Downed carries only the speaker: a Listener clause there could never be true
	TestFalse(TEXT("A Listener fact is rejected where only the speaker exists"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Listener.Role == guard"), Condition, Errors, SmoresDialog::GetEventSubjects(EBarkEvent::Downed)));
	TestTrue(FString::Printf(TEXT("...naming who is missing (got '%s')"), *Errors), Errors.Contains(TEXT("Listener")));

	// StandingWithSpeaker reads two subjects, and both have to be there
	TestFalse(TEXT("A fact needing the player is rejected where there is no player"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("StandingWithSpeaker < 0"), Condition, Errors, EDialogSubject::Speaker));
	TestTrue(TEXT("...and accepted where there is one"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("StandingWithSpeaker < 0"), Condition, Errors, EDialogSubject::Speaker | EDialogSubject::Player));

	TestTrue(TEXT("A Listener fact is accepted where the event carries a listener"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Listener.Role == guard"), Condition, Errors, SmoresDialog::GetEventSubjects(EBarkEvent::TradeOpened)));

	// the real facts and the real events agree: every built-in fact is usable somewhere
	const FDialogFactRegistry BuiltIn = FDialogFactRegistry::MakeBuiltIn();

	for (const FDialogFact& Fact : BuiltIn.GetFacts())
	{
		const bool bUsableSomewhere = SmoresDialog::GetAllBarkEvents().ContainsByPredicate([&Fact](EBarkEvent Event)
		{
			return (Fact.Reads & ~SmoresDialog::GetEventSubjects(Event)) == EDialogSubject::None;
		});

		TestTrue(FString::Printf(TEXT("Built-in fact '%s' can be asked in at least one event - a fact no event carries is a promise the game doesn't keep"), *Fact.Name.ToString()), bUsableSomewhere);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogConditionUnknownIdTest,
	"Smores.Dialog.Condition.UnknownContentIdIsAWarning",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogConditionUnknownIdTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FDialogKnownIds KnownIds;
	KnownIds.IdsByDomain.Add(SmoresDialog::FactionDomain(), { FName(TEXT("Raiders")) });

	FDialogCondition Condition;
	FString Errors;
	TArray<FString> Warnings;

	// a faction nobody loaded may be another mod's - kept, and flagged
	TestTrue(TEXT("An unknown faction id still compiles"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Faction == Raidrs"), Condition, Errors, AllTestDialogSubjects(), &KnownIds, &Warnings));
	TestEqual(TEXT("...with one warning"), Warnings.Num(), 1);
	TestTrue(TEXT("...that names what wasn't found"), Warnings.Num() == 1 && Warnings[0].Contains(TEXT("no faction called 'Raidrs'")));

	TestTrue(TEXT("A known faction compiles"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Faction == Raiders"), Condition, Errors, AllTestDialogSubjects(), &KnownIds, &Warnings));
	TestEqual(TEXT("...silently"), Warnings.Num(), 0);

	TestTrue(TEXT("None - unaffiliated - is always known"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Faction == None"), Condition, Errors, AllTestDialogSubjects(), &KnownIds, &Warnings));
	TestEqual(TEXT("...silently"), Warnings.Num(), 0);

	TestTrue(TEXT("A domain with no known list isn't checked"),
		SmoresDialogConditionTest_Compile(Facts, TEXT("Speaker.Role == anything"), Condition, Errors, AllTestDialogSubjects(), &KnownIds, &Warnings));
	TestEqual(TEXT("...so there is nothing to warn about"), Warnings.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogConditionLineNumberTest,
	"Smores.Dialog.Condition.ErrorsCarryTheLineNumber",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogConditionLineNumberTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// header on line 1; the typo is on line 3, and line 5 follows a blank line 4
	const FString Csv =
		TEXT("Id,Event,Conditions,Text,Weight,Cooldown\n")
		TEXT("fine,Hurt,Speaker.Role == guard,Ow.,1,0\n")
		TEXT("typo,Hurt,Speaker.Rol == guard,Ow.,1,0\n")
		TEXT("\n")
		TEXT("wrong_subject,Downed,Listener.Role == guard,Down.,1,0\n");

	const FDialogLibrary Library = SmoresDialog::LoadPackages({ MakeTestCorePackage(Csv) }, Facts, nullptr);

	TestTrue(FString::Printf(TEXT("The unknown fact is reported against line 3:\n%s"), *DescribeDialogProblems(Library)),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("'Speaker.Rol' isn't a fact"), 3));
	TestTrue(FString::Printf(TEXT("The subject Downed doesn't carry is reported against line 5:\n%s"), *DescribeDialogProblems(Library)),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("Listener"), 5));
	TestEqual(TEXT("Only the good row loaded"), Library.Barks.Num(), 1);
	TestNotNull(TEXT("...and it is the one on line 2"), Library.FindBark(TEXT("core.fine")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
