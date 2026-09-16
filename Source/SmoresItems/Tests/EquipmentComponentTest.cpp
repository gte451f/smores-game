// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EquipmentComponent.h"
#include "InventoryComponent.h"
#include "SmoresRefusalReason.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The paperdoll: slot-type matching, taking one unit off a stack, and the swap that has to be
 *  all-or-nothing.
 *
 *  The load-bearing test here is the swap with nowhere to put the displaced item. Equipping
 *  into an occupied slot when the grid is full must fail having changed nothing - if it ever
 *  half-applies, the item it displaced is simply gone, and nothing on screen says so.
 */

/** An owner carrying both an inventory and an equipment component, the way a pawn does */
struct FTestPawnComponents
{
	UInventoryComponent* Inventory = nullptr;

	UEquipmentComponent* Equipment = nullptr;

	bool IsValid() const { return Inventory != nullptr && Equipment != nullptr; }
};

inline FTestPawnComponents MakeTestPawn(FSmoresTestWorld& TestWorld, int32 GridWidth, int32 GridHeight)
{
	FTestPawnComponents Pawn;

	AActor* Owner = TestWorld.SpawnOwner();

	if (!Owner)
	{
		return Pawn;
	}

	// both on one owner, because UEquipmentComponent finds the grid it takes from and returns to
	// through GetOwnerInventory()
	Pawn.Inventory = TestWorld.AddComponent<UInventoryComponent>(Owner);
	Pawn.Equipment = TestWorld.AddComponent<UEquipmentComponent>(Owner);

	if (Pawn.Inventory)
	{
		Pawn.Inventory->SetGridSize(GridWidth, GridHeight);
	}

	return Pawn;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentSlotLookupTest,
	"Smores.Items.Equipment.SlotLookup",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentSlotLookupTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 3.0f, 10, EEquipSlot::MainHand);
	UItemDefinition* Rock = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 1.0f, 1, EEquipSlot::None);

	TestTrue(TEXT("A wearable item reports its own slot"), UEquipmentComponent::GetSlotForItem(MakeTestItem(Sword)) == EEquipSlot::MainHand);
	TestTrue(TEXT("An item that isn't wearable reports no slot"), UEquipmentComponent::GetSlotForItem(MakeTestItem(Rock)) == EEquipSlot::None);
	TestTrue(TEXT("An empty item reports no slot"), UEquipmentComponent::GetSlotForItem(FInventoryItem()) == EEquipSlot::None);

	// Slot-type matching is the whole rule. A future session adding a strength or skill gate
	// here has to delete this assertion, which is the point at which it re-reads the reasoning:
	// the consequences of an untrained equip belong to combat resolution, never to inventory.
	const FInventoryItem AwkwardSword = MakeTestItem(Sword, 1, /*bStolen*/ true, /*Condition*/ 0.05f);

	TestTrue(TEXT("A stolen, nearly-destroyed item still equips - slot type is the only gate"),
		Pawn.Equipment->CanEquipItem(AwkwardSword, EEquipSlot::MainHand));

	TestFalse(TEXT("No item may be forced into a slot that isn't its own"), Pawn.Equipment->CanEquipItem(MakeTestItem(Sword), EEquipSlot::Head));
	TestFalse(TEXT("An unwearable item fits no slot"), Pawn.Equipment->CanEquipItem(MakeTestItem(Rock), EEquipSlot::MainHand));
	TestFalse(TEXT("EEquipSlot::None is never a place to put something"), Pawn.Equipment->CanEquipItem(MakeTestItem(Sword), EEquipSlot::None));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentTakesOneUnitTest,
	"Smores.Items.Equipment.EquipTakesOneUnitOffAStack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentTakesOneUnitTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	// a stackable wearable, so the "one unit" rule has something to bite on
	UItemDefinition* Dagger = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10, 1.0f, 5, EEquipSlot::MainHand);

	TestTrue(TEXT("A stack of three was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Dagger, 3), FIntPoint(0, 0), false));

	const int32 EntryId = GetOnlyEntryId(Pawn.Inventory);

	// EEquipSlot::None means "wherever this belongs" - the right-click path
	TestTrue(TEXT("Equipping with no slot named resolves to the item's own slot"), Pawn.Equipment->Equip(Pawn.Inventory, EntryId, EEquipSlot::None));

	TestTrue(TEXT("The main hand is now occupied"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::MainHand));

	// a pawn wears one of a thing, whatever the stack it came from held
	TestEqual(TEXT("Exactly one unit is worn"), Pawn.Equipment->GetEquippedItem(EEquipSlot::MainHand).Quantity, 1);

	if (TestEqual(TEXT("The stack is still in the grid"), Pawn.Inventory->GetEntries().Num(), 1))
	{
		TestEqual(TEXT("...with the other two still in it"), Pawn.Inventory->GetEntries()[0].Item.Quantity, 2);
	}

	// worn weight is reported separately from the grid's - nothing folds the two together yet
	TestEqual(TEXT("Worn weight counts the one worn unit"), Pawn.Equipment->GetTotalWeight(), 1.0f);
	TestEqual(TEXT("The grid's weight counts only what is still carried"), Pawn.Inventory->GetTotalWeight(), 2.0f);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentWrongSlotTest,
	"Smores.Items.Equipment.EquipRefusesWrongSlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentWrongSlotTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 3.0f, 10, EEquipSlot::MainHand);

	TestTrue(TEXT("The sword was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Sword), FIntPoint(0, 0), false));

	const int32 EntryId = GetOnlyEntryId(Pawn.Inventory);
	const FInventorySnapshot Before(Pawn.Inventory);

	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	// a drop on a specific paperdoll slot names one, and still has to match
	const bool bEquipped = Pawn.Equipment->EquipWithReason(Pawn.Inventory, EntryId, EEquipSlot::Head, Reason);

	TestFalse(TEXT("Equipping into a mismatched slot is refused"), bEquipped);
	TestTrue(TEXT("...and says why: the item isn't worn there"), Reason == ESmoresRefusalReason::WrongSlot);

	TestFalse(TEXT("Nothing is worn"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::Head));
	TestFalse(TEXT("...in either slot"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::MainHand));
	TestTrue(TEXT("...and the grid is untouched"), Before == FInventorySnapshot(Pawn.Inventory));

	// the plain forwarder must agree
	TestFalse(TEXT("The plain Equip forwarder refuses it too"), Pawn.Equipment->Equip(Pawn.Inventory, EntryId, EEquipSlot::Head));

	// a stale entry id is a bug or a race, not something to tell the player about
	ESmoresRefusalReason StaleReason = ESmoresRefusalReason::WrongSlot;

	TestFalse(TEXT("An unknown entry id is refused"), Pawn.Equipment->EquipWithReason(Pawn.Inventory, 9999, EEquipSlot::MainHand, StaleReason));
	TestTrue(TEXT("...silently, because the player didn't do anything wrong"), StaleReason == ESmoresRefusalReason::None);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentAllOrNothingSwapTest,
	"Smores.Items.Equipment.SwapWithNoRoomChangesNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentAllOrNothingSwapTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// start roomy enough to get the first item worn, then shrink the grid so the displaced item
	// has nowhere to go
	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	// the worn item is 2x1; the grid it would have to return to ends up 1x1
	UItemDefinition* Greatsword = MakeTestItemDefinition(TestWorld, FIntPoint(2, 1), 1, 6.0f, 40, EEquipSlot::MainHand);
	UItemDefinition* Knife = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 1.0f, 5, EEquipSlot::MainHand);

	TestTrue(TEXT("The greatsword was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Greatsword), FIntPoint(0, 0), false));
	TestTrue(TEXT("The greatsword was equipped"), Pawn.Equipment->Equip(Pawn.Inventory, GetOnlyEntryId(Pawn.Inventory), EEquipSlot::MainHand));

	TestEqual(TEXT("The grid is empty again"), Pawn.Inventory->GetEntries().Num(), 0);

	// one cell, holding the knife - so the greatsword can never come back
	TestTrue(TEXT("The grid was shrunk to a single cell"), Pawn.Inventory->SetGridSize(1, 1));
	TestTrue(TEXT("The knife fills it"), Pawn.Inventory->AddItemAt(MakeTestItem(Knife), FIntPoint(0, 0), false));

	const int32 KnifeEntryId = GetOnlyEntryId(Pawn.Inventory);
	const FInventorySnapshot Before(Pawn.Inventory);

	USmoresTestDelegateListener* Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	Pawn.Equipment->OnEquipmentChanged.AddDynamic(Listener, &USmoresTestDelegateListener::OnChanged);

	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;
	const bool bEquipped = Pawn.Equipment->EquipWithReason(Pawn.Inventory, KnifeEntryId, EEquipSlot::MainHand, Reason);

	// three things together, and all three matter - a swap that returned false having already
	// unequipped the greatsword would have destroyed it
	TestFalse(TEXT("The swap is refused when the displaced item has nowhere to go"), bEquipped);
	TestTrue(TEXT("...naming NoRoom rather than WrongSlot"), Reason == ESmoresRefusalReason::NoRoom);

	TestTrue(TEXT("The original item is still worn"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::MainHand));
	TestTrue(TEXT("...and it is still the greatsword"), Pawn.Equipment->GetEquippedItem(EEquipSlot::MainHand).Definition == Greatsword);

	TestTrue(*FString::Printf(TEXT("The grid is untouched. Before %s, after %s"), *Before.ToString(), *FInventorySnapshot(Pawn.Inventory).ToString()),
		Before == FInventorySnapshot(Pawn.Inventory));

	TestEqual(TEXT("Nothing was broadcast"), Listener->CallCount, 0);

	// the plain forwarder agrees
	TestFalse(TEXT("The plain Equip forwarder refuses it too"), Pawn.Equipment->Equip(Pawn.Inventory, KnifeEntryId, EEquipSlot::MainHand));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentSuccessfulSwapTest,
	"Smores.Items.Equipment.SwapReturnsDisplacedItemToTheGrid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentSuccessfulSwapTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 3.0f, 10, EEquipSlot::MainHand);
	UItemDefinition* Axe = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 4.0f, 12, EEquipSlot::MainHand);

	TestTrue(TEXT("The sword was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Sword), FIntPoint(0, 0), false));
	TestTrue(TEXT("The sword was equipped"), Pawn.Equipment->Equip(Pawn.Inventory, GetOnlyEntryId(Pawn.Inventory), EEquipSlot::MainHand));

	TestTrue(TEXT("The axe was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Axe), FIntPoint(0, 0), false));

	// with room to spare, the swap lands: the axe is worn and the sword comes back to the grid
	TestTrue(TEXT("The axe was equipped over the sword"), Pawn.Equipment->Equip(Pawn.Inventory, GetOnlyEntryId(Pawn.Inventory), EEquipSlot::MainHand));

	TestTrue(TEXT("The axe is now worn"), Pawn.Equipment->GetEquippedItem(EEquipSlot::MainHand).Definition == Axe);

	if (TestEqual(TEXT("The displaced sword is back in the grid"), Pawn.Inventory->GetEntries().Num(), 1))
	{
		TestTrue(TEXT("...and it is the sword"), Pawn.Inventory->GetEntries()[0].Item.Definition == Sword);
	}

	TestTrue(TEXT("Only one item is worn in that slot"), Pawn.Equipment->GetEquippedItems().Num() == 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentUnequipTest,
	"Smores.Items.Equipment.UnequipNeedsRoomInTheGrid",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentUnequipTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestPawnComponents Pawn = MakeTestPawn(TestWorld, 4, 4);

	if (!TestTrue(TEXT("The test pawn has both components"), Pawn.IsValid()))
	{
		return true;
	}

	UItemDefinition* Greatsword = MakeTestItemDefinition(TestWorld, FIntPoint(2, 1), 1, 6.0f, 40, EEquipSlot::MainHand);
	UItemDefinition* Knife = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 1.0f, 5, EEquipSlot::MainHand);

	TestTrue(TEXT("The greatsword was placed"), Pawn.Inventory->AddItemAt(MakeTestItem(Greatsword), FIntPoint(0, 0), false));
	TestTrue(TEXT("The greatsword was equipped"), Pawn.Equipment->Equip(Pawn.Inventory, GetOnlyEntryId(Pawn.Inventory), EEquipSlot::MainHand));

	// fill the grid so there is nowhere to take it off to
	TestTrue(TEXT("The grid was shrunk to a single cell"), Pawn.Inventory->SetGridSize(1, 1));
	TestTrue(TEXT("The knife fills it"), Pawn.Inventory->AddItemAt(MakeTestItem(Knife), FIntPoint(0, 0), false));

	const FInventorySnapshot Before(Pawn.Inventory);

	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	// taking it off into nothing would destroy it, so it stays worn
	TestFalse(TEXT("Unequipping into a full grid is refused"), Pawn.Equipment->UnequipWithReason(EEquipSlot::MainHand, Pawn.Inventory, Reason));
	TestTrue(TEXT("...naming NoRoom"), Reason == ESmoresRefusalReason::NoRoom);
	TestTrue(TEXT("The item is still worn"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::MainHand));
	TestTrue(TEXT("...and the grid is untouched"), Before == FInventorySnapshot(Pawn.Inventory));

	TestFalse(TEXT("The plain Unequip forwarder refuses it too"), Pawn.Equipment->Unequip(EEquipSlot::MainHand, Pawn.Inventory));

	// give it room, and it comes off
	TestTrue(TEXT("The grid was grown again"), Pawn.Inventory->SetGridSize(4, 4));
	TestTrue(TEXT("Unequipping now succeeds"), Pawn.Equipment->Unequip(EEquipSlot::MainHand, Pawn.Inventory));

	TestFalse(TEXT("The slot is empty"), Pawn.Equipment->IsSlotOccupied(EEquipSlot::MainHand));
	TestEqual(TEXT("The grid holds the knife and the greatsword"), Pawn.Inventory->GetEntries().Num(), 2);
	TestEqual(TEXT("Nothing is worn any more"), Pawn.Equipment->GetTotalWeight(), 0.0f);

	// an empty slot has nothing to take off, and nothing to say about it
	ESmoresRefusalReason EmptyReason = ESmoresRefusalReason::NoRoom;

	TestFalse(TEXT("Unequipping an empty slot is refused"), Pawn.Equipment->UnequipWithReason(EEquipSlot::Head, Pawn.Inventory, EmptyReason));
	TestTrue(TEXT("...silently"), EmptyReason == ESmoresRefusalReason::None);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
