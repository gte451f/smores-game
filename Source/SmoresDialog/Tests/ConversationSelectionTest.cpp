// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ConversationSelection.h"
#include "DialogMemoryComponent.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  Which conversation happens, and when one has to stop. Selection is Hades-style - of the
 *  eligible, the highest priority, then the one this squad saw least recently, then the one loaded
 *  first - over definitions built by hand, facts answered from a map and a memory record filled in
 *  directly. No script, no world.
 */

/** The ids, in order - "a, b" - so an ordering assertion reads at a glance */
static FString SmoresConversationSelectionTest_Ids(const TArray<const FConversationDefinition*>& Conversations)
{
	return FString::JoinBy(Conversations, TEXT(", "), [](const FConversationDefinition* Conversation) { return Conversation->Id.ToString(); });
}

static FString SmoresConversationSelectionTest_Selected(const TArray<const FConversationDefinition*>& Candidates, const FDialogFactRegistry& Facts, const FDialogMemoryRecord& Memory)
{
	const FConversationDefinition* Chosen = SmoresDialog::SelectConversation(Candidates, FDialogContext(), Facts, Memory);
	return Chosen ? Chosen->Id.ToString() : FString(TEXT("(none)"));
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationSelectionTest,
	"Smores.Dialog.Conversation.SelectionPriorityOnceRequires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/** The highest priority eligible greeting wins; attach:, requires: and once: each rule one out */
bool FSmoresConversationSelectionTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("bandit")));
	Answers->Add(TEXT("Flag:paid_toll"), FDialogValue::MakeBool(false));

	const FConversationDefinition Shakedown = MakeTestConversation(Facts, TEXT("t.Shakedown"), EConversationKind::Greeting, TEXT("Speaker.Role == bandit"), TEXT(""), 0, false, 0);
	const FConversationDefinition Paid = MakeTestConversation(Facts, TEXT("t.Paid"), EConversationKind::Greeting, TEXT("Speaker.Role == bandit"), TEXT("Flag(paid_toll)"), 10, false, 1);
	const FConversationDefinition Guard = MakeTestConversation(Facts, TEXT("t.Guard"), EConversationKind::Greeting, TEXT("Speaker.Role == guard"), TEXT(""), 50, false, 2);
	const FConversationDefinition Warning = MakeTestConversation(Facts, TEXT("t.Warning"), EConversationKind::Greeting, TEXT("Speaker.Role == bandit"), TEXT(""), 5, true, 3);

	if (!TestTrue(TEXT("The test conversations compiled"), !Shakedown.Id.IsNone() && !Paid.Id.IsNone() && !Guard.Id.IsNone() && !Warning.Id.IsNone()))
	{
		return true;
	}

	const TArray<const FConversationDefinition*> Candidates = { &Shakedown, &Paid, &Guard, &Warning };
	FDialogMemoryRecord Memory;

	// the guard's is highest, but attaches to someone else; the paid one waits on its flag
	TestEqual(TEXT("Highest priority among the eligible - the once-only warning"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Warning")));

	// seen once, it's used up
	Memory.MarkSeen(TEXT("t.Warning"));
	TestEqual(TEXT("A once: conversation this squad has seen is out"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Shakedown")));

	// the flag it requires, set
	Answers->Add(TEXT("Flag:paid_toll"), FDialogValue::MakeBool(true));
	TestEqual(TEXT("requires: holding lets a higher priority in"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Paid")));

	// a guard, now: only the guard's attaches
	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("guard")));
	TestEqual(TEXT("attach: decides whose it is"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Guard")));

	// nobody's: a trader has none
	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("trader")));
	TestEqual(TEXT("None eligible is none"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("(none)")));

	// and the account names every candidate
	TArray<FString> Explanation;
	SmoresDialog::SelectConversation(Candidates, FDialogContext(), Facts, Memory, &Explanation);
	TestEqual(TEXT("The explanation has a line per candidate and the result"), Explanation.Num(), Candidates.Num() + 1);

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationTieBreakTest,
	"Smores.Dialog.Conversation.SelectionTieBreak",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/** Equal priority: never seen beats seen, and seen longer ago beats seen lately; a full tie goes to load order */
bool FSmoresConversationTieBreakTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FConversationDefinition First = MakeTestConversation(Facts, TEXT("t.First"), EConversationKind::Greeting, TEXT(""), TEXT(""), 0, false, 0);
	const FConversationDefinition Second = MakeTestConversation(Facts, TEXT("t.Second"), EConversationKind::Greeting, TEXT(""), TEXT(""), 0, false, 1);
	const FConversationDefinition Third = MakeTestConversation(Facts, TEXT("t.Third"), EConversationKind::Greeting, TEXT(""), TEXT(""), 0, false, 2);

	const TArray<const FConversationDefinition*> Candidates = { &Third, &Second, &First };
	FDialogMemoryRecord Memory;

	TestEqual(TEXT("Nothing seen: the one loaded first, whatever order they are offered in"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.First")));

	Memory.MarkSeen(TEXT("t.First"));
	TestEqual(TEXT("Never seen beats seen"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Second")));

	Memory.MarkSeen(TEXT("t.Second"));
	Memory.MarkSeen(TEXT("t.Third"));
	TestEqual(TEXT("All seen: the one seen longest ago"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.First")));

	Memory.MarkSeen(TEXT("t.First"));
	TestEqual(TEXT("...which moves on as they are seen"), SmoresConversationSelectionTest_Selected(Candidates, Facts, Memory), FString(TEXT("t.Second")));

	const FDialogSeenEntry* Seen = Memory.FindSeen(TEXT("t.First"));

	if (TestNotNull(TEXT("Memory counts viewings"), Seen))
	{
		TestEqual(TEXT("...twice for the first"), Seen->TimesSeen, 2);
	}

	TestTrue(TEXT("Seen() by title alone matches in any package"), Memory.HasSeen(TEXT("First")));
	TestTrue(TEXT("...and by its qualified id"), Memory.HasSeen(TEXT("t.First")));
	TestFalse(TEXT("...but not another package's"), Memory.HasSeen(TEXT("other.First")));
	TestFalse(TEXT("An unseen one is unseen"), Memory.HasSeen(TEXT("Fourth")));

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationTopicListTest,
	"Smores.Dialog.Conversation.TopicListIsTheEligibleTopics",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 *  After a greeting the window offers exactly the eligible topics - highest priority first, then
 *  load order - and a faction-wide topic joins any member's list.
 */
bool FSmoresConversationTopicListTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	Answers->Add(TEXT("Speaker.Role"), FDialogValue::MakeName(TEXT("trader")));
	Answers->Add(TEXT("Speaker.Faction"), FDialogValue::MakeName(TEXT("TradersGuild")));
	Answers->Add(TEXT("Flag:heard_story"), FDialogValue::MakeBool(false));

	const FConversationDefinition Prices = MakeTestConversation(Facts, TEXT("t.Prices"), EConversationKind::Topic, TEXT("Speaker.Role == trader"), TEXT(""), 0, false, 0);
	const FConversationDefinition Story = MakeTestConversation(Facts, TEXT("t.Story"), EConversationKind::Topic, TEXT("Speaker.Role == trader"), TEXT("Flag(heard_story)"), 0, false, 1);
	const FConversationDefinition Guild = MakeTestConversation(Facts, TEXT("t.Guild"), EConversationKind::Topic, TEXT("Speaker.Faction == TradersGuild"), TEXT(""), 0, false, 2);
	const FConversationDefinition Urgent = MakeTestConversation(Facts, TEXT("t.Urgent"), EConversationKind::Topic, TEXT("Speaker.Role == trader"), TEXT(""), 5, true, 3);
	const FConversationDefinition Raiders = MakeTestConversation(Facts, TEXT("t.Raiders"), EConversationKind::Topic, TEXT("Speaker.Faction == Raiders"), TEXT(""), 9, false, 4);

	const TArray<const FConversationDefinition*> Topics = { &Raiders, &Guild, &Story, &Prices, &Urgent };
	FDialogMemoryRecord Memory;

	TestEqual(TEXT("Every eligible topic, highest priority first, then load order"),
		SmoresConversationSelectionTest_Ids(SmoresDialog::GetEligibleConversations(Topics, FDialogContext(), Facts, Memory)), FString(TEXT("t.Urgent, t.Prices, t.Guild")));

	Answers->Add(TEXT("Flag:heard_story"), FDialogValue::MakeBool(true));
	Memory.MarkSeen(TEXT("t.Urgent"));

	TestEqual(TEXT("A flag unlocks one; once: retires another"),
		SmoresConversationSelectionTest_Ids(SmoresDialog::GetEligibleConversations(Topics, FDialogContext(), Facts, Memory)), FString(TEXT("t.Prices, t.Story, t.Guild")));

	// seen, but not once: - topics can be raised again
	Memory.MarkSeen(TEXT("t.Prices"));

	TestEqual(TEXT("A topic already talked about is still offered"),
		SmoresConversationSelectionTest_Ids(SmoresDialog::GetEligibleConversations(Topics, FDialogContext(), Facts, Memory)), FString(TEXT("t.Prices, t.Story, t.Guild")));

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresConversationInterruptionTest,
	"Smores.Dialog.Conversation.EndsOnRangeDownAndCombat",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/** A window conversation ends when either side walks off, goes down, or a fight starts - and not otherwise */
bool FSmoresConversationInterruptionTest::RunTest(const FString& Parameters)
{
	FConversationWatch Calm;
	Calm.Distance = 200.0f;
	Calm.BreakOffRange = 500.0f;

	auto ReasonFor = [](const FConversationWatch& Watch)
	{
		EConversationEndReason Reason = EConversationEndReason::Finished;
		return SmoresDialog::GetConversationInterruption(Watch, Reason) ? StaticEnum<EConversationEndReason>()->GetNameStringByValue(static_cast<int64>(Reason)) : FString(TEXT("goes on"));
	};

	TestEqual(TEXT("Standing and talking goes on"), ReasonFor(Calm), FString(TEXT("goes on")));

	FConversationWatch Watch = Calm;
	Watch.Distance = 501.0f;
	TestEqual(TEXT("Out of range"), ReasonFor(Watch), FString(TEXT("WalkedAway")));

	Watch.bCheckRange = false;
	TestEqual(TEXT("...unless the debug exec opened it across the map"), ReasonFor(Watch), FString(TEXT("goes on")));

	Watch = Calm;
	Watch.bSomeoneGone = true;
	TestEqual(TEXT("Someone gone"), ReasonFor(Watch), FString(TEXT("WalkedAway")));

	Watch = Calm;
	Watch.bNpcDown = true;
	TestEqual(TEXT("The NPC down"), ReasonFor(Watch), FString(TEXT("Downed")));

	Watch = Calm;
	Watch.bSquadMemberDown = true;
	TestEqual(TEXT("The squad member down"), ReasonFor(Watch), FString(TEXT("Downed")));

	Watch = Calm;
	Watch.bNpcHostile = true;
	TestEqual(TEXT("The NPC turned hostile"), ReasonFor(Watch), FString(TEXT("Combat")));

	Watch = Calm;
	Watch.bEitherAttacking = true;
	TestEqual(TEXT("Somebody attacking"), ReasonFor(Watch), FString(TEXT("Combat")));

	Watch = Calm;
	Watch.bEitherHurt = true;
	TestEqual(TEXT("Somebody hurt"), ReasonFor(Watch), FString(TEXT("Combat")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
