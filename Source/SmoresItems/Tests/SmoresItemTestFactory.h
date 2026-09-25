// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagContainer.h"
#include "InventoryComponent.h"
#include "ItemDefinition.h"
#include "ItemModifierDefinition.h"
#include "LootTableDefinition.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Item definitions built in memory for a test, plus a way to snapshot a grid and compare it
 *  afterwards.
 *
 *  **Tests never load a UItemDefinition out of Content/.** A test that loads a real asset is
 *  testing that asset as much as the code, so a designer retuning a sword's weight breaks an
 *  unrelated inventory test and the failure points at the wrong place. Building the definition
 *  here also puts the test's inputs where the assertion is - a 2x3 footprint with a stack cap
 *  of 10 is readable in the test body instead of requiring someone to open the editor.
 */

/** Hands out a distinct DefinitionId per definition, so the sort's display-name tiebreak is well defined */
inline int32& SmoresTestItemCounter()
{
	static int32 Counter = 0;

	return Counter;
}

/**
 *  A definition with exactly the numbers the assertion cares about and defaults for the rest.
 *  Kept alive by the test world - a definition with no owner is otherwise collectable.
 */
inline UItemDefinition* MakeTestItemDefinition(
	FSmoresTestWorld& TestWorld,
	FIntPoint Footprint = FIntPoint(1, 1),
	int32 MaxStack = 1,
	float Weight = 0.0f,
	int32 BaseValue = 0,
	EEquipSlot EquipSlot = EEquipSlot::None,
	FName DefinitionId = NAME_None)
{
	UItemDefinition* Definition = TestWorld.NewKeptObject<UItemDefinition>();

	if (!Definition)
	{
		return nullptr;
	}

	Definition->DefinitionId = (DefinitionId == NAME_None)
		? FName(*FString::Printf(TEXT("TestItem_%03d"), SmoresTestItemCounter()++))
		: DefinitionId;

	// the repack's last-resort ordering compares display names, so every definition needs a
	// distinct one for a sort test to have a single correct answer
	Definition->DisplayName = FText::FromName(Definition->DefinitionId);

	Definition->FootprintWidth = Footprint.X;
	Definition->FootprintHeight = Footprint.Y;
	Definition->MaxStackSize = MaxStack;
	Definition->Weight = Weight;
	Definition->BaseValue = BaseValue;
	Definition->EquipSlot = EquipSlot;

	return Definition;
}

/**
 *  An item modifier with exactly the numbers the assertion cares about.
 *
 *  Same rule as the definitions above: **tests never load a UItemModifierDefinition out of
 *  Content/**, so retuning Bronze in the editor can't break an arithmetic test that happens to
 *  use it. The multipliers default to 1.0 here as they do on the asset, so a test that only
 *  cares about naming or stacking needn't pick numbers it doesn't use.
 */
inline UItemModifierDefinition* MakeTestModifier(
	FSmoresTestWorld& TestWorld,
	EItemModifierSlot Slot,
	const TCHAR* DisplayName,
	float WeightMultiplier = 1.0f,
	float ValueMultiplier = 1.0f,
	FLinearColor Tint = FLinearColor::White,
	const TCHAR* NamePattern = TEXT("{Modifier} {Item}"))
{
	UItemModifierDefinition* Modifier = TestWorld.NewKeptObject<UItemModifierDefinition>();

	if (!Modifier)
	{
		return nullptr;
	}

	Modifier->DefinitionId = FName(*FString::Printf(TEXT("TestModifier_%03d"), SmoresTestItemCounter()++));
	Modifier->DisplayName = FText::FromString(DisplayName);
	Modifier->Slot = Slot;
	Modifier->WeightMultiplier = WeightMultiplier;
	Modifier->ValueMultiplier = ValueMultiplier;
	Modifier->Tint = Tint;
	Modifier->NamePattern = FText::FromString(NamePattern);

	return Modifier;
}

/**
 *  A loot table with the roll count the assertion cares about and no entries yet - add them with
 *  the AddTestLoot* helpers below.
 *
 *  Same rule again: **tests never load a ULootTableDefinition out of Content/**, so rebalancing
 *  DA_Loot_CommonJunk can't break a test about how weights pick.
 */
inline ULootTableDefinition* MakeTestLootTable(FSmoresTestWorld& TestWorld, int32 MinRolls = 1, int32 MaxRolls = 1)
{
	ULootTableDefinition* Table = TestWorld.NewKeptObject<ULootTableDefinition>();

	if (!Table)
	{
		return nullptr;
	}

	Table->DefinitionId = FName(*FString::Printf(TEXT("TestLootTable_%03d"), SmoresTestItemCounter()++));
	Table->DisplayName = FText::FromName(Table->DefinitionId);
	Table->MinRolls = MinRolls;
	Table->MaxRolls = MaxRolls;

	return Table;
}

