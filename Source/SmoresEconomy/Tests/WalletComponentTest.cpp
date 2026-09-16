// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "WalletComponent.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The gold balance: what it refuses, what it leaves alone when it refuses, and what it says
 *  when it changes.
 *
 *  Money is the category where a silent arithmetic error costs most, and TrySpendGold is the
 *  half of a purchase that has to be able to fail cleanly - AStrategyPlayerController's
 *  transaction is only all-or-nothing because a refusal here mutates nothing. So every refusal
 *  case below asserts the balance afterwards rather than only the returned bool, which is the
 *  same shape of bug (a `false` that already spent something) as the partial-add family
 *  inventory-roadmap.md keeps warning about.
 */

/** A wallet on a fresh authoritative owner, with a listener already bound to OnGoldChanged */
struct FTestWallet
{
	UWalletComponent* Wallet = nullptr;

	USmoresTestDelegateListener* Listener = nullptr;

	bool IsValid() const { return Wallet != nullptr && Listener != nullptr; }
};

inline FTestWallet MakeTestWallet(FSmoresTestWorld& TestWorld)
{
	FTestWallet Result;

	Result.Wallet = TestWorld.SpawnComponent<UWalletComponent>();
	Result.Listener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();

	if (Result.IsValid())
	{
		Result.Wallet->OnGoldChanged.AddDynamic(Result.Listener, &USmoresTestDelegateListener::OnIntChanged);
	}

	return Result;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletShortBalanceTest,
	"Smores.Economy.Wallet.SpendShortOfBalanceRefuses",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletShortBalanceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(10);

	// one short is the interesting boundary - an off-by-one in CanAfford shows up here and
	// nowhere else
	TestFalse(TEXT("A purchase one gold short is refused"), Test.Wallet->TrySpendGold(11));
	TestEqual(TEXT("...and the balance is untouched"), Test.Wallet->GetGold(), 10);
	TestEqual(TEXT("...and nothing was broadcast beyond the original credit"), Test.Listener->CallCount, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletExactBalanceTest,
	"Smores.Economy.Wallet.SpendExactBalanceEmptiesIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletExactBalanceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(10);

	TestTrue(TEXT("Spending exactly the balance goes through"), Test.Wallet->TrySpendGold(10));
	TestEqual(TEXT("...and leaves the wallet empty rather than negative"), Test.Wallet->GetGold(), 0);
	TestEqual(TEXT("...and the balance broadcast carried the new zero"), Test.Listener->LastInt, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletNegativeSpendTest,
	"Smores.Economy.Wallet.SpendNegativeRefuses",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletNegativeSpendTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// a negative spend is a caller bug, so it should say so rather than quietly crediting
	AddExpectedMessagePlain(TEXT("use AddGold to credit"), ELogVerbosity::Warning,
		EAutomationExpectedMessageFlags::Contains, 0);

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(10);
	Test.Listener->Reset();

	// the failure mode this guards against is a negative debit reading as a credit: `Gold -= -5`
	TestFalse(TEXT("A negative spend is refused"), Test.Wallet->TrySpendGold(-5));
	TestEqual(TEXT("...and does not credit the wallet"), Test.Wallet->GetGold(), 10);
	TestEqual(TEXT("...and broadcasts nothing"), Test.Listener->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletZeroSpendTest,
	"Smores.Economy.Wallet.SpendZeroSucceedsSilently",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletZeroSpendTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(10);
	Test.Listener->Reset();

	// a free transaction still has to go through, or a zero-priced trade refuses for no reason
	TestTrue(TEXT("Spending nothing succeeds"), Test.Wallet->TrySpendGold(0));
	TestEqual(TEXT("...without touching the balance"), Test.Wallet->GetGold(), 10);
	TestEqual(TEXT("...and without announcing a change that didn't happen"), Test.Listener->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletCanAffordTest,
	"Smores.Economy.Wallet.CanAffordTreatsFreeAsAffordable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletCanAffordTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	// an empty wallet, which is the case where "is this affordable" is easiest to get wrong
	TestTrue(TEXT("A free thing is affordable with nothing in the wallet"), Test.Wallet->CanAfford(0));
	TestTrue(TEXT("A negative price is affordable too"), Test.Wallet->CanAfford(-5));
	TestFalse(TEXT("A price of one is not"), Test.Wallet->CanAfford(1));

	Test.Wallet->AddGold(10);

	TestTrue(TEXT("The exact balance is affordable"), Test.Wallet->CanAfford(10));
	TestFalse(TEXT("One past it is not"), Test.Wallet->CanAfford(11));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletAddGoldBroadcastTest,
	"Smores.Economy.Wallet.AddGoldBroadcastsTheNewBalance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletAddGoldBroadcastTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(25);

	TestEqual(TEXT("A credit broadcasts exactly once"), Test.Listener->CallCount, 1);
	TestEqual(TEXT("...carrying the new balance, not the amount added"), Test.Listener->LastInt, 25);

	Test.Wallet->AddGold(0);
	Test.Wallet->AddGold(-5);

	// AddGold only credits; debiting goes through TrySpendGold, which can actually fail
	TestEqual(TEXT("Crediting nothing, or less than nothing, leaves the balance alone"), Test.Wallet->GetGold(), 25);
	TestEqual(TEXT("...and broadcasts nothing"), Test.Listener->CallCount, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletSpendBroadcastTest,
	"Smores.Economy.Wallet.SpendBroadcastsOnlyOnSuccess",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletSpendBroadcastTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Test = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Wallet and listener created"), Test.IsValid()))
	{
		return true;
	}

	Test.Wallet->AddGold(25);
	Test.Listener->Reset();

	TestTrue(TEXT("A debit the balance covers goes through"), Test.Wallet->TrySpendGold(10));
	TestEqual(TEXT("...broadcasting once"), Test.Listener->CallCount, 1);
	TestEqual(TEXT("...with the balance left behind"), Test.Listener->LastInt, 15);

	TestFalse(TEXT("A debit it doesn't cover is refused"), Test.Wallet->TrySpendGold(100));
	TestEqual(TEXT("...and broadcasts nothing, so a UI bound to this never shows a spend that didn't happen"), Test.Listener->CallCount, 1);
	TestEqual(TEXT("...and leaves the balance where it was"), Test.Wallet->GetGold(), 15);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresWalletStartingGoldTest,
	"Smores.Economy.Wallet.StartingGoldIsAppliedAtBeginPlay",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresWalletStartingGoldTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	const FTestWallet Funded = MakeTestWallet(TestWorld);
	const FTestWallet Empty = MakeTestWallet(TestWorld);

	if (!TestTrue(TEXT("Both wallets created"), Funded.IsValid() && Empty.IsValid()))
	{
		return true;
	}

	// authored content, read on the server at BeginPlay - so it must be set before play starts
	Funded.Wallet->StartingGold = 40;
	Empty.Wallet->StartingGold = 0;

	TestEqual(TEXT("Nothing is credited before play begins"), Funded.Wallet->GetGold(), 0);

	if (!TestTrue(TEXT("The test world began play"), TestWorld.BeginPlay()))
	{
		TestWorld.ForwardErrors(this);

		return true;
	}

	TestEqual(TEXT("The authored starting balance is applied"), Funded.Wallet->GetGold(), 40);
	TestEqual(TEXT("...and announced like any other credit"), Funded.Listener->CallCount, 1);
	TestEqual(TEXT("...carrying the starting balance"), Funded.Listener->LastInt, 40);

	TestEqual(TEXT("A wallet starting at zero stays empty"), Empty.Wallet->GetGold(), 0);
	TestEqual(TEXT("...and announces nothing"), Empty.Listener->CallCount, 0);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
