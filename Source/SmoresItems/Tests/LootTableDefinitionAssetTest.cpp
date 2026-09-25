// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ItemDefinition.h"
#include "LootTableDefinition.h"
#include "Tests/SmoresDefinitionRules.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The **per-type** layer of the definition content sweeps, for loot tables. The base sweeps in
 *  SmoresCore already check the id, the name, uniqueness and id look-up; this adds only what is
 *  true of a table.
 *
 *  Nearly every rule here exists because a table fails *silently*. A roll that lands on an entry
 *  naming nothing yields nothing and logs a warning nobody reads; a zero weight is an entry that
 *  looks authored and never drops; a Tag entry for a tag no item carries is the same thing one
 *  step removed. None of that is visible in play except as "this chest seems a bit empty". And a
 *  table that names itself is the one failure that isn't silent at all - the roll is abandoned at
 *  the nesting limit - but it is far cheaper to refuse the asset than to find it in PIE.
 *
 *  **Zero is the value to worry about**, as it was for modifiers: a weight defaults to 1, so a 0
 *  was typed. Removing the entry is the honest way to disable it.
 *
 *  See ItemDefinitionAssetTest.cpp for why the sweeps run against real content and the rules are
 *  proved against an in-memory definition.
 */

/**
 *  Every rule a loot table has to satisfy, base rules first. RegisteredItems is what a Tag entry is
 *  checked against - every item in Content/ for the sweep, a hand-built list for the rule proof.
 *  Returns false and names the first problem.
 */
