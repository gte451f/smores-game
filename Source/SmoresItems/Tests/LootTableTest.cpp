// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "ItemModifierDefinition.h"
#include "LootTableDefinition.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Loot tables: how weights pick, how many picks a roll makes, how nesting descends and where it
 *  gives up, how a Tag entry resolves, how modifier pools choose, and - the one that matters most
 *  - that the same seed and id always roll the same thing.
 *
 *  These exercise UWeightedTableDefinition (SmoresCore) through its only concrete subclass. The
 *  base is abstract and knows nothing about items; testing it through ULootTableDefinition tests
 *  what a real roll actually does, which a test-only subclass wouldn't.
 *
 *  Every roll here draws from a fixed FRandomStream, so the distribution checks are not flaky:
 *  each is one fixed outcome, and the tolerance only has to be wide enough that a correct change
 *  to how the stream is consumed doesn't break it.
 */

/** No Tag entries in the table, so nothing for one to choose among */
static const TArray<UItemDefinition*> SmoresLootTest_NoCandidates;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableWeightsTest,
	"Smores.Items.LootTable.WeightsShapeTheDistribution",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableWeightsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Common = MakeTestItemDefinition(TestWorld);
	UItemDefinition* Rare = MakeTestItemDefinition(TestWorld);
	UItemDefinition* Disabled = MakeTestItemDefinition(TestWorld);

	ULootTableDefinition* Table = MakeTestLootTable(TestWorld);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	AddTestLootItem(Table, Common, 3);
	AddTestLootItem(Table, Rare, 1);
	AddTestLootItem(Table, Disabled, 0);

	TestEqual(TEXT("A zero weight adds nothing to the total"), Table->GetTotalWeight(), 4);

	FRandomStream Stream(1234);

	const int32 Rolls = 4000;
	int32 CommonCount = 0;
	int32 RareCount = 0;
	int32 DisabledCount = 0;

	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		TArray<FInventoryItem> Items;
		Table->RollLoot(Stream, SmoresLootTest_NoCandidates, Items);

		for (const FInventoryItem& Item : Items)
		{
			CommonCount += (Item.Definition == Common) ? 1 : 0;
			RareCount += (Item.Definition == Rare) ? 1 : 0;
			DisabledCount += (Item.Definition == Disabled) ? 1 : 0;
		}
	}

	TestEqual(TEXT("Every one-pick roll produced exactly one item"), CommonCount + RareCount + DisabledCount, Rolls);
	TestEqual(TEXT("A weight-zero entry is never picked"), DisabledCount, 0);

	// 3 : 1 is 3000 : 1000 over 4000 rolls. The standard deviation is about 27, so +-200 is far
	// outside chance - a failure here means the weights are being read wrongly, not bad luck
	TestTrue(FString::Printf(TEXT("A weight of 3 against 1 lands about three times in four (%d of %d)"), CommonCount, Rolls),
		CommonCount > 2800 && CommonCount < 3200);

	// a table nothing in which has any weight picks nothing at all, rather than its first entry
	ULootTableDefinition* Weightless = MakeTestLootTable(TestWorld, 3, 3);

	if (TestNotNull(TEXT("Weightless table created"), Weightless))
	{
		AddTestLootItem(Weightless, Common, 0);

		TestEqual(TEXT("A table with no weight picks no entry"), Weightless->PickEntry(Stream), static_cast<int32>(INDEX_NONE));

		TArray<FInventoryItem> Items;
		Weightless->RollLoot(Stream, SmoresLootTest_NoCandidates, Items);

		TestEqual(TEXT("...and rolls nothing"), Items.Num(), 0);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableRollCountTest,
	"Smores.Items.LootTable.RollCountAndQuantityStayInRange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableRollCountTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Item = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10);
	ULootTableDefinition* Table = MakeTestLootTable(TestWorld, 2, 4);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	AddTestLootItem(Table, Item, 1, 2, 5);

	FRandomStream Stream(99);

	TSet<int32> PickCountsSeen;
	TSet<int32> QuantitiesSeen;
	bool bPickCountInRange = true;
	bool bQuantityInRange = true;

	for (int32 Roll = 0; Roll < 500; ++Roll)
	{
		TArray<FInventoryItem> Items;
		Table->RollLoot(Stream, SmoresLootTest_NoCandidates, Items);

		PickCountsSeen.Add(Items.Num());
		bPickCountInRange &= Items.Num() >= 2 && Items.Num() <= 4;

		for (const FInventoryItem& Rolled : Items)
		{
			QuantitiesSeen.Add(Rolled.Quantity);
			bQuantityInRange &= Rolled.Quantity >= 2 && Rolled.Quantity <= 5;
		}
	}

	TestTrue(TEXT("Every roll made between MinRolls and MaxRolls picks"), bPickCountInRange);
	TestTrue(TEXT("...and both ends of the range, inclusive, actually occur"), PickCountsSeen.Contains(2) && PickCountsSeen.Contains(4));
	TestTrue(TEXT("Every quantity lay between MinQuantity and MaxQuantity"), bQuantityInRange);
	TestTrue(TEXT("...and both ends of that range occur too"), QuantitiesSeen.Contains(2) && QuantitiesSeen.Contains(5));

	// a Max below Min is an authoring slip, read as Min rather than as "no picks"
	Table->MinRolls = 3;
	Table->MaxRolls = 1;

	TArray<FInventoryItem> Inverted;
	Table->RollLoot(Stream, SmoresLootTest_NoCandidates, Inverted);

	TestEqual(TEXT("MaxRolls below MinRolls reads as MinRolls"), Inverted.Num(), 3);

	// zero is a real roll count: "this table sometimes yields nothing"
	Table->MinRolls = 0;
	Table->MaxRolls = 0;

	TArray<FInventoryItem> None;
	Table->RollLoot(Stream, SmoresLootTest_NoCandidates, None);

	TestEqual(TEXT("A roll count of zero yields nothing"), None.Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableNestingTest,
	"Smores.Items.LootTable.SubTablesRollInFull",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableNestingTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Junk = MakeTestItemDefinition(TestWorld);
	UItemDefinition* Coin = MakeTestItemDefinition(TestWorld);

	// the inner table always makes two picks of its own, whatever the table naming it says
	ULootTableDefinition* Inner = MakeTestLootTable(TestWorld, 2, 2);
	ULootTableDefinition* Outer = MakeTestLootTable(TestWorld, 3, 3);

	if (!TestNotNull(TEXT("Inner table created"), Inner) || !TestNotNull(TEXT("Outer table created"), Outer))
	{
		return true;
	}

	AddTestLootItem(Inner, Junk);
	AddTestLootSubTable(Outer, Inner);

	FRandomStream Stream(7);

	TArray<FInventoryItem> Items;
	TArray<FName> Sources;

	TestTrue(TEXT("A nested roll completes"), Outer->RollLoot(Stream, SmoresLootTest_NoCandidates, Items, &Sources));

	TestEqual(TEXT("Three outer picks of a two-pick table yield six items"), Items.Num(), 6);
	TestEqual(TEXT("...with one source per item"), Sources.Num(), Items.Num());

	bool bAllFromInner = true;

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		bAllFromInner &= Items[Index].Definition == Junk && Sources[Index] == Inner->DefinitionId;
	}

	TestTrue(TEXT("Each came from the inner table, and says so"), bAllFromInner);

	// a sibling item entry sits alongside a sub-table exactly as a second item entry would
	AddTestLootItem(Outer, Coin);

	TSet<FName> SourcesSeen;

	for (int32 Roll = 0; Roll < 100; ++Roll)
	{
		TArray<FInventoryItem> Mixed;
		TArray<FName> MixedSources;
		Outer->RollLoot(Stream, SmoresLootTest_NoCandidates, Mixed, &MixedSources);

		SourcesSeen.Append(MixedSources);
	}

	TestTrue(TEXT("A table mixing a sub-table and an item yields from both"),
		SourcesSeen.Contains(Inner->DefinitionId) && SourcesSeen.Contains(Outer->DefinitionId));

	// a Nothing entry is weight given to an empty pick
	ULootTableDefinition* Empty = MakeTestLootTable(TestWorld, 5, 5);

	if (TestNotNull(TEXT("Empty table created"), Empty))
	{
		AddTestLootEntry(Empty, ELootEntryKind::Nothing);

		TArray<FInventoryItem> Nothing;
		Empty->RollLoot(Stream, SmoresLootTest_NoCandidates, Nothing);

		TestEqual(TEXT("A Nothing entry yields nothing, however often it is picked"), Nothing.Num(), 0);
	}

	// two branches naming one shared table is a diamond, not a loop - it ends
	ULootTableDefinition* Left = MakeTestLootTable(TestWorld);
	ULootTableDefinition* Right = MakeTestLootTable(TestWorld);
	ULootTableDefinition* Top = MakeTestLootTable(TestWorld);

	if (Left && Right && Top)
	{
		AddTestLootSubTable(Left, Inner);
		AddTestLootSubTable(Right, Inner);
		AddTestLootSubTable(Top, Left);
		AddTestLootSubTable(Top, Right);

		TestFalse(TEXT("A table shared by two branches is not a nesting loop"), Top->ContainsNestingLoop());
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableSelfNestingTest,
	"Smores.Items.LootTable.SelfNestingStopsAtTheDepthLimit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableSelfNestingTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Item = MakeTestItemDefinition(TestWorld);

	// names itself as its only entry, four picks a level - left alone this would roll forever,
	// and even stopping each branch at the depth limit would cost 4^8 picks
	ULootTableDefinition* Loop = MakeTestLootTable(TestWorld, 4, 4);

	// and two tables naming each other, which is the same loop one step removed
	ULootTableDefinition* Ping = MakeTestLootTable(TestWorld);
	ULootTableDefinition* Pong = MakeTestLootTable(TestWorld);

	if (!TestNotNull(TEXT("Tables created"), Loop) || !Ping || !Pong)
	{
		return true;
	}

	AddTestLootSubTable(Loop, Loop);

	AddTestLootItem(Ping, Item);
	AddTestLootSubTable(Ping, Pong);
	AddTestLootSubTable(Pong, Ping);

	TestTrue(TEXT("A table naming itself is a nesting loop"), Loop->ContainsNestingLoop());
	TestTrue(TEXT("...and so are two tables naming each other"), Ping->ContainsNestingLoop());

	AddExpectedMessagePlain(TEXT("nests more than"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);

	FRandomStream Stream(3);

	TArray<FInventoryItem> Items;

	TestFalse(TEXT("A roll of a self-nesting table reports that it was abandoned"), Loop->RollLoot(Stream, SmoresLootTest_NoCandidates, Items));
	TestEqual(TEXT("...having yielded nothing, since it never reached a payload"), Items.Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableTagTest,
	"Smores.Items.LootTable.TagEntryPicksOnlyTaggedItems",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableTagTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FGameplayTag Mineral = GetTestMineralTag();

	if (!TestTrue(TEXT("Item.Mineral is defined in Config/DefaultGameplayTags.ini"), Mineral.IsValid()))
	{
		return true;
	}

	UItemDefinition* Ore = MakeTestItemDefinition(TestWorld);
	UItemDefinition* Salt = MakeTestItemDefinition(TestWorld);
	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld);

	Ore->Tags.AddTag(Mineral);
	Salt->Tags.AddTag(Mineral);

	ULootTableDefinition* Table = MakeTestLootTable(TestWorld);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	FLootTableEntry& TagEntry = AddTestLootEntry(Table, ELootEntryKind::Tag);
	TagEntry.Tag = Mineral;

	const TArray<UItemDefinition*> Candidates = { Apple, Salt, Ore };

	FRandomStream Stream(11);

	int32 OreCount = 0;
	int32 SaltCount = 0;
	int32 AppleCount = 0;

	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		TArray<FInventoryItem> Items;
		Table->RollLoot(Stream, Candidates, Items);

		for (const FInventoryItem& Item : Items)
		{
			OreCount += (Item.Definition == Ore) ? 1 : 0;
			SaltCount += (Item.Definition == Salt) ? 1 : 0;
			AppleCount += (Item.Definition == Apple) ? 1 : 0;
		}
	}

	TestEqual(TEXT("An untagged item is never picked by a Tag entry"), AppleCount, 0);
	TestTrue(TEXT("...and every tagged one is"), OreCount > 0 && SaltCount > 0);
	TestEqual(TEXT("Every roll produced one item"), OreCount + SaltCount, 200);

	// the Asset Manager hands back candidates in scan order, which isn't stable - the same seed has
	// to land on the same item whichever order they arrive in
	const TArray<UItemDefinition*> Reversed = { Ore, Salt, Apple };

	FRandomStream First(5);
	FRandomStream Second(5);

	TArray<FInventoryItem> FromFirst;
	TArray<FInventoryItem> FromSecond;

	for (int32 Roll = 0; Roll < 20; ++Roll)
	{
		Table->RollLoot(First, Candidates, FromFirst);
		Table->RollLoot(Second, Reversed, FromSecond);
	}

	TestEqual(TEXT("Candidate order doesn't change what a seed rolls"), DescribeRolledItems(FromSecond), DescribeRolledItems(FromFirst));

	// a tag nothing carries is an authoring slip, not a crash: nothing, and a warning
	AddExpectedMessagePlain(TEXT("that no item carries"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	const TArray<UItemDefinition*> OnlyApple = { Apple };

	TArray<FInventoryItem> Unmatched;
	Table->RollLoot(Stream, OnlyApple, Unmatched);

	TestEqual(TEXT("A Tag entry no candidate carries yields nothing"), Unmatched.Num(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableModifierPoolTest,
	"Smores.Items.LootTable.ModifierPoolsPickOnePerSlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableModifierPoolTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Spear = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1000);

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"));
	UItemModifierDefinition* Iron = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Iron"));
	UItemModifierDefinition* WellMade = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Well-Made"));

	ULootTableDefinition* Table = MakeTestLootTable(TestWorld);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	// "a spear, bronze or iron, occasionally well-made"
	FLootTableEntry& Entry = AddTestLootItem(Table, Spear);

	FLootModifierPool& MaterialPool = Entry.ModifierPools.AddDefaulted_GetRef();
	AddTestModifierChoice(MaterialPool, Bronze, 1);
	AddTestModifierChoice(MaterialPool, Iron, 1);

	FLootModifierPool& QualityPool = Entry.ModifierPools.AddDefaulted_GetRef();
	AddTestModifierChoice(QualityPool, WellMade, 1);
	AddTestModifierChoice(QualityPool, nullptr, 3);

	FRandomStream Stream(21);

	TArray<FInventoryItem> Rolled;

	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		Table->RollLoot(Stream, SmoresLootTest_NoCandidates, Rolled);
	}

	int32 BronzeCount = 0;
	int32 IronCount = 0;
	int32 QualityCount = 0;
	bool bExactlyOneMaterial = true;

	for (const FInventoryItem& Item : Rolled)
	{
		BronzeCount += (Item.GetModifier(EItemModifierSlot::Material) == Bronze) ? 1 : 0;
		IronCount += (Item.GetModifier(EItemModifierSlot::Material) == Iron) ? 1 : 0;
		QualityCount += Item.HasModifier(EItemModifierSlot::Quality) ? 1 : 0;

		bExactlyOneMaterial &= Item.HasModifier(EItemModifierSlot::Material) && Item.Modifiers.Num() <= 2;
	}

	TestTrue(TEXT("Every rolled spear came out with one material"), bExactlyOneMaterial);
	TestTrue(TEXT("...bronze and iron both"), BronzeCount > 0 && IronCount > 0);
	TestTrue(TEXT("A quality pool weighted against 'none' lands sometimes, not always"), QualityCount > 0 && QualityCount < Rolled.Num());

	// the stacking consequence the roadmap calls out: a bronze and an iron spear are two piles
	UInventoryComponent* Chest = MakeTestInventory(TestWorld, 4, 4);

	if (TestNotNull(TEXT("Inventory created"), Chest) && TestTrue(TEXT("The whole roll fits"), Chest->AddItemsAllOrNothing(Rolled)))
	{
		TestEqual(TEXT("Two materials times with-and-without quality is four piles, not one"), Chest->GetEntries().Num(), 4);
		TestEqual(TEXT("...holding every spear rolled"), GetTotalQuantity(Chest), Rolled.Num());
	}

	// a later pool is a fallback for an earlier one that came up empty, because AddModifier
	// refuses to fill a slot twice: "bronze half the time, otherwise iron"
	ULootTableDefinition* Fallback = MakeTestLootTable(TestWorld);

	if (TestNotNull(TEXT("Fallback table created"), Fallback))
	{
		FLootTableEntry& FallbackEntry = AddTestLootItem(Fallback, Spear);

		FLootModifierPool& Preferred = FallbackEntry.ModifierPools.AddDefaulted_GetRef();
		AddTestModifierChoice(Preferred, Bronze, 1);
		AddTestModifierChoice(Preferred, nullptr, 1);

		FLootModifierPool& Otherwise = FallbackEntry.ModifierPools.AddDefaulted_GetRef();
		AddTestModifierChoice(Otherwise, Iron, 1);

		TArray<FInventoryItem> FallbackRolled;

		for (int32 Roll = 0; Roll < 100; ++Roll)
		{
			Fallback->RollLoot(Stream, SmoresLootTest_NoCandidates, FallbackRolled);
		}

		int32 FallbackBronze = 0;
		int32 FallbackIron = 0;
		bool bNeverTwoMaterials = true;

		for (const FInventoryItem& Item : FallbackRolled)
		{
			FallbackBronze += (Item.GetModifier(EItemModifierSlot::Material) == Bronze) ? 1 : 0;
			FallbackIron += (Item.GetModifier(EItemModifierSlot::Material) == Iron) ? 1 : 0;
			bNeverTwoMaterials &= Item.Modifiers.Num() == 1;
		}

		TestTrue(TEXT("A second pool filling a filled slot is refused, never stacked on"), bNeverTwoMaterials);
		TestEqual(TEXT("...so every spear is bronze or iron"), FallbackBronze + FallbackIron, FallbackRolled.Num());
		TestTrue(TEXT("...and both happen"), FallbackBronze > 0 && FallbackIron > 0);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableDeterminismTest,
	"Smores.Items.LootTable.SameSeedAndIdRollTheSame",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableDeterminismTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"));

	// enough variety - four items, a quantity range, a modifier and a variable roll count - that two
	// streams landing on the same result by chance is vanishingly unlikely
	ULootTableDefinition* Table = MakeTestLootTable(TestWorld, 1, 3);

	if (!TestNotNull(TEXT("Table created"), Table))
	{
		return true;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		FLootTableEntry& Entry = AddTestLootItem(Table, MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 20), 1, 1, 9);

		FLootModifierPool& Pool = Entry.ModifierPools.AddDefaulted_GetRef();
		AddTestModifierChoice(Pool, Bronze, 1);
		AddTestModifierChoice(Pool, nullptr, 1);
	}

	auto RollFor = [Table](int32 WorldSeed, const FGuid& ContainerId)
	{
		FRandomStream Stream = UWeightedTableDefinition::MakeRollStream(WorldSeed, ContainerId);

		TArray<FInventoryItem> Items;
		Table->RollLoot(Stream, SmoresLootTest_NoCandidates, Items);

		return DescribeRolledItems(Items);
	};

	const FGuid ChestId(0x1111, 0x2222, 0x3333, 0x4444);

	const FString Original = RollFor(7, ChestId);

	TestFalse(TEXT("The roll produced something to compare"), Original.IsEmpty());
	TestEqual(TEXT("The same world seed and id roll the same thing, every time"), RollFor(7, ChestId), Original);

	bool bAnotherChestDiffers = false;
	bool bAnotherWorldDiffers = false;

	for (uint32 Other = 1; Other <= 8; ++Other)
	{
		bAnotherChestDiffers |= RollFor(7, FGuid(Other, 0, 0, 0)) != Original;
		bAnotherWorldDiffers |= RollFor(7 + Other, ChestId) != Original;
	}

	TestTrue(TEXT("A different chest in the same world rolls something different"), bAnotherChestDiffers);
	TestTrue(TEXT("The same chest in a different campaign rolls something different"), bAnotherWorldDiffers);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresLootTableStreamPinnedTest,
	"Smores.Items.LootTable.RollStreamIsPinned",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresLootTableStreamPinnedTest::RunTest(const FString& Parameters)
{
	// These two numbers are the CRC-32 of (world seed, id) - computed outside the engine, with
	// Python's zlib.crc32 over the same five little-endian words. They are pinned because changing
	// how a seed and an id mix silently re-rolls every container in every saved campaign. If this
	// fails, somebody changed MakeRollStream: that has to be a decision, not an accident.
	TestEqual(TEXT("World 0, id (1,2,3,4) seeds the stream it always has"),
		UWeightedTableDefinition::MakeRollStream(0, FGuid(1, 2, 3, 4)).GetInitialSeed(), 1282081847);

	TestEqual(TEXT("...and so does a large seed with a large id"),
		UWeightedTableDefinition::MakeRollStream(42, FGuid(0xDEADBEEF, 0x01234567, 0x89ABCDEF, 0x0BADF00D)).GetInitialSeed(), -673969526);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
