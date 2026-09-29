// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "StrategyTargetActions.h"
#include "StrategyTargetInfo.h"
#include "ActionOrderComponent.h"
#include "HealthComponent.h"
#include "SmoresInteractable.h"
#include "Tests/SmoresTestWorld.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Engine/World.h"

/*
 *  The walk-over-to-act order (UActionOrderComponent) - a state machine with counted outcomes,
 *  which is exactly testing.md's case. A test world has no navmesh, so no real walk can happen:
 *  every test here stands in for the walk with UActionOrderComponent::SetApproachForTest and then
 *  teleports the unit and reports the walk finished, which is all the order ever sees of a walk.
 *
 *  The host is UTestActionOrderHost, which answers the arrival re-check through the real rules
 *  (FStrategyTargetActions::FindActionFor) and counts what it's asked to do.
 *
 *  Also here: the server's order validation, the door, and the shared ISmoresInteractable interface.
 */

/** Units log their combat reactions at Warning, and a hit spawns a damage number no test has a class for */
#define EXPECT_ACTION_ORDER_COMBAT_LOGS() \
	AddExpectedMessagePlain(TEXT("[Combat]"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0)

/**
 *  Beginning play gives the test world a GameState with a record store on it, and a unit that
 *  registers with one while authored with neither a record key nor a definition says so - true,
 *  and beside the point of an order test.
 */
#define EXPECT_ACTION_ORDER_RECORD_LOGS() \
	AddExpectedMessagePlain(TEXT("with no PlacedRecordId"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0); \
	AddExpectedMessagePlain(TEXT("has no CharacterDefinition"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0)

/** Spawns one of the test stand-in actors, or null if the world isn't up. Uniquely prefixed - see testing.md on unity builds. */
template <typename TActor>
static TActor* SmoresActionOrderTest_Spawn(FSmoresTestWorld& TestWorld, const FVector& Location)
{
	UWorld* World = TestWorld.GetWorld();

	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return World->SpawnActor<TActor>(TActor::StaticClass(), FTransform(Location), SpawnParameters);
}

/**
 *  One order under test: a squad member, the host standing in for its controller, and a counted
 *  stand-in for the walk that answers Walking unless told otherwise.
 */
struct FSmoresActionOrderRig
{
	ATestStrategyPlayerUnit* Pawn = nullptr;
	UActionOrderComponent* Order = nullptr;
	UTestActionOrderHost* Host = nullptr;

	/** How many walks the order has asked for */
	TSharedRef<int32> WalkCount = MakeShared<int32>(0);

	/** What the stand-in walk answers */
	TSharedRef<EActionApproachResult> WalkAnswer = MakeShared<EActionApproachResult>(EActionApproachResult::Walking);
};

static bool SmoresActionOrderTest_Build(FSmoresTestWorld& TestWorld, FSmoresActionOrderRig& Rig, const FVector& PawnLocation)
{
	Rig.Pawn = SmoresActionOrderTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, PawnLocation);
	Rig.Host = TestWorld.NewKeptObject<UTestActionOrderHost>();

	if (!Rig.Pawn || !Rig.Host)
	{
		return false;
	}

	Rig.Order = Rig.Pawn->GetActionOrder();

	if (!Rig.Order)
	{
		return false;
	}

	Rig.Host->Squad.Add(Rig.Pawn);
	Rig.Order->SetHostForTest(Rig.Host);

	TSharedRef<int32> WalkCount = Rig.WalkCount;
	TSharedRef<EActionApproachResult> WalkAnswer = Rig.WalkAnswer;

	Rig.Order->SetApproachForTest([WalkCount, WalkAnswer](AActor*)
	{
		++(*WalkCount);
		return *WalkAnswer;
	});

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderInReachTest,
	"Smores.Strategy.ActionOrder.InReachActsOnceAtOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderInReachTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(100.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	TestTrue(TEXT("The order is accepted"), Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot()));

	TestEqual(TEXT("Standing beside it, nobody walks anywhere"), *Rig.WalkCount, 0);
	TestEqual(TEXT("...the rules are asked once"), Rig.Host->CanPerformCount, 1);
	TestEqual(TEXT("...and the action is carried out exactly once"), Rig.Host->PerformCount, 1);
	TestTrue(TEXT("...and it was the loot"), Rig.Host->LastPerformedAction == StrategyTargetAction::Loot());
	TestFalse(TEXT("...and the order is over"), Rig.Order->HasOrder());
	TestEqual(TEXT("...with nothing to report"), Rig.Host->EndedCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderArrivalTest,
	"Smores.Strategy.ActionOrder.ArrivalInReachActsOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderArrivalTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	TestTrue(TEXT("The order is accepted"), Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot()));

	TestEqual(TEXT("From across the room, the unit sets off"), *Rig.WalkCount, 1);
	TestEqual(TEXT("...and nothing is done yet"), Rig.Host->PerformCount, 0);
	TestTrue(TEXT("...while the order waits"), Rig.Order->HasOrder());
	TestTrue(TEXT("...for that chest"), Rig.Order->GetOrderTarget() == Chest);

	// the walk, as far as the order can tell: the unit is now beside it and the walk has ended
	Rig.Pawn->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Rig.Order->HandleApproachFinished();

	TestEqual(TEXT("Arriving within reach carries the action out"), Rig.Host->PerformCount, 1);
	TestFalse(TEXT("...and ends the order"), Rig.Order->HasOrder());

	// a stray report after the order is over - an abort from a walk that has since been replaced
	Rig.Order->HandleApproachFinished();

	TestEqual(TEXT("A second 'arrival' does nothing"), Rig.Host->PerformCount, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderAlreadyThereTest,
	"Smores.Strategy.ActionOrder.AlreadyAtGoalStillActs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderAlreadyThereTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	// The path follower's "already at goal" makes no request, so no finished callback will ever
	// come. A unit that answers it as if it were walking waits forever - the trap this asserts.
	ATestStrategyPlayerUnit* Pawn = Rig.Pawn;

	Rig.Order->SetApproachForTest([Pawn](AActor*)
	{
		Pawn->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
		return EActionApproachResult::AlreadyThere;
	});

	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());

	TestEqual(TEXT("Already at the goal, the action is carried out without waiting for a walk"), Rig.Host->PerformCount, 1);
	TestFalse(TEXT("...and the order is over"), Rig.Order->HasOrder());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderPlayerMoveTest,
	"Smores.Strategy.ActionOrder.PlayerOrderCancelsSilently",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderPlayerMoveTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	EXPECT_ACTION_ORDER_COMBAT_LOGS();

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyContainer* OtherChest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector(0.0f, 5000.0f, 0.0f));
	ATestStrategyNPC* Enemy = SmoresActionOrderTest_Spawn<ATestStrategyNPC>(TestWorld, FVector(-5000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Chests spawned"), OtherChest) || !TestNotNull(TEXT("Enemy spawned"), Enemy)
		|| !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	// a move order: the catch-all, and the most common way an order ends
	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());
	Rig.Pawn->MoveToLocation(FVector(3000.0f, 3000.0f, 0.0f), /*bLeadUnit*/ false);

	TestFalse(TEXT("A move order ends the pending order"), Rig.Order->HasOrder());
	TestEqual(TEXT("...without a word - the player did it"), Rig.Host->EndedCount, 0);

	Rig.Pawn->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Rig.Order->HandleApproachFinished();

	TestEqual(TEXT("...and a late walk report acts on nothing"), Rig.Host->PerformCount, 0);

	// an attack order ends it too
	Rig.Pawn->SetActorLocation(FVector(3000.0f, 0.0f, 0.0f));
	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());
	Rig.Pawn->AttackTarget(Enemy);

	TestFalse(TEXT("An attack order ends it"), Rig.Order->HasOrder());
	TestEqual(TEXT("...silently"), Rig.Host->EndedCount, 0);

	// and so does another action order, which replaces it
	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());
	Rig.Order->IssueOrder(OtherChest, StrategyTargetAction::Loot());

	TestTrue(TEXT("A second action order replaces the first"), Rig.Order->GetOrderTarget() == OtherChest);
	TestEqual(TEXT("...silently"), Rig.Host->EndedCount, 0);
	TestEqual(TEXT("...and nothing was done to the first chest"), Rig.Host->PerformCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderDownedTest,
	"Smores.Strategy.ActionOrder.GoingDownFailsWithNotice",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderDownedTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	EXPECT_ACTION_ORDER_COMBAT_LOGS();

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	EXPECT_ACTION_ORDER_RECORD_LOGS();

	// BeginPlay is what binds the unit to its own health going down
	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		return true;
	}

	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());

	Rig.Pawn->GetHealth()->TakeDamage(1000.0f);

	TestTrue(TEXT("The pawn went down on the way"), Rig.Pawn->IsDowned());
	TestFalse(TEXT("...which ends the order"), Rig.Order->HasOrder());
	TestEqual(TEXT("...and says so, once"), Rig.Host->EndedCount, 1);
	TestTrue(TEXT("...as going down"), Rig.Host->LastEnd == EActionOrderEnd::ActorDown);
	TestEqual(TEXT("...having done nothing"), Rig.Host->PerformCount, 0);

	TestFalse(TEXT("A downed unit can't be given an order at all"), Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot()));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderRetaliationTest,
	"Smores.Strategy.ActionOrder.FightingBackFailsWithNotice",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderRetaliationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	EXPECT_ACTION_ORDER_COMBAT_LOGS();

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyNPC* Attacker = SmoresActionOrderTest_Spawn<ATestStrategyNPC>(TestWorld, FVector(6000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestNotNull(TEXT("Attacker spawned"), Attacker)
		|| !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	EXPECT_ACTION_ORDER_RECORD_LOGS();

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		return true;
	}

	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());

	// hit by someone: the unit swings back, which the player didn't ask for
	Rig.Pawn->GetHealth()->TakeDamage(10.0f, Attacker);

	TestFalse(TEXT("Fighting back ends the order"), Rig.Order->HasOrder());
	TestEqual(TEXT("...and says so"), Rig.Host->EndedCount, 1);
	TestTrue(TEXT("...as stopping to fight"), Rig.Host->LastEnd == EActionOrderEnd::ActorFighting);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderCannotReachTest,
	"Smores.Strategy.ActionOrder.NoPathFailsAfterRetries",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderCannotReachTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());

	// every walk ends where it started - a shut door, a partial path - and the order tries again
	Rig.Order->HandleApproachFinished();

	TestTrue(TEXT("Arriving out of reach walks again"), Rig.Order->HasOrder());
	TestEqual(TEXT("...a second walk"), *Rig.WalkCount, 2);
	TestEqual(TEXT("...counted as a retry"), Rig.Order->GetRetryCount(), 1);

	Rig.Order->HandleApproachFinished();

	TestEqual(TEXT("...and a third"), *Rig.WalkCount, 3);

	Rig.Order->HandleApproachFinished();

	// two retries by default: the first walk and two more, then give up
	TestFalse(TEXT("After the retries, the order gives up"), Rig.Order->HasOrder());
	TestEqual(TEXT("...without walking a fourth time"), *Rig.WalkCount, 3);
	TestEqual(TEXT("...saying so once"), Rig.Host->EndedCount, 1);
	TestTrue(TEXT("...as Can't get there"), Rig.Host->LastEnd == EActionOrderEnd::CannotReach && Rig.Host->LastReason == ESmoresRefusalReason::CannotReach);
	TestEqual(TEXT("...having done nothing"), Rig.Host->PerformCount, 0);

	// a walk that can't even start fails straight away
	Rig.Host->ResetCounts();
	*Rig.WalkAnswer = EActionApproachResult::Failed;

	Rig.Order->IssueOrder(Chest, StrategyTargetAction::Loot());

	TestFalse(TEXT("No walk at all fails at once"), Rig.Order->HasOrder());
	TestTrue(TEXT("...as Can't get there"), Rig.Host->EndedCount == 1 && Rig.Host->LastReason == ESmoresRefusalReason::CannotReach);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderHostileOnArrivalTest,
	"Smores.Strategy.ActionOrder.TurnedHostileRefusesOnArrival",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderHostileOnArrivalTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestStrategyNPC* NPC = SmoresActionOrderTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	Rig.Order->IssueOrder(NPC, StrategyTargetAction::Talk());

	// they turned on the squad while it was walking over
	NPC->MakeHostileForTest();

	Rig.Pawn->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Rig.Order->HandleApproachFinished();

	TestEqual(TEXT("On arrival the rules are asked again"), Rig.Host->CanPerformCount, 1);
	TestEqual(TEXT("...and nothing is done"), Rig.Host->PerformCount, 0);
	TestEqual(TEXT("...and the refusal is reported"), Rig.Host->EndedCount, 1);
	TestTrue(TEXT("...as the menu would have refused it"), Rig.Host->LastEnd == EActionOrderEnd::Refused && Rig.Host->LastReason == ESmoresRefusalReason::NotInteractable);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderTargetGoneTest,
	"Smores.Strategy.ActionOrder.TargetGoneFailsWithNotice",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderTargetGoneTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	FSmoresActionOrderRig Rig;

	ATestWorldItem* Item = SmoresActionOrderTest_Spawn<ATestWorldItem>(TestWorld, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Item spawned"), Item) || !TestTrue(TEXT("Rig built"), SmoresActionOrderTest_Build(TestWorld, Rig, FVector(3000.0f, 0.0f, 0.0f))))
	{
		return true;
	}

	Item->SetItem(MakeTestItem(MakeTestItemDefinition(TestWorld), 1));

	Rig.Order->IssueOrder(Item, StrategyTargetAction::PickUp());

	// somebody else picked it up first
	Item->Destroy();

	Rig.Order->HandleApproachFinished();

	TestFalse(TEXT("A target that's gone ends the order"), Rig.Order->HasOrder());
	TestTrue(TEXT("...and says so"), Rig.Host->EndedCount == 1 && Rig.Host->LastEnd == EActionOrderEnd::TargetGone);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresActionOrderValidationTest,
	"Smores.Strategy.ActionOrder.ServerValidatesTheRequest",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresActionOrderValidationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresActionOrderTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Mine = SmoresActionOrderTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Theirs = SmoresActionOrderTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(0.0f, 3000.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("My pawn spawned"), Mine) || !TestNotNull(TEXT("Their pawn spawned"), Theirs))
	{
		return true;
	}

	const TArray<AStrategyUnit*> MySquad = { Mine };
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	TestTrue(TEXT("My own squad member may be sent to talk"), FStrategyTargetActions::ValidateActionOrder(Mine, NPC, StrategyTargetAction::Talk(), MySquad, Reason));

	// the case the roadmap names: a client naming a unit it doesn't own
	TestFalse(TEXT("Another player's unit is rejected"), FStrategyTargetActions::ValidateActionOrder(Theirs, NPC, StrategyTargetAction::Talk(), MySquad, Reason));
	TestTrue(TEXT("...with nothing to tell the player - it was never a legal request"), Reason == ESmoresRefusalReason::None);

	// Attack and Examine are not orders - combat walks itself over, and looking walks nowhere
	TestFalse(TEXT("Attack is not an order"), FStrategyTargetActions::ValidateActionOrder(Mine, NPC, StrategyTargetAction::Attack(), MySquad, Reason));
	TestFalse(TEXT("Examine is not an order"), FStrategyTargetActions::ValidateActionOrder(Mine, NPC, StrategyTargetAction::Examine(), MySquad, Reason));

	// something the target doesn't offer at all
	TestFalse(TEXT("Loot on someone standing is rejected"), FStrategyTargetActions::ValidateActionOrder(Mine, NPC, StrategyTargetAction::Loot(), MySquad, Reason));

	// something offered but greyed out - refused with the entry's own reason
	NPC->MakeHostileForTest();

	TestFalse(TEXT("Talk to a hostile is rejected"), FStrategyTargetActions::ValidateActionOrder(Mine, NPC, StrategyTargetAction::Talk(), MySquad, Reason));
	TestTrue(TEXT("...saying they won't deal with you"), Reason == ESmoresRefusalReason::NotInteractable);

	// only Heal is something a unit may do to itself
	TestFalse(TEXT("A unit can't be ordered to talk to itself"), FStrategyTargetActions::ValidateActionOrder(Mine, Mine, StrategyTargetAction::Talk(), MySquad, Reason));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWorldDoorTest,
	"Smores.Strategy.Door.OpensShutsAndReaches",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWorldDoorTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestWorldDoor* Door = SmoresActionOrderTest_Spawn<ATestWorldDoor>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Near = SmoresActionOrderTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(150.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Far = SmoresActionOrderTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(1000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Door spawned"), Door) || !TestNotNull(TEXT("Near pawn spawned"), Near) || !TestNotNull(TEXT("Far pawn spawned"), Far))
	{
		return true;
	}

	TestFalse(TEXT("A door starts shut unless authored open"), Door->IsOpen());

	Door->SetOpen(true);
	TestTrue(TEXT("SetOpen opens it"), Door->IsOpen());

	Door->SetOpen(true);
	TestTrue(TEXT("...and opening an open door leaves it open"), Door->IsOpen());

	Door->SetOpen(false);
	TestFalse(TEXT("SetOpen shuts it again"), Door->IsOpen());

	TestTrue(TEXT("A squad member beside it is within reach"), Door->IsInRangeOf(Near));
	TestFalse(TEXT("...one across the room isn't"), Door->IsInRangeOf(Far));

	// a client can't swing a door - it asks the server, through an action order
	Door->SetRole(ROLE_SimulatedProxy);
	Door->SetOpen(true);

	TestFalse(TEXT("Off-authority, SetOpen changes nothing"), Door->IsOpen());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInteractableInterfaceTest,
	"Smores.Strategy.Interactable.EveryTargetKindIsInteractable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInteractableInterfaceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_ACTION_ORDER_COMBAT_LOGS();

	ATestStrategyContainer* Chest = SmoresActionOrderTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyNPC* NPC = SmoresActionOrderTest_Spawn<ATestStrategyNPC>(TestWorld, FVector(500.0f, 0.0f, 0.0f));
	ATestWorldItem* Item = SmoresActionOrderTest_Spawn<ATestWorldItem>(TestWorld, FVector(0.0f, 500.0f, 0.0f));
	ATestWorldDoor* Door = SmoresActionOrderTest_Spawn<ATestWorldDoor>(TestWorld, FVector(0.0f, -500.0f, 0.0f));

	if (!TestNotNull(TEXT("Chest spawned"), Chest) || !TestNotNull(TEXT("NPC spawned"), NPC)
		|| !TestNotNull(TEXT("Item spawned"), Item) || !TestNotNull(TEXT("Door spawned"), Door))
	{
		return true;
	}

	// IInventoryHolder derives from ISmoresInteractable, and the cast has to find it through the
	// derived interface - the fallback, if this ever breaks, is holders implementing both
	TestNotNull(TEXT("A container is interactable through IInventoryHolder"), Cast<ISmoresInteractable>(Chest));
	TestNotNull(TEXT("...so is a unit"), Cast<ISmoresInteractable>(NPC));
	TestNotNull(TEXT("...and a loose item"), Cast<ISmoresInteractable>(Item));
	TestNotNull(TEXT("A door is interactable in its own right"), Cast<ISmoresInteractable>(Door));

	NPC->SetNameForTest(FText::FromString(TEXT("Bandit")));

	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(NPC);

	TestTrue(TEXT("A holder's interaction name is its holder name"), Interactable && Interactable->GetInteractionDisplayName().EqualTo(NPC->GetHolderDisplayName()));

	// examining describes, in words - never a stat, a skill or a health figure
	NPC->GetHealth()->TakeDamage(60.0f);

	const FString Examined = NPC->GetExamineText().ToString();

	TestTrue(TEXT("Examining a hurt unit says so in words"), Examined.Contains(TEXT("hurt")));

	bool bAnyDigit = false;

	for (const TCHAR Character : Examined)
	{
		bAnyDigit |= FChar::IsDigit(Character);
	}

	TestFalse(TEXT("...and never with a number"), bAnyDigit);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
