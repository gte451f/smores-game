// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BarkBubbleSchedule.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The rules behind the bark bubbles floating over speakers: how long a line stays up, one bubble
 *  per speaker, a replacement restarting the clock, the fade and the expiry, and bubbles stacking
 *  instead of overdrawing. Where a bubble lands on screen and how it looks are PIE judgments; these
 *  are the parts that can be wrong while looking fine.
 *
 *  Speakers are spawned actors, since a bubble belongs to one - and a bare NewObject<UObject>() trips
 *  an abstract-class ensure (testing.md). Time is plain seconds handed in.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkBubbleLifetimeTest,
	"Smores.UI.BarkBubbles.LifetimeIsAFloorPlusPerCharacterCapped",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkBubbleLifetimeTest::RunTest(const FString& Parameters)
{
	FBarkBubbleTiming Timing;
	Timing.FloorSeconds = 2.0f;
	Timing.PerCharacterSeconds = 0.1f;
	Timing.CapSeconds = 6.0f;

	TestEqual(TEXT("An empty line gets the floor"), Timing.GetLifetime(0), 2.0f);
	TestEqual(TEXT("Each character adds its share"), Timing.GetLifetime(10), 3.0f);
	TestEqual(TEXT("...so a longer line stays longer"), Timing.GetLifetime(30), 5.0f);
	TestEqual(TEXT("A very long line stops at the cap"), Timing.GetLifetime(500), 6.0f);
	TestEqual(TEXT("A nonsense negative length gets the floor, not less"), Timing.GetLifetime(-5), 2.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkBubbleOnePerSpeakerTest,
	"Smores.UI.BarkBubbles.OnePerSpeakerAndAReplacementRestartsTheClock",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkBubbleOnePerSpeakerTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const AActor* Ada = TestWorld.SpawnOwner();
	const AActor* Bandit = TestWorld.SpawnOwner();

	FBarkBubbleSchedule Schedule;
	Schedule.Timing.FloorSeconds = 2.0f;
	Schedule.Timing.PerCharacterSeconds = 0.0f;
	Schedule.Timing.CapSeconds = 10.0f;
	Schedule.Timing.FadeSeconds = 1.0f;

	Schedule.Show(Ada, FText::FromString(TEXT("Coin first.")), 0.0);
	Schedule.Show(Bandit, FText::FromString(TEXT("That's close enough.")), 0.0);

	TestEqual(TEXT("Two speakers, two bubbles"), Schedule.GetEntries().Num(), 2);

	const uint32 FirstSerial = Schedule.Find(Ada) ? Schedule.Find(Ada)->Serial : 0;

	Schedule.Show(Ada, FText::FromString(TEXT("Questions never.")), 1.5);

	TestEqual(TEXT("A second line from the same speaker replaces their bubble rather than adding one"), Schedule.GetEntries().Num(), 2);

	const FBarkBubbleEntry* Replaced = Schedule.Find(Ada);

	if (!TestNotNull(TEXT("The speaker still has a bubble"), Replaced))
	{
		return true;
	}

	TestEqual(TEXT("...showing the new line"), Replaced->Text.ToString(), FString(TEXT("Questions never.")));
	TestNotEqual(TEXT("...marked as a different line, so a widget redraws it"), Replaced->Serial, FirstSerial);
	TestTrue(TEXT("...and keeping its place, so a stack doesn't reshuffle every time someone speaks"), Schedule.GetEntries()[0].Speaker.Get() == Ada);

	// the old line would have finished fading at 3.0 (2s up, 1s fade); the replacement runs from 1.5
	Schedule.Prune(3.2);

	TestNotNull(TEXT("The replaced bubble's clock restarted - it outlives the first line's time"), Schedule.Find(Ada));
	TestEqual(TEXT("...at full opacity until its own time is up"), Schedule.GetOpacity(*Schedule.Find(Ada), 3.2), 1.0f);
	TestNull(TEXT("The other speaker's bubble kept its own clock and is gone"), Schedule.Find(Bandit));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkBubbleExpiryTest,
	"Smores.UI.BarkBubbles.FadesThenExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkBubbleExpiryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	AActor* Ada = TestWorld.SpawnOwner();
	AActor* Bandit = TestWorld.SpawnOwner();

	FBarkBubbleSchedule Schedule;
	Schedule.Timing.FloorSeconds = 2.0f;
	Schedule.Timing.PerCharacterSeconds = 0.0f;
	Schedule.Timing.CapSeconds = 10.0f;
	Schedule.Timing.FadeSeconds = 1.0f;

	Schedule.Show(Ada, FText::FromString(TEXT("Road's long.")), 10.0);

	const FBarkBubbleEntry* Entry = Schedule.Find(Ada);

	if (!TestNotNull(TEXT("The line is up"), Entry))
	{
		return true;
	}

	TestEqual(TEXT("Full opacity while the line is up"), Schedule.GetOpacity(*Entry, 11.9), 1.0f);
	TestEqual(TEXT("Halfway through the fade, half opacity"), Schedule.GetOpacity(*Entry, 12.5), 0.5f, 0.001f);
	TestEqual(TEXT("Nothing left once the fade is over"), Schedule.GetOpacity(*Entry, 13.0), 0.0f);

	Schedule.Prune(12.9);
	TestNotNull(TEXT("Still there, faintly, just before the fade ends"), Schedule.Find(Ada));

	Schedule.Prune(13.0);
	TestNull(TEXT("Gone once the fade ends"), Schedule.Find(Ada));

	// a speaker who goes away takes their bubble with them, whatever its clock says
	Schedule.Show(Bandit, FText::FromString(TEXT("Finish it, then.")), 20.0);
	Bandit->Destroy();
	Schedule.Prune(20.1);

	TestEqual(TEXT("A destroyed speaker's bubble is dropped at once"), Schedule.GetEntries().Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresBarkBubbleStackTest,
	"Smores.UI.BarkBubbles.OverlappingBubblesStackUpward",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresBarkBubbleStackTest::RunTest(const FString& Parameters)
{
	const float Gap = 4.0f;

	auto Overlap = [](const FBox2D& A, const FBox2D& B)
	{
		return A.Min.X < B.Max.X && B.Min.X < A.Max.X && A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y;
	};

	// two speakers standing close: their bubbles land almost on top of each other
	TArray<FBox2D> Pair = {
		FBox2D(FVector2D(100.0, 200.0), FVector2D(300.0, 240.0)),
		FBox2D(FVector2D(150.0, 210.0), FVector2D(350.0, 250.0))
	};

	SmoresBarkBubbles::StackBoxes(Pair, Gap);

	TestEqual(TEXT("The first bubble keeps its place"), Pair[0].Min.Y, 200.0);
	TestFalse(TEXT("The second no longer overlaps it"), Overlap(Pair[0], Pair[1]));
	TestEqual(TEXT("...because it moved up to sit just above it"), Pair[1].Max.Y, 200.0 - Gap);
	TestEqual(TEXT("...keeping its height"), Pair[1].Max.Y - Pair[1].Min.Y, 40.0);
	TestEqual(TEXT("...and its column - it never moves sideways"), Pair[1].Min.X, 150.0);

	// three in a crowd settle into a column, each clear of every other
	TArray<FBox2D> Crowd = {
		FBox2D(FVector2D(0.0, 100.0), FVector2D(200.0, 140.0)),
		FBox2D(FVector2D(20.0, 100.0), FVector2D(220.0, 140.0)),
		FBox2D(FVector2D(40.0, 100.0), FVector2D(240.0, 140.0))
	};

	SmoresBarkBubbles::StackBoxes(Crowd, Gap);

	TestFalse(TEXT("First and second clear"), Overlap(Crowd[0], Crowd[1]));
	TestFalse(TEXT("First and third clear"), Overlap(Crowd[0], Crowd[2]));
	TestFalse(TEXT("Second and third clear"), Overlap(Crowd[1], Crowd[2]));
	TestTrue(TEXT("The third sits above the second"), Crowd[2].Max.Y <= Crowd[1].Min.Y - Gap);

	// people standing apart keep their bubbles where they are
	TArray<FBox2D> Apart = {
		FBox2D(FVector2D(0.0, 100.0), FVector2D(200.0, 140.0)),
		FBox2D(FVector2D(200.0, 100.0), FVector2D(400.0, 140.0)),
		FBox2D(FVector2D(0.0, 300.0), FVector2D(200.0, 340.0))
	};

	const TArray<FBox2D> Before = Apart;

	SmoresBarkBubbles::StackBoxes(Apart, Gap);

	TestTrue(TEXT("Side by side, merely touching, nothing moves"), Apart[1] == Before[1]);
	TestTrue(TEXT("One well below another, nothing moves"), Apart[2] == Before[2]);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
