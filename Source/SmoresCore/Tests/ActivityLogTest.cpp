// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresActivityLog.h"
#include "Engine/LocalPlayer.h"
#include "Tests/SmoresTestWorld.h"

namespace
{
	/**
	 *  A log with a bare local player behind it and nothing else.
	 *
	 *  USmoresActivityLog is a ULocalPlayerSubsystem in play, but nothing the ring buffer does
	 *  touches its local player - Initialize() is never called and there is no state behind it to
	 *  initialise. Constructing one directly is what lets the part worth testing (eviction,
	 *  filtering, the broadcast) be tested without a viewport, a player or a game instance.
	 *
	 *  **The outer chain has to be real, though, and it is two links long.**
	 *  ULocalPlayerSubsystem declares ClassWithin = ULocalPlayer and ULocalPlayer declares
	 *  ClassWithin = UEngine, and UE enforces both at construction: an outer that doesn't match
	 *  raises a handled ensure ("created in invalid Outer"), which the automation harness turns
	 *  into a test failure. So the log is outered to a bare ULocalPlayer, and that player to
	 *  GEngine. Neither is initialised and neither is ever asked anything.
	 *
	 *  Worth the comment rather than a quiet fix, because of how it fails: the ensure fires only
	 *  once per call site per session, so *one* of these six tests failed and the other five
	 *  passed on exactly the same mistake. A green run is not evidence the outer is right.
	 *
	 *  Both objects are kept alive for the test's duration: a collect part-way through would pull
	 *  them out from under the assertions.
	 */
	USmoresActivityLog* MakeTestLog(FSmoresTestWorld& TestWorld, int32 Capacity)
	{
		if (!GEngine)
		{
			return nullptr;
		}

		ULocalPlayer* OuterPlayer = NewObject<ULocalPlayer>(GEngine);

		TestWorld.KeepAlive(OuterPlayer);

		USmoresActivityLog* Log = NewObject<USmoresActivityLog>(OuterPlayer);

		TestWorld.KeepAlive(Log);

		if (Log)
		{
			Log->SetCapacity(Capacity);
		}

		return Log;
	}

	/** One squad line, worded from a number so the assertions can name which one they expected */
	void PostNumbered(USmoresActivityLog* Log, int32 Number, EActivityCategory Category = EActivityCategory::Squad)
	{
		Log->Post(Category, EActivitySeverity::Normal, FText::AsNumber(Number));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogPostTest,
	"Smores.Core.ActivityLog.PostStoresAndStamps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogPostTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 8);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	const FActivityEntry Posted = Log->Post(EActivityCategory::Comms, EActivitySeverity::Good,
		FText::FromString(TEXT("Sold a sword")), FText::FromString(TEXT("Marla")));

	TestEqual(TEXT("One entry is held"), Log->GetEntries().Num(), 1);
	TestTrue(TEXT("...with the category it was posted under"), Log->GetEntries()[0].Category == EActivityCategory::Comms);
	TestTrue(TEXT("...and the severity"), Log->GetEntries()[0].Severity == EActivitySeverity::Good);
	TestEqual(TEXT("...and the source"), Log->GetEntries()[0].Source.ToString(), FString(TEXT("Marla")));

	// Ids are what the feed widget compares, so a zero id would make two identical lines
	// indistinguishable from the same line drawn twice
	TestTrue(TEXT("The entry got a nonzero id"), Posted.Id != 0);
	TestTrue(TEXT("...and a timestamp"), Posted.Timestamp > 0.0);

	const FActivityEntry Second = Log->Post(EActivityCategory::Squad, EActivitySeverity::Bad,
		FText::FromString(TEXT("Took damage")));

	TestTrue(TEXT("The next entry got a different id"), Second.Id != Posted.Id);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogEvictionTest,
	"Smores.Core.ActivityLog.EvictsOldestAtCapacity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogEvictionTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 3);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	for (int32 Number = 1; Number <= 5; ++Number)
	{
		PostNumbered(Log, Number);
	}

	// the whole point of a fixed-capacity store: it never grows, and what it drops is the oldest
	TestEqual(TEXT("The store is capped"), Log->GetEntries().Num(), 3);
	TestEqual(TEXT("...holding the newest, oldest first"), Log->GetEntries()[0].Text.ToString(), FText::AsNumber(3).ToString());
	TestEqual(TEXT("...through to the last posted"), Log->GetEntries()[2].Text.ToString(), FText::AsNumber(5).ToString());

