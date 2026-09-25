// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ConversationSelection.h"
#include "DialogEffects.h"
#include "DialogFacts.h"
#include "DialogMemoryComponent.h"
#include "PlayerStandingComponent.h"
#include "WalletComponent.h"
#include "GameFramework/PlayerState.h"
#include "Tests/SmoresDialogTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The effects a conversation can run, and the squad memory they write - against real components on
 *  a real player state, since authority, the wallet's all-or-nothing debit and per-player memory are
 *  exactly what is being checked. The world is the harness's throwaway one: an actor spawned into
 *  it holds authority, so each effect runs its real path.
 */

/** A player state carrying what the effects reach: the wallet, standing, and dialog memory */
static APlayerState* SmoresDialogEffectsTest_SpawnPlayer(FSmoresTestWorld& TestWorld)
{
	UWorld* World = TestWorld.GetWorld();
	APlayerState* Player = World ? World->SpawnActor<APlayerState>() : nullptr;

	if (Player)
	{
		TestWorld.AddComponent<UWalletComponent>(Player);
		TestWorld.AddComponent<UPlayerStandingComponent>(Player);
		TestWorld.AddComponent<UDialogMemoryComponent>(Player);
	}

	return Player;
}

static FDialogEffectContext SmoresDialogEffectsTest_Context(FSmoresTestWorld& TestWorld, APlayerState* Player)
{
	FDialogEffectContext Context;
	Context.World = TestWorld.GetWorld();
	Context.Player = Player;
	return Context;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogEffectsMoneyTest,
	"Smores.Dialog.Effects.MoneyIsAllOrNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 *  TakeMoney takes all of it or none: too little in the wallet refuses with CannotAfford and leaves
 *  the balance untouched, which is why its choice is guarded by gold() and shows greyed instead.
 *  GiveMoney pays; both tell the feed what moved.
 */
bool FSmoresDialogEffectsMoneyTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	APlayerState* Player = SmoresDialogEffectsTest_SpawnPlayer(TestWorld);
	UWalletComponent* Wallet = Player ? Player->FindComponentByClass<UWalletComponent>() : nullptr;

	if (!TestNotNull(TEXT("A player with a wallet"), Wallet))
	{
		return true;
	}

	const FDialogEffectRegistry Effects = FDialogEffectRegistry::MakeBuiltIn();
	const FDialogEffectContext Context = SmoresDialogEffectsTest_Context(TestWorld, Player);

	Wallet->AddGold(10);

	const FDialogEffectOutcome TooMuch = Effects.Run(TEXT("TakeMoney"), Context, { TEXT("20") });
	TestFalse(TEXT("TakeMoney 20 from 10 is refused"), TooMuch.bDone);
	TestTrue(TEXT("...because it can't be afforded"), TooMuch.Refusal == ESmoresRefusalReason::CannotAfford);
	TestEqual(TEXT("...and takes nothing"), Wallet->GetGold(), 10);
	TestTrue(TEXT("...and says nothing moved"), TooMuch.FeedLine.IsEmpty());

	const FDialogEffectOutcome Enough = Effects.Run(TEXT("TakeMoney"), Context, { TEXT("5") });
	TestTrue(TEXT("TakeMoney 5 from 10 goes through"), Enough.bDone);
	TestEqual(TEXT("...taking exactly 5"), Wallet->GetGold(), 5);
	TestFalse(TEXT("...and tells the feed"), Enough.FeedLine.IsEmpty());

	const FDialogEffectOutcome Given = Effects.Run(TEXT("GiveMoney"), Context, { TEXT("7") });
	TestTrue(TEXT("GiveMoney 7 goes through"), Given.bDone);
	TestEqual(TEXT("...and pays 7"), Wallet->GetGold(), 12);

	// gold() is the same wallet, as the script sees it
	const FDialogFactRegistry Facts = FDialogFactRegistry::MakeBuiltIn();
	FDialogContext FactContext;
	FactContext.Player = Player;

	TestEqual(TEXT("gold() reads the wallet"), Facts.Ask(TEXT("Gold"), FactContext).Number, 12.0);

	TestFalse(TEXT("An unknown effect is refused"), Effects.Run(TEXT("Explode"), Context, {}).bDone);
	TestFalse(TEXT("...and so is the wrong number of words"), Effects.Run(TEXT("GiveMoney"), Context, {}).bDone);

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogEffectsAuthorityTest,
	"Smores.Dialog.Effects.RefuseOffAuthority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 *  Every effect changes shared state, so off-authority every one refuses with nothing changed. The
 *  player state is spawned with authority, then demoted to a simulated proxy - what a client's copy
 *  of someone's player state is.
 */
bool FSmoresDialogEffectsAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	APlayerState* Player = SmoresDialogEffectsTest_SpawnPlayer(TestWorld);
	UWalletComponent* Wallet = Player ? Player->FindComponentByClass<UWalletComponent>() : nullptr;
	UPlayerStandingComponent* Standing = Player ? Player->FindComponentByClass<UPlayerStandingComponent>() : nullptr;
	UDialogMemoryComponent* Memory = Player ? Player->FindComponentByClass<UDialogMemoryComponent>() : nullptr;

	if (!TestTrue(TEXT("A player with a wallet, standing and memory"), Wallet && Standing && Memory))
	{
		return true;
	}

	Wallet->AddGold(50);
	Player->SetRole(ROLE_SimulatedProxy);

	if (!TestFalse(TEXT("The player state no longer has authority"), Player->HasAuthority()))
	{
		return true;
	}

	// the components warn when a mutator is reached off-authority; the effects must not reach them
	const FDialogEffectRegistry Effects = FDialogEffectRegistry::MakeBuiltIn();
	const FDialogEffectContext Context = SmoresDialogEffectsTest_Context(TestWorld, Player);

	TestFalse(TEXT("TakeMoney refuses"), Effects.Run(TEXT("TakeMoney"), Context, { TEXT("20") }).bDone);
	TestFalse(TEXT("GiveMoney refuses"), Effects.Run(TEXT("GiveMoney"), Context, { TEXT("20") }).bDone);
	TestFalse(TEXT("ChangeStanding refuses"), Effects.Run(TEXT("ChangeStanding"), Context, { TEXT("Raiders"), TEXT("-10") }).bDone);
	TestFalse(TEXT("SetFlag refuses"), Effects.Run(TEXT("SetFlag"), Context, { TEXT("met_bandit") }).bDone);

	TestEqual(TEXT("The wallet is untouched"), Wallet->GetGold(), 50);
	TestEqual(TEXT("The standing is untouched"), Standing->GetStanding(TEXT("Raiders")), 0);
	TestFalse(TEXT("The flag is unset"), Memory->HasFlag(TEXT("met_bandit")));

	// and the memory's own gate, for anything that reaches it some other way
	TestFalse(TEXT("The memory itself refuses a flag off-authority"), Memory->SetFlag(TEXT("met_bandit"), true));
	TestFalse(TEXT("...and a viewing"), Memory->MarkSeen(TEXT("core.Shakedown")));

	Player->SetRole(ROLE_Authority);

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogEffectsStandingAndFlagsTest,
	"Smores.Dialog.Effects.StandingAndFlags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/** ChangeStanding moves this player's standing and says by how much; SetFlag sets and clears; Flag() reads it back */
bool FSmoresDialogEffectsStandingAndFlagsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	APlayerState* Player = SmoresDialogEffectsTest_SpawnPlayer(TestWorld);
	UPlayerStandingComponent* Standing = Player ? Player->FindComponentByClass<UPlayerStandingComponent>() : nullptr;
	UDialogMemoryComponent* Memory = Player ? Player->FindComponentByClass<UDialogMemoryComponent>() : nullptr;

	if (!TestTrue(TEXT("A player with standing and memory"), Standing && Memory))
	{
		return true;
	}

	const FDialogEffectRegistry Effects = FDialogEffectRegistry::MakeBuiltIn();
	const FDialogEffectContext Context = SmoresDialogEffectsTest_Context(TestWorld, Player);

	const FDialogEffectOutcome Lowered = Effects.Run(TEXT("ChangeStanding"), Context, { TEXT("Raiders"), TEXT("-10") });
	TestTrue(TEXT("ChangeStanding goes through"), Lowered.bDone);
	TestEqual(TEXT("...moving standing by -10"), Standing->GetStanding(TEXT("Raiders")), -10);
	TestFalse(TEXT("...and tells the feed"), Lowered.FeedLine.IsEmpty());

	const FDialogFactRegistry Facts = FDialogFactRegistry::MakeBuiltIn();
	FDialogContext FactContext;
	FactContext.Player = Player;

	TestFalse(TEXT("Flag(met_bandit) starts false"), Facts.Ask(TEXT("Flag"), FactContext, TEXT("met_bandit")).bBool);

	TestTrue(TEXT("SetFlag goes through"), Effects.Run(TEXT("SetFlag"), Context, { TEXT("met_bandit") }).bDone);
	TestTrue(TEXT("...and Flag() reads it back"), Facts.Ask(TEXT("Flag"), FactContext, TEXT("met_bandit")).bBool);

	TestTrue(TEXT("Setting it again is still done"), Effects.Run(TEXT("SetFlag"), Context, { TEXT("met_bandit") }).bDone);
	TestEqual(TEXT("...and remembered once"), Memory->GetRecord().Flags.Num(), 1);

	TestTrue(TEXT("SetFlag name false clears it"), Effects.Run(TEXT("SetFlag"), Context, { TEXT("met_bandit"), TEXT("false") }).bDone);
	TestFalse(TEXT("...so Flag() is false again"), Facts.Ask(TEXT("Flag"), FactContext, TEXT("met_bandit")).bBool);

	return true;
}

