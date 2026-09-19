// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ItemDefinition.h"
#include "Tests/SmoresDefinitionRules.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The **per-type** layer of the definition content sweeps, for items. SmoresCore's
 *  Tests/SmoresDefinitionAssetTest.cpp already asserts the rules every definition shares - an id,
 *  a name, no duplicate id within a type, and that the id actually resolves through the Asset
 *  Manager. This file adds only what is true of an item and of nothing else, and is the worked
 *  example for the same file a future faction or character definition will want.
 *
 *  Tests/SmoresDefinitionRules.h explains why these sweeps run against real content rather than
 *  in-memory inputs, and why they need EditorContext.
 *
 *  The *rules themselves* are proved against an in-memory definition rather than by adding a
 *  broken asset to Content/ - see MalformedDefinitionIsRejected. Deliberately malforming real
 *  content to watch a test fail is hand work in the editor that demonstrates nothing the
 *  in-memory case doesn't demonstrate for free. That proof covers the base rules too, since
 *  UItemDefinition is the concrete definition type they can be exercised through.
 */

/** Every rule an item definition has to satisfy, base rules first. Returns false and names the first problem. */
inline bool ValidateItemDefinition(const UItemDefinition* Definition, FString& OutProblem)
{
	if (!ValidateSmoresDefinition(Definition, OutProblem))
	{
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemDefinitionWellFormedTest,
	"Smores.Content.ItemDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(UItemDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason, which is the failure mode
	// testing.md's "check the number, not the colour" rule exists for
	if (!TestTrue(TEXT("At least one UItemDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	TArray<FString> MissingIcons;

	for (const FAssetData& AssetData : Assets)
	{
		const UItemDefinition* Definition = Cast<UItemDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateItemDefinition(Definition, Problem))
		{
			// name the offending asset - a failure saying only "a definition is malformed" costs
			// whoever reads it a manual sweep of the whole folder
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));

			continue;
		}

		if (!Definition->Icon)
		{
			MissingIcons.Add(AssetData.GetSoftObjectPath().ToString());
		}
	}

	// An icon is required of an *item* in a way no shared definition field could ever be, which is
	// the whole reason Icon stayed on this class instead of moving up to USmoresDefinition. It is
	// a warning rather than an error only because no item art has been made yet and every asset
	// would fail; promote it to AddError the moment the first icon is authored.
	if (MissingIcons.Num() > 0)
	{
		AddWarning(FString::Printf(TEXT("%d item definition(s) have no Icon: %s"),
			MissingIcons.Num(), *FString::Join(MissingIcons, TEXT(", "))));
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

	// a freshly constructed definition has no DefinitionId and no DisplayName, which is exactly
	// the shape of a definition somebody created in the editor and hasn't filled in yet
	UItemDefinition* Definition = TestWorld.NewKeptObject<UItemDefinition>();

	if (!TestNotNull(TEXT("Definition created"), Definition))
	{
		return true;
	}

	FString Problem;

	TestFalse(TEXT("A definition with no DefinitionId is rejected"), ValidateItemDefinition(Definition, Problem));
	TestTrue(TEXT("...and the failure names the field"), Problem.Contains(TEXT("DefinitionId")));

	Definition->DefinitionId = FName(TEXT("RuleProof"));

	TestFalse(TEXT("A definition with no DisplayName is rejected"), ValidateItemDefinition(Definition, Problem));
	TestTrue(TEXT("...and the failure names that field instead"), Problem.Contains(TEXT("DisplayName")));

	Definition->DisplayName = FText::FromString(TEXT("Rule Proof"));

	TestTrue(TEXT("A definition with both is accepted"), ValidateItemDefinition(Definition, Problem));

	// the id an item answers to is {ItemDefinition, DefinitionId}; the type half has to match the
	// config entry or nothing resolves - see EveryDefinitionResolvesById
	TestEqual(TEXT("An item's primary asset id is typed ItemDefinition"),
		Definition->GetPrimaryAssetId().ToString(), FString(TEXT("ItemDefinition:RuleProof")));

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
