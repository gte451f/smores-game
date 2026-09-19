// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresDefinitionLibrary.h"
#include "Tests/SmoresDefinitionRules.h"

/**
 *  The base layer of the definition content sweeps - see Tests/SmoresDefinitionRules.h for the
 *  two-layer split and why these tests look unlike everything else in the suite.
 *
 *  Every definition type the project ever adds is covered by this file the moment its first
 *  asset exists, with nothing here to update. The per-type rules that go beyond "has an id, has
 *  a name" belong next to the type, as SmoresItems' ItemDefinitionAssetTest.cpp does for items.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDefinitionWellFormedTest,
	"Smores.Content.Definitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(USmoresDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason, which is the failure mode
	// testing.md's "check the number, not the colour" rule exists for
	if (!TestTrue(TEXT("At least one USmoresDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	for (const FAssetData& AssetData : Assets)
	{
		const USmoresDefinition* Definition = Cast<USmoresDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateSmoresDefinition(Definition, Problem))
		{
			// name the offending asset - a failure saying only "a definition is malformed" costs
			// whoever reads it a manual sweep of the whole content tree
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDefinitionUniqueIdTest,
	"Smores.Content.Definitions.DefinitionIdsAreUniqueWithinType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDefinitionUniqueIdTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(USmoresDefinition::StaticClass(), Assets);

	if (!TestTrue(TEXT("At least one USmoresDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	// keyed by the *whole* FPrimaryAssetId, because an id only has to be unique within its type -
	// an item and a faction may both legitimately call themselves "Ironclan"
	TMap<FPrimaryAssetId, FString> SeenIds;

	for (const FAssetData& AssetData : Assets)
	{
		const USmoresDefinition* Definition = Cast<USmoresDefinition>(AssetData.GetAsset());

		if (!Definition || Definition->DefinitionId.IsNone())
		{
			// EveryAssetIsWellFormed owns that failure; reporting it twice helps nobody
			continue;
		}

		const FPrimaryAssetId AssetId(Definition->GetDefinitionType(), Definition->DefinitionId);
		const FString AssetPath = AssetData.GetSoftObjectPath().ToString();

		if (const FString* ExistingPath = SeenIds.Find(AssetId))
		{
			AddError(FString::Printf(TEXT("%s is used by both %s and %s - something will resolve the wrong one"),
				*AssetId.ToString(), **ExistingPath, *AssetPath));

			continue;
		}

		SeenIds.Add(AssetId, AssetPath);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDefinitionLookupTest,
	"Smores.Content.Definitions.EveryDefinitionResolvesById",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDefinitionLookupTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(USmoresDefinition::StaticClass(), Assets);

	if (!TestTrue(TEXT("At least one USmoresDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	// The asset registry finding a definition and the *Asset Manager* being able to hand it back
	// by id are two different things: the second needs a PrimaryAssetTypesToScan entry in
	// Config/DefaultGame.ini whose PrimaryAssetType matches the type's GetDefinitionType().
	// Forgetting that entry for a new definition type breaks every record, loot table and save
	// that names one of its assets, and breaks nothing at all at compile time - so this is the
	// test that fails when a later slice adds a type and not the config line.
	for (const FAssetData& AssetData : Assets)
	{
		const USmoresDefinition* Definition = Cast<USmoresDefinition>(AssetData.GetAsset());

		if (!Definition || Definition->DefinitionId.IsNone())
		{
			continue;
		}

		const USmoresDefinition* Resolved = USmoresDefinitionLibrary::FindDefinition(
			Definition->GetDefinitionType(), Definition->DefinitionId);

		if (!Resolved)
		{
			AddError(FString::Printf(
				TEXT("%s does not resolve by id - is '%s' registered under PrimaryAssetTypesToScan in Config/DefaultGame.ini?"),
				*AssetData.GetSoftObjectPath().ToString(), *Definition->GetDefinitionType().ToString()));

			continue;
		}

		if (Resolved != Definition)
		{
			AddError(FString::Printf(TEXT("%s resolves by id to a different asset, %s"),
				*AssetData.GetSoftObjectPath().ToString(), *Resolved->GetPathName()));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDefinitionTypesRegisteredTest,
	"Smores.Content.Definitions.EveryAssetIsEnumerableByType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresDefinitionTypesRegisteredTest::RunTest(const FString& Parameters)
{
	TArray<FPrimaryAssetType> DefinitionTypes;
	USmoresDefinitionLibrary::GetDefinitionTypes(DefinitionTypes);

	// EveryDefinitionResolvesById passes vacuously if the registry sweep finds no assets at all;
	// this asserts the config side directly, so "nobody registered anything" can't hide
	if (!TestTrue(TEXT("At least one USmoresDefinition type is registered with the Asset Manager"), DefinitionTypes.Num() > 0))
	{
		return true;
	}

	// Every id the Asset Manager can *enumerate*, per type. Resolving a known id and being able to
	// list the ids are different questions, and the second is the one a config entry with the wrong
	// Directories path gets wrong: register ItemDefinition against /Game/Items, author the ninth
	// item under /Game/Mods, and it resolves by name while never appearing in any listing.
	TMap<FPrimaryAssetType, TSet<FName>> EnumeratedIds;

	for (const FPrimaryAssetType& DefinitionType : DefinitionTypes)
	{
		TArray<FName> DefinitionIds;
		USmoresDefinitionLibrary::GetDefinitionIds(DefinitionType, DefinitionIds);

		EnumeratedIds.Add(DefinitionType, TSet<FName>(DefinitionIds));
	}

	TArray<FAssetData> Assets;
	GatherDefinitionAssets(USmoresDefinition::StaticClass(), Assets);

	if (!TestTrue(TEXT("At least one USmoresDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	for (const FAssetData& AssetData : Assets)
	{
		const USmoresDefinition* Definition = Cast<USmoresDefinition>(AssetData.GetAsset());

		if (!Definition || Definition->DefinitionId.IsNone())
		{
			// EveryAssetIsWellFormed owns that failure
			continue;
		}

		const FPrimaryAssetType DefinitionType = Definition->GetDefinitionType();
		const TSet<FName>* TypeIds = EnumeratedIds.Find(DefinitionType);

		if (!TypeIds)
		{
			AddError(FString::Printf(TEXT("%s reports type '%s', which no PrimaryAssetTypesToScan entry registers"),
				*AssetData.GetSoftObjectPath().ToString(), *DefinitionType.ToString()));

			continue;
		}

		if (!TypeIds->Contains(Definition->DefinitionId))
		{
			AddError(FString::Printf(
				TEXT("%s is not listed under '%s' - does that type's PrimaryAssetTypesToScan entry cover this asset's folder?"),
				*AssetData.GetSoftObjectPath().ToString(), *DefinitionType.ToString()));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
