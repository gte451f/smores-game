// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogLibrary.h"
#include "DialogLoader.h"
#include "ConversationScript.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  Loading conversations: what a node's headers become, and everything ysc lets through that the
 *  game can't - a line with a made-up id, a function or effect nobody has, the wrong number of
 *  words, an unknown #reason:, a bad header, a stale compile, and what an Ambient conversation may
 *  do. Each is refused for that conversation alone, with its file and line, while its neighbours load.
 *
 *  The scripts are the tests' own (Tests/Conversations); the line a problem should name is found in
 *  the script's text here rather than written as a number, so editing a fixture can't quietly shift
 *  what is being checked.
 */

/** The 1-based line of Text holding Needle, or 0 */
static int32 SmoresConversationLoaderTest_LineOf(const FString& Text, const TCHAR* Needle)
{
	TArray<FString> Lines;
	Text.ParseIntoArray(Lines, TEXT("\n"), /*InCullEmpty*/ false);

	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Contains(Needle, ESearchCase::CaseSensitive))
		{
			return Index + 1;
		}
	}

	return 0;
}

/** True if an error mentioning Fragment is pinned to File at the line holding Needle */
static bool SmoresConversationLoaderTest_HasErrorAt(const FDialogLibrary& Library, const TCHAR* Fragment, const FString& File, const FString& Source, const TCHAR* Needle)
{
	const int32 Line = SmoresConversationLoaderTest_LineOf(Source, Needle);

	return Line > 0 && Library.Problems.ContainsByPredicate([Fragment, &File, Line](const FDialogProblem& Problem)
	{
		return Problem.Severity == EDialogProblemSeverity::Error && Problem.File == File && Problem.Line == Line && Problem.Message.Contains(Fragment);
	});
}

