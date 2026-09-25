// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "LootTableDefinition.h"
#include "ItemDefinition.h"
#include "ItemModifierDefinition.h"
#include "SmoresDefinitionLibrary.h"
#include "SmoresItems.h"

// Has to match the PrimaryAssetType in Config/DefaultGame.ini's PrimaryAssetTypesToScan entry for
// loot tables, or USmoresDefinitionLibrary::FindDefinition will never resolve a table id.
// Spelled out rather than derived from the class name so the two are visibly the same string.
const FPrimaryAssetType ULootTableDefinition::DefinitionType = FPrimaryAssetType(TEXT("LootTableDefinition"));

int32 ULootTableDefinition::GetEntryWeight(int32 EntryIndex) const
{
	return Entries.IsValidIndex(EntryIndex) ? Entries[EntryIndex].Weight : 0;
}

const UWeightedTableDefinition* ULootTableDefinition::GetEntrySubTable(int32 EntryIndex) const
{
	if (!Entries.IsValidIndex(EntryIndex) || Entries[EntryIndex].Kind != ELootEntryKind::Table)
	{
		return nullptr;
	}

	// a Sub-table entry with no table set falls through to ResolveEntry as a payload, which warns
	return Entries[EntryIndex].Table.Get();
}

bool ULootTableDefinition::RollLoot(FRandomStream& Stream, const TArray<UItemDefinition*>& TagCandidates, TArray<FInventoryItem>& OutItems, TArray<FName>* OutSourceTableIds) const
{
	return RollEntries(Stream, [&](const UWeightedTableDefinition& Table, int32 EntryIndex)
	{
		// every table a loot table can nest is itself a loot table - the entry's pointer is typed
		const ULootTableDefinition* LootTable = Cast<ULootTableDefinition>(&Table);

		if (!LootTable || !LootTable->Entries.IsValidIndex(EntryIndex))
		{
			return;
		}

		FInventoryItem Rolled;

		if (!LootTable->ResolveEntry(LootTable->Entries[EntryIndex], Stream, TagCandidates, Rolled))
		{
			return;
		}

		OutItems.Add(Rolled);

		if (OutSourceTableIds)
		{
			OutSourceTableIds->Add(LootTable->DefinitionId);
		}
	});
}

bool ULootTableDefinition::RollLoot(FRandomStream& Stream, TArray<FInventoryItem>& OutItems, TArray<FName>* OutSourceTableIds) const
{
	return RollLoot(Stream, GatherRegisteredItems(), OutItems, OutSourceTableIds);
}

TArray<UItemDefinition*> ULootTableDefinition::GatherRegisteredItems()
{
	TArray<FName> ItemIds;
	USmoresDefinitionLibrary::GetDefinitionIds(UItemDefinition::DefinitionType, ItemIds);

	TArray<UItemDefinition*> Items;
	Items.Reserve(ItemIds.Num());

	for (const FName& ItemId : ItemIds)
	{
		if (UItemDefinition* Item = Cast<UItemDefinition>(USmoresDefinitionLibrary::FindDefinition(UItemDefinition::DefinitionType, ItemId)))
		{
			Items.Add(Item);
		}
	}

	return Items;
}

UItemDefinition* ULootTableDefinition::PickTaggedItem(const FGameplayTag& Tag, const TArray<UItemDefinition*>& TagCandidates, FRandomStream& Stream)
{
	if (!Tag.IsValid())
	{
		return nullptr;
	}

	TArray<UItemDefinition*> Matches;

	for (UItemDefinition* Candidate : TagCandidates)
	{
		// HasTag rather than HasTagExact, so an item tagged with a child tag still counts
		if (Candidate && Candidate->Tags.HasTag(Tag))
		{
			Matches.AddUnique(Candidate);
		}
	}

	if (Matches.IsEmpty())
	{
		return nullptr;
	}

	// the candidates arrive in Asset Manager scan order, which isn't stable - sorting by id is what
	// makes "any mineral" land on the same mineral for the same seed on every machine
	Matches.Sort([](const UItemDefinition& A, const UItemDefinition& B)
	{
		return A.DefinitionId.LexicalLess(B.DefinitionId);
	});

	return Matches[Stream.RandRange(0, Matches.Num() - 1)];
}

UItemModifierDefinition* ULootTableDefinition::PickModifier(const FLootModifierPool& Pool, FRandomStream& Stream)
{
	int32 TotalWeight = 0;

	for (const FLootModifierChoice& Choice : Pool.Choices)
	{
		TotalWeight += FMath::Max(Choice.Weight, 0);
	}

	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	int32 Roll = Stream.RandRange(0, TotalWeight - 1);

	for (const FLootModifierChoice& Choice : Pool.Choices)
	{
		const int32 Weight = FMath::Max(Choice.Weight, 0);

		if (Roll < Weight)
		{
			return Choice.Modifier;
		}

		Roll -= Weight;
	}

	return nullptr;
}

bool ULootTableDefinition::ResolveEntry(const FLootTableEntry& Entry, FRandomStream& Stream, const TArray<UItemDefinition*>& TagCandidates, FInventoryItem& OutItem) const
{
	UItemDefinition* Definition = nullptr;

	switch (Entry.Kind)
	{
	case ELootEntryKind::Nothing:
		return false;

	case ELootEntryKind::Table:
		// a Sub-table entry only lands here when it names no table - a real one is rolled by the base
		UE_LOG(LogSmoresItems, Warning, TEXT("%s has a Sub-table entry naming no table - it yields nothing."), *GetNameSafe(this));
		return false;

	case ELootEntryKind::Item:
		Definition = Entry.Item;

		if (!Definition)
		{
			UE_LOG(LogSmoresItems, Warning, TEXT("%s has an Item entry naming no item - it yields nothing."), *GetNameSafe(this));
			return false;
		}

		break;

	case ELootEntryKind::Tag:
		Definition = PickTaggedItem(Entry.Tag, TagCandidates, Stream);

		if (!Definition)
		{
			UE_LOG(LogSmoresItems, Warning, TEXT("%s has a Tag entry for '%s' that no item carries - it yields nothing."),
				*GetNameSafe(this), *Entry.Tag.ToString());
			return false;
		}

		break;
	}

	const int32 MinQuantity = FMath::Max(Entry.MinQuantity, 1);
	const int32 Quantity = Stream.RandRange(MinQuantity, FMath::Max(MinQuantity, Entry.MaxQuantity));

	OutItem = FInventoryItem(Definition, Quantity);

	for (const FLootModifierPool& Pool : Entry.ModifierPools)
	{
		// AddModifier refusing a filled slot is the rule, not an error: it is what makes a later
		// pool a fallback for an earlier one that came up empty (see FLootModifierPool)
		if (UItemModifierDefinition* Modifier = PickModifier(Pool, Stream))
		{
			OutItem.AddModifier(Modifier);
		}
	}

	return true;
}
