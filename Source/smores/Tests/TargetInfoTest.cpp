// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "StrategyPlayerController.h"
#include "StrategyTargetInfo.h"
#include "HealthComponent.h"
#include "Tests/SmoresTestWorld.h"
#include "Engine/World.h"

/**
 *  The action with this id, or null if the row doesn't offer it.
 *
 *  Uniquely prefixed rather than living in an anonymous namespace: UE's unity builds concatenate
 *  several test .cpp files into one translation unit, where two anonymous namespaces merge and a
 *  shared helper name becomes a redefinition. See testing.md.
 */
static const FTargetAction* SmoresTargetInfoTest_FindAction(const FStrategyTargetInfo& Info, FName ActionId)
{
	return Info.Actions.FindByPredicate([ActionId](const FTargetAction& Candidate)
	{
		return Candidate.Id == ActionId;
	});
}

/** Spawns one of the test stand-in actors at a world location, or null if the world isn't up */
template <typename TActor>
static TActor* SmoresTargetInfoTest_Spawn(FSmoresTestWorld& TestWorld, const FVector& Location)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoNoTargetTest,
	"Smores.Strategy.TargetInfo.NothingTargetedIsEmpty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoNoTargetTest::RunTest(const FString& Parameters)
{
	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(nullptr, TArray<AStrategyUnit*>());

	TestFalse(TEXT("Nothing targeted means nothing to show"), Info.HasTarget());
	TestEqual(TEXT("...and no action row at all"), Info.Actions.Num(), 0);
	TestTrue(TEXT("...and no distance to print"), Info.DistanceMeters < 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoContainerTest,
	"Smores.Strategy.TargetInfo.ContainerOffersOpen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoContainerTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyContainer* Container = SmoresTargetInfoTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Container spawned"), Container) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo InRange = AStrategyPlayerController::BuildTargetInfo(Container, Selection);

	TestTrue(TEXT("A targeted container is something to show"), InRange.HasTarget());
	TestFalse(TEXT("...with no health bar, because a chest has no health"), InRange.bHasHealth);
	TestEqual(TEXT("...and exactly one thing you can do to it"), InRange.Actions.Num(), 1);
	TestEqual(TEXT("...measured from the selected pawn, in metres"), InRange.DistanceMeters, 1.0f, 0.01f);

	const FTargetAction* Open = SmoresTargetInfoTest_FindAction(InRange, StrategyTargetAction::Open());

	if (!TestNotNull(TEXT("The one action is Open"), Open))
	{
		return true;
	}

	TestTrue(TEXT("...enabled, since the pawn is standing next to it"), Open->bEnabled);
	TestTrue(TEXT("...with no reason to give"), Open->DisabledReason == ESmoresRefusalReason::None);

	// walk the pawn well past the container's own interaction sphere
	Pawn->SetActorLocation(FVector(5000.0f, 0.0f, 0.0f));

	const FStrategyTargetInfo OutOfRange = AStrategyPlayerController::BuildTargetInfo(Container, Selection);

	const FTargetAction* DistantOpen = SmoresTargetInfoTest_FindAction(OutOfRange, StrategyTargetAction::Open());

	if (!TestNotNull(TEXT("Open is still offered from across the level"), DistantOpen))
	{
		return true;
	}

	// disabled-but-visible, not absent: the player can see that opening it is a thing, and why
	// they can't right now
	TestFalse(TEXT("...but disabled"), DistantOpen->bEnabled);
	TestTrue(TEXT("...naming TooFar"), DistantOpen->DisabledReason == ESmoresRefusalReason::TooFar);
	TestEqual(TEXT("...and the distance reads 50m"), OutOfRange.DistanceMeters, 50.0f, 0.01f);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoNeutralPersonTest,
	"Smores.Strategy.TargetInfo.NeutralPersonOffersTalkAndAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoNeutralPersonTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(NPC, Selection);

	TestTrue(TEXT("A targeted person is something to show"), Info.HasTarget());
	TestTrue(TEXT("...with a health bar"), Info.bHasHealth);
	TestEqual(TEXT("...reading full, since nothing has hit them"), Info.HealthFraction, 1.0f, 0.001f);
	TestEqual(TEXT("...and two things you can do to them"), Info.Actions.Num(), 2);

	const FTargetAction* Talk = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Talk());
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack());

	if (!TestNotNull(TEXT("Talk is offered"), Talk) || !TestNotNull(TEXT("Attack is offered"), Attack))
	{
		return true;
	}

	TestTrue(TEXT("Talk is enabled - they're neutral and within reach"), Talk->bEnabled);
	TestTrue(TEXT("Attack is enabled - they aren't fighting yet"), Attack->bEnabled);

	// Attack deliberately has no range gate: DoAttackCommand sends the squad to close the
	// distance, so "too far to attack" is not a thing this game has
	Pawn->SetActorLocation(FVector(5000.0f, 0.0f, 0.0f));

	const FStrategyTargetInfo Distant = AStrategyPlayerController::BuildTargetInfo(NPC, Selection);

	const FTargetAction* DistantTalk = SmoresTargetInfoTest_FindAction(Distant, StrategyTargetAction::Talk());
	const FTargetAction* DistantAttack = SmoresTargetInfoTest_FindAction(Distant, StrategyTargetAction::Attack());

	if (!TestNotNull(TEXT("Talk is still offered from a distance"), DistantTalk)
		|| !TestNotNull(TEXT("Attack is still offered from a distance"), DistantAttack))
	{
		return true;
	}

	TestFalse(TEXT("Talk goes disabled out of reach"), DistantTalk->bEnabled);
	TestTrue(TEXT("...naming TooFar"), DistantTalk->DisabledReason == ESmoresRefusalReason::TooFar);
	TestTrue(TEXT("Attack stays enabled, because the squad walks over"), DistantAttack->bEnabled);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoHostilePersonTest,
	"Smores.Strategy.TargetInfo.HostilePersonRefusesTalk",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoHostilePersonTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	// hostility without the self-hunting machinery - see ATestStrategyNPC::MakeHostileForTest for
	// why the real setter can't be used from a test
	NPC->MakeHostileForTest();

	TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(NPC, Selection);

	TestEqual(TEXT("A hostile person still offers both actions"), Info.Actions.Num(), 2);

	const FTargetAction* Talk = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Talk());
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack());

	if (!TestNotNull(TEXT("Talk is offered"), Talk) || !TestNotNull(TEXT("Attack is offered"), Attack))
	{
		return true;
	}

	// this is the case the whole disabled-but-visible rule exists for: standing right next to
	// someone trying to kill you and seeing *why* you can't trade with them
	TestFalse(TEXT("Talk is disabled even standing next to them"), Talk->bEnabled);
	TestTrue(TEXT("...naming NotInteractable rather than TooFar"), Talk->DisabledReason == ESmoresRefusalReason::NotInteractable);

	TestFalse(TEXT("Attack is disabled - the fight is already running"), Attack->bEnabled);
	TestTrue(TEXT("...with no reason given, because that isn't a refusal"), Attack->DisabledReason == ESmoresRefusalReason::None);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoBodyTest,
	"Smores.Strategy.TargetInfo.BodyOffersLootAndNothingElse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoBodyTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	// UHealthComponent::Kill warns on the way past; assert it did rather than merely suppressing it
	AddExpectedMessagePlain(TEXT("died"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	if (!TestNotNull(TEXT("The NPC has health to lose"), NPC->GetHealth()))
	{
		return true;
	}

	NPC->GetHealth()->Kill();

	TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(NPC, Selection);

	TestTrue(TEXT("A body is still something to show"), Info.HasTarget());
	TestEqual(TEXT("...reading empty on the health bar"), Info.HealthFraction, 0.0f, 0.001f);

	// the row is Loot and only Loot - talking to a corpse and attacking one are both things the
	// rules already forbid, so offering them greyed out would be teaching a rule that doesn't exist
	TestEqual(TEXT("...and offering exactly one action"), Info.Actions.Num(), 1);

	const FTargetAction* Loot = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Loot());

	if (!TestNotNull(TEXT("The one action is Loot"), Loot))
	{
		return true;
	}

	TestTrue(TEXT("...enabled, since the pawn is standing over them"), Loot->bEnabled);
	TestNull(TEXT("Talk is not offered on a body at all"), SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Talk()));
	TestNull(TEXT("...nor is Attack"), SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack()));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoOwnPawnTest,
	"Smores.Strategy.TargetInfo.OwnPawnOffersNoActions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoOwnPawnTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyPlayerUnit* Target = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Selected = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Target pawn spawned"), Target) || !TestNotNull(TEXT("Selected pawn spawned"), Selected))
	{
		return true;
	}

	TArray<AStrategyUnit*> Selection = { Selected };

	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(Target, Selection);

	TestTrue(TEXT("One of your own squad is still something to show"), Info.HasTarget());
	TestTrue(TEXT("...with a health bar"), Info.bHasHealth);

	// The case the roadmap called out by name: an empty row, not a row of disabled everything.
	// Every verb that applies to your own pawn already has a route that isn't this panel.
	TestEqual(TEXT("...and an empty action row rather than disabled buttons"), Info.Actions.Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoDistanceTest,
	"Smores.Strategy.TargetInfo.DistanceIsFromNearestSelectedUnit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoDistanceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyContainer* Container = SmoresTargetInfoTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Near = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(400.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Far = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Container spawned"), Container)
		|| !TestNotNull(TEXT("Near pawn spawned"), Near)
		|| !TestNotNull(TEXT("Far pawn spawned"), Far))
	{
		return true;
	}

	// far one first, so a bug that takes the first unit rather than the nearest reads 30m
	TArray<AStrategyUnit*> Selection = { Far, Near };

	const FStrategyTargetInfo Info = AStrategyPlayerController::BuildTargetInfo(Container, Selection);

	TestEqual(TEXT("The distance is to the nearest selected unit, not the first"), Info.DistanceMeters, 4.0f, 0.01f);

	// with nothing selected there is nothing to measure from, and the panel is told so rather than
	// being handed a zero it would print as "0m"
	const FStrategyTargetInfo Unselected = AStrategyPlayerController::BuildTargetInfo(Container, TArray<AStrategyUnit*>());

	TestTrue(TEXT("With nothing selected the distance is unknown, not zero"), Unselected.DistanceMeters < 0.0f);
	TestTrue(TEXT("...and the target is still worth showing"), Unselected.HasTarget());

	const FTargetAction* Open = SmoresTargetInfoTest_FindAction(Unselected, StrategyTargetAction::Open());

	if (!TestNotNull(TEXT("Open is still offered"), Open))
	{
		return true;
	}

	TestFalse(TEXT("...but disabled, since nobody is selected to walk over and do it"), Open->bEnabled);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
