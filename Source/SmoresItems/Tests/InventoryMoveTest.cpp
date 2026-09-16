// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Moving and transferring: the static MoveItem/MoveItemCounted pair that serves both
 *  repositioning inside one grid and transferring between two.
 *
 *  Two things here are asserted harder than the return value: that a refused move leaves *both*
 *  grids byte-identical, and that a move truncated by the destination's stack cap reports what
 *  actually changed hands. The second is the bug that would have overcharged a player for goods
 *  they never received.
 */


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryNoSwapTest,
	"Smores.Items.Inventory.MoveItemNeverSwaps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryNoSwapTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	UItemDefinition* DefinitionA = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);
	UItemDefinition* DefinitionB = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	TestTrue(TEXT("The source item was placed"), Source->AddItemAt(MakeTestItem(DefinitionA), FIntPoint(0, 0), false));
	TestTrue(TEXT("The occupying item was placed"), Dest->AddItemAt(MakeTestItem(DefinitionB), FIntPoint(0, 0), false));

	const FInventorySnapshot SourceBefore(Source);
	const FInventorySnapshot DestBefore(Dest);

	int32 QuantityMoved = -1;

	// dropping onto something that can't stack is rejected whole - there is no swap, because two
	// differently-shaped footprints have no well-defined exchange
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(0, 0), false, 0, QuantityMoved);

	TestFalse(TEXT("A drop onto a non-stackable occupant is refused"), bMoved);
	TestEqual(TEXT("Nothing changed hands"), QuantityMoved, 0);

	// the refusal has to be inert, not merely false - asserting only the bool would pass even if
	// the item had been destroyed on the way
	const FInventorySnapshot SourceAfter(Source);
	const FInventorySnapshot DestAfter(Dest);

	TestTrue(*FString::Printf(TEXT("The source grid is unchanged. Before %s, after %s"), *SourceBefore.ToString(), *SourceAfter.ToString()),
		SourceBefore == SourceAfter);
	TestTrue(*FString::Printf(TEXT("The destination grid is unchanged. Before %s, after %s"), *DestBefore.ToString(), *DestAfter.ToString()),
		DestBefore == DestAfter);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryFullStackDropTest,
	"Smores.Items.Inventory.MoveItemOntoFullStackRefused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryFullStackDropTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	// same definition and stackable, but the destination stack is already at its cap
	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	TestTrue(TEXT("The source stack was placed"), Source->AddItemAt(MakeTestItem(Definition, 3), FIntPoint(0, 0), false));
	TestTrue(TEXT("A full destination stack was placed"), Dest->AddItemAt(MakeTestItem(Definition, 5), FIntPoint(0, 0), false));

	const FInventorySnapshot SourceBefore(Source);
	const FInventorySnapshot DestBefore(Dest);

	int32 QuantityMoved = -1;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(0, 0), false, 0, QuantityMoved);

	TestFalse(TEXT("A drop onto a stack with no space left is refused"), bMoved);
	TestEqual(TEXT("Nothing changed hands"), QuantityMoved, 0);

	TestTrue(TEXT("The source grid is unchanged"), SourceBefore == FInventorySnapshot(Source));
	TestTrue(TEXT("The destination grid is unchanged"), DestBefore == FInventorySnapshot(Dest));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryCrossGridMoveTest,
	"Smores.Items.Inventory.MoveItemTransfersAtomically",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryCrossGridMoveTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	TestTrue(TEXT("The source stack was placed"), Source->AddItemAt(MakeTestItem(Definition, 7), FIntPoint(0, 0), false));

	int32 QuantityMoved = 0;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(2, 2), false, 0, QuantityMoved);

	TestTrue(TEXT("The whole entry transferred"), bMoved);
	TestEqual(TEXT("All seven units changed hands"), QuantityMoved, 7);

	// removed from the source in the same operation that added it to the destination - there is
	// no window where the item is in both grids or in neither
	TestEqual(TEXT("The source no longer holds it"), Source->GetEntries().Num(), 0);

	if (TestEqual(TEXT("The destination holds exactly one entry"), Dest->GetEntries().Num(), 1))
	{
		const FInventoryEntry& Landed = Dest->GetEntries()[0];

		TestEqual(TEXT("...with the whole quantity"), Landed.Item.Quantity, 7);
		TestTrue(TEXT("...at the cell it was dropped on"), Landed.AnchorCell == FIntPoint(2, 2));
		TestTrue(TEXT("...and the same definition"), Landed.Item.Definition == Definition);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryPartialMoveTest,
	"Smores.Items.Inventory.MoveItemSplitsSourceStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryPartialMoveTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 20);

	TestTrue(TEXT("The source stack of ten was placed"), Source->AddItemAt(MakeTestItem(Definition, 10), FIntPoint(0, 0), false));

	int32 QuantityMoved = 0;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(1, 1), false, 4, QuantityMoved);

	TestTrue(TEXT("The partial move succeeded"), bMoved);
	TestEqual(TEXT("Four units moved"), QuantityMoved, 4);

	if (TestEqual(TEXT("The source keeps its entry"), Source->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...with the remainder still in it"), Source->GetEntries()[0].Item.Quantity, 6);
	}

	if (TestEqual(TEXT("The destination gained one entry"), Dest->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...holding the moved units"), Dest->GetEntries()[0].Item.Quantity, 4);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryMergeTruncationTest,
	"Smores.Items.Inventory.MoveItemCountedReportsTruncatedMerge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryMergeTruncationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	// cap of ten, and the destination stack is already holding eight of them
	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	TestTrue(TEXT("The source stack of five was placed"), Source->AddItemAt(MakeTestItem(Definition, 5), FIntPoint(0, 0), false));
	TestTrue(TEXT("The near-full destination stack was placed"), Dest->AddItemAt(MakeTestItem(Definition, 8), FIntPoint(0, 0), false));

	int32 QuantityMoved = 0;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(0, 0), false, 5, QuantityMoved);

	// The move succeeds, and it moved two of the five asked for. A purchase reading only the
	// bool here charges for five - which is exactly the bug this assertion exists to catch.
	TestTrue(TEXT("A merge that only partly fits still reports success"), bMoved);
	TestEqual(TEXT("...but reports only the two units that actually fit, not the five requested"), QuantityMoved, 2);

	if (TestEqual(TEXT("The destination still holds one entry"), Dest->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...topped up to its cap"), Dest->GetEntries()[0].Item.Quantity, 10);
	}

	if (TestEqual(TEXT("The source still holds one entry"), Source->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...with the three that wouldn't fit left behind"), Source->GetEntries()[0].Item.Quantity, 3);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryShallowDestinationTest,
	"Smores.Items.Inventory.MoveItemClampsToDestinationCap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryShallowDestinationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Source = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Dest = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Source inventory created"), Source) || !TestNotNull(TEXT("Destination inventory created"), Dest))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	// a pawn's pack taking from a storefront shelf: the destination stacks shallower than the
	// source, so the whole stack cannot land in one entry
	Dest->StackMultiplier = 0.5f;

	TestEqual(TEXT("The destination's effective cap is half the definition's"), Dest->GetEffectiveMaxStack(Definition), 5);

	TestTrue(TEXT("The source stack of ten was placed"), Source->AddItemAt(MakeTestItem(Definition, 10), FIntPoint(0, 0), false));

	int32 QuantityMoved = 0;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Source, GetOnlyEntryId(Source), Dest, FIntPoint(2, 2), false, 0, QuantityMoved);

	TestTrue(TEXT("The move succeeded"), bMoved);
	TestEqual(TEXT("Only what the destination could hold moved"), QuantityMoved, 5);

	if (TestEqual(TEXT("The source keeps an entry"), Source->GetEntries().Num(), 1))
	{
		// the remainder stays put rather than being quietly destroyed by the placement clamp
		TestEqual(TEXT("...holding the five that stayed behind"), Source->GetEntries()[0].Item.Quantity, 5);
	}

	if (TestEqual(TEXT("The destination gained one entry"), Dest->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...filled to its shallower cap"), Dest->GetEntries()[0].Item.Quantity, 5);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryRepositionTest,
	"Smores.Items.Inventory.MoveItemRepositionReportsWholeEntry",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryRepositionTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	TestTrue(TEXT("The stack was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 7), FIntPoint(0, 0), false));

	const int32 EntryId = GetOnlyEntryId(Inventory);

	int32 QuantityMoved = 0;
	const bool bMoved = UInventoryComponent::MoveItemCounted(Inventory, EntryId, Inventory, FIntPoint(3, 3), false, 0, QuantityMoved);

	TestTrue(TEXT("The reposition succeeded"), bMoved);

	// nothing changed hands, but nothing was left behind either
	TestEqual(TEXT("A pure reposition reports the whole entry as moved"), QuantityMoved, 7);

	if (TestEqual(TEXT("There is still exactly one entry"), Inventory->GetEntries().Num(), 1))
	{
		const FInventoryEntry& Moved = Inventory->GetEntries()[0];

		TestEqual(TEXT("The entry kept its id rather than being destroyed and recreated"), Moved.EntryId, EntryId);
		TestTrue(TEXT("...and sits at the new cell"), Moved.AnchorCell == FIntPoint(3, 3));
		TestEqual(TEXT("...with its quantity intact"), Moved.Item.Quantity, 7);
	}

	// dropped back exactly where it started is a no-op, not a move
	const FInventorySnapshot Before(Inventory);

	int32 NoopQuantity = -1;
	TestFalse(TEXT("Dropping an entry back where it already sits changes nothing"),
		UInventoryComponent::MoveItemCounted(Inventory, EntryId, Inventory, FIntPoint(3, 3), false, 0, NoopQuantity));
	TestEqual(TEXT("...and reports nothing moved"), NoopQuantity, 0);
	TestTrue(TEXT("...leaving the grid untouched"), Before == FInventorySnapshot(Inventory));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryMoveGuardsTest,
	"Smores.Items.Inventory.MoveItemRejectsBadArguments",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryMoveGuardsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	TestTrue(TEXT("An entry was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 2), FIntPoint(0, 0), false));

	const FInventorySnapshot Before(Inventory);

	int32 QuantityMoved = -1;

	TestFalse(TEXT("A null source is refused"),
		UInventoryComponent::MoveItemCounted(nullptr, 0, Inventory, FIntPoint(1, 1), false, 0, QuantityMoved));
	TestFalse(TEXT("A null destination is refused"),
		UInventoryComponent::MoveItemCounted(Inventory, 0, nullptr, FIntPoint(1, 1), false, 0, QuantityMoved));
	TestFalse(TEXT("An unknown entry id is refused"),
		UInventoryComponent::MoveItemCounted(Inventory, 9999, Inventory, FIntPoint(1, 1), false, 0, QuantityMoved));

	TestEqual(TEXT("None of them reported anything moved"), QuantityMoved, 0);
	TestTrue(TEXT("...and none of them touched the grid"), Before == FInventorySnapshot(Inventory));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
