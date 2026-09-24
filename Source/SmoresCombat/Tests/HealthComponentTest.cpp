// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HealthComponent.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The Alive / Downed / Dead state machine, and the timer behind Downed.
 *
 *  The load-bearing test here is KillCancelsPendingRecovery. A kill landing on an already-Downed
 *  unit has to cancel the recovery that was in flight, or the timer fires a few seconds later
 *  and stands the corpse back up. That failure is silent, delayed, and reproducible only by
 *  waiting - which makes it invisible in PIE unless someone happens to still be watching the
 *  body fifteen seconds later, and is the single best argument in this project for having tests
 *  at all.
 *
 *  Two things about the harness this group leans on. Health defaults to MaxHealth's default, so
 *  most cases below need no BeginPlay; BeginPlaySeedsFullHealth is the one that changes MaxHealth
 *  and therefore does. And every case that ticks a timer lowers DownedDurationSeconds first -
 *  ticking through the authored 15 second default at 100fps is 1500 iterations for no benefit.
 */

/** A health component on a fresh authoritative owner, with listeners bound to all four delegates */
struct FTestHealth
{
	UHealthComponent* Health = nullptr;

	USmoresTestDelegateListener* Damaged = nullptr;

	USmoresTestDelegateListener* Downed = nullptr;

	USmoresTestDelegateListener* Recovered = nullptr;

	USmoresTestDelegateListener* Died = nullptr;

	bool IsValid() const
	{
		return Health != nullptr && Damaged != nullptr && Downed != nullptr && Recovered != nullptr && Died != nullptr;
	}

	void ResetListeners()
	{
		Damaged->Reset();
		Downed->Reset();
		Recovered->Reset();
		Died->Reset();
	}
};

inline FTestHealth MakeTestHealth(FSmoresTestWorld& TestWorld)
{
	FTestHealth Test;

	Test.Health = TestWorld.SpawnComponent<UHealthComponent>();
	Test.Damaged = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Test.Downed = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Test.Recovered = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Test.Died = TestWorld.NewKeptObject<USmoresTestDelegateListener>();

	if (Test.IsValid())
	{
		// OnDamaged carries the instigator, so it binds the actor-shaped handler; the other three
		// take no parameters
		Test.Health->OnDamaged.AddDynamic(Test.Damaged, &USmoresTestDelegateListener::OnActorChanged);
		Test.Health->OnDowned.AddDynamic(Test.Downed, &USmoresTestDelegateListener::OnChanged);
		Test.Health->OnRecovered.AddDynamic(Test.Recovered, &USmoresTestDelegateListener::OnChanged);
		Test.Health->OnDied.AddDynamic(Test.Died, &USmoresTestDelegateListener::OnChanged);
	}

	return Test;
}

/**
 *  Every hit spawns a floating damage number, and no test sets a class for it to spawn - so the
 *  component warns, every time, by design. Asserting the warning is stronger than suppressing
 *  it and keeps the run green rather than yellow.
 */