// -----------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogMemoryPerPlayerTest,
	"Smores.Dialog.Memory.IsPerPlayer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 *  Dialog state is squad-scoped: one player's flags and conversations are never another's. Two
 *  players; one sets a flag and sees a conversation; the other's Flag(), Seen() and the
 *  conversation waiting on the flag all stay as they were.
 */
bool FSmoresDialogMemoryPerPlayerTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;
	APlayerState* PlayerA = SmoresDialogEffectsTest_SpawnPlayer(TestWorld);
	APlayerState* PlayerB = SmoresDialogEffectsTest_SpawnPlayer(TestWorld);
	UDialogMemoryComponent* MemoryA = UDialogMemoryComponent::Get(PlayerA);
	UDialogMemoryComponent* MemoryB = UDialogMemoryComponent::Get(PlayerB);

	if (!TestTrue(TEXT("Two players with their own memory"), MemoryA && MemoryB))
	{
		return true;
	}

	const FDialogEffectRegistry Effects = FDialogEffectRegistry::MakeBuiltIn();
	const FDialogFactRegistry Facts = FDialogFactRegistry::MakeBuiltIn();

	TestTrue(TEXT("A sets a flag"), Effects.Run(TEXT("SetFlag"), SmoresDialogEffectsTest_Context(TestWorld, PlayerA), { TEXT("paid_toll") }).bDone);
	TestTrue(TEXT("A sees a conversation"), MemoryA->MarkSeen(TEXT("core.Shakedown")));

	FDialogContext ContextA;
	ContextA.Player = PlayerA;

	FDialogContext ContextB;
	ContextB.Player = PlayerB;

	TestTrue(TEXT("A's Flag() is set"), Facts.Ask(TEXT("Flag"), ContextA, TEXT("paid_toll")).bBool);
	TestFalse(TEXT("B's isn't"), Facts.Ask(TEXT("Flag"), ContextB, TEXT("paid_toll")).bBool);
	TestTrue(TEXT("A's Seen() is true"), Facts.Ask(TEXT("Seen"), ContextA, TEXT("Shakedown")).bBool);
	TestFalse(TEXT("B's isn't"), Facts.Ask(TEXT("Seen"), ContextB, TEXT("Shakedown")).bBool);

	// the conversation that waits on the flag is A's to have, not B's
	FConversationDefinition Paid;
	Paid.Id = TEXT("core.Paid");
	TArray<FString> Errors;
	TArray<FString> Warnings;

	if (!TestTrue(TEXT("requires: Flag(paid_toll) compiles against the game's facts"),
		SmoresDialog::CompileCondition(TEXT("Flag(paid_toll)"), Facts, EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player, nullptr, Paid.Requires, Errors, Warnings)))
	{
		return true;
	}

	TestTrue(TEXT("Eligible for A"), SmoresDialog::IsConversationEligible(Paid, ContextA, Facts, MemoryA->GetRecord()));
	TestFalse(TEXT("Not for B"), SmoresDialog::IsConversationEligible(Paid, ContextB, Facts, MemoryB->GetRecord()));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
