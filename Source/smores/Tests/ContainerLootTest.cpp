// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "InventoryComponent.h"
#include "LootTableDefinition.h"
#include "WorldSeedComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"
#include "Engine/World.h"

/**
 *  A container rolling its loot table at BeginPlay: seeded from the world seed plus its own placed
 *  key, stocked on top of its hand-authored StartingItems, and refused whole when the roll doesn't
 *  fit.
 *
 *  Lives in the smores module rather than SmoresItems because AStrategyContainer is abstract and
 *  its concrete stand-in lives here - see SmoresStrategyTestActors.h. The roll itself is tested
 *  in SmoresItems' LootTableTest.cpp; this is only the wiring a real chest adds on top of it.
 *
 *  Every test begins play first: a container spawned into a world that hasn't begun play never
 *  gets a BeginPlay of its own, and so would never roll at all - which would make "these two
 *  containers hold the same thing" pass on two empty grids.
 */

/** Spawns the container stand-in authored the way a placed chest is, and lets it begin play */
static ATestStrategyContainer* SmoresContainerLootTest_Spawn(FSmoresTestWorld& TestWorld, ULootTableDefinition* Table,
	const FGuid& Key, const TArray<FInventoryItem>& StartingItems = TArray<FInventoryItem>(), FIntPoint GridSize = FIntPoint(8, 8))
{
	UWorld* World = TestWorld.GetWorld();

	if (!World)
	{
		return nullptr;
	}

	ATestStrategyContainer* Container = World->SpawnActorDeferred<ATestStrategyContainer>(ATestStrategyContainer::StaticClass(),
		FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Container)
	{
		return nullptr;
	}

	Container->SetContentsForTest(Table, Key, StartingItems);
	Container->GetInventory()->SetGridSize(GridSize.X, GridSize.Y);
	Container->FinishSpawning(FTransform::Identity);

	return Container;
}