#define EXPECT_DAMAGE_NUMBER_WARNING() \
	AddExpectedMessagePlain(TEXT("DamageNumberActorClass unset"), ELogVerbosity::Warning, \
		EAutomationExpectedMessageFlags::Contains, 0)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthBeginPlayTest,
	"Smores.Combat.Health.BeginPlaySeedsFullHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthBeginPlayTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	Test.Health->MaxHealth = 50.0f;

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	TestEqual(TEXT("Health starts at the authored maximum"), Test.Health->GetHealth(), 50.0f);
	TestTrue(TEXT("...and the owner starts on its feet"), Test.Health->GetHealthState() == EHealthState::Alive);
	TestFalse(TEXT("...and is not incapacitated"), Test.Health->IsIncapacitated());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthDamageTest,
	"Smores.Combat.Health.DamageReducesHealthAndBroadcasts",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthDamageTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	AActor* Attacker = TestWorld.SpawnOwner();

	Test.Health->TakeDamage(30.0f, Attacker);

	TestEqual(TEXT("Damage comes off the health total"), Test.Health->GetHealth(), 70.0f);
	TestTrue(TEXT("...and a survivable hit leaves the owner on its feet"), Test.Health->GetHealthState() == EHealthState::Alive);
	TestEqual(TEXT("...broadcasting the hit once"), Test.Damaged->CallCount, 1);

	// the instigator is what auto-retaliation reads, so it has to survive the trip
	TestTrue(TEXT("...naming whoever dealt it"), Test.Damaged->LastActor.Get() == Attacker);
	TestEqual(TEXT("...and announcing no down"), Test.Downed->CallCount, 0);
	TestEqual(TEXT("...and no death"), Test.Died->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthNonPositiveDamageTest,
	"Smores.Combat.Health.NonPositiveDamageChangesNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthNonPositiveDamageTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	const float StartingHealth = Test.Health->GetHealth();

	Test.Health->TakeDamage(0.0f);
	Test.Health->TakeDamage(-25.0f);

	// a negative hit healing the target is the failure this rules out
	TestEqual(TEXT("A hit for nothing or less leaves health where it was"), Test.Health->GetHealth(), StartingHealth);
	TestTrue(TEXT("...and the state where it was"), Test.Health->GetHealthState() == EHealthState::Alive);
	TestEqual(TEXT("...and broadcasts nothing"), Test.Damaged->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthLethalDamageTest,
	"Smores.Combat.Health.LethalDamageGoesDownedNotDead",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthLethalDamageTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	// past zero rather than exactly to it, because health must clamp rather than go negative
	Test.Health->TakeDamage(150.0f);

	TestEqual(TEXT("Health clamps at zero rather than going negative"), Test.Health->GetHealth(), 0.0f);
	TestTrue(TEXT("A lethal hit goes Downed"), Test.Health->GetHealthState() == EHealthState::Downed);
	TestEqual(TEXT("...announcing the down once"), Test.Downed->CallCount, 1);

	// damage is deliberately not wired to kill - what should actually kill a unit is a design
	// decision that calls Kill() itself
	TestEqual(TEXT("...and never announcing a death"), Test.Died->CallCount, 0);

	// the retaliation broadcast is skipped on a hit that downs, since Downed() clears the
	// target's attack state again immediately
	TestEqual(TEXT("...and not announcing the hit for retaliation"), Test.Damaged->CallCount, 0);

	TestTrue(TEXT("A Downed owner is incapacitated"), Test.Health->IsIncapacitated());
	TestTrue(TEXT("...and reports itself Downed"), Test.Health->IsDowned());
	TestFalse(TEXT("...but not dead"), Test.Health->IsDead());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthIncapacitatedDamageTest,
	"Smores.Combat.Health.DamageWhileIncapacitatedIsIgnored",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthIncapacitatedDamageTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();
	AddExpectedMessagePlain(TEXT("died"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	// no ticking here, so the recovery timer never gets a chance to run and the owner stays
	// Downed for the whole test
	Test.Health->TakeDamage(100.0f);
	Test.ResetListeners();

	Test.Health->TakeDamage(10.0f);

	TestTrue(TEXT("A Downed owner takes no further damage"), Test.Health->GetHealthState() == EHealthState::Downed);
	TestEqual(TEXT("...its health stays at zero"), Test.Health->GetHealth(), 0.0f);
	TestEqual(TEXT("...and nothing is broadcast"), Test.Damaged->CallCount + Test.Downed->CallCount, 0);

	Test.Health->Kill();
	Test.ResetListeners();

	Test.Health->TakeDamage(10.0f);

	TestTrue(TEXT("A Dead owner takes no further damage either"), Test.Health->GetHealthState() == EHealthState::Dead);
	TestEqual(TEXT("...and nothing is broadcast"), Test.Damaged->CallCount + Test.Died->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthKillTest,
	"Smores.Combat.Health.KillFromAliveGoesDeadOnce",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthKillTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	AddExpectedMessagePlain(TEXT("died"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	Test.Health->Kill();

	TestTrue(TEXT("A kill from Alive goes straight to Dead"), Test.Health->GetHealthState() == EHealthState::Dead);
	TestEqual(TEXT("...taking health to zero"), Test.Health->GetHealth(), 0.0f);
	TestEqual(TEXT("...and announcing the death once"), Test.Died->CallCount, 1);
	TestEqual(TEXT("...without ever passing through Downed"), Test.Downed->CallCount, 0);

	Test.Health->Kill();

	// Dead is terminal, so a second kill is a no-op rather than a second funeral
	TestEqual(TEXT("Killing an already-dead owner announces nothing further"), Test.Died->CallCount, 1);
	TestTrue(TEXT("...and leaves it Dead"), Test.Health->GetHealthState() == EHealthState::Dead);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthKillCancelsRecoveryTest,
	"Smores.Combat.Health.KillCancelsPendingRecovery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthKillCancelsRecoveryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();
	AddExpectedMessagePlain(TEXT("died"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	// timers only advance in a world that has begun play - see FSmoresTestWorld::Tick
	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	// the authored default is 15 seconds; a fifth of a second proves the same thing in 20 ticks
	Test.Health->DownedDurationSeconds = 0.2f;

	Test.Health->TakeDamage(100.0f);

	if (!TestTrue(TEXT("The owner is Downed with a recovery in flight"), Test.Health->IsDowned()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	Test.Health->Kill();
	Test.ResetListeners();

	TestTrue(TEXT("A kill on a Downed owner goes Dead"), Test.Health->GetHealthState() == EHealthState::Dead);

	// this is the whole point: wait out the recovery that was already scheduled
	TestTrue(TEXT("The world ticked past the recovery that was pending"), TestWorld.TickFor(1.0f));

	TestTrue(TEXT("...and the corpse is still Dead well past the recovery it had pending"), Test.Health->GetHealthState() == EHealthState::Dead);
	TestEqual(TEXT("...with health still at zero"), Test.Health->GetHealth(), 0.0f);
	TestEqual(TEXT("...and no recovery ever announced"), Test.Recovered->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthRecoveryTest,
	"Smores.Combat.Health.RecoveryTimerRestoresFullHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthRecoveryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	// timers only advance in a world that has begun play - see FSmoresTestWorld::Tick
	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	Test.Health->DownedDurationSeconds = 0.2f;

	Test.Health->TakeDamage(100.0f);

	TestTrue(TEXT("The owner is Downed"), Test.Health->IsDowned());

	// short of the duration, so the timer has genuinely not fired yet
	TestTrue(TEXT("The world ticked"), TestWorld.TickFor(0.1f));

	TestTrue(TEXT("...and stays Downed until the duration is up"), Test.Health->IsDowned());
	TestEqual(TEXT("...with nothing announced yet"), Test.Recovered->CallCount, 0);

	TestTrue(TEXT("The world ticked past the recovery duration"), TestWorld.TickFor(0.3f));

	TestTrue(TEXT("Once the timer fires the owner is back on its feet"), Test.Health->GetHealthState() == EHealthState::Alive);
	TestEqual(TEXT("...at full health rather than the zero it went down at"), Test.Health->GetHealth(), Test.Health->MaxHealth);
	TestEqual(TEXT("...announcing the recovery once"), Test.Recovered->CallCount, 1);
	TestEqual(TEXT("...and never announcing a death"), Test.Died->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthIncapacitatedQueryTest,
	"Smores.Combat.Health.IncapacitatedCoversDownedAndDead",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthIncapacitatedQueryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_DAMAGE_NUMBER_WARNING();
	AddExpectedMessagePlain(TEXT("died"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	const FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	Test.Health->DownedDurationSeconds = 60.0f;

	// IsIncapacitated is the check nearly every gameplay rule actually wants, so all three
	// states are asserted against all three queries
	TestFalse(TEXT("An Alive owner is not incapacitated"), Test.Health->IsIncapacitated());
	TestFalse(TEXT("...not Downed"), Test.Health->IsDowned());
	TestFalse(TEXT("...and not Dead"), Test.Health->IsDead());

	Test.Health->TakeDamage(100.0f);

	TestTrue(TEXT("A Downed owner is incapacitated"), Test.Health->IsIncapacitated());
	TestTrue(TEXT("...and Downed"), Test.Health->IsDowned());
	TestFalse(TEXT("...and not Dead"), Test.Health->IsDead());

	Test.Health->Kill();

	TestTrue(TEXT("A Dead owner is incapacitated"), Test.Health->IsIncapacitated());
	TestFalse(TEXT("...and no longer reports itself Downed"), Test.Health->IsDowned());
	TestTrue(TEXT("...and is Dead"), Test.Health->IsDead());

	TestWorld.ForwardErrors(this);

	return true;
}

/**
 *  RestoreState is how a character record hands its health back to the actor standing in for it,
 *  so it has to behave like the transition it stands in for: broadcast the state it enters, never
 *  a damage event, and leave the recovery timer in the state a real knockdown would. The Dead case
 *  is the one that matters - a unit restored as Dead that later stood up would be the reloaded
 *  save standing the boss back up.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresHealthRestoreStateTest,
	"Smores.Combat.Health.RestoreStateBroadcastsAndRearmsTimer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresHealthRestoreStateTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	FTestHealth Test = MakeTestHealth(TestWorld);

	if (!TestTrue(TEXT("Health component and listeners created"), Test.IsValid()))
	{
		return true;
	}

	Test.Health->DownedDurationSeconds = 0.2f;

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	TestTrue(TEXT("Restoring a wounded Alive state is accepted"), Test.Health->RestoreState(40.0f, EHealthState::Alive));
	TestEqual(TEXT("...and sets the health"), Test.Health->GetHealth(), 40.0f);
	TestEqual(TEXT("...without a damage broadcast - nothing was hit"), Test.Damaged->CallCount, 0);
	TestEqual(TEXT("...or a recovery broadcast - the state didn't change"), Test.Recovered->CallCount, 0);

	Test.Health->RestoreState(500.0f, EHealthState::Alive);

	TestEqual(TEXT("Restored health is clamped to MaxHealth"), Test.Health->GetHealth(), Test.Health->MaxHealth);

	// Downed: broadcasts once, and gets back up after the full duration like a real knockdown
	Test.Health->RestoreState(80.0f, EHealthState::Downed);

	TestTrue(TEXT("Restoring Downed downs it"), Test.Health->IsDowned());
	TestEqual(TEXT("...at zero health whatever was passed"), Test.Health->GetHealth(), 0.0f);
	TestEqual(TEXT("...broadcasting OnDowned once"), Test.Downed->CallCount, 1);
	TestTrue(TEXT("The world ticked past the recovery"), TestWorld.TickFor(0.3f));
	TestFalse(TEXT("...and a restored knockdown recovers on its own"), Test.Health->IsIncapacitated());
	TestEqual(TEXT("...broadcasting OnRecovered"), Test.Recovered->CallCount, 1);

	// Dead restored over a pending recovery: the recovery must not fire
	Test.ResetListeners();
	Test.Health->RestoreState(0.0f, EHealthState::Downed);
	Test.Health->RestoreState(0.0f, EHealthState::Dead);

	TestTrue(TEXT("Restoring Dead kills it"), Test.Health->IsDead());
	TestEqual(TEXT("...broadcasting OnDied"), Test.Died->CallCount, 1);
	TestTrue(TEXT("The world ticked past where the recovery would have fired"), TestWorld.TickFor(0.3f));
	TestTrue(TEXT("...and it is still Dead - the pending recovery was cancelled"), Test.Health->IsDead());
	TestEqual(TEXT("...with no recovery broadcast"), Test.Recovered->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

#undef EXPECT_DAMAGE_NUMBER_WARNING

#endif // WITH_DEV_AUTOMATION_TESTS