/** Loads one test script as the base game's package */
static FDialogLibrary SmoresConversationLoaderTest_Load(const FDialogFactRegistry& Facts, const TCHAR* Name, FString& OutSource, TFunctionRef<void(FDialogPackageSource&)> Tamper)
{
	FDialogPackageSource Core = MakeTestCorePackage();
	AddTestConversationFiles(Core, Name);

	if (const FDialogSourceFile* Yarn = FindTestSourceFile(Core, FString::Printf(TEXT("conversations/%s.yarn"), Name)))
	{
		OutSource = Yarn->Contents;
	}

	Tamper(Core);

	return SmoresDialog::LoadPackages({ Core }, Facts, nullptr);
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationLoadsHeadersTest,
	"Smores.Dialog.Conversation.LoadsNodesWithHeaders",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  A node with our headers is a conversation and one without is a plain node; the headers become
 *  its metadata; every line's text reaches the package's texts with the speaker cue taken off,
 *  keyed by its qualified id; a #reason: tag lands on its choice; and a translation of a
 *  conversation line loses a cue the translator kept.
 */
bool FSmoresConversationLoadsHeadersTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FString Source;
	const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("shakedown"), Source, [](FDialogPackageSource& Core)
	{
		Core.Files.Add({ TEXT("localization/fr/conversations.csv"), TEXT("Id,Text\nshakedown_toll,\"Bandit: P\u00E9age. Vingt pi\u00E8ces d'or.\"\n") });
	});

	TestEqual(TEXT("Loads cleanly"), DescribeDialogProblems(Library), FString(TEXT("(no problems)")));

	const FConversationDefinition* Shakedown = Library.FindConversation(TEXT("core.Shakedown"));

	if (!TestNotNull(TEXT("The node with headers is a conversation"), Shakedown))
	{
		return true;
	}

	TestNull(TEXT("The node without them isn't"), Library.FindConversation(TEXT("core.Choices")));
	TestEqual(TEXT("One conversation in all"), Library.Conversations.Num(), 1);
	TestTrue(TEXT("kind: Greeting"), Shakedown->Kind == EConversationKind::Greeting);
	TestEqual(TEXT("attach: is its condition"), Shakedown->Attach.ToString(), FString(TEXT("Speaker.Role == bandit")));
	TestEqual(TEXT("No requires: means nothing more is required"), Shakedown->Requires.GetSpecificity(), 0);
	TestEqual(TEXT("Its title line"), Shakedown->Line, SmoresConversationLoaderTest_LineOf(Source, TEXT("title: Shakedown")));
	TestTrue(TEXT("It carries its script"), Shakedown->Script.IsValid());

	const FDialogText* Toll = Library.FindText(TEXT("core.shakedown_toll"));

	if (TestNotNull(TEXT("A line's text is kept under its qualified id"), Toll))
	{
		TestEqual(TEXT("...with the speaker cue taken off"), Toll->SourceText, FString(TEXT("Toll road. Twenty gold, or you walk back the way you came.")));
	}

	TestEqual(TEXT("Every line's text is kept"), Library.Texts.Num(), 7);

	if (Shakedown->Script.IsValid())
	{
		const FConversationLineInfo* Pay = Shakedown->Script->FindLine(TEXT("line:shakedown_pay"));

		if (TestNotNull(TEXT("The pay choice is in the script"), Pay))
		{
			TestTrue(TEXT("...as a choice"), Pay->bIsOption && Pay->bHasCondition);
			TestEqual(TEXT("...carrying its #reason:"), Pay->ReasonKey, FName(TEXT("not_enough_money")));
			TestTrue(TEXT("...which is CannotAfford"), Pay->Reason == ESmoresRefusalReason::CannotAfford);
		}
	}

	const FDialogTranslation* French = Library.Translations.FindByPredicate([](const FDialogTranslation& Translation) { return Translation.LineId == FName(TEXT("core.shakedown_toll")); });

	if (TestNotNull(TEXT("A conversation line can be translated like a bark"), French))
	{
		TestEqual(TEXT("...and a cue the translator kept is taken off"), French->Text, FString(TEXT("P\u00E9age. Vingt pi\u00E8ces d'or.")));
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationLoadChecksTest,
	"Smores.Dialog.Conversation.LoadChecksNameFileAndLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  Every mistake in checks.yarn costs its own conversation, with an error pinned to the line a
 *  writer should look at - and the one conversation with nothing wrong loads beside them.
 */
bool FSmoresConversationLoadChecksTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FString Source;
	const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("checks"), Source, [](FDialogPackageSource&) {});
	const FString File = TEXT("conversations/checks.yarn");

	auto ExpectError = [&](const TCHAR* What, const TCHAR* Fragment, const TCHAR* Needle)
	{
		TestTrue(FString::Printf(TEXT("%s: an error at the line with '%s'\n%s"), What, Needle, *DescribeDialogProblems(Library)),
			SmoresConversationLoaderTest_HasErrorAt(Library, Fragment, File, Source, Needle));
	};

	TestNotNull(TEXT("The conversation with nothing wrong loads beside the broken ones"), Library.FindConversation(TEXT("core.Fine")));
	TestEqual(TEXT("...and it is the only one"), Library.Conversations.Num(), 1);

	ExpectError(TEXT("A line without a #line: id"), TEXT("has no #line: id of its own"), TEXT("never given an id"));
	ExpectError(TEXT("An unknown function"), TEXT("calls wealth()"), TEXT("wealth()"));
	ExpectError(TEXT("An unknown effect"), TEXT("runs <<GiveGold>>"), TEXT("<<GiveGold"));
	ExpectError(TEXT("An effect with the wrong number of words"), TEXT("<<TakeMoney>> takes 1 word"), TEXT("<<TakeMoney>>"));
	ExpectError(TEXT("A fact called with the wrong number of values"), TEXT("flag() takes 1 value, and this passes 0"), TEXT("<<if flag()>>"));
	ExpectError(TEXT("An unknown #reason: key"), TEXT("#reason:too_poor isn't a reason"), TEXT("Bribe him."));
	ExpectError(TEXT("A plain node with an unknown effect"), TEXT("runs <<Explode>>"), TEXT("<<Explode>>"));
	ExpectError(TEXT("...skips the conversation that plays it"), TEXT("it plays node 'Broken'"), TEXT("title: UsesBroken"));
	ExpectError(TEXT("A kind that isn't one"), TEXT("kind: 'Banter' isn't Greeting, Topic or Ambient"), TEXT("kind: Banter"));
	ExpectError(TEXT("A greeting with no attach:"), TEXT("needs attach:"), TEXT("title: NoAttach"));
	ExpectError(TEXT("A topic with no label:"), TEXT("a Topic needs label:"), TEXT("title: TopicNoLabel"));

	for (const TCHAR* Broken : { TEXT("UnknownFunction"), TEXT("UnknownCommand"), TEXT("WrongCount"), TEXT("WrongArity"), TEXT("BadReason"),
		TEXT("Untagged"), TEXT("UsesBroken"), TEXT("BadKind"), TEXT("NoAttach"), TEXT("TopicNoLabel") })
	{
		TestNull(FString::Printf(TEXT("%s was skipped"), Broken), Library.FindConversation(FName(*FString::Printf(TEXT("core.%s"), Broken))));
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationAmbientRulesTest,
	"Smores.Dialog.Conversation.AmbientRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  Banter plays with no window, so an Ambient conversation can't offer a choice or open trade, must
 *  name its participants, may say lines only as one of them, and attaches to nobody.
 */
bool FSmoresConversationAmbientRulesTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FString Source;
	const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("ambient"), Source, [](FDialogPackageSource&) {});
	const FString File = TEXT("conversations/ambient.yarn");

	auto ExpectError = [&](const TCHAR* What, const TCHAR* Fragment, const TCHAR* Needle)
	{
		TestTrue(FString::Printf(TEXT("%s: an error at the line with '%s'\n%s"), What, Needle, *DescribeDialogProblems(Library)),
			SmoresConversationLoaderTest_HasErrorAt(Library, Fragment, File, Source, Needle));
	};

	const FConversationDefinition* Good = Library.FindConversation(TEXT("core.GoodBanter"));

	if (TestNotNull(TEXT("A well-formed banter loads"), Good))
	{
		TestTrue(TEXT("...as Ambient"), Good->Kind == EConversationKind::Ambient);
		TestEqual(TEXT("...with its participants in order"), FString::JoinBy(Good->Participants, TEXT(", "), [](FName Name) { return Name.ToString(); }), FString(TEXT("A, B")));
	}

	TestEqual(TEXT("Nothing else does"), Library.Conversations.Num(), 1);

	ExpectError(TEXT("A choice"), TEXT("can't offer choices"), TEXT("-> Answer."));
	ExpectError(TEXT("A speaker who isn't a participant"), TEXT("'C' isn't one of its participants (A, B)"), TEXT("C: Nobody."));
	ExpectError(TEXT("An effect that needs the window"), TEXT("<<OpenTrade>> needs the conversation window"), TEXT("<<OpenTrade>>"));
	ExpectError(TEXT("No participants:"), TEXT("needs participants:"), TEXT("title: BanterNoParticipants"));
	ExpectError(TEXT("An attach:"), TEXT("attaches to nobody"), TEXT("attach: Speaker.Role == guard"));

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationStaleCompileTest,
	"Smores.Dialog.Conversation.StaleCompileIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  A writer who edits a .yarn and forgets to recompile would test old lines. Two ways it is caught:
 *  in the editor, a .yarnc older than its .yarn; and anywhere, a #line: id in the .yarn that the
 *  compiled lines table doesn't have.
 */
bool FSmoresConversationStaleCompileTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);
	FString Source;

#if WITH_EDITOR
	{
		const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("shakedown"), Source, [](FDialogPackageSource& Core)
		{
			FDialogSourceFile* Yarn = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarn"));
			FDialogSourceFile* Compiled = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarnc"));

			if (Yarn && Compiled)
			{
				Compiled->Timestamp = FDateTime(2026, 9, 1, 12, 0, 0);
				Yarn->Timestamp = FDateTime(2026, 9, 1, 12, 5, 0);
			}
		});

		TestEqual(TEXT("A .yarn edited after its compile loads nothing"), Library.Conversations.Num(), 0);
		TestTrue(TEXT("...and says to recompile"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("was edited after shakedown.yarnc was compiled")));
	}

	{
		const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("shakedown"), Source, [](FDialogPackageSource& Core)
		{
			FDialogSourceFile* Yarn = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarn"));
			FDialogSourceFile* Compiled = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarnc"));

			if (Yarn && Compiled)
			{
				Yarn->Timestamp = FDateTime(2026, 9, 1, 12, 0, 0);
				Compiled->Timestamp = FDateTime(2026, 9, 1, 12, 5, 0);
			}
		});

		TestEqual(TEXT("A compile newer than its .yarn loads"), Library.Conversations.Num(), 1);
	}
