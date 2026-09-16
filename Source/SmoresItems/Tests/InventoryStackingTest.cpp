// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Stack caps, what may merge with what, and the counted-return family.
 *
 *  The counted-return tests are the highest-value ones in the project. This codebase has
 *  shipped the same bug three times - a "did it work?" bool returning true having taken only
 *  part of the quantity - and every case of it is three lines to assert.
 */


//~ Stack caps

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryEffectiveMaxStackTest,
	"Smores.Items.Inventory.EffectiveMaxStackScalesWithHolder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryEffectiveMaxStackTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	Inventory->StackMultiplier = 1.0f;
	TestEqual(TEXT("At a multiplier of 1 the cap is the definition's own MaxStackSize"), Inventory->GetEffectiveMaxStack(Definition), 10);

	// this is what lets a storefront shelf stack deeper than a backpack with no per-transfer
	// special case anywhere
	Inventory->StackMultiplier = 2.5f;
	TestEqual(TEXT("The holder's multiplier scales the cap, floored"), Inventory->GetEffectiveMaxStack(Definition), 25);

	// a multiplier can never scale a stack out of existence
	Inventory->StackMultiplier = 0.05f;
	TestEqual(TEXT("A multiplier that would floor to zero still leaves room for one"), Inventory->GetEffectiveMaxStack(Definition), 1);

	TestEqual(TEXT("A null definition has no cap at all"), Inventory->GetEffectiveMaxStack(nullptr), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryCanStackWithTest,
	"Smores.Items.Inventory.CanStackWithRules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryCanStackWithTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Stackable = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);
	UItemDefinition* OtherStackable = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);
	UItemDefinition* Unstackable = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	const FInventoryItem A = MakeTestItem(Stackable, 2);
	const FInventoryItem B = MakeTestItem(Stackable, 3);

	TestTrue(TEXT("Two instances of the same stackable definition merge"), A.CanStackWith(B));

	TestFalse(TEXT("Different definitions never merge"), A.CanStackWith(MakeTestItem(OtherStackable, 1)));

	// MaxStackSize of 1 means "one of these per entry", whatever else matches
	const FInventoryItem Single = MakeTestItem(Unstackable, 1);
	TestFalse(TEXT("A definition that isn't stackable never merges, even with itself"), Single.CanStackWith(MakeTestItem(Unstackable, 1)));

	// a stolen unit must not launder itself clean by merging into an honest stack
	TestFalse(TEXT("A stolen instance will not merge into a clean stack"), A.CanStackWith(MakeTestItem(Stackable, 1, /*bStolen*/ true)));
	TestTrue(TEXT("Two stolen instances of the same definition do merge"),
		MakeTestItem(Stackable, 1, true).CanStackWith(MakeTestItem(Stackable, 1, true)));

	// Deliberate, and asserted here so a future session that "fixes" it has to delete a test
	// saying not to: stackable goods are bulk materials, and splitting a stack per wear value
	// would fragment it uselessly.
	const FInventoryItem Pristine = MakeTestItem(Stackable, 1, false, 1.0f);
	const FInventoryItem Battered = MakeTestItem(Stackable, 1, false, 0.2f);

	TestTrue(TEXT("Condition is deliberately ignored when deciding whether two stacks merge"), Pristine.CanStackWith(Battered));

	// an item holding nothing is not a stack partner
	TestFalse(TEXT("An empty item merges with nothing"), FInventoryItem().CanStackWith(A));
	TestFalse(TEXT("...and nothing merges with an empty item"), A.CanStackWith(FInventoryItem()));

	TestWorld.ForwardErrors(this);

	return true;
}

