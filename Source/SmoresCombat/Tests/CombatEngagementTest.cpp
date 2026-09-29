// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CombatComponent.h"
#include "HealthComponent.h"
#include "UObject/EnumProperty.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The engagement state behind the danger flash: one signal per fight rather than one per hit,
 *  escalation signalling again inside a fight, and the timeout that ends one.
 *
 *  Every case drives the real attack path rather than calling NoteHostileAttention by hand.
 *  UCombatComponent::AttackTarget is reachable without a skeletal mesh because the attacker has no
 *  AttackMontages - PerformAttack records the target and stops before any montage - and
 *  ApplyAttackDamage is what the hit-frame anim notify would call. Attacker and victim both sit at
 *  the origin, so every swing is in range.
 *
 *  Timers need BeginPlay (testing.md), which is also what binds the victim's combat component to
 *  its health. The ticks are coarse (0.1s) because the engagement timeout is ten seconds and none
 *  of these cases cares about sub-tenth precision.
 */

/** One victim with health and combat, one attacker with combat, and every danger signal the victim raised */
struct FSmoresEngagementRig
{
	AActor* Victim = nullptr;

	UHealthComponent* VictimHealth = nullptr;

	UCombatComponent* VictimCombat = nullptr;

	UCombatComponent* AttackerCombat = nullptr;

	/** Every signal the victim broadcast, in order */
	TArray<EDangerSignal> Signals;

	/** Broadcasts that named some actor other than the victim - should stay zero */
	int32 WrongUnitCount = 0;

	bool IsValid() const
	{
		return Victim != nullptr && VictimHealth != nullptr && VictimCombat != nullptr && AttackerCombat != nullptr;
	}

	/** The signals so far as "Entered, Wounded", so a failed count says what actually arrived */
	FString Describe() const
	{
		TArray<FString> Names;

		for (EDangerSignal Signal : Signals)
		{
			Names.Add(StaticEnum<EDangerSignal>()->GetNameStringByValue(static_cast<int64>(Signal)));
		}

		return Names.Num() > 0 ? FString::Join(Names, TEXT(", ")) : TEXT("(none)");
	}

	/** One swing that lands: what the hit-frame notify does after AttackTarget found the victim in range */
	void Hit() const
	{
		AttackerCombat->ApplyAttackDamage();
	}
};

/**
 *  Builds the rig and listens to the victim. The rig must outlive the test world, because the
 *  listener captures it - so every test declares the rig *before* the world, and C++ tears the
 *  world down first.
 */
static bool SmoresEngagementTest_Build(FSmoresTestWorld& TestWorld, FSmoresEngagementRig& Rig)
{
	Rig.Victim = TestWorld.SpawnOwner();
	Rig.VictimHealth = TestWorld.AddComponent<UHealthComponent>(Rig.Victim);
	Rig.VictimCombat = TestWorld.AddComponent<UCombatComponent>(Rig.Victim);
	Rig.AttackerCombat = TestWorld.SpawnComponent<UCombatComponent>();

	if (!Rig.IsValid())
	{
		return false;
	}

	FSmoresEngagementRig* RigPtr = &Rig;

	Rig.VictimCombat->OnDangerSignal.AddLambda([RigPtr](AActor* Unit, EDangerSignal Signal)
	{
		if (Unit != RigPtr->Victim)
		{
			++RigPtr->WrongUnitCount;
		}

		RigPtr->Signals.Add(Signal);
	});

	return true;
}

/**
 *  Sets a combat component's DangerTrigger the way the Details panel would.
 *
 *  It is a protected EditAnywhere property - a designer's setting, not an API - so a test reaches
 *  it through reflection rather than the class growing a setter only tests would call. A rename
 *  makes this return false, which the test asserts, rather than silently testing the default.
 */
static bool SmoresEngagementTest_SetDangerTrigger(UCombatComponent* Combat, EDangerTrigger Trigger)
{
	FEnumProperty* Property = FindFProperty<FEnumProperty>(UCombatComponent::StaticClass(), TEXT("DangerTrigger"));

	if (!Property || !Combat)
	{
		return false;
	}

	Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Combat), static_cast<int64>(Trigger));

	return true;
}

/**
 *  Combat logs every step of a swing at Warning, prefixed "[Combat]" - and with no damage number
 *  class set, every hit also warns that it can't spawn one. All of it is expected here. Asserting
 *  it (Occurrences 0 = one or more) keeps the run green rather than yellow.
 */
