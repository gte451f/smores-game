// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GamePace.h"
#include "TimePaceComponent.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  A tier's name, so a failed assertion says "Octuple, expected Quadruple" rather than comparing
 *  two integers nobody can read.
 *
 *  A uniquely-prefixed static rather than an anonymous-namespace helper: UE's unity builds
 *  concatenate several test .cpp files into one translation unit, where two files' anonymous
 *  namespaces merge and a shared helper name becomes a redefinition. See testing.md.
 */
static FString SmoresTimePaceTest_PaceName(EGamePace Pace)
{
	return StaticEnum<EGamePace>()->GetNameStringByValue(static_cast<int64>(Pace));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceLadderMatchesEnumTest,
	"Smores.Core.TimePace.LadderMatchesEnum",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceLadderMatchesEnumTest::RunTest(const FString& Parameters)
{
	const TArray<EGamePace>& Ladder = UTimePaceComponent::GetPaceLadder();

	const UEnum* PaceEnum = StaticEnum<EGamePace>();

	if (!TestNotNull(TEXT("EGamePace is a reflected enum"), PaceEnum))
	{
		return true;
	}

	// NumEnums() counts the hidden _MAX entry UHT appends, which is not a real tier
	const int32 TierCount = PaceEnum->NumEnums() - 1;

	// The ladder is EGamePace's declaration order written out by hand, and the two drifting apart
	// is the one way this can be wrong without anything failing to compile: a tier missing from
	// the ladder is simply unreachable with `-` and `=`, and nothing says so.
	TestEqual(TEXT("The ladder holds every tier the enum declares"), Ladder.Num(), TierCount);

	for (int32 Index = 0; Index < TierCount; ++Index)
	{
		const EGamePace Expected = static_cast<EGamePace>(PaceEnum->GetValueByIndex(Index));

		if (Ladder.IsValidIndex(Index))
		{
			TestEqual(*FString::Printf(TEXT("Ladder rung %d is the tier declared at that position"), Index),
				SmoresTimePaceTest_PaceName(Ladder[Index]), SmoresTimePaceTest_PaceName(Expected));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceDilationMatchesTierTest,
	"Smores.Core.TimePace.DilationMatchesTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceDilationMatchesTierTest::RunTest(const FString& Parameters)
{
	// the readout says "2x", so the world had better run at 2x - these are the numbers the label
	// is promising, asserted rather than assumed
	TestEqual(TEXT("1/3x"), UTimePaceComponent::GetDilationForPace(EGamePace::Third), 1.0f / 3.0f, 0.0001f);
	TestEqual(TEXT("1/2x"), UTimePaceComponent::GetDilationForPace(EGamePace::Half), 0.5f);
	TestEqual(TEXT("3/4x"), UTimePaceComponent::GetDilationForPace(EGamePace::ThreeQuarters), 0.75f);
	TestEqual(TEXT("1x"), UTimePaceComponent::GetDilationForPace(EGamePace::Normal), 1.0f);
	TestEqual(TEXT("2x"), UTimePaceComponent::GetDilationForPace(EGamePace::Double), 2.0f);
	TestEqual(TEXT("4x"), UTimePaceComponent::GetDilationForPace(EGamePace::Quadruple), 4.0f);
	TestEqual(TEXT("8x"), UTimePaceComponent::GetDilationForPace(EGamePace::Octuple), 8.0f);

	// Paused is deliberately not 0 - AWorldSettings clamps to MinGlobalTimeDilation, which is
	// 0.0001 by default, so passing 0 would silently become this anyway. Asserting it stays above
	// zero is asserting that the value the label claims is the value the world can actually hold.
	const float PausedDilation = UTimePaceComponent::GetDilationForPace(EGamePace::Paused);

	TestTrue(TEXT("Paused is above zero, so AWorldSettings won't clamp it to something else"), PausedDilation > 0.0f);
	TestTrue(TEXT("...and slow enough to be frozen by any standard a player can perceive"), PausedDilation <= 0.001f);

	// every tier on the ladder runs slower than the one above it - the whole reason the ladder is
	// ordinal arithmetic rather than a lookup
	const TArray<EGamePace>& Ladder = UTimePaceComponent::GetPaceLadder();

	for (int32 Index = 1; Index < Ladder.Num(); ++Index)
	{
		TestTrue(*FString::Printf(TEXT("Ladder rung %d is faster than the one below it"), Index),
			UTimePaceComponent::GetDilationForPace(Ladder[Index]) > UTimePaceComponent::GetDilationForPace(Ladder[Index - 1]));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceSteppingMovesOneTierTest,
	"Smores.Core.TimePace.SteppingMovesOneTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceSteppingMovesOneTierTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A step faster from 1x is 2x"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Normal, 1)), SmoresTimePaceTest_PaceName(EGamePace::Double));

	TestEqual(TEXT("A step slower from 1x is 3/4x"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Normal, -1)), SmoresTimePaceTest_PaceName(EGamePace::ThreeQuarters));

	TestEqual(TEXT("Two steps faster from 1x is 4x"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Normal, 2)), SmoresTimePaceTest_PaceName(EGamePace::Quadruple));

	TestEqual(TEXT("A step of zero stays put"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Half, 0)), SmoresTimePaceTest_PaceName(EGamePace::Half));

	// stepping out of pause is the ordinary case, not a special one - `=` while frozen moves to
	// the slowest running tier rather than needing the pause key
	TestEqual(TEXT("A step faster from paused is the slowest running tier"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Paused, 1)), SmoresTimePaceTest_PaceName(EGamePace::Third));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceSteppingClampsTest,
	"Smores.Core.TimePace.SteppingClampsAtBothEnds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceSteppingClampsTest::RunTest(const FString& Parameters)
{
	// clamping rather than wrapping, and this is the assertion that says so: `=` held down at the
	// top of the ladder must not drop the player back to paused, which is the shape this bug would
	// take and would be maddening to reproduce on purpose
	TestEqual(TEXT("A step faster from the top stays at the top"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Octuple, 1)), SmoresTimePaceTest_PaceName(EGamePace::Octuple));

	TestEqual(TEXT("A step slower from the bottom stays paused"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Paused, -1)), SmoresTimePaceTest_PaceName(EGamePace::Paused));

	TestEqual(TEXT("A wild step up lands on the top rung"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Normal, 100)), SmoresTimePaceTest_PaceName(EGamePace::Octuple));

	TestEqual(TEXT("A wild step down lands on paused"),
		SmoresTimePaceTest_PaceName(UTimePaceComponent::StepPace(EGamePace::Normal, -100)), SmoresTimePaceTest_PaceName(EGamePace::Paused));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceSetPaceRemembersResumeTest,
	"Smores.Core.TimePace.SetPaceRemembersResumeTier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceSetPaceRemembersResumeTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTimePaceComponent* TimePace = TestWorld.SpawnComponent<UTimePaceComponent>();

	if (!TestNotNull(TEXT("Pace component created"), TimePace))
	{
		return true;
	}

	TestTrue(TEXT("The world starts at real time"), TimePace->GetPace() == EGamePace::Normal);
	TestFalse(TEXT("...and not paused"), TimePace->IsPaused());

	TimePace->SetPace(EGamePace::Quadruple);

	TestTrue(TEXT("Setting a tier takes"), TimePace->GetPace() == EGamePace::Quadruple);

	TimePace->SetPace(EGamePace::Paused);

	TestTrue(TEXT("Pausing pauses"), TimePace->IsPaused());
	TestTrue(TEXT("...and remembers the speed it was running at"), TimePace->GetResumePace() == EGamePace::Quadruple);

	TimePace->SetPace(TimePace->GetResumePace());

	TestTrue(TEXT("Unpausing comes back to that same speed"), TimePace->GetPace() == EGamePace::Quadruple);

	// pausing twice in a row must not overwrite the remembered tier with "Paused", which would
	// leave the player stuck frozen with nothing to come back to
	TimePace->SetPace(EGamePace::Paused);
	TimePace->SetPace(EGamePace::Paused);

	TestTrue(TEXT("Pausing twice still remembers the running speed"), TimePace->GetResumePace() == EGamePace::Quadruple);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTimePaceWithoutAuthorityIsSilentTest,
	"Smores.Core.TimePace.SetPaceWithoutAuthorityIsSilent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTimePaceWithoutAuthorityIsSilentTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// Deliberately ownerless, the same sanctioned exception Smores.Items.Inventory.
	// SortWithoutAuthorityIsSilent uses: with no owner there is no authority, so this exercises
	// the gate's refusing branch. It is not a real client - that needs a net driver and waits on
	// multiplayer - but it does prove the gate refuses rather than merely that it exists.
	UTimePaceComponent* TimePace = TestWorld.NewKeptObject<UTimePaceComponent>();

	if (!TestNotNull(TEXT("Pace component created"), TimePace))
	{
		return true;
	}

	TimePace->SetPace(EGamePace::Octuple);

	TestTrue(TEXT("A non-authority SetPace changes nothing at all"), TimePace->GetPace() == EGamePace::Normal);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
