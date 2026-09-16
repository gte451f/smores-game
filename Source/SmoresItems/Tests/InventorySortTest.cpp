// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "SmoresRefusalReason.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The repack: merge what can merge, order the rest by one criterion, re-place it from the
 *  top-left with no gaps.
 *
 *  This is the best-suited thing in the whole project to an automated test - pure,
 *  deterministic, all-or-nothing arithmetic over the grid, where every rule is invisible on
 *  screen. A player cannot tell an abandoned repack from an already-sorted grid by looking,
 *  and neither can the bool; that distinction is the reason SortEntriesWithReason exists and
 *  the reason these tests assert the reason as well as the return value.
 */


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortCriteriaTest,
	"Smores.Items.Inventory.SortCriteriaOrderDescending",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortCriteriaTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// Three items whose weight, value and quantity orderings all disagree. A grid where the
	// three criteria agree would pass whatever the sort actually did.
	//
	//          weight   value   quantity
	//   Anvil     10.0       1          1
	//   Bead       2.0      20          2
	//   Cloth      2.5       5          5
	UItemDefinition* Anvil = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100, /*Weight*/ 10.0f, /*BaseValue*/ 1);
	UItemDefinition* Bead = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100, /*Weight*/ 1.0f, /*BaseValue*/ 10);
	UItemDefinition* Cloth = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100, /*Weight*/ 0.5f, /*BaseValue*/ 1);

	TestTrue(TEXT("Anvil placed"), Inventory->AddItemAt(MakeTestItem(Anvil, 1), FIntPoint(0, 0), false));
	TestTrue(TEXT("Bead placed"), Inventory->AddItemAt(MakeTestItem(Bead, 2), FIntPoint(1, 0), false));
	TestTrue(TEXT("Cloth placed"), Inventory->AddItemAt(MakeTestItem(Cloth, 5), FIntPoint(2, 0), false));

	// heaviest stack first: 10.0, then 2.5, then 2.0
	TestTrue(TEXT("Sorting by weight changed the arrangement"), Inventory->SortEntries(EInventorySortCriterion::Weight));

	const TArray<const UItemDefinition*> ByWeight = GetDefinitionOrder(Inventory);
	const TArray<const UItemDefinition*> ExpectedByWeight = { Anvil, Cloth, Bead };

	TestTrue(*FString::Printf(TEXT("Weight orders heaviest first. Got: %s"), *DescribeOrder(ByWeight)), ByWeight == ExpectedByWeight);

	// most valuable stack first: 20, then 5, then 1
	TestTrue(TEXT("Sorting by value changed the arrangement"), Inventory->SortEntries(EInventorySortCriterion::Value));

	const TArray<const UItemDefinition*> ByValue = GetDefinitionOrder(Inventory);
	const TArray<const UItemDefinition*> ExpectedByValue = { Bead, Cloth, Anvil };

	TestTrue(*FString::Printf(TEXT("Value orders most valuable first. Got: %s"), *DescribeOrder(ByValue)), ByValue == ExpectedByValue);

	// biggest stack first: 5, then 2, then 1
	TestTrue(TEXT("Sorting by quantity changed the arrangement"), Inventory->SortEntries(EInventorySortCriterion::Quantity));

	const TArray<const UItemDefinition*> ByQuantity = GetDefinitionOrder(Inventory);
	const TArray<const UItemDefinition*> ExpectedByQuantity = { Cloth, Bead, Anvil };

	TestTrue(*FString::Printf(TEXT("Quantity orders the biggest stack first. Got: %s"), *DescribeOrder(ByQuantity)), ByQuantity == ExpectedByQuantity);

	// the repack packs from the top-left with no gaps
	TestTrue(TEXT("The first entry sits in the corner"), Inventory->GetEntries()[0].AnchorCell == FIntPoint(0, 0));
	TestTrue(TEXT("The second sits next to it"), Inventory->GetEntries()[1].AnchorCell == FIntPoint(1, 0));
	TestTrue(TEXT("The third after that"), Inventory->GetEntries()[2].AnchorCell == FIntPoint(2, 0));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortIdempotentTest,
	"Smores.Items.Inventory.SortIsAFixedPoint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortIdempotentTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// Two items identical in every figure the comparator looks at except their id, and
	// unstackable so they cannot be merged away. Sort is not stable, so without the EntryId
	// tiebreak these two could swap places on every repack and "sorted" would be a state the
	// grid never settles into.
	UItemDefinition* Twin = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, /*Weight*/ 3.0f, /*BaseValue*/ 3);

	TestTrue(TEXT("The first twin was placed away from the corner"), Inventory->AddItemAt(MakeTestItem(Twin), FIntPoint(2, 0), false));
	TestTrue(TEXT("The second twin was placed in the corner"), Inventory->AddItemAt(MakeTestItem(Twin), FIntPoint(0, 0), false));

	const int32 FirstId = Inventory->GetEntries()[0].EntryId;
	const int32 SecondId = Inventory->GetEntries()[1].EntryId;

	TestTrue(TEXT("The first repack moved something"), Inventory->SortEntries(EInventorySortCriterion::Weight));

	// lower id first among equals
	TestEqual(TEXT("The lower id took the corner"), Inventory->GetEntries()[0].EntryId, FirstId);
	TestEqual(TEXT("...and the higher id followed it"), Inventory->GetEntries()[1].EntryId, SecondId);

	const FInventorySnapshot Sorted(Inventory);

	USmoresTestDelegateListener* Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Inventory->OnInventoryChanged.AddDynamic(Listener, &USmoresTestDelegateListener::OnChanged);

	ESmoresRefusalReason Reason = ESmoresRefusalReason::NoRoom;

	// an already-sorted grid is a fixed point: no change, no reason, no redraw
	TestFalse(TEXT("Sorting an already-sorted grid reports no change"), Inventory->SortEntriesWithReason(EInventorySortCriterion::Weight, Reason));
	TestTrue(TEXT("...and reports no reason, because nothing went wrong"), Reason == ESmoresRefusalReason::None);
	TestTrue(TEXT("...and mutates nothing"), Sorted == FInventorySnapshot(Inventory));
	TestEqual(TEXT("...and broadcasts no change nobody needs"), Listener->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortDeterminismTest,
	"Smores.Items.Inventory.SortIsIndependentOfInsertionOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortDeterminismTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* First = MakeTestInventory(TestWorld, 4, 4);
	UInventoryComponent* Second = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("First inventory created"), First) || !TestNotNull(TEXT("Second inventory created"), Second))
	{
		return true;
	}

	UItemDefinition* Anvil = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100, 10.0f, 1);
	UItemDefinition* Bead = MakeTestItemDefinition(TestWorld, FIntPoint(2, 1), 100, 1.0f, 10);
	UItemDefinition* Cloth = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 100, 0.5f, 1);

	// the same three items, added in opposite orders
	First->AddItem(MakeTestItem(Anvil, 1));
	First->AddItem(MakeTestItem(Bead, 2));
	First->AddItem(MakeTestItem(Cloth, 5));

	Second->AddItem(MakeTestItem(Cloth, 5));
	Second->AddItem(MakeTestItem(Bead, 2));
	Second->AddItem(MakeTestItem(Anvil, 1));

	First->SortEntries(EInventorySortCriterion::Weight);
	Second->SortEntries(EInventorySortCriterion::Weight);

	// ids will differ between the two grids; everything else must not
	TestEqual(TEXT("Both grids hold the same number of entries"), First->GetEntries().Num(), Second->GetEntries().Num());

	TestTrue(
		*FString::Printf(TEXT("Two grids with the same contents sort to identical placements.\n  first:  %s\n  second: %s"),
			*DescribePlacements(First), *DescribePlacements(Second)),
		DescribePlacements(First) == DescribePlacements(Second));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortConsolidationTest,
	"Smores.Items.Inventory.SortConsolidatesStacks",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortConsolidationTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// AddItemAt places without merging, which is the only way to build the two half-stacks a
	// repack is supposed to pour together
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);
		UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100);

		Inventory->AddItemAt(MakeTestItem(Definition, 3), FIntPoint(0, 0), false);
		Inventory->AddItemAt(MakeTestItem(Definition, 4), FIntPoint(1, 0), false);

		TestTrue(TEXT("The repack ran"), Inventory->SortEntries(EInventorySortCriterion::Quantity));
		TestEqual(TEXT("Two partial stacks of the same item became one"), Inventory->GetEntries().Num(), 1);
		TestEqual(TEXT("...holding every unit of both"), GetTotalQuantity(Inventory), 7);
	}

	// a pair CanStackWith rejects stays two, so a repack cannot launder a stolen flag away
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);
		UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100);

		Inventory->AddItemAt(MakeTestItem(Definition, 3, /*bStolen*/ false), FIntPoint(0, 0), false);
		Inventory->AddItemAt(MakeTestItem(Definition, 3, /*bStolen*/ true), FIntPoint(1, 0), false);

		Inventory->SortEntries(EInventorySortCriterion::Quantity);

		TestEqual(TEXT("A stolen stack does not merge into a clean one"), Inventory->GetEntries().Num(), 2);
		TestEqual(TEXT("...and no unit is lost keeping them apart"), GetTotalQuantity(Inventory), 6);
	}

	// merging never exceeds the holder's effective cap
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);
		UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

		Inventory->AddItemAt(MakeTestItem(Definition, 4), FIntPoint(0, 0), false);
		Inventory->AddItemAt(MakeTestItem(Definition, 4), FIntPoint(1, 0), false);

		Inventory->SortEntries(EInventorySortCriterion::Quantity);

		TestEqual(TEXT("Eight units over a cap of five need two entries"), Inventory->GetEntries().Num(), 2);
		TestEqual(TEXT("...and every unit survives the merge"), GetTotalQuantity(Inventory), 8);

		for (const FInventoryEntry& Entry : Inventory->GetEntries())
		{
			TestTrue(
				*FString::Printf(TEXT("No entry exceeds the cap of five (found %d)"), Entry.Item.Quantity),
				Entry.Item.Quantity <= 5);
		}
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortAbandonedTest,
	"Smores.Items.Inventory.SortAbandonedLeavesGridUntouched",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortAbandonedTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	AddExpectedMessagePlain(TEXT("could not repack"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 3, 3);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// Nine cells holding eight cells' worth of item, arranged so that first-fit packing in value
	// order strands the biggest one:
	//
	//   current        by value, first-fit
	//   K K G          G B B B        Gem takes the corner, the Bar claims a whole row,
	//   K K .          . . .          and the 2x2 Keg no longer has a 2x2 anywhere to go.
	//   B B B          . . .
	UItemDefinition* Keg = MakeTestItemDefinition(TestWorld, FIntPoint(2, 2), 1, 1.0f, /*BaseValue*/ 10);
	UItemDefinition* Bar = MakeTestItemDefinition(TestWorld, FIntPoint(3, 1), 1, 1.0f, /*BaseValue*/ 50);
	UItemDefinition* Gem = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 1.0f, /*BaseValue*/ 100);

	TestTrue(TEXT("The keg was placed"), Inventory->AddItemAt(MakeTestItem(Keg), FIntPoint(0, 0), false));
	TestTrue(TEXT("The bar was placed"), Inventory->AddItemAt(MakeTestItem(Bar), FIntPoint(0, 2), false));
	TestTrue(TEXT("The gem was placed"), Inventory->AddItemAt(MakeTestItem(Gem), FIntPoint(2, 0), false));

	const FInventorySnapshot Before(Inventory);

	USmoresTestDelegateListener* Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Inventory->OnInventoryChanged.AddDynamic(Listener, &USmoresTestDelegateListener::OnChanged);

	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;
	const bool bSorted = Inventory->SortEntriesWithReason(EInventorySortCriterion::Value, Reason);

	TestFalse(TEXT("A repack that cannot re-place everything is abandoned"), bSorted);

	// the one outcome the player cannot work out for themselves - an abandoned repack leaves a
	// grid that looks exactly like one that was already sorted
	TestTrue(TEXT("...and says so, rather than staying silent"), Reason == ESmoresRefusalReason::NoRoom);

	// all-or-nothing: not one item may be dropped or shuffled by a repack that gave up
	const FInventorySnapshot After(Inventory);

	TestTrue(*FString::Printf(TEXT("The grid is byte-for-byte what it was.\n  before: %s\n  after:  %s"), *Before.ToString(), *After.ToString()),
		Before == After);

	TestEqual(TEXT("...and nothing was broadcast"), Listener->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortForwarderTest,
	"Smores.Items.Inventory.SortForwarderAgreesWithReasonVariant",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortForwarderTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	AddExpectedMessagePlain(TEXT("could not repack"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	// A forwarder that drifts from the function it forwards to is the standing hazard of this
	// whole family - AddItem/AddItemCounted, MoveItem/MoveItemCounted, Equip/EquipWithReason.
	// Each case below is run through both entry points and the bools must agree.

	// the abandoned repack
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 3, 3);

		UItemDefinition* Keg = MakeTestItemDefinition(TestWorld, FIntPoint(2, 2), 1, 1.0f, 10);
		UItemDefinition* Bar = MakeTestItemDefinition(TestWorld, FIntPoint(3, 1), 1, 1.0f, 50);
		UItemDefinition* Gem = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 1.0f, 100);

		Inventory->AddItemAt(MakeTestItem(Keg), FIntPoint(0, 0), false);
		Inventory->AddItemAt(MakeTestItem(Bar), FIntPoint(0, 2), false);
		Inventory->AddItemAt(MakeTestItem(Gem), FIntPoint(2, 0), false);

		ESmoresRefusalReason Reason = ESmoresRefusalReason::None;
		const bool bInformative = Inventory->SortEntriesWithReason(EInventorySortCriterion::Value, Reason);
		const bool bPlain = Inventory->SortEntries(EInventorySortCriterion::Value);

		TestTrue(TEXT("Both entry points refuse the abandoned repack"), bInformative == bPlain && bPlain == false);
		TestTrue(TEXT("...and the informative one names NoRoom"), Reason == ESmoresRefusalReason::NoRoom);
	}

	// the already-sorted grid
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);
		UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 2.0f, 2);

		Inventory->AddItem(MakeTestItem(Definition));

		// sort once to reach the settled arrangement, then ask both entry points again
		Inventory->SortEntries(EInventorySortCriterion::Weight);

		ESmoresRefusalReason Reason = ESmoresRefusalReason::NoRoom;
		const bool bInformative = Inventory->SortEntriesWithReason(EInventorySortCriterion::Weight, Reason);
		const bool bPlain = Inventory->SortEntries(EInventorySortCriterion::Weight);

		TestTrue(TEXT("Both entry points report no change on a sorted grid"), bInformative == bPlain && bPlain == false);
		TestTrue(TEXT("...and the informative one names no reason at all"), Reason == ESmoresRefusalReason::None);
	}

	// an empty grid has nothing to do and nothing to complain about
	{
		UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

		ESmoresRefusalReason Reason = ESmoresRefusalReason::NoRoom;

		TestFalse(TEXT("An empty grid reports no change"), Inventory->SortEntriesWithReason(EInventorySortCriterion::Weight, Reason));
		TestTrue(TEXT("...with no reason"), Reason == ESmoresRefusalReason::None);
		TestFalse(TEXT("...and the forwarder agrees"), Inventory->SortEntries(EInventorySortCriterion::Weight));
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortNoAuthorityTest,
	"Smores.Items.Inventory.SortWithoutAuthorityIsSilent",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortNoAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// An ownerless component, which is the one way to reach HasOwnerAuthority() == false without
	// a net driver. It is not a client - a real non-authority test needs two connected instances
	// and waits on multiplayer - but it does exercise the gate's refusing branch, and it is the
	// exact construct every *other* test here must avoid (see SmoresTestWorld.h).
	UInventoryComponent* Ownerless = TestWorld.NewKeptObject<UInventoryComponent>();

	if (!TestNotNull(TEXT("An ownerless component was created"), Ownerless))
	{
		return true;
	}

	TestFalse(TEXT("A component with no owner has no authority"), Ownerless->HasOwnerAuthority());

	ESmoresRefusalReason Reason = ESmoresRefusalReason::NoRoom;

	TestFalse(TEXT("Sorting without authority does nothing"), Ownerless->SortEntriesWithReason(EInventorySortCriterion::Weight, Reason));

	// the player did nothing wrong, so there is nothing to tell them
	TestTrue(TEXT("...and reports no reason"), Reason == ESmoresRefusalReason::None);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySortPreservesEntriesTest,
	"Smores.Items.Inventory.SortPreservesIdsAndQuantities",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySortPreservesEntriesTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// three different definitions, so nothing can merge and every entry must survive intact
	UItemDefinition* Anvil = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 100, 10.0f, 1);
	UItemDefinition* Bead = MakeTestItemDefinition(TestWorld, FIntPoint(2, 1), 100, 1.0f, 10);
	UItemDefinition* Cloth = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 100, 0.5f, 1);

	Inventory->AddItem(MakeTestItem(Anvil, 1));
	Inventory->AddItem(MakeTestItem(Bead, 2));
	Inventory->AddItem(MakeTestItem(Cloth, 5));

	TArray<int32> IdsBefore;

	for (const FInventoryEntry& Entry : Inventory->GetEntries())
	{
		IdsBefore.Add(Entry.EntryId);
	}

	const int32 QuantityBefore = GetTotalQuantity(Inventory);

	Inventory->SortEntries(EInventorySortCriterion::Value);

	TArray<int32> IdsAfter;

	for (const FInventoryEntry& Entry : Inventory->GetEntries())
	{
		IdsAfter.Add(Entry.EntryId);
	}

	TestEqual(TEXT("No entry was lost in the repack"), IdsAfter.Num(), IdsBefore.Num());

	// a UI holding an id across a client to server round trip still resolves afterwards
	for (const int32 Id : IdsBefore)
	{
		TestTrue(*FString::Printf(TEXT("Entry id %d survived the repack"), Id), IdsAfter.Contains(Id));
		TestTrue(*FString::Printf(TEXT("Entry id %d still resolves to a real entry"), Id), Inventory->GetEntry(Id).IsValidEntry());
	}

	TestEqual(TEXT("Total quantity is unchanged by a repack"), GetTotalQuantity(Inventory), QuantityBefore);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