/** Appends an entry of the given kind with default fields and returns it. Don't hold the reference across another append. */
inline FLootTableEntry& AddTestLootEntry(ULootTableDefinition* Table, ELootEntryKind Kind, int32 Weight = 1)
{
	FLootTableEntry& Entry = Table->Entries.AddDefaulted_GetRef();
	Entry.Kind = Kind;
	Entry.Weight = Weight;

	return Entry;
}

/** Appends an Item entry yielding [MinQuantity, MaxQuantity] of Item */
inline FLootTableEntry& AddTestLootItem(ULootTableDefinition* Table, UItemDefinition* Item, int32 Weight = 1, int32 MinQuantity = 1, int32 MaxQuantity = 1)
{
	FLootTableEntry& Entry = AddTestLootEntry(Table, ELootEntryKind::Item, Weight);
	Entry.Item = Item;
	Entry.MinQuantity = MinQuantity;
	Entry.MaxQuantity = MaxQuantity;

	return Entry;
}

/** Appends a Sub-table entry rolling SubTable in full */
inline FLootTableEntry& AddTestLootSubTable(ULootTableDefinition* Table, ULootTableDefinition* SubTable, int32 Weight = 1)
{
	FLootTableEntry& Entry = AddTestLootEntry(Table, ELootEntryKind::Table, Weight);
	Entry.Table = SubTable;

	return Entry;
}

/** Appends one choice to a modifier pool. A null Modifier is the "leave it bare" choice. */
inline void AddTestModifierChoice(FLootModifierPool& Pool, UItemModifierDefinition* Modifier, int32 Weight = 1)
{
	FLootModifierChoice& Choice = Pool.Choices.AddDefaulted_GetRef();
	Choice.Modifier = Modifier;
	Choice.Weight = Weight;
}

/**
 *  Item.Mineral, from Config/DefaultGameplayTags.ini. Tags are the one input these tests take from
 *  the project rather than building in memory: an FGameplayTag can't be minted for a name the tag
 *  manager doesn't know. That is config, not content - no designer retuning an asset can change it.
 *  Invalid (and the test should fail on it) if the ini entry has gone.
 */
inline FGameplayTag GetTestMineralTag()
{
	return FGameplayTag::RequestGameplayTag(FName(TEXT("Item.Mineral")), /*ErrorIfNotFound*/ false);
}

/** Every item in a rolled list, as "Id xN" lines, so two rolls can be compared as strings */
inline FString DescribeRolledItems(const TArray<FInventoryItem>& Items)
{
	FString Result;

	for (const FInventoryItem& Item : Items)
	{
		Result += FString::Printf(TEXT("%s x%d"), *Item.GetItemId().ToString(), Item.Quantity);

		for (const TObjectPtr<UItemModifierDefinition>& Modifier : Item.Modifiers)
		{
			Result += FString::Printf(TEXT(" +%s"), Modifier ? *Modifier->DefinitionId.ToString() : TEXT("None"));
		}

		Result += TEXT("; ");
	}

	return Result;
}

/** An instance of a test definition, ready to hand to AddItem/AddItemAt */
inline FInventoryItem MakeTestItem(UItemDefinition* Definition, int32 Quantity = 1, bool bStolen = false, float Condition = 1.0f)
{
	FInventoryItem Item(Definition, Quantity);
	Item.bStolen = bStolen;
	Item.Condition = Condition;

	return Item;
}

/**
 *  An inventory of the given size on a fresh authoritative owner.
 *
 *  These shared helpers live in the header rather than in an anonymous namespace per test file
 *  on purpose: UE's unity builds concatenate several .cpp files into one translation unit, and
 *  two files each defining `MakeTestInventory` in an anonymous namespace would collide the day
 *  adaptive unity decides to include them together.
 */
inline UInventoryComponent* MakeTestInventory(FSmoresTestWorld& TestWorld, int32 Width, int32 Height)
{
	UInventoryComponent* Inventory = TestWorld.SpawnComponent<UInventoryComponent>();

	if (Inventory)
	{
		Inventory->SetGridSize(Width, Height);
	}

	return Inventory;
}

