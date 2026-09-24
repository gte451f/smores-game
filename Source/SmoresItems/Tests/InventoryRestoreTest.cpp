// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EquipmentComponent.h"
#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Restoring a stored grid and paperdoll - how a character record hands its carried and worn
 *  items back to the actor standing in for it (see game-data.md).
 *
 *  Both restores return a count rather than a bool for the reason testing.md's first rule gives:
 *  a restore that dropped one bad entry still "worked", and the caller has to be able to tell.
 *  So each test asserts the partial case specifically, and that what was dropped is exactly what
 *  should have been.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresInventoryRestoreEntriesTest,
	"Smores.Items.Inventory.RestoreEntriesKeepsIdsAndDropsWhatDoesntFit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryRestoreEntriesTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);
	USmoresTestDelegateListener* Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();

	if (!TestNotNull(TEXT("Inventory created"), Inventory) || !TestNotNull(TEXT("Listener created"), Listener))
	{
		return true;
	}

	Inventory->OnInventoryChanged.AddDynamic(Listener, &USmoresTestDelegateListener::OnChanged);

	UItemDefinition* Small = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), /*MaxStack*/ 5);
	UItemDefinition* Wide = MakeTestItemDefinition(TestWorld, FIntPoint(2, 1), 1);

	// something already in the grid, which the restore has to replace rather than add to
	TestTrue(TEXT("A pre-existing entry was placed"), Inventory->AddItemAt(MakeTestItem(Small), FIntPoint(3, 3), false));

	TArray<FInventoryEntry> Stored;
	Stored.Emplace(7, MakeTestItem(Small, 3), FIntPoint(0, 0), false);			// fine
	Stored.Emplace(12, MakeTestItem(Wide), FIntPoint(0, 1), true);				// fine, rotated to 1x2
	Stored.Emplace(20, MakeTestItem(Small), FIntPoint(0, 2), false);			// overlaps the rotated wide item
	Stored.Emplace(7, MakeTestItem(Small), FIntPoint(3, 0), false);				// duplicate id
	Stored.Emplace(21, MakeTestItem(Wide), FIntPoint(3, 3), false);				// falls off the right edge
	Stored.Emplace(22, FInventoryItem(), FIntPoint(2, 2), false);				// empty item
	Stored.Emplace(9, MakeTestItem(Small, 99), FIntPoint(2, 0), false);			// fine, quantity over the cap

	AddExpectedMessagePlain(TEXT("RestoreEntries on"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 4);

	Listener->Reset();

	const int32 Restored = Inventory->RestoreEntries(Stored);

	TestEqual(TEXT("Three of the seven stored entries landed"), Restored, 3);
	TestEqual(TEXT("...and the grid holds exactly those - the pre-existing entry is gone"), Inventory->GetEntries().Num(), 3);
	TestEqual(TEXT("The restore broadcast once"), Listener->CallCount, 1);

	const FInventoryEntry First = Inventory->GetEntry(7);
	const FInventoryEntry Rotated = Inventory->GetEntry(12);
	const FInventoryEntry Capped = Inventory->GetEntry(9);

	TestTrue(TEXT("Entry 7 kept its id"), First.IsValidEntry());
	TestEqual(TEXT("...its anchor"), First.AnchorCell, FIntPoint(0, 0));
	TestEqual(TEXT("...and its quantity"), First.Item.Quantity, 3);
	TestTrue(TEXT("Entry 12 kept its rotation"), Rotated.IsValidEntry() && Rotated.bRotated);
	TestEqual(TEXT("A stored quantity over the cap is clamped to it"), Capped.Item.Quantity, 5);
	TestFalse(TEXT("The overlapping entry was dropped"), Inventory->GetEntry(20).IsValidEntry());
	TestFalse(TEXT("The off-grid entry was dropped"), Inventory->GetEntry(21).IsValidEntry());

	// the id counter has to move past everything restored, or the next add collides with entry 12
	TestTrue(TEXT("A new item still places after the restore"), Inventory->AddItemAt(MakeTestItem(Wide), FIntPoint(1, 3), false));

	const int32 NewId = Inventory->GetEntries().Last().EntryId;

	TestTrue(TEXT("...with an id none of the restored entries hold"), NewId != 7 && NewId != 12 && NewId != 9);
	TestTrue(TEXT("...above the highest restored id"), NewId > 12);

	TestEqual(TEXT("Restoring an empty set empties the grid"), Inventory->RestoreEntries({}), 0);
	TestEqual(TEXT("...and leaves nothing placed"), Inventory->GetEntries().Num(), 0);

	// the gate's refusing branch: an ownerless component has no authority (testing.md's one
	// sanctioned exception - not a client, but it proves the gate refuses rather than merely exists)
	UInventoryComponent* Ownerless = NewObject<UInventoryComponent>(GetTransientPackage());
	TestWorld.KeepAlive(Ownerless);

	TestEqual(TEXT("Off-authority, RestoreEntries refuses with INDEX_NONE"), Ownerless->RestoreEntries(Stored), INDEX_NONE);
	TestEqual(TEXT("...having placed nothing"), Ownerless->GetEntries().Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresEquipmentRestoreTest,
	"Smores.Items.Equipment.RestoreEquippedItemsDropsWhatCantBeWorn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresEquipmentRestoreTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UEquipmentComponent* Equipment = TestWorld.SpawnComponent<UEquipmentComponent>();

	if (!TestNotNull(TEXT("Equipment created"), Equipment))
	{
		return true;
	}

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), 1, 0.0f, 0, EEquipSlot::MainHand);
	UItemDefinition* Helmet = MakeTestItemDefinition(TestWorld, FIntPoint(2, 2), 1, 0.0f, 0, EEquipSlot::Head);
	UItemDefinition* Knife = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 4, 0.0f, 0, EEquipSlot::MainHand);

	TArray<FEquippedItem> Stored;
	Stored.Emplace(EEquipSlot::MainHand, MakeTestItem(Sword));
	Stored.Emplace(EEquipSlot::Body, MakeTestItem(Helmet));			// not worn there
	Stored.Emplace(EEquipSlot::MainHand, MakeTestItem(Knife));		// slot already restored
	Stored.Emplace(EEquipSlot::Head, MakeTestItem(Helmet, 3));		// fine, quantity forced to 1
	Stored.Emplace(EEquipSlot::Feet, FInventoryItem());				// empty

	AddExpectedMessagePlain(TEXT("RestoreEquippedItems on"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 3);

	TestEqual(TEXT("Two of the five stored items are worn"), Equipment->RestoreEquippedItems(Stored), 2);
	TestTrue(TEXT("The first main-hand item won"), Equipment->GetEquippedItem(EEquipSlot::MainHand).Definition == Sword);
	TestTrue(TEXT("The helmet is on the head"), Equipment->GetEquippedItem(EEquipSlot::Head).Definition == Helmet);
	TestEqual(TEXT("...as a single one, whatever the stored quantity said"), Equipment->GetEquippedItem(EEquipSlot::Head).Quantity, 1);
	TestFalse(TEXT("Nothing was forced onto the body"), Equipment->IsSlotOccupied(EEquipSlot::Body));

	UEquipmentComponent* Ownerless = NewObject<UEquipmentComponent>(GetTransientPackage());
	TestWorld.KeepAlive(Ownerless);

	TestEqual(TEXT("Off-authority, RestoreEquippedItems refuses with INDEX_NONE"), Ownerless->RestoreEquippedItems(Stored), INDEX_NONE);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