	// ids keep counting past an eviction - the feed uses them to tell entries apart, so reusing
	// one would make an evicted line and a fresh line compare equal
	TestEqual(TEXT("Ids were not reset by eviction"), Log->GetEntries()[2].Id, 5);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogShrinkTest,
	"Smores.Core.ActivityLog.ShrinkingCapacityEvictsImmediately",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogShrinkTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 10);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	for (int32 Number = 1; Number <= 6; ++Number)
	{
		PostNumbered(Log, Number);
	}

	Log->SetCapacity(2);

	// the invariant has to hold the moment SetCapacity returns, not after the next post - a feed
	// reading the log in between would otherwise draw more lines than the log claims to hold
	TestEqual(TEXT("Shrinking evicted straight away"), Log->GetEntries().Num(), 2);
	TestEqual(TEXT("...keeping the newest"), Log->GetEntries()[1].Text.ToString(), FText::AsNumber(6).ToString());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogFilterTest,
	"Smores.Core.ActivityLog.CategoryFilterReturnsItsOwn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogFilterTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 16);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	PostNumbered(Log, 1, EActivityCategory::Squad);
	PostNumbered(Log, 2, EActivityCategory::Comms);
	PostNumbered(Log, 3, EActivityCategory::Squad);
	PostNumbered(Log, 4, EActivityCategory::Comms);
	PostNumbered(Log, 5, EActivityCategory::Squad);

	TestEqual(TEXT("Unfiltered is everything"), Log->GetEntries().Num(), 5);
	TestEqual(TEXT("Squad returns its own"), Log->GetEntries(EActivityCategory::Squad).Num(), 3);
	TestEqual(TEXT("Comms returns its own"), Log->GetEntries(EActivityCategory::Comms).Num(), 2);

	// the QUESTS tab is an empty stub by design, not by accident - nothing posts to it, and a
	// filter that quietly returned something else would hide that
	TestEqual(TEXT("Quests is empty"), Log->GetEntries(EActivityCategory::Quests).Num(), 0);

	// order within a category has to survive the filter, or the feed reads out of sequence
	TestEqual(TEXT("Filtering kept the order"), Log->GetEntries(EActivityCategory::Squad)[0].Text.ToString(), FText::AsNumber(1).ToString());
	TestEqual(TEXT("...oldest first"), Log->GetEntries(EActivityCategory::Squad)[2].Text.ToString(), FText::AsNumber(5).ToString());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogBroadcastTest,
	"Smores.Core.ActivityLog.BroadcastsOncePerPost",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogBroadcastTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 2);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	// OnEntryAdded is a plain (non-dynamic) multicast delegate, so a lambda can listen directly -
	// USmoresTestDelegateListener has no handler shaped for an FActivityEntry, and giving the
	// delegate an int32 payload just to reuse that listener would be the tail wagging the dog
	int32 CallCount = 0;
	int32 LastId = 0;
	int32 CountAtBroadcast = 0;

	Log->OnEntryAdded.AddLambda([&CallCount, &LastId, &CountAtBroadcast, Log](const FActivityEntry& Entry)
	{
		++CallCount;
		LastId = Entry.Id;
		CountAtBroadcast = Log->GetEntries().Num();
	});

	Log->Post(EActivityCategory::Squad, EActivitySeverity::Normal, FText::FromString(TEXT("First")));

	TestEqual(TEXT("One post, one broadcast"), CallCount, 1);
	TestTrue(TEXT("...carrying the entry that was added"), LastId != 0);

	Log->Post(EActivityCategory::Squad, EActivitySeverity::Normal, FText::FromString(TEXT("Second")));
	Log->Post(EActivityCategory::Squad, EActivitySeverity::Normal, FText::FromString(TEXT("Third")));

	TestEqual(TEXT("Three posts, three broadcasts"), CallCount, 3);

	// the third post evicted the first, and the handler has to see the settled store - a listener
	// that rebuilt from GetEntries() on the broadcast would otherwise draw a list one eviction out
	// of date on exactly the frames it matters
	TestEqual(TEXT("The broadcast ran after eviction"), CountAtBroadcast, 2);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActivityLogClearTest,
	"Smores.Core.ActivityLog.ClearEmptiesWithoutBroadcasting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActivityLogClearTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	USmoresActivityLog* Log = MakeTestLog(TestWorld, 8);

	if (!TestNotNull(TEXT("Log created"), Log))
	{
		return true;
	}

	int32 CallCount = 0;

	Log->OnEntryAdded.AddLambda([&CallCount](const FActivityEntry& Entry) { ++CallCount; });

	PostNumbered(Log, 1);
	PostNumbered(Log, 2);

	Log->Clear();

	TestEqual(TEXT("Clear emptied the store"), Log->GetEntries().Num(), 0);

	// OnEntryAdded means "an entry was added", and nothing was - a clear that broadcast would
	// make every listener redraw a list it had just been told was longer
	TestEqual(TEXT("...and broadcast nothing of its own"), CallCount, 2);

	// ids still don't restart, so a widget holding an evicted entry can never mistake it for a new one
	TestEqual(TEXT("Ids continue past a clear"), Log->Post(EActivityCategory::Squad, EActivitySeverity::Normal, FText::FromString(TEXT("After"))).Id, 3);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
