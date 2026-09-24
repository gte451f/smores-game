// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CharacterDefinition.h"
#include "FactionDefinition.h"
#include "SmoresDefinitionLibrary.h"
#include "Tests/SmoresDefinitionRules.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The per-type layer of the definition content sweeps, for characters - see SmoresItems'
 *  ItemDefinitionAssetTest.cpp for the shape and SmoresDefinitionRules.h for why a sweep runs
 *  against real content at all.
 *
 *  Every rule here guards something that fails *silently* at runtime, which is the test for
 *  whether a per-type file earns its place (game-data.md):
 *
 *    - A DefaultFactionId that doesn't resolve is deliberately *allowed* at runtime (a save with a
 *      stripped mod's faction must still load) and only warned about - so the sweep is the one
 *      place a typo in an authored asset gets caught before it ships.
 *    - A non-unique definition with no NamePool names every one of its characters after the
 *      definition itself - a town of people all called "Bandit" - and nothing errors.
 *    - A blank pool entry, or a loadout entry naming no item, quietly produces a nameless
 *      character or a missing item.
 *    - A negative attribute is never authored on purpose. (Nothing reads attributes yet, so what
 *      zero means is undecided; the sweep stays out of that until the first resolution math does.)
 */

/** Every rule a character definition has to satisfy, base rules first. Returns false and names the first problem. */
inline bool ValidateCharacterDefinition(const UCharacterDefinition* Definition, bool bCheckFactionResolves, FString& OutProblem)
{
	if (!ValidateSmoresDefinition(Definition, OutProblem))
	{
		return false;
	}

	if (!Definition->bUnique && Definition->NamePool.Num() == 0)
	{
		OutProblem = TEXT("NamePool is empty on a non-unique character - every one of them would be named after the definition");

		return false;
	}

	for (int32 Index = 0; Index < Definition->NamePool.Num(); ++Index)
	{
		if (Definition->NamePool[Index].IsEmptyOrWhitespace())
		{
			OutProblem = FString::Printf(TEXT("NamePool[%d] is blank"), Index);

			return false;
		}
	}

	for (int32 Index = 0; Index < Definition->DefaultLoadout.Num(); ++Index)
	{
		const FInventoryItem& Item = Definition->DefaultLoadout[Index];

		if (Item.IsEmpty())
		{
			OutProblem = FString::Printf(TEXT("DefaultLoadout[%d] names no item"), Index);

			return false;
		}

		if (Item.Quantity < 1)
		{
			OutProblem = FString::Printf(TEXT("DefaultLoadout[%d] has quantity %d"), Index, Item.Quantity);

			return false;
		}
	}

	if (Definition->BaseAttributes.GetLowest() < 0.0f)
	{
		OutProblem = TEXT("a BaseAttributes value is negative");

		return false;
	}

	// only in the sweep over real content - the in-memory proof below has no faction assets to resolve against
	if (bCheckFactionResolves && !Definition->DefaultFactionId.IsNone()
		&& !USmoresDefinitionLibrary::FindDefinition(UFactionDefinition::DefinitionType, Definition->DefaultFactionId))
	{
		OutProblem = FString::Printf(TEXT("DefaultFactionId %s doesn't resolve to a faction definition"), *Definition->DefaultFactionId.ToString());

		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterDefinitionWellFormedTest,
	"Smores.Content.CharacterDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(UCharacterDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason
	if (!TestTrue(TEXT("At least one UCharacterDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	for (const FAssetData& AssetData : Assets)
	{
		FString Problem;

		if (!ValidateCharacterDefinition(Cast<UCharacterDefinition>(AssetData.GetAsset()), /*bCheckFactionResolves*/ true, Problem))
		{
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterDefinitionRuleTest,
	"Smores.Content.CharacterDefinitions.MalformedDefinitionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterDefinitionRuleTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterDefinition* Definition = TestWorld.NewKeptObject<UCharacterDefinition>();

	if (!TestNotNull(TEXT("Definition created"), Definition))
	{
		return true;
	}

	Definition->DefinitionId = FName(TEXT("RuleProof"));
	Definition->DisplayName = FText::FromString(TEXT("Rule Proof"));

	FString Problem;

	TestFalse(TEXT("A non-unique character with no NamePool is rejected"), ValidateCharacterDefinition(Definition, false, Problem));
	TestTrue(TEXT("...and the failure names the field"), Problem.Contains(TEXT("NamePool")));

	Definition->bUnique = true;

	TestTrue(TEXT("A unique character needs no pool - it is named by DisplayName"), ValidateCharacterDefinition(Definition, false, Problem));

	Definition->bUnique = false;
	Definition->NamePool = { FText::FromString(TEXT("Ada")), FText::FromString(TEXT("  ")) };

	TestFalse(TEXT("A blank pool entry is rejected"), ValidateCharacterDefinition(Definition, false, Problem));

	Definition->NamePool = { FText::FromString(TEXT("Ada")) };

	TestTrue(TEXT("A filled pool is accepted"), ValidateCharacterDefinition(Definition, false, Problem));

	TestEqual(TEXT("A character's primary asset id is typed CharacterDefinition"),
		Definition->GetPrimaryAssetId().ToString(), FString(TEXT("CharacterDefinition:RuleProof")));

	Definition->DefaultLoadout = { FInventoryItem() };

	TestFalse(TEXT("A loadout entry naming no item is rejected"), ValidateCharacterDefinition(Definition, false, Problem));

	Definition->DefaultLoadout.Reset();
	Definition->BaseAttributes.Agility = -1.0f;

	TestFalse(TEXT("A negative attribute is rejected"), ValidateCharacterDefinition(Definition, false, Problem));

	Definition->BaseAttributes.Agility = 10.0f;
	Definition->DefaultFactionId = FName(TEXT("SmoresTest_NoSuchFaction"));

	TestFalse(TEXT("A faction id that resolves to nothing is rejected by the sweep"), ValidateCharacterDefinition(Definition, true, Problem));
	TestTrue(TEXT("...naming the id"), Problem.Contains(TEXT("SmoresTest_NoSuchFaction")));

	TestFalse(TEXT("A null definition is rejected rather than crashing the sweep"), ValidateCharacterDefinition(nullptr, false, Problem));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
