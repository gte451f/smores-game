// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SpikeConversation.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/**
 *  The conversation-player spike's tests - THROWAWAY with the rest of the module.
 *
 *  Every test runs twice, once per language (Smores.DialogSpike.<Test>.Yarn / .Ink), over the same
 *  scene read from the real files in Mods/example/conversations - loaded the way a player's
 *  computer would load a mod. Nothing here has a screen, which is the nearest this project can
 *  get to "a server with no UI": the launcher engine can't build a real dedicated server.
 */

namespace
{
	/** Stands in for the game: a wallet the script asks about, and a record of what it told us to do */
	struct FSpikeTestGame
	{
		int32 Gold = 50;
		TArray<FString> Commands;

		FSpikeGameHooks MakeHooks()
		{
			FSpikeGameHooks Hooks;
			Hooks.GetGold = [this]() { return Gold; };
			Hooks.RunCommand = [this](const FString& Name, const TArray<FString>& Arguments)
			{
				Commands.Add(Name + TEXT(" ") + FString::Join(Arguments, TEXT(" ")));
			};
			return Hooks;
		}
	};

	using FSpikeConversationFactory = TFunction<TUniquePtr<ISpikeConversation>(FSpikeGameHooks)>;

	/** Loads the scene once in the given language; each call of the result is a new conversation over it */
	FSpikeConversationFactory LoadSpikeScene(const FString& Language, const FString& Directory, FString& OutError)
	{
		if (Language == TEXT("Yarn"))
		{
			TSharedPtr<FSpikeYarnScript> Script = SmoresDialogSpike::LoadYarnScript(Directory, SmoresDialogSpike::GetSceneName(), OutError);
			if (!Script)
			{
				return nullptr;
			}
			TSharedRef<FSpikeYarnScript> Loaded = Script.ToSharedRef();
			return [Loaded](FSpikeGameHooks Hooks) { return SmoresDialogSpike::MakeYarnConversation(Loaded, TEXT("Shakedown"), MoveTemp(Hooks)); };
		}

		TSharedPtr<FSpikeInkScript> Script = SmoresDialogSpike::LoadInkScript(Directory, SmoresDialogSpike::GetSceneName(), OutError);
		if (!Script)
		{
			return nullptr;
		}
		TSharedRef<FSpikeInkScript> Loaded = Script.ToSharedRef();
		return [Loaded](FSpikeGameHooks Hooks) { return SmoresDialogSpike::MakeInkConversation(Loaded, MoveTemp(Hooks)); };
	}

	/** Where the conversation is, in one line - for failure messages */
	FString DescribeSpikeStep(const ISpikeConversation& Conversation)
	{
		switch (Conversation.GetState())
		{
		case ESpikeState::Line:
			return FString::Printf(TEXT("line %s (%s: \"%s\")"), *Conversation.GetLine().Id, *Conversation.GetLine().Speaker, *Conversation.GetLine().Text);
		case ESpikeState::Choices:
		{
			TArray<FString> Ids;
			for (const FSpikeChoice& Choice : Conversation.GetChoices())
			{
				Ids.Add(FString::Printf(TEXT("%s \"%s\"%s"), *Choice.Id, *Choice.Text, Choice.bAvailable ? TEXT("") : TEXT(" (unavailable)")));
			}
			return FString::Printf(TEXT("choices [%s]"), *FString::Join(Ids, TEXT(", ")));
		}
		case ESpikeState::Ended:
			return TEXT("ended");
		default:
			return FString::Printf(TEXT("failed: %s"), *Conversation.GetError());
		}
	}

	/** True if the conversation is showing exactly this line; reports what it showed instead if not */
	bool ExpectSpikeLine(FAutomationTestBase& Test, const ISpikeConversation& Conversation, const FString& Id, const FString& Speaker, const FString& Text)
	{
		if (Conversation.GetState() != ESpikeState::Line || Conversation.GetLine().Id != Id)
		{
			Test.AddError(FString::Printf(TEXT("expected line %s, got %s"), *Id, *DescribeSpikeStep(Conversation)));
			return false;
		}
		Test.TestEqual(*FString::Printf(TEXT("%s's speaker"), *Id), Conversation.GetLine().Speaker, Speaker);
		Test.TestEqual(*FString::Printf(TEXT("%s's text"), *Id), Conversation.GetLine().Text, Text);
		return true;
	}

