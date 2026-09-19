// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ItemModifierDefinition.h"
#include "Tests/SmoresDefinitionRules.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The **per-type** layer of the definition content sweeps, for item modifiers. SmoresCore's
 *  Tests/SmoresDefinitionAssetTest.cpp already asserts the rules every definition shares - an id,
 *  a name, no duplicate id within a type, and that the id actually resolves through the Asset
 *  Manager - and picked these assets up the moment the type existed. This file adds only what is
 *  true of a modifier.
 *
 *  It exists because a modifier's numbers are multipliers, and a multiplier has a failure mode a
 *  weight or a price doesn't: an unfilled field defaults to 1.0 and is invisible, but a field
 *  authored to 0 silently erases whatever it multiplies. A masterwork spear weighing nothing and
 *  costing nothing reads as a bug in the inventory rather than in the asset.
 *
 *  See ItemDefinitionAssetTest.cpp for why these sweeps run against real content and why the
 *  rules themselves are proved against an in-memory definition instead.
 */

/** Every rule a modifier definition has to satisfy, base rules first. Returns false and names the first problem. */
inline bool ValidateItemModifierDefinition(const UItemModifierDefinition* Modifier, FString& OutProblem)
{
	if (!ValidateSmoresDefinition(Modifier, OutProblem))
	{
		return false;
	}

	// zero is the dangerous value here, not a negative one: it multiplies an item's weight or
	// price away to nothing while looking like a deliberately cheap material
	if (Modifier->WeightMultiplier <= 0.0f)
	{
		OutProblem = FString::Printf(TEXT("WeightMultiplier %.3f is not above zero"), Modifier->WeightMultiplier);

		return false;
	}

	if (Modifier->ValueMultiplier <= 0.0f)
	{
		OutProblem = FString::Printf(TEXT("ValueMultiplier %.3f is not above zero"), Modifier->ValueMultiplier);

		return false;
	}

	if (Modifier->ConditionMultiplier <= 0.0f)
	{
		OutProblem = FString::Printf(TEXT("ConditionMultiplier %.3f is not above zero"), Modifier->ConditionMultiplier);

		return false;
	}

	// the pattern is how the modifier's name reaches the player, and its word order is what a
	// translator reorders. An asset without one still composes, via an English-order fallback -
	// which is exactly the thing authoring this field is meant to avoid, so say so.
	if (Modifier->NamePattern.IsEmpty())
	{
		OutProblem = TEXT("NamePattern is empty, so the name falls back to hard-coded English word order");

		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierDefinitionWellFormedTest,
	"Smores.Content.ItemModifierDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(UItemModifierDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason
	if (!TestTrue(TEXT("At least one UItemModifierDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	for (const FAssetData& AssetData : Assets)
	{
		const UItemModifierDefinition* Modifier = Cast<UItemModifierDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateItemModifierDefinition(Modifier, Problem))
		{
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierDefinitionRuleTest,
	"Smores.Content.ItemModifierDefinitions.MalformedDefinitionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierDefinitionRuleTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemModifierDefinition* Modifier = TestWorld.NewKeptObject<UItemModifierDefinition>();

	if (!TestNotNull(TEXT("Modifier created"), Modifier))
	{
		return true;
	}

	FString Problem;

	TestFalse(TEXT("A modifier with no DefinitionId is rejected"), ValidateItemModifierDefinition(Modifier, Problem));

	Modifier->DefinitionId = FName(TEXT("RuleProof"));
	Modifier->DisplayName = FText::FromString(TEXT("Rule Proof"));

	TestFalse(TEXT("A modifier with no NamePattern is rejected"), ValidateItemModifierDefinition(Modifier, Problem));
	TestTrue(TEXT("...and the failure names the field"), Problem.Contains(TEXT("NamePattern")));

	Modifier->NamePattern = FText::FromString(TEXT("{Modifier} {Item}"));

	TestTrue(TEXT("A filled-in modifier is accepted"), ValidateItemModifierDefinition(Modifier, Problem));

	// the id a modifier answers to is {ItemModifierDefinition, DefinitionId}; the type half has
	// to match the config entry or nothing resolves - see EveryDefinitionResolvesById
	TestEqual(TEXT("A modifier's primary asset id is typed ItemModifierDefinition"),
		Modifier->GetPrimaryAssetId().ToString(), FString(TEXT("ItemModifierDefinition:RuleProof")));

	// this is the one that matters: a zero multiplier is authored, not defaulted, and it erases
	// whatever it touches rather than reducing it
	Modifier->ValueMultiplier = 0.0f;

	TestFalse(TEXT("A zero value multiplier is rejected rather than silently making items free"),
		ValidateItemModifierDefinition(Modifier, Problem));

	Modifier->ValueMultiplier = 1.0f;
	Modifier->WeightMultiplier = 0.0f;

	TestFalse(TEXT("...and so is a zero weight multiplier"), ValidateItemModifierDefinition(Modifier, Problem));

	TestFalse(TEXT("A null modifier is rejected rather than crashing the sweep"),
		ValidateItemModifierDefinition(nullptr, Problem));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
