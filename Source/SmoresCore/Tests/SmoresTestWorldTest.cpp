// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/SmoresTestWorld.h"

/**
 *  The harness testing itself, and the reason it is worth a test of its own: every other test
 *  in this project assumes that an actor spawned into FSmoresTestWorld holds ROLE_Authority. If
 *  that stopped being true, no test would fail - every authority-gated mutator would quietly
 *  no-op and the whole suite would go green having exercised nothing. This is the one place
 *  that assumption is checked directly, so it breaks here, loudly, instead of everywhere,
 *  silently.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTestWorldAuthorityTest,
	"Smores.Core.TestWorld.SpawnedActorHasAuthority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTestWorldAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	if (!TestTrue(TEXT("The test world was created"), TestWorld.IsValid()))
	{
		return true;
	}

	AActor* Owner = TestWorld.SpawnOwner();

	if (!TestNotNull(TEXT("An owner actor was spawned into the test world"), Owner))
	{
		return true;
	}

	TestTrue(TEXT("An actor spawned into a world with no net driver holds authority"), Owner->HasAuthority());
	TestEqual(TEXT("Its local role is ROLE_Authority"), static_cast<int32>(Owner->GetLocalRole()), static_cast<int32>(ROLE_Authority));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