	/**
	 *  True if the choices a player can actually pick are exactly these ids, in order.
	 *
	 *  "Can pick" because the languages differ in how a failed condition looks: Ink leaves the
	 *  choice out, Yarn keeps it in the list marked unavailable. The pick-able list is the same.
	 */
	bool ExpectSpikeChoices(FAutomationTestBase& Test, const ISpikeConversation& Conversation, const TArray<FString>& Ids)
	{
		TArray<FString> Offered;
		if (Conversation.GetState() == ESpikeState::Choices)
		{
			for (const FSpikeChoice& Choice : Conversation.GetChoices())
			{
				if (Choice.bAvailable)
				{
					Offered.Add(Choice.Id);
				}
			}
		}

		if (Offered != Ids)
		{
			Test.AddError(FString::Printf(TEXT("expected choices [%s], got %s"), *FString::Join(Ids, TEXT(", ")), *DescribeSpikeStep(Conversation)));
			return false;
		}
		return true;
	}

	/** Picks the choice with this id, wherever it sits in the list - positions differ between the languages */
	bool ChooseSpike(FAutomationTestBase& Test, ISpikeConversation& Conversation, const FString& Id)
	{
		const TArray<FSpikeChoice>& Choices = Conversation.GetChoices();
		const int32 Index = Conversation.GetState() == ESpikeState::Choices ? Choices.IndexOfByPredicate([&Id](const FSpikeChoice& Choice) { return Choice.Id == Id; }) : INDEX_NONE;
		if (Index == INDEX_NONE || !Conversation.Choose(Index))
		{
			Test.AddError(FString::Printf(TEXT("couldn't choose %s at %s"), *Id, *DescribeSpikeStep(Conversation)));
			return false;
		}
		return true;
	}

	void AddSpikeLanguages(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands)
	{
		for (const TCHAR* Language : { TEXT("Yarn"), TEXT("Ink") })
		{
			OutBeautifiedNames.Add(Language);
			OutTestCommands.Add(Language);
		}
	}
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FSmoresDialogSpikePlaysTheSceneTest,
	"Smores.DialogSpike.PlaysTheScene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FSmoresDialogSpikePlaysTheSceneTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	AddSpikeLanguages(OutBeautifiedNames, OutTestCommands);
}

/**
 *  The whole "ask, then refuse" path: every line's id, speaker and text; the choices at each point;
 *  the script's own memory removing the question once asked; and the command arriving with its
 *  arguments, in order, before the line that follows it.
 */
bool FSmoresDialogSpikePlaysTheSceneTest::RunTest(const FString& Language)
{
	FString Error;
	const FSpikeConversationFactory Make = LoadSpikeScene(Language, SmoresDialogSpike::GetSceneDirectory(), Error);
	if (!Make)
	{
		AddError(FString::Printf(TEXT("couldn't load the %s scene: %s"), *Language, *Error));
		return false;
	}

	FSpikeTestGame Game;
	TUniquePtr<ISpikeConversation> Talk = Make(Game.MakeHooks());

	if (!TestTrue(TEXT("starts"), Talk->Start()))
	{
		AddError(Talk->GetError());
		return false;
	}

	if (!ExpectSpikeLine(*this, *Talk, TEXT("shakedown_toll"), TEXT("Bandit"), TEXT("Toll road. Twenty gold, or you walk back the way you came.")))
	{
		return false;
	}

	Talk->Advance();
	if (!ExpectSpikeChoices(*this, *Talk, { TEXT("shakedown_pay"), TEXT("shakedown_ask"), TEXT("shakedown_refuse") }))
	{
		return false;
	}
	TestEqual(TEXT("the question's text"), Talk->GetChoices()[1].Text, FString(TEXT("Who says it's your road?")));

	ChooseSpike(*this, *Talk, TEXT("shakedown_ask"));
	if (!ExpectSpikeLine(*this, *Talk, TEXT("shakedown_twelve"), TEXT("Bandit"), TEXT("The twelve of us in those rocks say so.")))
	{
		return false;
	}

	// Back at the choices, and the script remembers it was asked.
	Talk->Advance();
	if (!ExpectSpikeChoices(*this, *Talk, { TEXT("shakedown_pay"), TEXT("shakedown_refuse") }))
	{
		return false;
	}

	TestEqual(TEXT("no command before refusing"), Game.Commands.Num(), 0);
	ChooseSpike(*this, *Talk, TEXT("shakedown_refuse"));
	TestEqual(TEXT("the commands the script gave"), FString::Join(Game.Commands, TEXT(" | ")), FString(TEXT("ChangeStanding bandits -10")));

	if (!ExpectSpikeLine(*this, *Talk, TEXT("shakedown_wrong"), TEXT("Bandit"), TEXT("Wrong answer.")))
	{
		return false;
	}

	Talk->Advance();
	TestEqual(TEXT("ends after the last line"), DescribeSpikeStep(*Talk), FString(TEXT("ended")));
	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FSmoresDialogSpikePayFollowsGoldTest,
	"Smores.DialogSpike.PayChoiceFollowsGold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FSmoresDialogSpikePayFollowsGoldTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	AddSpikeLanguages(OutBeautifiedNames, OutTestCommands);
}

