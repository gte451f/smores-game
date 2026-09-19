// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "ItemDefinition.h"
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
				&& A.Item.Condition == B.Item.Condition;

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