#define EXPECT_COMBAT_LOG_WARNINGS() \
	AddExpectedMessagePlain(TEXT("[Combat]"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementOneSignalPerFightTest,
	"Smores.Combat.Engagement.OneSignalPerEngagementNotPerHit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementOneSignalPerFightTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	// plenty of health, so no hit here gets anywhere near the wound floor - this case is about
	// ordinary hits and nothing else
	Rig.VictimHealth->MaxHealth = 1000.0f;

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	TestFalse(TEXT("A unit starts clear"), Rig.VictimCombat->IsEngaged());

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestTrue(TEXT("Being targeted engages (the default trigger)"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...and signals once [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);

	for (int32 Swing = 0; Swing < 5; ++Swing)
	{
		// the auto-attack loop calls AttackTarget again before every swing, so a real fight is
		// this pair on repeat
		Rig.AttackerCombat->AttackTarget(Rig.Victim);
		Rig.Hit();
	}

	TestEqual(TEXT("Five hits landed"), Rig.VictimHealth->GetHealth(), 875.0f);
	TestEqual(FString::Printf(TEXT("...and still only the one signal [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);
	TestTrue(TEXT("...which was Entered"), Rig.Signals.Num() > 0 && Rig.Signals[0] == EDangerSignal::Entered);
	TestEqual(TEXT("Every signal named the victim"), Rig.WrongUnitCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementEscalationTest,
	"Smores.Combat.Engagement.EscalationSignalsAgain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementEscalationTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	// 100 health, 25 a hit, and the wound floor at the default half: 75, then 50 (the floor),
	// then 25, then down
	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	Rig.Hit();

	TestEqual(FString::Printf(TEXT("A hit above the floor adds nothing [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);

	Rig.Hit();

	TestEqual(FString::Printf(TEXT("Falling to the floor signals again [%s]"), *Rig.Describe()), Rig.Signals.Num(), 2);
	TestTrue(TEXT("...as Wounded"), Rig.Signals.Num() >= 2 && Rig.Signals[1] == EDangerSignal::Wounded);

	Rig.Hit();

	TestEqual(FString::Printf(TEXT("Staying below the floor is not a new signal [%s]"), *Rig.Describe()), Rig.Signals.Num(), 2);

	Rig.Hit();

	TestTrue(TEXT("The fourth hit knocked the victim down"), Rig.VictimHealth->IsDowned());
	TestEqual(FString::Printf(TEXT("Going down signals again [%s]"), *Rig.Describe()), Rig.Signals.Num(), 3);
	TestTrue(TEXT("...as Down"), Rig.Signals.Num() >= 3 && Rig.Signals[2] == EDangerSignal::Down);

	// dying is going down for good, and still worse news than the knockdown before it
	Rig.VictimHealth->Kill();

	TestEqual(FString::Printf(TEXT("Being killed while engaged signals too [%s]"), *Rig.Describe()), Rig.Signals.Num(), 4);
	TestTrue(TEXT("...as Down"), Rig.Signals.Num() >= 4 && Rig.Signals[3] == EDangerSignal::Down);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementReentryTest,
	"Smores.Combat.Engagement.LeavingAndReenteringSignalsAgain",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementReentryTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	// Attention every six seconds keeps a ten-second timeout from ever running out - twelve seconds
	// after the fight started, it is still the same fight. This is the half of the pair that
	// proves the timeout is measured from the *last* attention, not the first.
	TestTrue(TEXT("The world ticked"), TestWorld.TickFor(6.0f, 0.1f));

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestTrue(TEXT("The world ticked again"), TestWorld.TickFor(6.0f, 0.1f));

	TestTrue(TEXT("Still engaged twelve seconds in, with attention six seconds ago"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...and still the one signal [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);

	// and the other half: with nothing hostile for longer than the timeout, the fight is over
	TestTrue(TEXT("The world ticked out the timeout"), TestWorld.TickFor(11.0f, 0.1f));

	TestFalse(TEXT("No attention for longer than the timeout ends the engagement"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...silently [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestTrue(TEXT("A fresh attack engages again"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...and signals again [%s]"), *Rig.Describe()), Rig.Signals.Num(), 2);
	TestTrue(TEXT("...as Entered"), Rig.Signals.Num() >= 2 && Rig.Signals[1] == EDangerSignal::Entered);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementHitTriggerTest,
	"Smores.Combat.Engagement.HitTriggerIgnoresTargeting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementHitTriggerTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	if (!TestTrue(TEXT("DangerTrigger is still a reflected property"), SmoresEngagementTest_SetDangerTrigger(Rig.VictimCombat, EDangerTrigger::Hit)))
	{
		return true;
	}

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestFalse(TEXT("Under the Hit trigger, being targeted doesn't engage"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...or signal [%s]"), *Rig.Describe()), Rig.Signals.Num(), 0);

	Rig.Hit();

	TestTrue(TEXT("A landed hit does"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...once [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);

	// being targeted can't start a fight under this trigger, but it still counts as the enemy
	// being there - so it keeps one going past the timeout
	TestTrue(TEXT("The world ticked"), TestWorld.TickFor(6.0f, 0.1f));

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestTrue(TEXT("The world ticked again"), TestWorld.TickFor(6.0f, 0.1f));

	TestTrue(TEXT("Targeting alone kept the engagement alive"), Rig.VictimCombat->IsEngaged());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementKnockdownFromClearTest,
	"Smores.Combat.Engagement.KnockdownFromClearSignalsOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementKnockdownFromClearTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	// Under the Hit trigger a unit can be knocked down by the very hit that starts its fight. That
	// is one piece of news, not two - ApplyAttackDamage reports the hit *after* the damage lands,
	// so the knockdown finds the unit still clear and stays quiet, and the hit then enters.
	if (!TestTrue(TEXT("DangerTrigger is still a reflected property"), SmoresEngagementTest_SetDangerTrigger(Rig.VictimCombat, EDangerTrigger::Hit)))
	{
		return true;
	}

	Rig.VictimHealth->MaxHealth = 25.0f;

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	Rig.AttackerCombat->AttackTarget(Rig.Victim);
	Rig.Hit();

	TestTrue(TEXT("One hit knocked the victim down"), Rig.VictimHealth->IsDowned());
	TestEqual(FString::Printf(TEXT("...and signalled once [%s]"), *Rig.Describe()), Rig.Signals.Num(), 1);
	TestTrue(TEXT("...as Entered"), Rig.Signals.Num() > 0 && Rig.Signals[0] == EDangerSignal::Entered);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementAttackerEngagesTest,
	"Smores.Combat.Engagement.AttackingEngagesTheAttacker",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementAttackerEngagesTest::RunTest(const FString& Parameters)
{
	// the attacker's own signals, alongside the rig's record of the victim's. Declared before the
	// world for the rig's reason: the listener captures it, so it has to outlive the world.
	int32 AttackerSignals = 0;

	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	EXPECT_COMBAT_LOG_WARNINGS();

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	Rig.AttackerCombat->OnDangerSignal.AddLambda([&AttackerSignals](AActor*, EDangerSignal) { ++AttackerSignals; });

	// Joining a fight is entering one, whatever the trigger says - DangerTrigger only decides how
	// early being *attacked* counts. Set to Hit here so this case would fail if the trigger ever
	// started gating a unit's own attacks.
	if (!TestTrue(TEXT("DangerTrigger is still a reflected property"), SmoresEngagementTest_SetDangerTrigger(Rig.AttackerCombat, EDangerTrigger::Hit)))
	{
		return true;
	}

	// Well out of range, so AttackTarget takes the "walk over first" branch. This is the case Jim
	// hit in PIE: two squad members sent in on one H press, and only the one the enemy turned on
	// flashed. The second was still walking over, and had not been attacked by anyone.
	AActor* Attacker = Rig.AttackerCombat->GetOwner();
	USceneComponent* AttackerRoot = NewObject<USceneComponent>(Attacker);
	Attacker->SetRootComponent(AttackerRoot);
	AttackerRoot->RegisterComponent();

	TestTrue(TEXT("The attacker moved out of range"), Attacker->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f)));

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	Rig.AttackerCombat->AttackTarget(Rig.Victim);

	TestTrue(TEXT("An attack order engages the attacker before it reaches the target"), Rig.AttackerCombat->IsEngaged());
	TestEqual(TEXT("...and signals it once"), AttackerSignals, 1);

	// arrive and fight: every swing comes back through AttackTarget, and none of it is news
	Attacker->SetActorLocation(FVector::ZeroVector);

	for (int32 Swing = 0; Swing < 3; ++Swing)
	{
		Rig.AttackerCombat->AttackTarget(Rig.Victim);
		Rig.Hit();
	}

	TestEqual(TEXT("Three hits landed"), Rig.VictimHealth->GetHealth(), 25.0f);
	TestEqual(TEXT("The attacker's fight is still the one signal"), AttackerSignals, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEngagementRestoreIsSilentTest,
	"Smores.Combat.Engagement.RestoredDownIsSilent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEngagementRestoreIsSilentTest::RunTest(const FString& Parameters)
{
	FSmoresEngagementRig Rig;
	FSmoresTestWorld TestWorld;

	// no EXPECT_COMBAT_LOG_WARNINGS here: nothing swings, so nothing warns, and an expected
	// message that never arrives fails the test

	if (!TestTrue(TEXT("Rig built"), SmoresEngagementTest_Build(TestWorld, Rig)))
	{
		return true;
	}

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);
		return true;
	}

	// A unit loaded from its character record already down broadcasts OnDowned / OnDied through
	// RestoreState, exactly as a knockdown does. Nothing hostile happened - a save loading must not
	// flash the squad bar.
	TestTrue(TEXT("Restored as Downed"), Rig.VictimHealth->RestoreState(0.0f, EHealthState::Downed));
	TestTrue(TEXT("Restored as Dead"), Rig.VictimHealth->RestoreState(0.0f, EHealthState::Dead));

	TestFalse(TEXT("A restored state doesn't engage"), Rig.VictimCombat->IsEngaged());
	TestEqual(FString::Printf(TEXT("...or signal [%s]"), *Rig.Describe()), Rig.Signals.Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
