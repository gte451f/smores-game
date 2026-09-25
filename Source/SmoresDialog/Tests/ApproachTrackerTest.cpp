// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ApproachTracker.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The Approached bark's edge-trigger: an NPC notices a squad on the way in, not for as long as it
 *  stands there, and not again for that player until the cooldown has passed *and* the squad has
 *  left and come back. Every one of those is a rule a player would only ever notice as "the trader
 *  won't shut up" or "the guard never says anything", which is why it is pinned here rather than
 *  judged in PIE.
 *
 *  NPCs and players are identities only, so each is a spawned actor - a bare NewObject<UObject>()
 *  trips an abstract-class ensure that fires once per session (testing.md). Where everyone stands is
 *  plain vectors.
 */

namespace SmoresApproachTest
{
	constexpr float Range = 800.0f;
	constexpr double Cooldown = 60.0;

	/** Someone standing at the origin who might be approached */
	static FApproachSpeaker Npc(const AActor* Who, bool bCanSpeak = true, bool bIsSquadMember = false)
	{
		FApproachSpeaker Speaker;
		Speaker.Key = FObjectKey(Who);
		Speaker.Location = FVector::ZeroVector;
		Speaker.bCanSpeak = bCanSpeak;
		Speaker.bIsSquadMember = bIsSquadMember;
		return Speaker;
	}

	/** Player's squad, each member standing that many cm from the origin along X */
	static FApproachSquad Squad(const AActor* Player, const TArray<double>& Distances)
	{
		FApproachSquad Result;
		Result.Player = FObjectKey(Player);

		for (const double Distance : Distances)
		{
			Result.MemberLocations.Add(FVector(Distance, 0.0, 0.0));
		}

		return Result;
	}