//~ The counted-return family

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryPartialAddTest,
	"Smores.Items.Inventory.AddItemCountedReportsPartialAdd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryPartialAddTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// the partial add logs a warning naming what wouldn't fit; expecting it here both keeps the
	// run clean and asserts the code actually said something
	AddExpectedMessagePlain(TEXT("has no room for"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	// four cells, and an unstackable item, so exactly four units can ever land
	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 2, 2);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	int32 QuantityAdded = 0;
	const bool bAddedAll = Inventory->AddItemCounted(MakeTestItem(Definition, 6), QuantityAdded);

	// this is the bug shape the whole suite exists for: "it worked" must not mean "four of the
	// six worked"
	TestFalse(TEXT("AddItemCounted reports failure when it could not take the whole quantity"), bAddedAll);
	TestEqual(TEXT("...and reports the number that actually landed, not the number asked for"), QuantityAdded, 4);

	TestEqual(TEXT("The grid holds exactly that many entries"), Inventory->GetEntries().Num(), 4);
	TestEqual(TEXT("...and exactly that many units"), GetTotalQuantity(Inventory), 4);
	TestEqual(TEXT("The grid is full"), Inventory->GetFreeCellCount(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryAddItemKeepsWhatFitTest,
	"Smores.Items.Inventory.AddItemKeepsWhatFitOnPartialAdd",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryAddItemKeepsWhatFitTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	AddExpectedMessagePlain(TEXT("has no room for"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 2, 2);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1);

	// the plain bool forwarder must agree with the counted version about success...
	TestFalse(TEXT("AddItem returns false when only part of the quantity fit"), Inventory->AddItem(MakeTestItem(Definition, 6)));

	// ...and, crucially, a partial add is not rolled back. A caller that destroys its source on
	// a false return would be deleting the four units that did land.
	TestEqual(TEXT("The units that fit are kept rather than rolled back"), GetTotalQuantity(Inventory), 4);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryStackSplitTest,
	"Smores.Items.Inventory.AddItemSplitsAboveEffectiveCap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryStackSplitTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 8, 8);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	// cap of 5 a stack, adding 12 at once
	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	int32 QuantityAdded = 0;

	TestTrue(TEXT("The whole quantity was taken"), Inventory->AddItemCounted(MakeTestItem(Definition, 12), QuantityAdded));
	TestEqual(TEXT("...all twelve units of it"), QuantityAdded, 12);

	TestEqual(TEXT("It split into as many entries as the cap requires"), Inventory->GetEntries().Num(), 3);
	TestEqual(TEXT("No unit was lost in the splitting"), GetTotalQuantity(Inventory), 12);

	for (const FInventoryEntry& Entry : Inventory->GetEntries())
	{
		TestTrue(
			*FString::Printf(TEXT("No entry exceeds the effective cap of 5 (found %d)"), Entry.Item.Quantity),
			Entry.Item.Quantity <= 5);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryMergeBeforePlacingTest,
	"Smores.Items.Inventory.AddItemMergesBeforeConsumingCells",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryMergeBeforePlacingTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	// a partial stack already sitting in the grid
	TestTrue(TEXT("A stack of two was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 2), FIntPoint(0, 0), false));

	const int32 FreeCellsBefore = Inventory->GetFreeCellCount();

	// three of these four top the existing stack up to its cap of five; only the fourth needs a cell
	TestTrue(TEXT("Four more were added"), Inventory->AddItem(MakeTestItem(Definition, 4)));

	TestEqual(TEXT("Only one new entry was needed"), Inventory->GetEntries().Num(), 2);
	TestEqual(TEXT("Every unit is accounted for"), GetTotalQuantity(Inventory), 6);
	TestEqual(TEXT("Merging consumed only one further cell"), Inventory->GetFreeCellCount(), FreeCellsBefore - 1);

	TestEqual(TEXT("The existing stack was filled to its cap first"), Inventory->GetEntries()[0].Item.Quantity, 5);
	TestEqual(TEXT("...and the remainder went to the new entry"), Inventory->GetEntries()[1].Item.Quantity, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryAddItemAtClampsTest,
	"Smores.Items.Inventory.AddItemAtClampsToEffectiveCap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryAddItemAtClampsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

	if (!TestNotNull(TEXT("Inventory created"), Inventory))
	{
		return true;
	}

	UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	// AddItemAt places at an explicit cell without merging, and clamps rather than splitting -
	// worth knowing, because unlike AddItem the excess here is simply gone
	TestTrue(TEXT("The oversized stack was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 99), FIntPoint(0, 0), false));

	if (TestEqual(TEXT("One entry was created"), Inventory->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("Its quantity was clamped to the effective cap"), Inventory->GetEntries()[0].Item.Quantity, 5);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