/**
 *  gold() decides the "Pay the toll" choice. Where the languages differ, this records the
 *  difference rather than papering over it: Yarn still offers the choice, marked unavailable (the
 *  "a disabled action still shows, with its reason" rule), while Ink leaves it out.
 */
bool FSmoresDialogSpikePayFollowsGoldTest::RunTest(const FString& Language)
{
	FString Error;
	const FSpikeConversationFactory Make = LoadSpikeScene(Language, SmoresDialogSpike::GetSceneDirectory(), Error);
	if (!Make)
	{
		AddError(FString::Printf(TEXT("couldn't load the %s scene: %s"), *Language, *Error));
		return false;
	}

	// Too poor.
	{
		FSpikeTestGame Game;
		Game.Gold = 5;
		TUniquePtr<ISpikeConversation> Talk = Make(Game.MakeHooks());
		Talk->Start();
		Talk->Advance();

		ExpectSpikeChoices(*this, *Talk, { TEXT("shakedown_ask"), TEXT("shakedown_refuse") });

		if (Language == TEXT("Yarn"))
		{
			const TArray<FSpikeChoice>& Choices = Talk->GetChoices();
			if (TestEqual(TEXT("Yarn keeps all three in the list"), Choices.Num(), 3))
			{
				TestEqual(TEXT("the first is paying"), Choices[0].Id, FString(TEXT("shakedown_pay")));
				TestFalse(TEXT("...marked unavailable"), Choices[0].bAvailable);
				TestFalse(TEXT("an unavailable choice can't be picked"), Talk->Choose(0));
				TestTrue(TEXT("still at the choices"), Talk->GetState() == ESpikeState::Choices);
			}
			AddInfo(TEXT("Yarn: a choice you can't afford is shown, marked unavailable"));
		}
		else
		{
			TestEqual(TEXT("Ink leaves it out"), Talk->GetChoices().Num(), 2);
			AddInfo(TEXT("Ink: a choice you can't afford is left out entirely"));
		}

		TestEqual(TEXT("nothing was bought"), Game.Commands.Num(), 0);
	}

	// Rich enough.
	{
		FSpikeTestGame Game;
		Game.Gold = 50;
		TUniquePtr<ISpikeConversation> Talk = Make(Game.MakeHooks());
		Talk->Start();
		Talk->Advance();

		if (!ExpectSpikeChoices(*this, *Talk, { TEXT("shakedown_pay"), TEXT("shakedown_ask"), TEXT("shakedown_refuse") }))
		{
			return false;
		}

		ChooseSpike(*this, *Talk, TEXT("shakedown_pay"));
		TestEqual(TEXT("the commands the script gave"), FString::Join(Game.Commands, TEXT(" | ")), FString(TEXT("TakeMoney 20")));
		ExpectSpikeLine(*this, *Talk, TEXT("shakedown_paid"), TEXT("Bandit"), TEXT("Pleasure doing business. Road's yours."));

		Talk->Advance();
		TestEqual(TEXT("ends after paying"), DescribeSpikeStep(*Talk), FString(TEXT("ended")));
	}

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FSmoresDialogSpikeTwoAtOnceTest,
	"Smores.DialogSpike.TwoConversationsOverOneScript",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FSmoresDialogSpikeTwoAtOnceTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	AddSpikeLanguages(OutBeautifiedNames, OutTestCommands);
}

