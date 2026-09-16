// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "ItemDefinition.h"
#include "Modules/ModuleManager.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  A smoke test over the real UItemDefinition assets under Content/, and a different shape from
 *  every other test in this project: everything else builds its inputs in memory precisely so a
 *  designer retuning an asset can't break it. This group does the opposite on purpose - it is
 *  not asserting what a sword weighs, it is asserting that every definition is *well formed*,
 *  which is a rule about the content tree rather than about any one asset.
 *
 *  The two failures it catches are both silent. A definition with no ItemId resolves to nothing
 *  and its items quietly can't stack or be looked up; two definitions sharing an ItemId are
 *  worse, because something will resolve the wrong one and nothing will say so.
 *
 *  The *rule itself* is proved against an in-memory definition rather than by adding a broken
 *  asset to Content/ - see MalformedDefinitionIsRejected. Deliberately malforming real content
 *  to watch a test fail is hand work in the editor that demonstrates nothing the in-memory case
 *  doesn't demonstrate for free.
 *
 *  These need EditorContext and the asset registry, so they will not run in a headless *game*
 *  target. The run command in testing.md uses UnrealEditor-Cmd, which covers them.
 */

/** Every rule a definition asset has to satisfy. Returns false and names the first problem. */
inline bool ValidateItemDefinition(const UItemDefinition* Definition, FString& OutProblem)
{
	if (!Definition)
	{
		OutProblem = TEXT("the asset failed to load as a UItemDefinition");

		return false;
	}

	// the stable identity everything else resolves through - an asset without one is unreachable
	// by id no matter what it is named
	if (Definition->ItemId.IsNone())
	{
		OutProblem = TEXT("ItemId is None");

		return false;
	}

	if (Definition->DisplayName.IsEmpty())
	{
		OutProblem = TEXT("DisplayName is empty");

		return false;
	}

	// the editor clamps these, but a value authored before a clamp existed, or set from code,
	// isn't re-clamped on load
	if (Definition->FootprintWidth < 1 || Definition->FootprintWidth > 16)
	{
		OutProblem = FString::Printf(TEXT("FootprintWidth %d is outside 1-16"), Definition->FootprintWidth);

		return false;
	}

	if (Definition->FootprintHeight < 1 || Definition->FootprintHeight > 16)
	{
		OutProblem = FString::Printf(TEXT("FootprintHeight %d is outside 1-16"), Definition->FootprintHeight);

		return false;
	}

	if (Definition->MaxStackSize < 1)
	{
		OutProblem = FString::Printf(TEXT("MaxStackSize %d is below 1"), Definition->MaxStackSize);

		return false;
	}

	return true;
}

/** Every UItemDefinition asset under /Game, with the registry scan forced to finish first */
inline void GatherItemDefinitionAssets(TArray<FAssetData>& OutAssets)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// a headless run can still be scanning when the test starts, and an unfinished scan reports
	// no assets - which would pass this whole group having looked at nothing
	AssetRegistry.SearchAllAssets(/*bSynchronousSearch*/ true);

	FARFilter Filter;
	Filter.ClassPaths.Add(UItemDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;

	AssetRegistry.GetAssets(Filter, OutAssets);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemDefinitionWellFormedTest,
	"Smores.Content.ItemDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherItemDefinitionAssets(Assets);

	// a sweep that found nothing is green for the wrong reason, which is the failure mode
	// testing.md's "check the number, not the colour" rule exists for
	if (!TestTrue(TEXT("At least one UItemDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	for (const FAssetData& AssetData : Assets)
	{
		const UItemDefinition* Definition = Cast<UItemDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateItemDefinition(Definition, Problem))
		{
			// name the offending asset - a failure saying only "a definition is malformed" costs
			// whoever reads it a manual sweep of the whole folder
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemDefinitionUniqueIdTest,
	"Smores.Content.ItemDefinitions.ItemIdsAreUnique",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemDefinitionUniqueIdTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherItemDefinitionAssets(Assets);

	if (!TestTrue(TEXT("At least one UItemDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	TMap<FName, FString> SeenIds;

	for (const FAssetData& AssetData : Assets)
	{
		const UItemDefinition* Definition = Cast<UItemDefinition>(AssetData.GetAsset());

		if (!Definition || Definition->ItemId.IsNone())
		{
			// EveryAssetIsWellFormed owns that failure; reporting it twice helps nobody
			continue;
		}

		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();

		if (const FString* ExistingPath = SeenIds.Find(Definition->ItemId))
		{
			AddError(FString::Printf(TEXT("ItemId '%s' is used by both %s and %s - something will resolve the wrong one"),
				*Definition->ItemId.ToString(), **ExistingPath, *AssetPath));

			continue;
		}

		SeenIds.Add(Definition->ItemId, AssetPath);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemDefinitionRuleTest,
	"Smores.Content.ItemDefinitions.MalformedDefinitionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemDefinitionRuleTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// a freshly constructed definition has no ItemId and no DisplayName, which is exactly the
	// shape of a definition somebody created in the editor and hasn't filled in yet
	UItemDefinition* Definition = TestWorld.NewKeptObject<UItemDefinition>();

	if (!TestNotNull(TEXT("Definition created"), Definition))
	{
		return true;
	}

	FString Problem;

	TestFalse(TEXT("A definition with no ItemId is rejected"), ValidateItemDefinition(Definition, Problem));
	TestTrue(TEXT("...and the failure names the field"), Problem.Contains(TEXT("ItemId")));

	Definition->ItemId = FName(TEXT("RuleProof"));

	TestFalse(TEXT("A definition with no DisplayName is rejected"), ValidateItemDefinition(Definition, Problem));
	TestTrue(TEXT("...and the failure names that field instead"), Problem.Contains(TEXT("DisplayName")));

	Definition->DisplayName = FText::FromString(TEXT("Rule Proof"));

	TestTrue(TEXT("A definition with both is accepted"), ValidateItemDefinition(Definition, Problem));

	Definition->FootprintWidth = 0;

	TestFalse(TEXT("A footprint below the grid clamp is rejected"), ValidateItemDefinition(Definition, Problem));

	Definition->FootprintWidth = 17;

	TestFalse(TEXT("...and so is one above it"), ValidateItemDefinition(Definition, Problem));

	Definition->FootprintWidth = 2;
	Definition->MaxStackSize = 0;

	// a cap of zero means nothing can ever be placed, which reads as an empty grid rather than
	// as a broken asset
	TestFalse(TEXT("A stack cap below one is rejected"), ValidateItemDefinition(Definition, Problem));

	TestFalse(TEXT("A null definition is rejected rather than crashing the sweep"), ValidateItemDefinition(nullptr, Problem));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
