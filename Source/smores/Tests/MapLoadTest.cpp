// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

/**
 *  The project's two maps still load.
 *
 *  This lives in the `smores` module because the maps belong to the game framework rather than
 *  to any feature module, and it is the cheapest possible guard against the one thing that
 *  actually breaks a map without anyone noticing until they open it: a C++ class rename or a
 *  module move that leaves a placed actor pointing at a class that no longer exists.
 *
 *  It loads the package rather than opening the level. Opening one needs a PIE session and a
 *  human to look at the result, which is the far side of this project's testing boundary;
 *  loading the package exercises every asset reference in it, which is where the breakage is.
 *
 *  The engine's own Project.Blueprints smoke test is the other half of this and needs no code
 *  here - only to be run, per testing.md's "Useful variations" table.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresMapLoadTest,
	"Smores.Content.Maps.LevelsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresMapLoadTest::RunTest(const FString& Parameters)
{
	// LVL_Strategy and Lvl_MainMenu are the only two maps in the project - LevelPrototyping
	// holds scratch levels that are allowed to be broken
	const TCHAR* MapPackageNames[] =
	{
		TEXT("/Game/Variant_Strategy/LVL_Strategy"),
		TEXT("/Game/MainMenu/Lvl_MainMenu")
	};

	for (const TCHAR* PackageName : MapPackageNames)
	{
		UPackage* Package = LoadPackage(nullptr, PackageName, LOAD_None);

		if (!TestNotNull(*FString::Printf(TEXT("%s loads"), PackageName), Package))
		{
			continue;
		}

		// a map package that loaded but holds no world is a package that isn't a map any more
		TestNotNull(
			*FString::Printf(TEXT("%s contains a world"), PackageName),
			UWorld::FindWorldInPackage(Package));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
