// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ConversationPlayer.h"
#include "DialogLibrary.h"
#include "DialogLoader.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  The conversation player, playing the bandit shakedown - the four tests the Ink-vs-Yarn spike
 *  proved the player with, carried over onto the real loader and player.
 *
 *  The script is the tests' own copy (Tests/Conversations/shakedown), loaded through LoadPackages
 *  the way a package is, and played with the test facts: gold() answers from a map, and every
 *  command the script runs is written down rather than carried out. Nothing here has a screen or a
 *  world, which is the nearest this project gets to "a server with no UI".
 */

/** The game, as the shakedown sees it: a wallet it asks about, and a record of what it told us to do */
struct FSmoresConversationTestGame
{
	TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();

	FDialogFactRegistry Facts;

	TArray<FString> Commands;

	explicit FSmoresConversationTestGame(int32 Gold)
		: Facts(MakeTestDialogFacts(Answers))
	{
		Answers->Add(FName(TEXT("Gold")), FDialogValue::MakeNumber(Gold));
	}

	TUniquePtr<FDialogConversationPlayer> Start(const FConversationDefinition& Conversation)
	{
		FConversationPlayerSetup Setup;
		Setup.Script = Conversation.Script;
		Setup.StartNode = Conversation.LocalId.ToString();
		Setup.Facts = &Facts;
		Setup.RunCommand = [this](FName Command, const TArray<FString>& Arguments)
		{
			Commands.Add(FString::Printf(TEXT("%s %s"), *Command.ToString(), *FString::Join(Arguments, TEXT(" "))).TrimEnd());
			return true;
		};

		TUniquePtr<FDialogConversationPlayer> Player = MakeUnique<FDialogConversationPlayer>(MoveTemp(Setup));
		Player->Start();
		return Player;
	}
};

/** The fixture shakedown, loaded as the base game's package. Its facts must be the ones the player will ask. */
static FDialogLibrary SmoresConversationTest_LoadShakedown(FAutomationTestBase& Test, const FDialogFactRegistry& Facts)
{
	FDialogPackageSource Core = MakeTestCorePackage();

	if (!AddTestConversationFiles(Core, TEXT("shakedown")))
	{
		Test.AddError(FString::Printf(TEXT("the test script is missing from %s"), *GetTestConversationDirectory()));
	}

	FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

	if (Library.Problems.Num() > 0)
	{
		Test.AddError(TEXT("the shakedown should load cleanly:\n") + DescribeDialogProblems(Library));
	}

	return Library;
}

/** Where the conversation is, in one line - for failure messages */
static FString SmoresConversationTest_Describe(const FDialogConversationPlayer& Player)
{
	switch (Player.GetState())
	{
	case EConversationPlayerState::Line:
		return FString::Printf(TEXT("line %s (%s: \"%s\")"), *Player.GetLine().LineId.ToString(), *Player.GetLine().SpeakerCue, *Player.GetLine().SourceText);

	case EConversationPlayerState::Choices:
	{
		TArray<FString> Ids;

		for (const FConversationPlayerChoice& Choice : Player.GetChoices())
		{
			Ids.Add(FString::Printf(TEXT("%s%s"), !Choice.bShown ? TEXT("-") : !Choice.bAvailable ? TEXT("!") : TEXT(""), *Choice.LineId.ToString()));
		}

		return FString::Printf(TEXT("choices [%s]"), *FString::Join(Ids, TEXT(", ")));
	}

	case EConversationPlayerState::Ended:
		return TEXT("ended");

	default:
		return FString::Printf(TEXT("failed: %s"), *Player.GetError());
	}
}