#endif

	{
		const FDialogLibrary Library = SmoresConversationLoaderTest_Load(Facts, TEXT("shakedown"), Source, [](FDialogPackageSource& Core)
		{
			if (FDialogSourceFile* Yarn = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarn")))
			{
				Yarn->Contents.ReplaceInline(TEXT("Bandit: Wrong answer. #line:shakedown_wrong"), TEXT("Bandit: Wrong answer. #line:shakedown_wrong\n    Bandit: Now run. #line:shakedown_run"));
			}
		});

		TestEqual(TEXT("A line added and never compiled loads nothing from the file"), Library.Conversations.Num(), 0);
		TestTrue(TEXT("...and names the line"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("#line:shakedown_run isn't in shakedown-Lines.csv")));
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationIdsShareThePackageTest,
	"Smores.Dialog.Conversation.LineIdsShareThePackage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  A conversation line's id and a bark's share the package's string table, so the same local id
 *  twice in one package is refused - while the same id in two packages is two different lines.
 */
bool FSmoresConversationIdsShareThePackageTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// the bark takes shakedown_toll first; the conversation's line of that id is refused, costing the
	// conversation, and the report says where the id was first used
	{
		FDialogPackageSource Core = MakeTestCorePackage(MakeTestBarkCsv({ TEXT("shakedown_toll,TradeOpened,,Hello.,1,0") }));
		AddTestConversationFiles(Core, TEXT("shakedown"));

		const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

		TestNotNull(TEXT("The bark keeps its id"), Library.FindBark(TEXT("core.shakedown_toll")));
		TestTrue(TEXT("The line reusing it is refused"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("#line:shakedown_toll is already used in this package (barks/test.csv")));
		TestNull(TEXT("...and so is its conversation"), Library.FindConversation(TEXT("core.Shakedown")));
	}

	// the same script in two packages is two sets of lines and two conversations
	{
		FDialogPackageSource Core = MakeTestCorePackage();
		AddTestConversationFiles(Core, TEXT("shakedown"));

		FDialogPackageSource Mod = MakeTestDialogPackage(TEXT("other"), TEXT("other"), { TEXT("core") });
		AddTestConversationFiles(Mod, TEXT("shakedown"));

		const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core, Mod }, Facts, nullptr);

		TestEqual(TEXT("Both load cleanly"), DescribeDialogProblems(Library), FString(TEXT("(no problems)")));
		TestNotNull(TEXT("core's"), Library.FindConversation(TEXT("core.Shakedown")));
		TestNotNull(TEXT("...and the mod's"), Library.FindConversation(TEXT("other.Shakedown")));
		TestNotNull(TEXT("...each with its own lines"), Library.FindText(TEXT("other.shakedown_toll")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