/** Total units of everything placed, however many entries they are spread across */
inline int32 GetTotalQuantity(const UInventoryComponent* Inventory)
{
	int32 Total = 0;

	if (Inventory)
	{
		for (const FInventoryEntry& Entry : Inventory->GetEntries())
		{
			Total += Entry.Item.Quantity;
		}
	}

	return Total;
}

/** The id of the only entry in a grid, or INDEX_NONE */
inline int32 GetOnlyEntryId(const UInventoryComponent* Inventory)
{
	return (Inventory && Inventory->GetEntries().Num() == 1) ? Inventory->GetEntries()[0].EntryId : INDEX_NONE;
}

/** FIntPoint has no TestEqual overload worth relying on, so cells are compared by hand and printed like this */
inline FString CellToString(FIntPoint Cell)
{
	return FString::Printf(TEXT("(%d,%d)"), Cell.X, Cell.Y);
}

/** The placed order as definitions, so an expected ordering reads as a list */
inline TArray<const UItemDefinition*> GetDefinitionOrder(const UInventoryComponent* Inventory)
{
	TArray<const UItemDefinition*> Order;

	if (Inventory)
	{
		for (const FInventoryEntry& Entry : Inventory->GetEntries())
		{
			Order.Add(Entry.Item.Definition);
		}
	}

	return Order;
}

/** Everything about an arrangement except the ids, for comparing two independently-built grids */
inline FString DescribePlacements(const UInventoryComponent* Inventory)
{
	FString Result;

	if (Inventory)
	{
		for (const FInventoryEntry& Entry : Inventory->GetEntries())
		{
			Result += FString::Printf(TEXT("%s x%d @(%d,%d)%s; "),
				*Entry.Item.GetItemId().ToString(),
				Entry.Item.Quantity,
				Entry.AnchorCell.X,
				Entry.AnchorCell.Y,
				Entry.bRotated ? TEXT(" rot") : TEXT(""));
		}
	}

	return Result;
}

/** A definition ordering, printable in a failure message */
inline FString DescribeOrder(const TArray<const UItemDefinition*>& Order)
{
	FString Result;

	for (const UItemDefinition* Definition : Order)
	{
		Result += FString::Printf(TEXT("%s "), *GetNameSafe(Definition));
	}

	return Result;
}

/**
 *  A copy of everything about a grid that a mutator could change, so an all-or-nothing failure
 *  can be asserted as "nothing moved" rather than merely "it returned false".
 *
 *  Entry order is part of the comparison deliberately: a repack that reordered the array while
 *  landing every item back on its original cell has still changed the grid, and a snapshot that
 *  ignored order would call that unchanged.
 */
struct FInventorySnapshot
{
	explicit FInventorySnapshot(const UInventoryComponent* Inventory)
	{
		if (!Inventory)
		{
			return;
		}

		GridSize = Inventory->GetGridSize();
		Entries = Inventory->GetEntries();
	}

	bool operator==(const FInventorySnapshot& Other) const
	{
		if (GridSize != Other.GridSize || Entries.Num() != Other.Entries.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FInventoryEntry& A = Entries[Index];
			const FInventoryEntry& B = Other.Entries[Index];

			const bool bSame = A.EntryId == B.EntryId
				&& A.AnchorCell == B.AnchorCell
				&& A.bRotated == B.bRotated
				&& A.Item.Definition == B.Item.Definition
				&& A.Item.Quantity == B.Item.Quantity
				&& A.Item.bStolen == B.Item.bStolen
				&& A.Item.Condition == B.Item.Condition
				&& A.Item.HasSameModifiersAs(B.Item);

			if (!bSame)
			{
				return false;
			}
		}

		return true;
	}

	bool operator!=(const FInventorySnapshot& Other) const { return !(*this == Other); }

	/** Readable in a failure message, so a mismatch says what moved rather than just that something did */
	FString ToString() const
	{
		FString Result = FString::Printf(TEXT("%dx%d ["), GridSize.X, GridSize.Y);

		for (const FInventoryEntry& Entry : Entries)
		{
			Result += FString::Printf(TEXT("{id=%d %s x%d @(%d,%d)%s} "),
				Entry.EntryId,
				*Entry.Item.GetItemId().ToString(),
				Entry.Item.Quantity,
				Entry.AnchorCell.X,
				Entry.AnchorCell.Y,
				Entry.bRotated ? TEXT(" rot") : TEXT(""));
		}

		return Result + TEXT("]");
	}

	FIntPoint GridSize = FIntPoint::ZeroValue;

	TArray<FInventoryEntry> Entries;
};

#endif // WITH_DEV_AUTOMATION_TESTS