/** True if the player is showing exactly this line, said by the NPC; reports what it showed instead if not */
static bool SmoresConversationTest_ExpectLine(FAutomationTestBase& Test, const FDialogConversationPlayer& Player, const TCHAR* Id, const TCHAR* Text)
{
	if (Player.GetState() != EConversationPlayerState::Line || Player.GetLine().LineId != FName(Id))
	{
		Test.AddError(FString::Printf(TEXT("expected line %s, got %s"), Id, *SmoresConversationTest_Describe(Player)));
		return false;
	}

	Test.TestEqual(FString::Printf(TEXT("%s's speaker cue"), Id), Player.GetLine().SpeakerCue, FString(TEXT("Bandit")));
	Test.TestTrue(FString::Printf(TEXT("%s is said by the NPC"), Id), Player.GetLine().SpeakerSlot == SmoresDialog::NpcSlot);
	Test.TestEqual(FString::Printf(TEXT("%s's text, cue removed"), Id), Player.GetLine().SourceText, FString(Text));
	return true;
}

/**
 *  True if the choices are exactly these, in order. "!" in front means shown greyed out (unavailable,
 *  with a #reason:), "-" means left off (unavailable, no reason). Yarn itself never hides a choice;
 *  the window decides from the tag.
 */
static bool SmoresConversationTest_ExpectChoices(FAutomationTestBase& Test, const FDialogConversationPlayer& Player, const FString& Expected)
{
	const FString Described = SmoresConversationTest_Describe(Player);

	if (Described != FString::Printf(TEXT("choices [%s]"), *Expected))
	{
		Test.AddError(FString::Printf(TEXT("expected choices [%s], got %s"), *Expected, *Described));
		return false;
	}

	return true;
}