/** A table with enough variety that two different seeds landing on the same contents by chance is vanishingly unlikely */
static ULootTableDefinition* SmoresContainerLootTest_VariedTable(FSmoresTestWorld& TestWorld)
{
	ULootTableDefinition* Table = MakeTestLootTable(TestWorld, 2, 4);

	if (Table)
	{
		for (int32 Index = 0; Index < 5; ++Index)
		{
			AddTestLootItem(Table, MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 50), 1, 1, 9);
		}
	}

	return Table;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresContainerLootSameKeyTest,
	"Smores.Strategy.ContainerLoot.SameKeyRollsTheSameContents",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresContainerLootSameKeyTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	if (!TestTrue(TEXT("The world began play"), TestWorld.BeginPlay()))
	{
		return true;
	}

	// BeginPlay brings the real BP_StrategyGameState up, and the seed component with it - set the
	// seed explicitly so nothing here depends on what the Blueprint authors
	UWorldSeedComponent* Seed = UWorldSeedComponent::Get(TestWorld.GetWorld());

	if (!TestNotNull(TEXT("The GameState carries a world seed component"), Seed))
	{
		return true;
	}

	Seed->SetWorldSeed(1234);

	ULootTableDefinition* Table = SmoresContainerLootTest_VariedTable(TestWorld);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	const FGuid Key(0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD);

	ATestStrategyContainer* First = SmoresContainerLootTest_Spawn(TestWorld, Table, Key);
	ATestStrategyContainer* Second = SmoresContainerLootTest_Spawn(TestWorld, Table, Key);

	if (!TestNotNull(TEXT("First container spawned"), First) || !TestNotNull(TEXT("Second container spawned"), Second))
	{
		return true;
	}

	const FString Contents = DescribePlacements(First->GetInventory());

	TestFalse(TEXT("The container rolled its table at BeginPlay"), Contents.IsEmpty());

	// the same chest in the same campaign - a reload, played out inside one session
	TestEqual(TEXT("The same key in the same world rolls the same contents, placed the same way"),
		DescribePlacements(Second->GetInventory()), Contents);

	bool bAnotherKeyDiffers = false;

	for (uint32 Other = 1; Other <= 6 && !bAnotherKeyDiffers; ++Other)
	{
		ATestStrategyContainer* Elsewhere = SmoresContainerLootTest_Spawn(TestWorld, Table, FGuid(Other, 0, 0, 0));

		bAnotherKeyDiffers = Elsewhere && DescribePlacements(Elsewhere->GetInventory()) != Contents;
	}

	TestTrue(TEXT("A different chest with the same table holds something different"), bAnotherKeyDiffers);

	bool bAnotherCampaignDiffers = false;

	for (int32 OtherSeed = 1; OtherSeed <= 6 && !bAnotherCampaignDiffers; ++OtherSeed)
	{
		Seed->SetWorldSeed(1234 + OtherSeed);

		ATestStrategyContainer* Replayed = SmoresContainerLootTest_Spawn(TestWorld, Table, Key);

		bAnotherCampaignDiffers = Replayed && DescribePlacements(Replayed->GetInventory()) != Contents;
	}

	TestTrue(TEXT("The same chest in a different campaign holds something different"), bAnotherCampaignDiffers);

	// StartingItems and the table coexist: the authored item is there, and so is the roll on top
	Seed->SetWorldSeed(1234);

	UItemDefinition* Letter = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1));

	ATestStrategyContainer* Authored = SmoresContainerLootTest_Spawn(TestWorld, Table, Key, { MakeTestItem(Letter) });

	if (TestNotNull(TEXT("Authored container spawned"), Authored))
	{
		const UInventoryComponent* Inventory = Authored->GetInventory();

		const bool bHasLetter = Inventory->GetEntries().ContainsByPredicate([Letter](const FInventoryEntry& Entry)
		{
			return Entry.Item.Definition == Letter;
		});

		TestTrue(TEXT("A container with StartingItems still holds them"), bHasLetter);
		TestEqual(TEXT("...alongside the whole roll"), GetTotalQuantity(Inventory), GetTotalQuantity(First->GetInventory()) + 1);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresContainerLootNoRoomTest,
	"Smores.Strategy.ContainerLoot.RollThatDoesNotFitAddsNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresContainerLootNoRoomTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	if (!TestTrue(TEXT("The world began play"), TestWorld.BeginPlay()))
	{
		return true;
	}

	UItemDefinition* Keepsake = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1));
	UItemDefinition* Coin = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);

	// forty coins, every time, stacking ten to a cell - four cells' worth. Beside the keepsake a 2x2
	// grid has three cells free, so AddItem would have kept thirty and dropped ten. All-or-nothing
	// keeps none, which is the point: a short chest nobody notices is worse than an empty one that warns.
	ULootTableDefinition* Table = MakeTestLootTable(TestWorld, 1, 1);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	AddTestLootItem(Table, Coin, 1, 40, 40);

	AddExpectedMessagePlain(TEXT("none of them were added"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("couldn't fit them all"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	ATestStrategyContainer* Cramped = SmoresContainerLootTest_Spawn(TestWorld, Table, FGuid(1, 1, 1, 1), { MakeTestItem(Keepsake) }, FIntPoint(2, 2));

	if (TestNotNull(TEXT("Container spawned"), Cramped))
	{
		const UInventoryComponent* Inventory = Cramped->GetInventory();

		TestEqual(TEXT("A roll that only partly fits adds nothing - not thirty of the forty coins, none"), Inventory->GetEntries().Num(), 1);

		if (Inventory->GetEntries().Num() == 1)
		{
			TestTrue(TEXT("...so the StartingItems are all that's there"), Inventory->GetEntries()[0].Item.Definition == Keepsake);
		}
	}

	// the same roll with room for it lands whole, split across four stacks
	ATestStrategyContainer* Roomy = SmoresContainerLootTest_Spawn(TestWorld, Table, FGuid(1, 1, 1, 1), { MakeTestItem(Keepsake) }, FIntPoint(3, 3));

	if (TestNotNull(TEXT("Roomy container spawned"), Roomy))
	{
		TestEqual(TEXT("With room, all forty coins and the keepsake are there"), GetTotalQuantity(Roomy->GetInventory()), 41);
	}

	// a container with a table and no key still rolls - from its actor name - and says so
	AddExpectedMessagePlain(TEXT("no PlacedContainerId"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	ATestStrategyContainer* Unkeyed = SmoresContainerLootTest_Spawn(TestWorld, Table, FGuid());

	if (TestNotNull(TEXT("Unkeyed container spawned"), Unkeyed))
	{
		TestEqual(TEXT("An unkeyed container still rolls its table"), GetTotalQuantity(Unkeyed->GetInventory()), 40);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
