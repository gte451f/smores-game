// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Entry lifetime, grid resizing, the weight readout, and the change delegate.
 *
 *  The weight group is worth more than it looks: weight and grid footprint are deliberately
 *  independent measures, and a capacity of zero deliberately means "unlimited" rather than
 *  "permanently overloaded". Both are the kind of rule a later session would otherwise
 *  "tidy up" without noticing it was a decision.
 */


//~ Entries and lifecycle

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryEntryIdStabilityTest,
	"Smores.Items.Inventory.EntryIdSurvivesRemoval",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryEntryIdStabilityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	for (int32 Column = 0; Column < 3; ++Column)
	{
		TestTrue(TEXT("An entry was placed"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(Column, 0), false));
	}

	if (!TestEqual(TEXT("Three entries were placed"), Inventory->GetEntries().Num(), 3))
	{
		return true;
	}

	const int32 FirstId = Inventory->GetEntries()[0].EntryId;
	const int32 MiddleId = Inventory->GetEntries()[1].EntryId;
	const int32 LastId = Inventory->GetEntries()[2].EntryId;

	TestTrue(TEXT("The three entries have distinct ids"), FirstId != MiddleId && MiddleId != LastId && FirstId != LastId);

	TestTrue(TEXT("The middle entry was removed"), Inventory->RemoveEntry(MiddleId));

	// unlike an array index, an id survives its neighbours going away - which is what lets the
	// UI hold one across a client to server round trip
	const FInventoryEntry Last = Inventory->GetEntry(LastId);

	TestTrue(TEXT("The last entry still resolves by id"), Last.IsValidEntry());
	TestEqual(TEXT("...to the same id"), Last.EntryId, LastId);
	TestTrue(TEXT("...still sitting where it was"), Last.AnchorCell == FIntPoint(2, 0));

	TestFalse(TEXT("The removed id no longer resolves"), Inventory->GetEntry(MiddleId).IsValidEntry());

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySetQuantityTest,
	"Smores.Items.Inventory.SetEntryQuantityClampsAndRemoves",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySetQuantityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	TestTrue(TEXT("An entry was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 1), FIntPoint(0, 0), false));

	const int32 EntryId = Inventory->GetEntries()[0].EntryId;

	TestTrue(TEXT("Setting a quantity above the cap succeeds"), Inventory->SetEntryQuantity(EntryId, 99));
	TestEqual(TEXT("...clamped to the effective cap"), Inventory->GetEntry(EntryId).Item.Quantity, 5);

	// already at that value, so nothing changed and nothing is broadcast
	TestFalse(TEXT("Setting it to the value it already holds reports no change"), Inventory->SetEntryQuantity(EntryId, 99));

	TestFalse(TEXT("An unknown entry id is refused"), Inventory->SetEntryQuantity(9999, 3));

	// emptying an entry removes the placement rather than leaving a zero-count ghost
	TestTrue(TEXT("Setting a quantity of zero succeeds"), Inventory->SetEntryQuantity(EntryId, 0));
	TestEqual(TEXT("...and removes the entry outright"), Inventory->GetEntries().Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryRemoveEntryTest,
	"Smores.Items.Inventory.RemoveEntryRejectsUnknownId",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryRemoveEntryTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	TestTrue(TEXT("An entry was placed"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(0, 0), false));

	const FInventorySnapshot Before(Inventory);

	TestFalse(TEXT("Removing an id that isn't there is refused"), Inventory->RemoveEntry(9999));
	TestFalse(TEXT("Removing INDEX_NONE is refused"), Inventory->RemoveEntry(INDEX_NONE));

	TestTrue(TEXT("...and neither touched the grid"), Before == FInventorySnapshot(Inventory));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventorySetGridSizeTest,
	"Smores.Items.Inventory.SetGridSizeDropsOnlyWhatNoLongerFits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventorySetGridSizeTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// shrinking the grid warns about each entry it drops
	AddExpectedMessagePlain(TEXT("no longer fits the resized"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	TestTrue(TEXT("An entry was placed inside the eventual smaller grid"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(0, 0), false));
	TestTrue(TEXT("An entry was placed outside it"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(7, 7), false));

	TestTrue(TEXT("The grid was resized"), Inventory->SetGridSize(4, 4));

	if (TestEqual(TEXT("Only the entry that no longer fits was dropped"), Inventory->GetEntries().Num(), 1))
	{
		TestTrue(TEXT("...and the one that still fits kept its place"), Inventory->GetEntries()[0].AnchorCell == FIntPoint(0, 0));
	}

	TestTrue(TEXT("The grid reports its new size"), Inventory->GetGridSize() == FIntPoint(4, 4));

	// resizing to the size it already is changes nothing
	TestFalse(TEXT("Resizing to the current size reports no change"), Inventory->SetGridSize(4, 4));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryEntryAtCellTest,
	"Smores.Items.Inventory.GetEntryIdAtCell",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryEntryAtCellTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(2, 2), 1);

	TestTrue(TEXT("A 2x2 entry was placed at (1,1)"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(1, 1), false));

	const int32 EntryId = Inventory->GetEntries()[0].EntryId;

	TestEqual(TEXT("Its anchor cell resolves to it"), Inventory->GetEntryIdAtCell(FIntPoint(1, 1)), EntryId);

	// any covered cell resolves, not only the anchor - this is what a drop under the cursor uses
	TestEqual(TEXT("A non-anchor cell of the same footprint resolves to it too"), Inventory->GetEntryIdAtCell(FIntPoint(2, 2)), EntryId);

	TestEqual(TEXT("A free cell resolves to nothing"), Inventory->GetEntryIdAtCell(FIntPoint(0, 0)), (int32)INDEX_NONE);
	TestEqual(TEXT("A cell past the right edge resolves to nothing"), Inventory->GetEntryIdAtCell(FIntPoint(4, 0)), (int32)INDEX_NONE);
	TestEqual(TEXT("A negative cell resolves to nothing"), Inventory->GetEntryIdAtCell(FIntPoint(-1, -1)), (int32)INDEX_NONE);

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Weight

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryWeightTest,
	"Smores.Items.Inventory.WeightIsIndependentOfFootprint",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryWeightTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// a bundle of cloth is bulky and light; an ingot is small and heavy. The two measures are
	// meant to disagree, so the test states a case where they visibly do.
	UItemDefinition* BulkyLight = MakeTestItemDefinition(TestWorld, FIntPoint(3, 3), 1, /*Weight*/ 0.5f);
	UItemDefinition* SmallHeavy = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, /*Weight*/ 10.0f);

	TestTrue(TEXT("The bulky light item was placed"), Inventory->AddItemAt(MakeTestItem(BulkyLight), FIntPoint(0, 0), false));
	TestTrue(TEXT("The small heavy item was placed"), Inventory->AddItemAt(MakeTestItem(SmallHeavy), FIntPoint(4, 0), false));

	TestEqual(TEXT("Total weight is the sum of unit weight x quantity"), Inventory->GetTotalWeight(), 10.5f);

	// nine cells weighing half a unit, one cell weighing ten - the orderings are opposites
	TestTrue(TEXT("The item occupying nine cells is the lighter of the two"),
		Inventory->GetEntries()[0].Item.GetTotalWeight() < Inventory->GetEntries()[1].Item.GetTotalWeight());

	TestEqual(TEXT("Occupied cells follow the footprint, not the weight"), Inventory->GetFreeCellCount(), 64 - 10);

	// quantity multiplies into the total
	UItemDefinition* Stackable = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10, /*Weight*/ 2.5f);

	TestTrue(TEXT("A stack of four was placed"), Inventory->AddItemAt(MakeTestItem(Stackable, 4), FIntPoint(5, 0), false));
	TestEqual(TEXT("A stack weighs its unit weight times its quantity"), Inventory->GetTotalWeight(), 20.5f);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryWeightCapacityTest,
	"Smores.Items.Inventory.WeightCapacityRules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryWeightCapacityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Heavy = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, /*Weight*/ 10.0f);
	UItemDefinition* Light = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, /*Weight*/ 0.5f);

	// A capacity of zero means unlimited, not "permanently overloaded". This is what a chest
	// wants - weight is a carried-density figure, and nothing static carries.
	Inventory->WeightCapacity = 0.0f;

	TestFalse(TEXT("A holder with no capacity authored has no weight limit"), Inventory->HasWeightLimit());

	TestTrue(TEXT("A heavy item was placed"), Inventory->AddItemAt(MakeTestItem(Heavy), FIntPoint(0, 0), false));

	TestFalse(TEXT("A chest holding a tonne is not over capacity"), Inventory->IsOverWeightCapacity());

	// now give it a real limit, sitting exactly on it
	Inventory->WeightCapacity = 10.0f;

	TestTrue(TEXT("A positive capacity is a real limit"), Inventory->HasWeightLimit());
	TestEqual(TEXT("It is carrying exactly its capacity"), Inventory->GetTotalWeight(), 10.0f);

	// strictly greater than, so sitting exactly on the limit is not over it
	TestFalse(TEXT("Exactly at capacity is not over capacity"), Inventory->IsOverWeightCapacity());

	TestTrue(TEXT("A light item was added on top"), Inventory->AddItemAt(MakeTestItem(Light), FIntPoint(1, 0), false));

	TestTrue(TEXT("A hair over capacity is over capacity"), Inventory->IsOverWeightCapacity());

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Delegates

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryChangeDelegateTest,
	"Smores.Items.Inventory.ChangeDelegateFiresOncePerSuccess",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryChangeDelegateTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	USmoresTestDelegateListener* Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();

	if (!TestNotNull(TEXT("Listener created"), Listener))
	{
		return true;
	}

	Inventory->OnInventoryChanged.AddDynamic(Listener, &USmoresTestDelegateListener::OnChanged);

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	TestTrue(TEXT("The item was added"), Inventory->AddItem(MakeTestItem(Definition)));
	TestEqual(TEXT("A successful add broadcasts exactly once"), Listener->CallCount, 1);

	// a refused mutator must not redraw the UI
	Listener->Reset();
	TestFalse(TEXT("Placing out of bounds is refused"), Inventory->AddItemAt(MakeTestItem(Definition), FIntPoint(99, 99), false));
	TestEqual(TEXT("A refused placement broadcasts nothing"), Listener->CallCount, 0);

	Listener->Reset();
	TestFalse(TEXT("Removing an unknown id is refused"), Inventory->RemoveEntry(9999));
	TestEqual(TEXT("A refused removal broadcasts nothing"), Listener->CallCount, 0);

	// one add that creates three entries is still one change
	Listener->Reset();
	TestTrue(TEXT("A quantity spanning three stacks was added"), Inventory->AddItem(MakeTestItem(Definition, 12)));
	TestEqual(TEXT("An add that splits across several entries still broadcasts once"), Listener->CallCount, 1);

	Listener->Reset();
	TestTrue(TEXT("A real entry was removed"), Inventory->RemoveEntry(Inventory->GetEntries()[0].EntryId));
	TestEqual(TEXT("A successful removal broadcasts exactly once"), Listener->CallCount, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
