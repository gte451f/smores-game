// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Grid arithmetic: what fits where, what overlaps what, and which way round an item lands.
 *  Everything here is a query rather than a mutation, so these are the cheapest tests in the
 *  project - no BeginPlay, no tick, and in most cases nothing placed at all.
 */


//~ The authority check - see SmoresTestWorldTest.cpp for why this matters so much

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryAuthorityTest,
	"Smores.Items.Inventory.AuthorityInTestWorld",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = TestWorld.SpawnComponent<UInventoryComponent>();

	if (!TestNotNull(TEXT("An inventory component was created on a spawned owner"), Inventory))
	{
		return true;
	}

	TestTrue(TEXT("The component reports owner authority, so its mutators run their real path"), Inventory->HasOwnerAuthority());

	// the assertion that actually proves it: a mutator that changes nothing would still return
	// false and leave an empty grid, which is indistinguishable from a no-op gate
	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld);

	TestTrue(TEXT("AddItem succeeds under authority"), Inventory->AddItem(MakeTestItem(Definition)));
	TestEqual(TEXT("...and the item is actually in the grid"), Inventory->GetEntries().Num(), 1);

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Placement and geometry

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryGridEdgeTest,
	"Smores.Items.Inventory.CanPlaceAtRejectsGridEdges",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryGridEdgeTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// a 2x2 in a 4x4, so the last legal anchor is (2,2)
	const FInventoryItem Item = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(2, 2)));

	TestFalse(TEXT("A footprint crossing the right edge is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(3, 0), false));
	TestFalse(TEXT("A footprint crossing the bottom edge is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(0, 3), false));
	TestFalse(TEXT("A footprint crossing the left edge is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(-1, 0), false));
	TestFalse(TEXT("A footprint crossing the top edge is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(0, -1), false));

	TestTrue(TEXT("The last anchor that fits exactly is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(2, 2), false));
	TestTrue(TEXT("The top-left anchor is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(0, 0), false));

	// an empty item is nothing, not something to place
	TestFalse(TEXT("An item with no definition can never be placed"), Inventory->CanPlaceAt(FInventoryItem(), FIntPoint(0, 0), false));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryOverlapTest,
	"Smores.Items.Inventory.CanPlaceAtOverlapAndAbut",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryOverlapTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 6, 6);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(2, 2));
	const FInventoryItem Item = MakeTestItem(Definition);

	TestTrue(TEXT("The blocking entry was placed at (0,0)"), Inventory->AddItemAt(Item, FIntPoint(0, 0), false));

	// overlap is rejected at every degree of it, including a single shared corner cell
	TestFalse(TEXT("A footprint overlapping by three cells is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(1, 0), false));
	TestFalse(TEXT("A footprint overlapping by one corner cell is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(1, 1), false));
	TestFalse(TEXT("Placing onto the exact same cells is rejected"), Inventory->CanPlaceAt(Item, FIntPoint(0, 0), false));

	// abutting is not overlapping - the placed entry ends at column 1, so column 2 is free
	TestTrue(TEXT("A footprint abutting on the right is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(2, 0), false));
	TestTrue(TEXT("A footprint abutting below is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(0, 2), false));
	TestTrue(TEXT("A footprint abutting diagonally is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(2, 2), false));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryIgnoreEntryTest,
	"Smores.Items.Inventory.CanPlaceAtIgnoresOwnEntry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryIgnoreEntryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 6, 6);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	const FInventoryItem Item = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(2, 2)));

	TestTrue(TEXT("The entry was placed at (0,0)"), Inventory->AddItemAt(Item, FIntPoint(0, 0), false));

	if (!TestEqual(TEXT("Exactly one entry is placed"), Inventory->GetEntries().Num(), 1))
	{
		return true;
	}

	const int32 EntryId = Inventory->GetEntries()[0].EntryId;

	// this is what makes a one-cell nudge legal: without the exclusion an entry collides with
	// the cells it is currently sitting on and can never be moved by less than its own width
	TestFalse(TEXT("A one-cell nudge collides with the entry's own cells"), Inventory->CanPlaceAt(Item, FIntPoint(1, 1), false));
	TestTrue(TEXT("...and is accepted once that entry is excluded"), Inventory->CanPlaceAt(Item, FIntPoint(1, 1), false, EntryId));
	TestTrue(TEXT("Re-anchoring onto exactly its own cells is accepted"), Inventory->CanPlaceAt(Item, FIntPoint(0, 0), false, EntryId));

	// the exclusion is one entry, not a free pass - a second entry still blocks
	TestTrue(TEXT("A second entry was placed at (4,0)"), Inventory->AddItemAt(Item, FIntPoint(4, 0), false));
	TestFalse(TEXT("Excluding the first entry does not excuse overlapping the second"), Inventory->CanPlaceAt(Item, FIntPoint(3, 0), false, EntryId));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryNaturalOrientationTest,
	"Smores.Items.Inventory.FindFreePlacementPrefersNaturalOrientation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryNaturalOrientationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// The grid is arranged so the two orientations disagree about where the item goes, and the
	// rotated answer is the one a naive scan would reach first:
	//
	//     . . . X      free cells are the top three columns of row 0,
	//     X X X .      and the bottom three rows of column 3
	//     X X X .
	//     X X X .
	//
	// A 1x3 (tall) fits only at (3,1). Rotated to 3x1 it fits at (0,0), which is scanned much
	// earlier. Natural orientation must still win, because the whole grid is swept unrotated
	// before anything is turned sideways.
	UItemDefinition* BlockerDefinition = MakeTestItemDefinition(TestWorld, FIntPoint(3, 3));
	UItemDefinition* CornerDefinition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1));

	TestTrue(TEXT("The 3x3 blocker was placed"), Inventory->AddItemAt(MakeTestItem(BlockerDefinition), FIntPoint(0, 1), false));
	TestTrue(TEXT("The corner blocker was placed"), Inventory->AddItemAt(MakeTestItem(CornerDefinition), FIntPoint(3, 0), false));

	const FInventoryItem TallItem = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(1, 3)));

	FIntPoint FoundCell = FIntPoint(-99, -99);
	bool bFoundRotated = true;

	if (!TestTrue(TEXT("A placement was found for the tall item"), Inventory->FindFreePlacement(TallItem, FoundCell, bFoundRotated)))
	{
		return true;
	}

	TestFalse(TEXT("The item came back unrotated, although a rotated fit was available earlier in scan order"), bFoundRotated);
	TestTrue(
		*FString::Printf(TEXT("It landed at the only cell its natural orientation fits, (3,1), not %s"), *CellToString(FoundCell)),
		FoundCell == FIntPoint(3, 1));

	// the rotated pass does still run when the natural one finds nothing at all
	const FInventoryItem WideItem = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(3, 1)));

	FIntPoint WideCell = FIntPoint(-99, -99);
	bool bWideRotated = false;

	if (TestTrue(TEXT("A placement was found for the wide item"), Inventory->FindFreePlacement(WideItem, WideCell, bWideRotated)))
	{
		TestTrue(
			*FString::Printf(TEXT("The wide item fits naturally along the free row at (0,0), not %s"), *CellToString(WideCell)),
			WideCell == FIntPoint(0, 0));
		TestFalse(TEXT("...unrotated"), bWideRotated);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryPlacementFailureTest,
	"Smores.Items.Inventory.FindFreePlacementFailureLeavesOutputs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryPlacementFailureTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 2, 2);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// nothing 3x3 will ever fit a 2x2 grid, in either orientation
	const FInventoryItem TooBig = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(3, 3)));

	FIntPoint Cell = FIntPoint(-7, -7);
	bool bRotated = true;

	TestFalse(TEXT("FindFreePlacement reports failure when nothing fits"), Inventory->FindFreePlacement(TooBig, Cell, bRotated));

	// the out-params are left alone on failure, so a caller that ignored the bool doesn't get a
	// plausible-looking (0,0) to act on
	TestTrue(*FString::Printf(TEXT("The cell out-param was left untouched, but it is %s"), *CellToString(Cell)), Cell == FIntPoint(-7, -7));
	TestTrue(TEXT("The rotation out-param was left untouched"), bRotated);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryCoversCellTest,
	"Smores.Items.Inventory.EntryCoversCell",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryCoversCellTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// a pure struct test - no component and no grid needed, only a definition
	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(2, 3));

	// rotated, so the 2x3 footprint reads as 3 wide by 2 tall, anchored at (1,1)
	const FInventoryEntry Entry(0, MakeTestItem(Definition), FIntPoint(1, 1), true);

	for (int32 Y = 1; Y <= 2; ++Y)
	{
		for (int32 X = 1; X <= 3; ++X)
		{
			TestTrue(
				*FString::Printf(TEXT("The rotated footprint covers %s"), *CellToString(FIntPoint(X, Y))),
				Entry.CoversCell(FIntPoint(X, Y)));
		}
	}

	// one cell past each edge
	TestFalse(TEXT("It does not cover the cell left of its anchor"), Entry.CoversCell(FIntPoint(0, 1)));
	TestFalse(TEXT("It does not cover the cell above its anchor"), Entry.CoversCell(FIntPoint(1, 0)));
	TestFalse(TEXT("It does not cover the cell past its right edge"), Entry.CoversCell(FIntPoint(4, 1)));
	TestFalse(TEXT("It does not cover the cell past its bottom edge"), Entry.CoversCell(FIntPoint(1, 3)));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryFootprintTest,
	"Smores.Items.Inventory.FootprintRotationSwapsAxes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryFootprintTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FInventoryItem Item = MakeTestItem(MakeTestItemDefinition(TestWorld, FIntPoint(2, 3)));

	TestTrue(TEXT("The natural footprint is the authored 2x3"), Item.GetFootprint(false) == FIntPoint(2, 3));
	TestTrue(TEXT("Rotation swaps width and height to 3x2"), Item.GetFootprint(true) == FIntPoint(3, 2));

	// an item that holds nothing has no extent at all, in either orientation
	const FInventoryItem Empty;

	TestTrue(TEXT("An empty item has a zero footprint"), Empty.GetFootprint(false) == FIntPoint::ZeroValue);
	TestTrue(TEXT("...rotated as well"), Empty.GetFootprint(true) == FIntPoint::ZeroValue);
	TestTrue(TEXT("An empty item reports itself empty"), Empty.IsEmpty());

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
