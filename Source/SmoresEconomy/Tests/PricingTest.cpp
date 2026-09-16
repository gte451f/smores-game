// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TraderComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  What one item costs, and what a quantity of them costs.
 *
 *  Two rules here are deliberate rather than incidental, and are asserted so a future session
 *  that wants to change one has to delete a test saying not to:
 *
 *  - a saleable item is never rounded down to free by a small markup, while a sell price may
 *    legitimately round to zero (the trader declining to pay for junk);
 *  - a negative quantity prices as zero rather than as a refund, which is the one place
 *    IPricingProvider's derived totals could have turned a bad caller into free money.
 *
 *  The margin case asserts buy > sell as an invariant rather than comparing two hardcoded
 *  numbers, so retuning the default markups doesn't break a test that isn't about them.
 */

/** A trader on a fresh authoritative owner, at its authored default markups */
inline UTraderComponent* MakeTestTrader(FSmoresTestWorld& TestWorld)
{
	return TestWorld.SpawnComponent<UTraderComponent>();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingUnitPriceTest,
	"Smores.Economy.Pricing.UnitPricesApplyMarkupAndMarkdown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingUnitPriceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	Trader->BuyMarkup = 1.5f;
	Trader->SellMarkdown = 0.5f;

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 3.0f, /*BaseValue*/ 10);

	const FInventoryItem Item = MakeTestItem(Sword);

	TestEqual(TEXT("The buy price is BaseValue times the markup"), Trader->GetUnitBuyPrice(Item), 15);
	TestEqual(TEXT("The sell price is BaseValue times the markdown"), Trader->GetUnitSellPrice(Item), 5);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingNeverFreeTest,
	"Smores.Economy.Pricing.SaleableItemNeverPricesToFree",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingNeverFreeTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	// a markup this small rounds 1 x 0.1 down to nothing, which would hand the player a free item
	Trader->BuyMarkup = 0.1f;
	Trader->SellMarkdown = 0.1f;

	UItemDefinition* Pebble = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 0.1f, /*BaseValue*/ 1);

	const FInventoryItem Item = MakeTestItem(Pebble);

	TestEqual(TEXT("Anything the designer priced still costs at least one gold to buy"), Trader->GetUnitBuyPrice(Item), 1);

	// the other direction is not a bug: a markdown steep enough to make a near-worthless item
	// worth nothing is the trader declining to pay for junk
	TestEqual(TEXT("...while the sell price may legitimately round to nothing"), Trader->GetUnitSellPrice(Item), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingWorthlessTest,
	"Smores.Economy.Pricing.WorthlessItemIsFreeBothWays",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingWorthlessTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	// a definition with no authored value is genuinely worthless rather than cheap, so the
	// never-round-to-free rule above must not drag it up to 1
	UItemDefinition* Junk = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 1, 0.0f, /*BaseValue*/ 0);

	const FInventoryItem Item = MakeTestItem(Junk);

	TestEqual(TEXT("An item worth nothing costs nothing"), Trader->GetUnitBuyPrice(Item), 0);
	TestEqual(TEXT("...and fetches nothing"), Trader->GetUnitSellPrice(Item), 0);

	const FInventoryItem Empty;

	TestEqual(TEXT("An item with no definition at all prices at zero rather than crashing"), Trader->GetUnitBuyPrice(Empty), 0);
	TestEqual(TEXT("...on both sides"), Trader->GetUnitSellPrice(Empty), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingMarginTest,
	"Smores.Economy.Pricing.BuyExceedsSellAtDefaultMargins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingMarginTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// deliberately left at the authored defaults - this test is about what those defaults mean,
	// so setting them here would assert nothing
	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 2), 1, 3.0f, /*BaseValue*/ 10);

	const FInventoryItem Item = MakeTestItem(Sword);

	const int32 BuyPrice = Trader->GetUnitBuyPrice(Item);
	const int32 SellPrice = Trader->GetUnitSellPrice(Item);

	// the invariant, not the numbers: retuning the markups is allowed, inverting the margin is not
	TestTrue(
		*FString::Printf(TEXT("The player pays more to buy (%d) than they are paid to sell (%d)"), BuyPrice, SellPrice),
		BuyPrice > SellPrice);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingTotalsTest,
	"Smores.Economy.Pricing.TotalsAreUnitTimesQuantity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingTotalsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	Trader->BuyMarkup = 1.5f;
	Trader->SellMarkdown = 0.5f;

	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10, 0.2f, /*BaseValue*/ 4);

	const FInventoryItem Item = MakeTestItem(Apple, 7);

	// the quantity is a parameter, not read off the item - a stack of seven can be priced for
	// three of them
	TestEqual(TEXT("A total is the unit buy price times the quantity asked for"), Trader->GetBuyPrice(Item, 7), 6 * 7);
	TestEqual(TEXT("...and the same on the sell side"), Trader->GetSellPrice(Item, 7), 2 * 7);
	TestEqual(TEXT("A quantity of one is just the unit price"), Trader->GetBuyPrice(Item, 1), 6);
	TestEqual(TEXT("A quantity of none costs nothing"), Trader->GetBuyPrice(Item, 0), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresPricingNegativeQuantityTest,
	"Smores.Economy.Pricing.NegativeQuantityPricesAsZero",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresPricingNegativeQuantityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UTraderComponent* Trader = MakeTestTrader(TestWorld);

	if (!TestNotNull(TEXT("Trader created"), Trader))
	{
		return true;
	}

	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 10, 0.2f, /*BaseValue*/ 4);

	const FInventoryItem Item = MakeTestItem(Apple, 3);

	// a negative total would read as a refund one caller away from being credited to the player
	TestEqual(TEXT("Buying a negative quantity prices at zero, not as a refund"), Trader->GetBuyPrice(Item, -3), 0);
	TestEqual(TEXT("...and so does selling one"), Trader->GetSellPrice(Item, -3), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