inline bool ValidateLootTableDefinition(const ULootTableDefinition* Table, const TArray<UItemDefinition*>& RegisteredItems, FString& OutProblem)
{
	if (!ValidateSmoresDefinition(Table, OutProblem))
	{
		return false;
	}

	if (Table->MaxRolls < Table->MinRolls)
	{
		OutProblem = FString::Printf(TEXT("MaxRolls %d is below MinRolls %d"), Table->MaxRolls, Table->MinRolls);

		return false;
	}

	if (Table->MaxRolls <= 0)
	{
		OutProblem = TEXT("MaxRolls is 0, so the table can never yield anything");

		return false;
	}

	if (Table->Entries.IsEmpty())
	{
		OutProblem = TEXT("it has no entries");

		return false;
	}

	for (int32 EntryIndex = 0; EntryIndex < Table->Entries.Num(); ++EntryIndex)
	{
		const FLootTableEntry& Entry = Table->Entries[EntryIndex];

		if (Entry.Weight <= 0)
		{
			OutProblem = FString::Printf(TEXT("entry %d has weight %d, so it is never picked - remove it instead"), EntryIndex, Entry.Weight);

			return false;
		}

		switch (Entry.Kind)
		{
		case ELootEntryKind::Item:
			if (!Entry.Item)
			{
				OutProblem = FString::Printf(TEXT("entry %d is an Item entry naming no item"), EntryIndex);

				return false;
			}
			break;

		case ELootEntryKind::Table:
			if (!Entry.Table)
			{
				OutProblem = FString::Printf(TEXT("entry %d is a Sub-table entry naming no table"), EntryIndex);

				return false;
			}
			break;

		case ELootEntryKind::Tag:
		{
			if (!Entry.Tag.IsValid())
			{
				OutProblem = FString::Printf(TEXT("entry %d is a Tag entry with no tag"), EntryIndex);

				return false;
			}

			const bool bAnyItemCarriesIt = RegisteredItems.ContainsByPredicate([&Entry](const UItemDefinition* Item)
			{
				return Item && Item->Tags.HasTag(Entry.Tag);
			});

			if (!bAnyItemCarriesIt)
			{
				OutProblem = FString::Printf(TEXT("entry %d picks among items tagged %s, and no item carries it"), EntryIndex, *Entry.Tag.ToString());

				return false;
			}
			break;
		}

		case ELootEntryKind::Nothing:
			break;
		}

		const bool bYieldsAnItem = Entry.Kind == ELootEntryKind::Item || Entry.Kind == ELootEntryKind::Tag;

		if (!bYieldsAnItem)
		{
			continue;
		}

		if (Entry.MinQuantity < 1 || Entry.MaxQuantity < Entry.MinQuantity)
		{
			OutProblem = FString::Printf(TEXT("entry %d has quantity range %d-%d"), EntryIndex, Entry.MinQuantity, Entry.MaxQuantity);

			return false;
		}

		for (int32 PoolIndex = 0; PoolIndex < Entry.ModifierPools.Num(); ++PoolIndex)
		{
			const FLootModifierPool& Pool = Entry.ModifierPools[PoolIndex];

			if (Pool.Choices.IsEmpty())
			{
				OutProblem = FString::Printf(TEXT("entry %d's modifier pool %d has no choices"), EntryIndex, PoolIndex);

				return false;
			}

			for (const FLootModifierChoice& Choice : Pool.Choices)
			{
				if (Choice.Weight <= 0)
				{
					OutProblem = FString::Printf(TEXT("entry %d's modifier pool %d has a choice of weight %d, which is never chosen"), EntryIndex, PoolIndex, Choice.Weight);

					return false;
				}
			}
		}
	}

	if (Table->ContainsNestingLoop())
	{
		OutProblem = TEXT("it names itself, directly or through another table, so a roll of it never ends");

		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableDefinitionWellFormedTest,
	"Smores.Content.LootTableDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(ULootTableDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason
	if (!TestTrue(TEXT("At least one ULootTableDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	// what a Tag entry is checked against is what a real roll would choose among
	const TArray<UItemDefinition*> RegisteredItems = ULootTableDefinition::GatherRegisteredItems();

	for (const FAssetData& AssetData : Assets)
	{
		const ULootTableDefinition* Table = Cast<ULootTableDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateLootTableDefinition(Table, RegisteredItems, Problem))
		{
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableDefinitionRuleTest,
	"Smores.Content.LootTableDefinitions.MalformedDefinitionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableDefinitionRuleTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ULootTableDefinition* Table = TestWorld.NewKeptObject<ULootTableDefinition>();
	UItemDefinition* Item = MakeTestItemDefinition(TestWorld);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	const TArray<UItemDefinition*> NoItems;
	FString Problem;

	TestFalse(TEXT("A table with no DefinitionId is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));

	Table->DefinitionId = FName(TEXT("RuleProof"));
	Table->DisplayName = FText::FromString(TEXT("Rule Proof"));

	TestFalse(TEXT("A table with no entries is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));

	AddTestLootItem(Table, Item);

	TestTrue(TEXT("A table with one real entry is accepted"), ValidateLootTableDefinition(Table, NoItems, Problem));

	TestEqual(TEXT("A table's primary asset id is typed LootTableDefinition"),
		Table->GetPrimaryAssetId().ToString(), FString(TEXT("LootTableDefinition:RuleProof")));

	// the silent ones: each yields nothing in play and says so only in a log
	Table->Entries[0].Weight = 0;

	TestFalse(TEXT("A zero-weight entry is rejected rather than silently never dropping"), ValidateLootTableDefinition(Table, NoItems, Problem));

	Table->Entries[0].Weight = 1;
	Table->Entries[0].Item = nullptr;

	TestFalse(TEXT("An Item entry naming no item is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));

	Table->Entries[0].Item = Item;
	Table->Entries[0].MaxQuantity = 0;

	TestFalse(TEXT("A quantity range running backwards is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));

	Table->Entries[0].MaxQuantity = 1;
	Table->MaxRolls = 0;
	Table->MinRolls = 0;

	TestFalse(TEXT("A table that can never roll anything is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));

	Table->MinRolls = 1;
	Table->MaxRolls = 1;

	// a tag is checked against the items that exist, not merely for being set
	const FGameplayTag Mineral = GetTestMineralTag();

	if (TestTrue(TEXT("Item.Mineral is defined in Config/DefaultGameplayTags.ini"), Mineral.IsValid()))
	{
		FLootTableEntry& TagEntry = AddTestLootEntry(Table, ELootEntryKind::Tag);
		TagEntry.Tag = Mineral;

		TestFalse(TEXT("A Tag entry no item carries is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));
		TestTrue(TEXT("...and the failure names the tag"), Problem.Contains(TEXT("Item.Mineral")));

		UItemDefinition* Ore = MakeTestItemDefinition(TestWorld);
		Ore->Tags.AddTag(Mineral);

		const TArray<UItemDefinition*> WithOre = { Item, Ore };

		TestTrue(TEXT("...and accepted once an item carries it"), ValidateLootTableDefinition(Table, WithOre, Problem));
	}

	// and the one that isn't silent - it abandons the roll - but is far cheaper caught here. Back to
	// the one valid item entry first, so the rejection below can only be for the loop.
	Table->Entries.SetNum(1);

	TestTrue(TEXT("The table is valid again before the loop is added"), ValidateLootTableDefinition(Table, NoItems, Problem));

	AddTestLootSubTable(Table, Table);

	TestFalse(TEXT("A table that names itself is rejected"), ValidateLootTableDefinition(Table, NoItems, Problem));
	TestTrue(TEXT("...for naming itself"), Problem.Contains(TEXT("names itself")));

	TestFalse(TEXT("A null table is rejected rather than crashing the sweep"), ValidateLootTableDefinition(nullptr, NoItems, Problem));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