	/** Near and far, for readability */
	constexpr double Inside = 400.0;
	constexpr double Outside = 2000.0;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresApproachEnteringFiresOnceTest,
	"Smores.Dialog.Approach.EnteringFiresOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresApproachEnteringFiresOnceTest::RunTest(const FString& Parameters)
{
	using namespace SmoresApproachTest;

	FSmoresTestWorld TestWorld;

	const AActor* Trader = TestWorld.SpawnOwner();
	const AActor* Player = TestWorld.SpawnOwner();

	FApproachTracker Tracker;
	const TArray<FApproachSpeaker> Speakers = { Npc(Trader) };

	TestEqual(TEXT("A squad out of range is no approach"), Tracker.Update(Speakers, { Squad(Player, { Outside }) }, Range, 0.0, Cooldown).Num(), 0);

	const TArray<FApproach> Arrived = Tracker.Update(Speakers, { Squad(Player, { Inside }) }, Range, 1.0, Cooldown);

	if (TestEqual(TEXT("Coming within range is one approach"), Arrived.Num(), 1))
	{
		TestEqual(TEXT("...by the NPC"), Arrived[0].SpeakerIndex, 0);
		TestEqual(TEXT("...to that squad"), Arrived[0].SquadIndex, 0);
		TestEqual(TEXT("...and its member who came near"), Arrived[0].MemberIndex, 0);
	}

	TestEqual(TEXT("Standing there is no second approach"), Tracker.Update(Speakers, { Squad(Player, { 300.0 }) }, Range, 2.0, Cooldown).Num(), 0);
	TestEqual(TEXT("...nor once the cooldown has run out, with the squad still standing there"), Tracker.Update(Speakers, { Squad(Player, { 300.0 }) }, Range, 100.0, Cooldown).Num(), 0);

	// the listener is whichever member came nearest, not the first in the list
	FApproachTracker Fresh;
	const TArray<FApproach> Nearest = Fresh.Update(Speakers, { Squad(Player, { 700.0, 300.0, Outside }) }, Range, 0.0, Cooldown);

	if (TestEqual(TEXT("A squad arriving together is one approach, not one per member"), Nearest.Num(), 1))
	{
		TestEqual(TEXT("...and its listener is the member nearest the NPC"), Nearest[0].MemberIndex, 1);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresApproachReturnWaitsTest,
	"Smores.Dialog.Approach.ComingBackWaitsForTheCooldown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresApproachReturnWaitsTest::RunTest(const FString& Parameters)
{
	using namespace SmoresApproachTest;

	FSmoresTestWorld TestWorld;

	const AActor* Trader = TestWorld.SpawnOwner();
	const AActor* Player = TestWorld.SpawnOwner();

	FApproachTracker Tracker;
	const TArray<FApproachSpeaker> Speakers = { Npc(Trader) };

	auto At = [&](double Distance, double Now)
	{
		return Tracker.Update(Speakers, { Squad(Player, { Distance }) }, Range, Now, Cooldown).Num();
	};

	TestEqual(TEXT("The first arrival is an approach"), At(Inside, 0.0), 1);
	TestEqual(TEXT("Leaving is not"), At(Outside, 5.0), 0);
	TestEqual(TEXT("Coming back inside the cooldown is not"), At(Inside, 10.0), 0);
	TestEqual(TEXT("Still standing there when the cooldown runs out is not - they have to leave and come back"), At(Inside, 70.0), 0);
	TestEqual(TEXT("Leaving again is not"), At(Outside, 75.0), 0);
	TestEqual(TEXT("Coming back after the cooldown is"), At(Inside, 80.0), 1);
	TestEqual(TEXT("...and that starts a fresh cooldown"), At(Outside, 85.0), 0);
	TestEqual(TEXT("...which a quick return is still inside"), At(Inside, 100.0), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresApproachNeverFiresTest,
	"Smores.Dialog.Approach.DownedNPCsAndSquadMembersNeverFire",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresApproachNeverFiresTest::RunTest(const FString& Parameters)
{
	using namespace SmoresApproachTest;

	FSmoresTestWorld TestWorld;

	const AActor* Bandit = TestWorld.SpawnOwner();
	const AActor* SquadMate = TestWorld.SpawnOwner();
	const AActor* Player = TestWorld.SpawnOwner();

	FApproachTracker Tracker;

	TestEqual(TEXT("Walking up to a downed NPC is no approach"),
		Tracker.Update({ Npc(Bandit, /*bCanSpeak*/ false) }, { Squad(Player, { Inside }) }, Range, 0.0, Cooldown).Num(), 0);

	TestEqual(TEXT("...nor is the NPC getting up with the squad already standing there"),
		Tracker.Update({ Npc(Bandit) }, { Squad(Player, { Inside }) }, Range, 1.0, Cooldown).Num(), 0);

	Tracker.Update({ Npc(Bandit) }, { Squad(Player, { Outside }) }, Range, 2.0, Cooldown);

	TestEqual(TEXT("Leaving and coming back to them on their feet is - the downed arrival never started a cooldown"),
		Tracker.Update({ Npc(Bandit) }, { Squad(Player, { Inside }) }, Range, 3.0, Cooldown).Num(), 1);

	// a squad member is never approached - by their own squad or anyone else's
	FApproachTracker Squads;
	const TArray<FApproachSpeaker> Member = { Npc(SquadMate, /*bCanSpeak*/ true, /*bIsSquadMember*/ true) };

	TestEqual(TEXT("A squad member is never approached"), Squads.Update(Member, { Squad(Player, { Outside }) }, Range, 0.0, Cooldown).Num(), 0);
	TestEqual(TEXT("...even when someone walks right up to them"), Squads.Update(Member, { Squad(Player, { Inside }) }, Range, 1.0, Cooldown).Num(), 0);
	TestEqual(TEXT("...and nothing is remembered about them"), Squads.GetTrackedCount(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresApproachPerPlayerTest,
	"Smores.Dialog.Approach.EachPlayerSeparately",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresApproachPerPlayerTest::RunTest(const FString& Parameters)
{
	using namespace SmoresApproachTest;

	FSmoresTestWorld TestWorld;

	const AActor* Trader = TestWorld.SpawnOwner();
	const AActor* Guard = TestWorld.SpawnOwner();
	const AActor* PlayerA = TestWorld.SpawnOwner();
	const AActor* PlayerB = TestWorld.SpawnOwner();

	FApproachTracker Tracker;
	const TArray<FApproachSpeaker> Speakers = { Npc(Trader) };

	const TArray<FApproach> First = Tracker.Update(Speakers, { Squad(PlayerA, { Inside }), Squad(PlayerB, { Outside }) }, Range, 0.0, Cooldown);

	if (TestEqual(TEXT("Player A's squad arriving is one approach"), First.Num(), 1))
	{
		TestEqual(TEXT("...by player A"), First[0].SquadIndex, 0);
	}

	const TArray<FApproach> Second = Tracker.Update(Speakers, { Squad(PlayerA, { Inside }), Squad(PlayerB, { Inside }) }, Range, 1.0, Cooldown);

	if (TestEqual(TEXT("Player B's squad arriving is an approach of its own - A's didn't use it up"), Second.Num(), 1))
	{
		TestEqual(TEXT("...by player B"), Second[0].SquadIndex, 1);
	}

	Tracker.Update(Speakers, { Squad(PlayerA, { Outside }), Squad(PlayerB, { Inside }) }, Range, 2.0, Cooldown);

	TestEqual(TEXT("Player A coming straight back is still inside A's cooldown"),
		Tracker.Update(Speakers, { Squad(PlayerA, { Inside }), Squad(PlayerB, { Inside }) }, Range, 3.0, Cooldown).Num(), 0);

	// and each NPC separately: one squad walking up to two people is two approaches
	FApproachTracker TwoNpcs;
	const TArray<FApproach> Both = TwoNpcs.Update({ Npc(Trader), Npc(Guard) }, { Squad(PlayerA, { Inside }) }, Range, 0.0, Cooldown);

	if (TestEqual(TEXT("A squad arriving by two NPCs at once is two approaches"), Both.Num(), 2))
	{
		TestEqual(TEXT("...one by the first"), Both[0].SpeakerIndex, 0);
		TestEqual(TEXT("...and one by the second"), Both[1].SpeakerIndex, 1);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresApproachMemoryBoundedTest,
	"Smores.Dialog.Approach.MemoryStaysBounded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresApproachMemoryBoundedTest::RunTest(const FString& Parameters)
{
	using namespace SmoresApproachTest;

	FSmoresTestWorld TestWorld;

	const AActor* Trader = TestWorld.SpawnOwner();
	const AActor* Player = TestWorld.SpawnOwner();

	FApproachTracker Tracker;
	const TArray<FApproachSpeaker> Speakers = { Npc(Trader) };

	Tracker.Update(Speakers, { Squad(Player, { Outside }) }, Range, 0.0, Cooldown);
	TestEqual(TEXT("A squad that has never come near is not remembered"), Tracker.GetTrackedCount(), 0);

	Tracker.Update(Speakers, { Squad(Player, { Inside }) }, Range, 1.0, Cooldown);
	TestEqual(TEXT("A squad standing near is remembered"), Tracker.GetTrackedCount(), 1);

	Tracker.Update(Speakers, { Squad(Player, { Outside }) }, Range, 2.0, Cooldown);
	TestEqual(TEXT("...and still is after it leaves, while the cooldown runs"), Tracker.GetTrackedCount(), 1);

	Tracker.Update(Speakers, { Squad(Player, { Outside }) }, Range, 62.0, Cooldown);
	TestEqual(TEXT("...and forgotten once it's gone and the cooldown is over"), Tracker.GetTrackedCount(), 0);

	TestEqual(TEXT("Forgetting it changes nothing: coming back is an approach"),
		Tracker.Update(Speakers, { Squad(Player, { Inside }) }, Range, 63.0, Cooldown).Num(), 1);

	Tracker.Update({}, { Squad(Player, { Inside }) }, Range, 64.0, Cooldown);
	TestEqual(TEXT("An NPC missing from an update (destroyed, streamed out) is forgotten"), Tracker.GetTrackedCount(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