/** Picks the choice with this id */
static bool SmoresConversationTest_Choose(FAutomationTestBase& Test, FDialogConversationPlayer& Player, const TCHAR* Id)
{
	const int32 Index = Player.GetState() == EConversationPlayerState::Choices
		? Player.GetChoices().IndexOfByPredicate([Id](const FConversationPlayerChoice& Choice) { return Choice.LineId == FName(Id); })
		: INDEX_NONE;

	if (Index == INDEX_NONE || !Player.Choose(Index))
	{
		Test.AddError(FString::Printf(TEXT("couldn't choose %s at %s"), Id, *SmoresConversationTest_Describe(Player)));
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationPlaysTheSceneTest,
	"Smores.Dialog.Conversation.PlaysTheScene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  The whole "ask, then refuse" path: every line's qualified id, speaker and text; the choices at
 *  each point; the script's own memory leaving the question off once asked (no #reason:, so it
 *  isn't shown greyed); and the command arriving with its arguments before the line after it.
 */
bool FSmoresConversationPlaysTheSceneTest::RunTest(const FString& Parameters)
{
	FSmoresConversationTestGame Game(50);
	const FDialogLibrary Library = SmoresConversationTest_LoadShakedown(*this, Game.Facts);
	const FConversationDefinition* Shakedown = Library.FindConversation(TEXT("core.Shakedown"));

	if (!TestNotNull(TEXT("The shakedown loaded as a conversation"), Shakedown))
	{
		return true;
	}

	TUniquePtr<FDialogConversationPlayer> Talk = Game.Start(*Shakedown);

	if (!SmoresConversationTest_ExpectLine(*this, *Talk, TEXT("core.shakedown_toll"), TEXT("Toll road. Twenty gold, or you walk back the way you came.")))
	{
		return true;
	}

	Talk->Advance();

	if (!SmoresConversationTest_ExpectChoices(*this, *Talk, TEXT("core.shakedown_pay, core.shakedown_ask, core.shakedown_refuse")))
	{
		return true;
	}

	TestEqual(TEXT("The question's text"), Talk->GetChoices()[1].SourceText, FString(TEXT("Who says it's your road?")));

	SmoresConversationTest_Choose(*this, *Talk, TEXT("core.shakedown_ask"));

	if (!SmoresConversationTest_ExpectLine(*this, *Talk, TEXT("core.shakedown_twelve"), TEXT("The twelve of us in those rocks say so.")))
	{
		return true;
	}

	// back at the choices, and the script remembers it was asked
	Talk->Advance();

	if (!SmoresConversationTest_ExpectChoices(*this, *Talk, TEXT("core.shakedown_pay, -core.shakedown_ask, core.shakedown_refuse")))
	{
		return true;
	}

	TestEqual(TEXT("No command before refusing"), Game.Commands.Num(), 0);

	SmoresConversationTest_Choose(*this, *Talk, TEXT("core.shakedown_refuse"));

	TestEqual(TEXT("The commands the script gave"), FString::Join(Game.Commands, TEXT(" | ")), FString(TEXT("ChangeStanding Raiders -10")));

	if (!SmoresConversationTest_ExpectLine(*this, *Talk, TEXT("core.shakedown_wrong"), TEXT("Wrong answer.")))
	{
		return true;
	}

	Talk->Advance();
	TestEqual(TEXT("Ends after the last line"), SmoresConversationTest_Describe(*Talk), FString(TEXT("ended")));

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationPayFollowsGoldTest,
	"Smores.Dialog.Conversation.PayChoiceFollowsGold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  gold() decides the "Pay the toll" choice. Too poor, it is shown greyed with its #reason: -
 *  CannotAfford, the refusal line's "not enough gold" - and can't be picked; with the gold,
 *  paying runs TakeMoney.
 */
bool FSmoresConversationPayFollowsGoldTest::RunTest(const FString& Parameters)
{
	{
		FSmoresConversationTestGame Game(5);
		const FDialogLibrary Library = SmoresConversationTest_LoadShakedown(*this, Game.Facts);
		const FConversationDefinition* Shakedown = Library.FindConversation(TEXT("core.Shakedown"));

		if (!TestNotNull(TEXT("The shakedown loaded"), Shakedown))
		{
			return true;
		}

		TUniquePtr<FDialogConversationPlayer> Talk = Game.Start(*Shakedown);
		Talk->Advance();

		if (SmoresConversationTest_ExpectChoices(*this, *Talk, TEXT("!core.shakedown_pay, core.shakedown_ask, core.shakedown_refuse")))
		{
			TestTrue(TEXT("It shows why - it can't be afforded"), Talk->GetChoices()[0].Reason == ESmoresRefusalReason::CannotAfford);
			TestFalse(TEXT("A greyed choice can't be picked"), Talk->Choose(0));
			TestTrue(TEXT("...and it stays at the choices"), Talk->GetState() == EConversationPlayerState::Choices);
		}

		TestEqual(TEXT("Nothing was bought"), Game.Commands.Num(), 0);
	}

	{
		FSmoresConversationTestGame Game(50);
		const FDialogLibrary Library = SmoresConversationTest_LoadShakedown(*this, Game.Facts);
		const FConversationDefinition* Shakedown = Library.FindConversation(TEXT("core.Shakedown"));

		if (!TestNotNull(TEXT("The shakedown loaded"), Shakedown))
		{
			return true;
		}

		TUniquePtr<FDialogConversationPlayer> Talk = Game.Start(*Shakedown);
		Talk->Advance();

		if (!SmoresConversationTest_ExpectChoices(*this, *Talk, TEXT("core.shakedown_pay, core.shakedown_ask, core.shakedown_refuse")))
		{
			return true;
		}

		TestTrue(TEXT("An available choice has no reason"), Talk->GetChoices()[0].Reason == ESmoresRefusalReason::None);

		SmoresConversationTest_Choose(*this, *Talk, TEXT("core.shakedown_pay"));
		TestEqual(TEXT("The commands the script gave"), FString::Join(Game.Commands, TEXT(" | ")), FString(TEXT("TakeMoney 20")));
		SmoresConversationTest_ExpectLine(*this, *Talk, TEXT("core.shakedown_paid"), TEXT("Pleasure doing business. Road's yours."));

		Talk->Advance();
		TestEqual(TEXT("Ends after paying"), SmoresConversationTest_Describe(*Talk), FString(TEXT("ended")));
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationTwoAtOnceTest,
	"Smores.Dialog.Conversation.TwoConversationsOverOneScript",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  Two squads talking to the same bandit: one loaded script, two conversations alive at once, each
 *  with its own place and its own memory, each telling only its own game what to do.
 */
bool FSmoresConversationTwoAtOnceTest::RunTest(const FString& Parameters)
{
	FSmoresConversationTestGame GameA(50);
	FSmoresConversationTestGame GameB(50);

	// one library, one script - both players run over the same loaded program
	const FDialogLibrary Library = SmoresConversationTest_LoadShakedown(*this, GameA.Facts);
	const FConversationDefinition* Shakedown = Library.FindConversation(TEXT("core.Shakedown"));

	if (!TestNotNull(TEXT("The shakedown loaded"), Shakedown))
	{
		return true;
	}

	TUniquePtr<FDialogConversationPlayer> A = GameA.Start(*Shakedown);
	TUniquePtr<FDialogConversationPlayer> B = GameB.Start(*Shakedown);

	// A asks, and comes back to choices with the question used up...
	A->Advance();
	SmoresConversationTest_Choose(*this, *A, TEXT("core.shakedown_ask"));
	A->Advance();

	if (!SmoresConversationTest_ExpectChoices(*this, *A, TEXT("core.shakedown_pay, -core.shakedown_ask, core.shakedown_refuse")))
	{
		return true;
	}

	// ...while B, started at the same time over the same script, is still on its first line, and
	// can still ask: A's memory isn't B's
	if (!SmoresConversationTest_ExpectLine(*this, *B, TEXT("core.shakedown_toll"), TEXT("Toll road. Twenty gold, or you walk back the way you came.")))
	{
		return true;
	}

	B->Advance();

	if (!SmoresConversationTest_ExpectChoices(*this, *B, TEXT("core.shakedown_pay, core.shakedown_ask, core.shakedown_refuse")))
	{
		return true;
	}

	// each one's command goes to its own game
	SmoresConversationTest_Choose(*this, *B, TEXT("core.shakedown_refuse"));
	SmoresConversationTest_Choose(*this, *A, TEXT("core.shakedown_pay"));

	TestEqual(TEXT("A's commands"), FString::Join(GameA.Commands, TEXT(" | ")), FString(TEXT("TakeMoney 20")));
	TestEqual(TEXT("B's commands"), FString::Join(GameB.Commands, TEXT(" | ")), FString(TEXT("ChangeStanding Raiders -10")));

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationBrokenFilesTest,
	"Smores.Dialog.Conversation.BrokenFilesAreReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

/**
 *  A missing compiled file and a garbage one each cost that file, with a report naming it - never
 *  a crash, never a half-loaded script - and the rest of the package still loads.
 */
bool FSmoresConversationBrokenFilesTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// missing: the .yarn with nothing compiled beside it
	{
		FDialogPackageSource Core = MakeTestCorePackage(MakeTestBarkCsv({ TEXT("greet,TradeOpened,,Hello.,1,0") }));
		AddTestConversationFiles(Core, TEXT("shakedown"));
		Core.Files.RemoveAll([](const FDialogSourceFile& File) { return File.Path.EndsWith(TEXT(".yarnc")); });

		const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

		TestEqual(TEXT("No conversation loads without its compiled file"), Library.Conversations.Num(), 0);
		TestTrue(TEXT("...and the report names the missing file"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("has no shakedown.yarnc beside it")));
		TestEqual(TEXT("The package's barks still load"), Library.Barks.Num(), 1);
	}

	// garbage: a writer's typo where the compiled program should be
	{
		FDialogPackageSource Core = MakeTestCorePackage();
		AddTestConversationFiles(Core, TEXT("shakedown"));

		if (FDialogSourceFile* Compiled = FindTestSourceFile(Core, TEXT("conversations/shakedown.yarnc")))
		{
			const FTCHARToUTF8 Garbage(TEXT("{ this is \"not\" a compiled script, just a writer's typo ["));
			Compiled->Bytes = TArray<uint8>(reinterpret_cast<const uint8*>(Garbage.Get()), Garbage.Length());
		}

		const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

		TestEqual(TEXT("No conversation loads from a garbage file"), Library.Conversations.Num(), 0);
		TestTrue(TEXT("...and the report says what's wrong with it"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("isn't a compiled Yarn program")));
		TestTrue(TEXT("...naming the compiled file"), Library.Problems.ContainsByPredicate([](const FDialogProblem& Problem) { return Problem.File == TEXT("conversations/shakedown.yarnc"); }));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
