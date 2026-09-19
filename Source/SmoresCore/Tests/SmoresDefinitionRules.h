// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "SmoresDefinition.h"

/**
 *  The shared half of the definition content sweeps, in two layers.
 *
 *  **Base rules** (here) apply to every USmoresDefinition in the project, whatever it defines:
 *  it has an id, and it has a name. **Per-type rules** live with the type - a footprint bound is
 *  a thing only an item has - and each type's own test calls ValidateSmoresDefinition first and
 *  then adds its own. Tests/SmoresDefinitionAssetTest.cpp sweeps the base layer across
 *  everything; SmoresItems' ItemDefinitionAssetTest.cpp is the worked example of a per-type one.
 *
 *  These sweeps are a different shape from every other test in the project. Everything else
 *  builds its inputs in memory precisely so a designer retuning an asset can't break it; this
 *  group does the opposite deliberately, because "every definition is well formed" is a rule
 *  about the content tree rather than about any one asset. The failures it catches are silent
 *  ones: a definition with no id is unreachable by id no matter what it is named, and two
 *  sharing an id are worse - something resolves the wrong one and nothing says so.
 *
 *  They need EditorContext and the asset registry, so they do not run in a headless *game*
 *  target. The run command in testing.md uses UnrealEditor-Cmd, which covers them.
 */

/** The rules every definition satisfies regardless of what it defines. Returns false and names the first problem. */
inline bool ValidateSmoresDefinition(const USmoresDefinition* Definition, FString& OutProblem)
{
	if (!Definition)
	{
		OutProblem = TEXT("the asset failed to load as a USmoresDefinition");

		return false;
	}

	// the stable identity everything else resolves through - records, loot tables, recipes and
	// saves all hold this rather than an asset pointer
	if (Definition->DefinitionId.IsNone())
	{
		OutProblem = TEXT("DefinitionId is None");

		return false;
	}

	if (Definition->DisplayName.IsEmpty())
	{
		OutProblem = TEXT("DisplayName is empty");

		return false;
	}

	return true;
}

/** Every asset of DefinitionClass (or a subclass) under /Game, with the registry scan forced to finish first */
inline void GatherDefinitionAssets(const UClass* DefinitionClass, TArray<FAssetData>& OutAssets)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// a headless run can still be scanning when the test starts, and an unfinished scan reports
	// no assets - which would pass this whole group having looked at nothing
	AssetRegistry.SearchAllAssets(/*bSynchronousSearch*/ true);

	FARFilter Filter;
	Filter.ClassPaths.Add(DefinitionClass->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;

	AssetRegistry.GetAssets(Filter, OutAssets);
}

#endif // WITH_DEV_AUTOMATION_TESTS