/**
 *  Two squads talking to the same bandit: one loaded script, two conversations alive at once, each
 *  with its own place and its own memory, and each telling only its own game what to do.
 */
bool FSmoresDialogSpikeTwoAtOnceTest::RunTest(const FString& Language)
{
	FString Error;
	const FSpikeConversationFactory Make = LoadSpikeScene(Language, SmoresDialogSpike::GetSceneDirectory(), Error);
	if (!Make)
	{
		AddError(FString::Printf(TEXT("couldn't load the %s scene: %s"), *Language, *Error));
		return false;
	}

	FSpikeTestGame GameA;
	FSpikeTestGame GameB;
	TUniquePtr<ISpikeConversation> A = Make(GameA.MakeHooks());
	TUniquePtr<ISpikeConversation> B = Make(GameB.MakeHooks());

	A->Start();
	B->Start();
	A->Advance();

	// A asks, and comes back to choices without the question...
	ChooseSpike(*this, *A, TEXT("shakedown_ask"));
	A->Advance();
	if (!ExpectSpikeChoices(*this, *A, { TEXT("shakedown_pay"), TEXT("shakedown_refuse") }))
	{
		return false;
	}

	// ...while B, started at the same time over the same script, is still on its first line, and
	// then gets all three choices: A's memory isn't B's.
	if (!ExpectSpikeLine(*this, *B, TEXT("shakedown_toll"), TEXT("Bandit"), TEXT("Toll road. Twenty gold, or you walk back the way you came.")))
	{
		return false;
	}
	B->Advance();
	if (!ExpectSpikeChoices(*this, *B, { TEXT("shakedown_pay"), TEXT("shakedown_ask"), TEXT("shakedown_refuse") }))
	{
		return false;
	}

	// Each one's command goes to its own game.
	ChooseSpike(*this, *B, TEXT("shakedown_refuse"));
	ChooseSpike(*this, *A, TEXT("shakedown_pay"));
	TestEqual(TEXT("A's commands"), FString::Join(GameA.Commands, TEXT(" | ")), FString(TEXT("TakeMoney 20")));
	TestEqual(TEXT("B's commands"), FString::Join(GameB.Commands, TEXT(" | ")), FString(TEXT("ChangeStanding bandits -10")));
	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FSmoresDialogSpikeBrokenFileTest,
	"Smores.DialogSpike.BrokenFileIsReported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FSmoresDialogSpikeBrokenFileTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	AddSpikeLanguages(OutBeautifiedNames, OutTestCommands);
}

/** A missing file and a garbage file each come back as an error message - never a crash, never a half-loaded script */
bool FSmoresDialogSpikeBrokenFileTest::RunTest(const FString& Language)
{
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogSpike"), TEXT("BrokenFileTest"), Language);
	IFileManager::Get().DeleteDirectory(*Directory, /*RequireExists*/ false, /*Tree*/ true);

	FString Error;
	TestFalse(TEXT("a missing file doesn't load"), static_cast<bool>(LoadSpikeScene(Language, Directory, Error)));
	TestFalse(TEXT("...and says why"), Error.IsEmpty());
	AddInfo(FString::Printf(TEXT("missing: %s"), *Error));

	const FString Garbage = TEXT("{ this is \"not\" a compiled script, just a writer's typo [");
	if (Language == TEXT("Yarn"))
	{
		FFileHelper::SaveStringToFile(Garbage, *FPaths::Combine(Directory, TEXT("shakedown.yarnc")));
		FFileHelper::SaveStringToFile(TEXT("id,text\n"), *FPaths::Combine(Directory, TEXT("shakedown-Lines.csv")));
	}
	else
	{
		FFileHelper::SaveStringToFile(Garbage, *FPaths::Combine(Directory, TEXT("shakedown.ink.json")));
	}

	Error.Reset();
	TestFalse(TEXT("a garbage file doesn't load"), static_cast<bool>(LoadSpikeScene(Language, Directory, Error)));
	TestFalse(TEXT("...and says why"), Error.IsEmpty());
	AddInfo(FString::Printf(TEXT("garbage: %s"), *Error));

	IFileManager::Get().DeleteDirectory(*Directory, /*RequireExists*/ false, /*Tree*/ true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
