// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "StrategyTargetActions.h"
#include "StrategyTargetInfo.h"
#include "HealthComponent.h"
#include "TraderComponent.h"
#include "CharacterDefinition.h"
#include "Tests/SmoresTestWorld.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Engine/World.h"

/*
 *  The target panel's and the right-click menu's action rows - both built by
 *  FStrategyTargetActions::BuildTargetInfo - asserted against the rules table in
 *  game-systems/action-menu.md, one test per row, plus the "who acts" rule and the placeholder odds.
 *
 *  An action row that quietly offers something the rules forbid is exactly the silent kind of
 *  wrong this suite exists for, which is why the rules were made a static taking plain arrays: a
 *  test world with no controller in it can ask any question the menu can.
 */

/**
 *  Health damage spawns a damage number no test has a class for, and units log their combat
 *  reactions at Warning - asserting them keeps the run green rather than yellow. Contains-match,
 *  one-or-more, since how many fire depends on the case.
 */
#define EXPECT_TARGET_INFO_COMBAT_LOGS() \
	AddExpectedMessagePlain(TEXT("[Combat]"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0)

/**
 *  The action with this id, or null if the row doesn't offer it.
 *
 *  Uniquely prefixed rather than living in an anonymous namespace: UE's unity builds concatenate
 *  several test .cpp files into one translation unit, where two anonymous namespaces merge and a
 *  shared helper name becomes a redefinition. See testing.md.
 */
static const FTargetAction* SmoresTargetInfoTest_FindAction(const FStrategyTargetInfo& Info, FName ActionId)
{
	return Info.Actions.FindByPredicate([ActionId](const FTargetAction& Candidate)
	{
		return Candidate.Id == ActionId;
	});
}

/** The row's ids in order, for comparing against the table */
static TArray<FName> SmoresTargetInfoTest_Ids(const FStrategyTargetInfo& Info)
{
	TArray<FName> Ids;

	for (const FTargetAction& Action : Info.Actions)
	{
		Ids.Add(Action.Id);
	}

	return Ids;
}

/** "Talk, Pickpocket, Examine" - for a failure message that says what the row actually was */
static FString SmoresTargetInfoTest_Describe(const TArray<FName>& Ids)
{
	TArray<FString> Names;

	for (const FName& Id : Ids)
	{
		Names.Add(Id.ToString());
	}

	return FString::Join(Names, TEXT(", "));
}

/** Spawns one of the test stand-in actors at a world location, or null if the world isn't up */
template <typename TActor>
static TActor* SmoresTargetInfoTest_Spawn(FSmoresTestWorld& TestWorld, const FVector& Location)
{
	UWorld* World = TestWorld.GetWorld();

	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	return World->SpawnActor<TActor>(TActor::StaticClass(), FTransform(Location), SpawnParameters);
}

/** A test-built definition of the given kind - so a unit can be made a creature without an authored asset */
static UCharacterDefinition* SmoresTargetInfoTest_MakeDefinition(FSmoresTestWorld& TestWorld, ECharacterKind Kind)
{
	UCharacterDefinition* Definition = TestWorld.NewKeptObject<UCharacterDefinition>();

	if (Definition)
	{
		Definition->Kind = Kind;
	}

	return Definition;
}

/** Asserts a row is exactly Expected, in order */
static bool SmoresTargetInfoTest_ExpectRow(FAutomationTestBase& Test, const FString& What, const FStrategyTargetInfo& Info, const TArray<FName>& Expected)
{
	const TArray<FName> Actual = SmoresTargetInfoTest_Ids(Info);

	if (Actual != Expected)
	{
		Test.AddError(FString::Printf(TEXT("%s: expected [%s], got [%s]"), *What, *SmoresTargetInfoTest_Describe(Expected), *SmoresTargetInfoTest_Describe(Actual)));
		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoNoTargetTest,
	"Smores.Strategy.TargetInfo.NothingTargetedIsEmpty",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoNoTargetTest::RunTest(const FString& Parameters)
{
	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(nullptr, TArray<AStrategyUnit*>(), TArray<AStrategyUnit*>());

	TestFalse(TEXT("Nothing targeted means nothing to show"), Info.HasTarget());
	TestEqual(TEXT("...and no action row at all"), Info.Actions.Num(), 0);
	TestTrue(TEXT("...and no distance to print"), Info.DistanceMeters < 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoNeutralPersonTest,
	"Smores.Strategy.TargetInfo.Row.NeutralPerson",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoNeutralPersonTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_TARGET_INFO_COMBAT_LOGS();

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(5000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	Pawn->SetNameForTest(FText::FromString(TEXT("Hana")));

	const TArray<AStrategyUnit*> Selection = { Pawn };
	const TArray<AStrategyUnit*> Squad = { Pawn };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Squad);

	TestTrue(TEXT("A targeted person is something to show"), Info.HasTarget());
	TestEqual(TEXT("...and the menu's header names who would go"), Info.ActorName.ToString(), FString(TEXT("Hana")));
	TestTrue(TEXT("...with a health bar"), Info.bHasHealth);
	TestEqual(TEXT("...measured from the selected pawn, in metres"), Info.DistanceMeters, 50.0f, 0.01f);
	TestTrue(TEXT("...and the struct remembers which actor it describes"), Info.Target.Get() == NPC);

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A neutral person who keeps no shop"), Info,
		{ StrategyTargetAction::Talk(), StrategyTargetAction::Pickpocket(), StrategyTargetAction::KnockOut(), StrategyTargetAction::Heal(), StrategyTargetAction::Attack(), StrategyTargetAction::Examine() });

	const FTargetAction* Talk = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Talk());
	const FTargetAction* Heal = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Heal());
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack());

	if (!Talk || !Heal || !Attack)
	{
		return true;
	}

	// the point of walking over: 50m away is no reason to grey anything out
	TestTrue(TEXT("Talk is enabled from across the level - someone walks over"), Talk->bEnabled);
	TestTrue(TEXT("...naming who would go"), Talk->ActorName.EqualTo(Pawn->GetHolderDisplayName()));
	TestEqual(TEXT("...and the key that does the same"), Talk->KeyHint.ToString(), FString(TEXT("T")));
	TestTrue(TEXT("Attack is enabled - they aren't fighting yet"), Attack->bEnabled);
	TestFalse(TEXT("Heal is greyed out on someone at full health"), Heal->bEnabled);
	TestTrue(TEXT("...with no reason given, because that isn't a refusal"), Heal->DisabledReason == ESmoresRefusalReason::None);

	// a trader gets Trade, straight after Talk
	TestWorld.AddComponent<UTraderComponent>(NPC);

	const FStrategyTargetInfo TraderInfo = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Squad);

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A neutral trader"), TraderInfo,
		{ StrategyTargetAction::Talk(), StrategyTargetAction::Trade(), StrategyTargetAction::Pickpocket(), StrategyTargetAction::KnockOut(), StrategyTargetAction::Heal(), StrategyTargetAction::Attack(), StrategyTargetAction::Examine() });

	const FTargetAction* Trade = SmoresTargetInfoTest_FindAction(TraderInfo, StrategyTargetAction::Trade());

	TestTrue(TEXT("Trade is enabled on a neutral trader"), Trade && Trade->bEnabled);

	// hurt them, and Heal comes on
	NPC->GetHealth()->TakeDamage(10.0f);

	const FStrategyTargetInfo HurtInfo = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Squad);
	const FTargetAction* HurtHeal = SmoresTargetInfoTest_FindAction(HurtInfo, StrategyTargetAction::Heal());

	TestTrue(TEXT("Heal is enabled once they're hurt"), HurtHeal && HurtHeal->bEnabled);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoHostilePersonTest,
	"Smores.Strategy.TargetInfo.Row.HostilePerson",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoHostilePersonTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	TestWorld.AddComponent<UTraderComponent>(NPC);

	// hostility without the self-hunting machinery - see ATestStrategyNPC::MakeHostileForTest for
	// why the real setter can't be used from a test
	NPC->MakeHostileForTest();

	const TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Selection);

	// the same row as a neutral trader - disabled-but-visible, never missing
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A hostile trader"), Info,
		{ StrategyTargetAction::Talk(), StrategyTargetAction::Trade(), StrategyTargetAction::Pickpocket(), StrategyTargetAction::KnockOut(), StrategyTargetAction::Heal(), StrategyTargetAction::Attack(), StrategyTargetAction::Examine() });

	const FTargetAction* Talk = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Talk());
	const FTargetAction* Trade = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Trade());
	const FTargetAction* Pickpocket = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Pickpocket());
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack());
	const FTargetAction* Examine = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Examine());

	if (!Talk || !Trade || !Pickpocket || !Attack || !Examine)
	{
		return true;
	}

	// the case the whole disabled-but-visible rule exists for: standing right next to someone
	// trying to kill you and seeing *why* you can't deal with them
	TestFalse(TEXT("Talk is disabled even standing next to them"), Talk->bEnabled);
	TestTrue(TEXT("...naming NotInteractable"), Talk->DisabledReason == ESmoresRefusalReason::NotInteractable);

	// GetTraderStock answers null for a hostile trader - the row must not use it to decide listing
	TestFalse(TEXT("Trade is greyed out on a hostile trader, not missing"), Trade->bEnabled);
	TestTrue(TEXT("...naming NotInteractable"), Trade->DisabledReason == ESmoresRefusalReason::NotInteractable);

	TestFalse(TEXT("Pickpocket is disabled on someone watching you"), Pickpocket->bEnabled);
	TestTrue(TEXT("...with no reason yet - awareness brings the real one"), Pickpocket->DisabledReason == ESmoresRefusalReason::None);
	TestTrue(TEXT("...and promises no odds while greyed out"), Pickpocket->Detail.IsEmpty());

	const FTargetAction* KnockOut = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::KnockOut());

	TestTrue(TEXT("Knock out is greyed out on someone already fighting you, with no reason yet"),
		KnockOut && !KnockOut->bEnabled && KnockOut->DisabledReason == ESmoresRefusalReason::None);

	TestFalse(TEXT("Attack is disabled - the fight is already running"), Attack->bEnabled);
	TestTrue(TEXT("...with no reason given, because that isn't a refusal"), Attack->DisabledReason == ESmoresRefusalReason::None);

	TestTrue(TEXT("Examine is always available"), Examine->bEnabled);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoDownedPersonTest,
	"Smores.Strategy.TargetInfo.Row.DownedAndDeadPerson",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoDownedPersonTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_TARGET_INFO_COMBAT_LOGS();

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Pawn spawned"), Pawn) || !TestNotNull(TEXT("NPC has health"), NPC->GetHealth()))
	{
		return true;
	}

	const TArray<AStrategyUnit*> Selection = { Pawn };

	NPC->GetHealth()->TakeDamage(1000.0f);

	if (!TestTrue(TEXT("The NPC went down"), NPC->IsDowned()))
	{
		return true;
	}

	const FStrategyTargetInfo Downed = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Selection);

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A downed person"), Downed,
		{ StrategyTargetAction::Loot(), StrategyTargetAction::Heal(), StrategyTargetAction::Kidnap(), StrategyTargetAction::Examine() });

	const FTargetAction* Loot = SmoresTargetInfoTest_FindAction(Downed, StrategyTargetAction::Loot());
	const FTargetAction* Kidnap = SmoresTargetInfoTest_FindAction(Downed, StrategyTargetAction::Kidnap());

	TestTrue(TEXT("Loot is enabled from 30m - someone walks over"), Loot && Loot->bEnabled);
	TestEqual(TEXT("...on the O key"), Loot ? Loot->KeyHint.ToString() : FString(), FString(TEXT("O")));
	TestTrue(TEXT("Kidnap is enabled, with odds for whoever would go"), Kidnap && Kidnap->bEnabled && !Kidnap->Detail.IsEmpty());

	NPC->GetHealth()->Kill();

	const FStrategyTargetInfo Dead = FStrategyTargetActions::BuildTargetInfo(NPC, Selection, Selection);

	// nobody is healed or carried off once they're dead
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A dead person"), Dead, { StrategyTargetAction::Loot(), StrategyTargetAction::Examine() });

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoCreatureTest,
	"Smores.Strategy.TargetInfo.Row.Creature",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoCreatureTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_TARGET_INFO_COMBAT_LOGS();

	ATestStrategyNPC* Beast = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Creature spawned"), Beast) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	Beast->SetDefinitionForTest(SmoresTargetInfoTest_MakeDefinition(TestWorld, ECharacterKind::Creature));

	TestFalse(TEXT("A creature-kind definition makes the unit a creature"), Beast->IsPerson());

	const TArray<AStrategyUnit*> Selection = { Pawn };

	// no talking, trading, robbing or healing - only a fight
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A creature on its feet"), FStrategyTargetActions::BuildTargetInfo(Beast, Selection, Selection),
		{ StrategyTargetAction::Attack(), StrategyTargetAction::Examine() });

	Beast->MakeHostileForTest();

	const FStrategyTargetInfo HostileBeast = FStrategyTargetActions::BuildTargetInfo(Beast, Selection, Selection);
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(HostileBeast, StrategyTargetAction::Attack());

	TestTrue(TEXT("Attack greys out on a creature already fighting you"), Attack && !Attack->bEnabled);

	Beast->GetHealth()->TakeDamage(1000.0f);

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A downed creature"), FStrategyTargetActions::BuildTargetInfo(Beast, Selection, Selection),
		{ StrategyTargetAction::Loot(), StrategyTargetAction::Examine() });

	Beast->GetHealth()->Kill();

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A dead creature"), FStrategyTargetActions::BuildTargetInfo(Beast, Selection, Selection),
		{ StrategyTargetAction::Loot(), StrategyTargetAction::Examine() });

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoOwnSquadTest,
	"Smores.Strategy.TargetInfo.Row.OwnAndOtherSquad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoOwnSquadTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_TARGET_INFO_COMBAT_LOGS();

	ATestStrategyPlayerUnit* Target = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Selected = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(100.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Stranger = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(0.0f, 100.0f, 0.0f));

	if (!TestNotNull(TEXT("Target pawn spawned"), Target) || !TestNotNull(TEXT("Selected pawn spawned"), Selected) || !TestNotNull(TEXT("Stranger spawned"), Stranger))
	{
		return true;
	}

	const TArray<AStrategyUnit*> Selection = { Selected };
	const TArray<AStrategyUnit*> Squad = { Target, Selected };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(Target, Selection, Squad);

	TestTrue(TEXT("One of your own squad is still something to show"), Info.HasTarget());
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("Your own squad member"), Info, { StrategyTargetAction::Heal(), StrategyTargetAction::Examine() });

	const FTargetAction* Heal = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Heal());

	TestTrue(TEXT("Heal is greyed out at full health"), Heal && !Heal->bEnabled && Heal->DisabledReason == ESmoresRefusalReason::None);

	Target->GetHealth()->TakeDamage(10.0f);

	const FStrategyTargetInfo Hurt = FStrategyTargetActions::BuildTargetInfo(Target, Selection, Squad);
	const FTargetAction* HurtHeal = SmoresTargetInfoTest_FindAction(Hurt, StrategyTargetAction::Heal());

	TestTrue(TEXT("...and on once they're hurt"), HurtHeal && HurtHeal->bEnabled);

	// a squad member that isn't in this player's squad belongs to another player
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("Another player's squad member"), FStrategyTargetActions::BuildTargetInfo(Stranger, Selection, Squad),
		{ StrategyTargetAction::Examine() });

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoContainerTest,
	"Smores.Strategy.TargetInfo.Row.Container",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoContainerTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyContainer* Container = SmoresTargetInfoTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(5000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Container spawned"), Container) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	const TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(Container, Selection, Selection);

	TestFalse(TEXT("A chest has no health bar"), Info.bHasHealth);

	// Loot, not Open: Open is a door's word now
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A container"), Info, { StrategyTargetAction::Loot(), StrategyTargetAction::Examine() });

	const FTargetAction* Loot = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Loot());

	if (!TestNotNull(TEXT("Loot is offered"), Loot))
	{
		return true;
	}

	// the decision this roadmap made: 50m away is not "too far" any more, because someone walks over
	TestTrue(TEXT("Loot is enabled from 50m away"), Loot->bEnabled);
	TestTrue(TEXT("...with no Too far away"), Loot->DisabledReason == ESmoresRefusalReason::None);
	TestEqual(TEXT("...on the O key"), Loot->KeyHint.ToString(), FString(TEXT("O")));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoDoorTest,
	"Smores.Strategy.TargetInfo.Row.Door",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoDoorTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestWorldDoor* Door = SmoresTargetInfoTest_Spawn<ATestWorldDoor>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(2000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Door spawned"), Door) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	const TArray<AStrategyUnit*> Selection = { Pawn };

	// one entry, never both - the one that would change it
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("A shut door"), FStrategyTargetActions::BuildTargetInfo(Door, Selection, Selection),
		{ StrategyTargetAction::Open(), StrategyTargetAction::Examine() });

	Door->SetOpen(true);

	const FStrategyTargetInfo Open = FStrategyTargetActions::BuildTargetInfo(Door, Selection, Selection);

	SmoresTargetInfoTest_ExpectRow(*this, TEXT("An open door"), Open, { StrategyTargetAction::Close(), StrategyTargetAction::Examine() });

	const FTargetAction* Close = SmoresTargetInfoTest_FindAction(Open, StrategyTargetAction::Close());

	TestTrue(TEXT("Close is enabled from across the room, on the O key"), Close && Close->bEnabled && Close->KeyHint.ToString() == TEXT("O"));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoItemTest,
	"Smores.Strategy.TargetInfo.Row.ItemOnTheGround",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoItemTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestWorldItem* Item = SmoresTargetInfoTest_Spawn<ATestWorldItem>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Pawn = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(2000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Item spawned"), Item) || !TestNotNull(TEXT("Pawn spawned"), Pawn))
	{
		return true;
	}

	Item->SetItem(MakeTestItem(MakeTestItemDefinition(TestWorld), 1));

	const TArray<AStrategyUnit*> Selection = { Pawn };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(Item, Selection, Selection);

	TestTrue(TEXT("A dropped item is something to show"), Info.HasTarget());
	SmoresTargetInfoTest_ExpectRow(*this, TEXT("An item on the ground"), Info, { StrategyTargetAction::PickUp(), StrategyTargetAction::Examine() });

	const FTargetAction* PickUp = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::PickUp());

	TestTrue(TEXT("Pick up is enabled from 20m - someone walks over"), PickUp && PickUp->bEnabled);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoNoOneSelectedTest,
	"Smores.Strategy.TargetInfo.NoOneSelectedGreysAllButExamine",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoNoOneSelectedTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyNPC* Hostile = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector(0.0f, 3000.0f, 0.0f));
	ATestStrategyPlayerUnit* Distant = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Hostile spawned"), Hostile) || !TestNotNull(TEXT("Pawn spawned"), Distant))
	{
		return true;
	}

	Hostile->MakeHostileForTest();

	// nothing selected, and the only squad member is nowhere near - nobody would go
	const TArray<AStrategyUnit*> Squad = { Distant };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(NPC, TArray<AStrategyUnit*>(), Squad);

	TestTrue(TEXT("Nobody named in the header"), Info.ActorName.IsEmpty());

	for (const FTargetAction& Action : Info.Actions)
	{
		if (Action.Id == StrategyTargetAction::Examine())
		{
			TestTrue(TEXT("Examine needs nobody"), Action.bEnabled);
			continue;
		}

		if (Action.Id == StrategyTargetAction::Heal())
		{
			// greyed for its own reason already (full health) - which stays the reason
			continue;
		}

		TestFalse(FString::Printf(TEXT("%s is greyed out with nobody selected"), *Action.Id.ToString()), Action.bEnabled);
		TestTrue(FString::Printf(TEXT("...%s naming NoOneSelected"), *Action.Id.ToString()), Action.DisabledReason == ESmoresRefusalReason::NoOneSelected);
	}

	// the target's own reason wins: selecting someone wouldn't make a hostile talk
	const FStrategyTargetInfo HostileInfo = FStrategyTargetActions::BuildTargetInfo(Hostile, TArray<AStrategyUnit*>(), Squad);
	const FTargetAction* HostileTalk = SmoresTargetInfoTest_FindAction(HostileInfo, StrategyTargetAction::Talk());

	TestTrue(TEXT("A hostile's Talk still says they won't deal with you"), HostileTalk && HostileTalk->DisabledReason == ESmoresRefusalReason::NotInteractable);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoWhoActsTest,
	"Smores.Strategy.TargetInfo.WhoActs",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoWhoActsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	EXPECT_TARGET_INFO_COMBAT_LOGS();

	ATestStrategyNPC* NPC = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Near = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(400.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Far = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Beside = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(0.0f, 100.0f, 0.0f));

	if (!TestNotNull(TEXT("NPC spawned"), NPC) || !TestNotNull(TEXT("Near pawn spawned"), Near)
		|| !TestNotNull(TEXT("Far pawn spawned"), Far) || !TestNotNull(TEXT("Beside pawn spawned"), Beside))
	{
		return true;
	}

	Near->SetNameForTest(FText::FromString(TEXT("Near")));
	Far->SetNameForTest(FText::FromString(TEXT("Far")));
	Beside->SetNameForTest(FText::FromString(TEXT("Beside")));

	const TArray<AStrategyUnit*> Squad = { Near, Far, Beside };
	const FName Talk = StrategyTargetAction::Talk();

	// 1. the nearest selected, not the first - far one first, so a bug taking the first reads "Far"
	TestTrue(TEXT("1. The nearest selected unit acts, not the first"), FStrategyTargetActions::ResolveActor(NPC, Talk, { Far, Near }, Squad) == Near);

	// 2. candidates exclude the target itself
	TestTrue(TEXT("2. The target never acts on itself..."), FStrategyTargetActions::ResolveActor(Near, Talk, { Near, Far }, Squad) == Far);

	// 3. a unit that is down can't act, and nobody else is quietly sent in its place
	Near->GetHealth()->TakeDamage(1000.0f);

	TestTrue(TEXT("3. A downed selected unit is passed over for one that can act"), FStrategyTargetActions::ResolveActor(NPC, Talk, { Near, Far }, Squad) == Far);
	TestNull(TEXT("...and with only the downed one selected, nobody acts"), FStrategyTargetActions::ResolveActor(NPC, Talk, { Near }, Squad));

	// 4. Heal alone falls back to the target, when it is selected and nobody else can act
	Far->GetHealth()->TakeDamage(10.0f);

	TestTrue(TEXT("4. Heal on the only selected unit is self-treatment"), FStrategyTargetActions::ResolveActor(Far, StrategyTargetAction::Heal(), { Far }, Squad) == Far);
	TestNull(TEXT("...and nothing else is"), FStrategyTargetActions::ResolveActor(Far, Talk, { Far }, Squad));

	// 5. nothing selected: a squad member already within reach acts
	TestTrue(TEXT("5. With nothing selected, a squad member within reach acts"), FStrategyTargetActions::ResolveActor(NPC, Talk, TArray<AStrategyUnit*>(), Squad) == Beside);

	Beside->SetActorLocation(FVector(0.0f, 2000.0f, 0.0f));

	TestNull(TEXT("...and with nobody within reach, nobody does"), FStrategyTargetActions::ResolveActor(NPC, Talk, TArray<AStrategyUnit*>(), Squad));

	// 6. an attack commits everyone able, and the row says so
	Beside->SetActorLocation(FVector(0.0f, 100.0f, 0.0f));

	const TArray<AStrategyUnit*> Everyone = { Near, Far, Beside };

	TestEqual(TEXT("6. An attack commits every selected unit able to fight"), FStrategyTargetActions::GetAttackers(NPC, Everyone).Num(), 2);

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(NPC, Everyone, Squad);
	const FTargetAction* Attack = SmoresTargetInfoTest_FindAction(Info, StrategyTargetAction::Attack());

	TestTrue(TEXT("...and the Attack row says everyone selected will go"), Attack && Attack->bEnabled && Attack->Detail.ToString().Contains(TEXT("Everyone")));
	TestTrue(TEXT("The header names the one who does everything else"), Info.ActorName.EqualTo(Beside->GetHolderDisplayName()));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoOddsTest,
	"Smores.Strategy.TargetInfo.PlaceholderOdds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoOddsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyNPC* Mark = SmoresTargetInfoTest_Spawn<ATestStrategyNPC>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Clumsy = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(200.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Nimble = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(0.0f, 200.0f, 0.0f));

	if (!TestNotNull(TEXT("Mark spawned"), Mark) || !TestNotNull(TEXT("Clumsy spawned"), Clumsy) || !TestNotNull(TEXT("Nimble spawned"), Nimble))
	{
		return true;
	}

	const FName Pickpocket = StrategyTargetAction::Pickpocket();

	// everyone at the default 10: an even chance
	TestEqual(TEXT("Evenly matched, the chance is even"), FStrategyTargetActions::GetPlaceholderSuccessChance(Pickpocket, Clumsy, Mark), 0.5f, 0.001f);
	TestEqual(TEXT("...and reads as a percentage"), FStrategyTargetActions::FormatSuccessChance(0.5f).ToString(), FString(TEXT("50% chance")));

	FCharacterAttributes Skilled;
	Skilled.Agility = 16.0f;
	Nimble->SetAttributesForTest(Skilled);

	TestEqual(TEXT("Six points of Agility over the mark's Perception is +18%"), FStrategyTargetActions::GetPlaceholderSuccessChance(Pickpocket, Nimble, Mark), 0.68f, 0.001f);

	// the menu's text changes with who would go - the whole point of the per-actor line
	// held in locals: FindAction points into the row, which must outlive the pointer
	const FStrategyTargetInfo ClumsyInfo = FStrategyTargetActions::BuildTargetInfo(Mark, { Clumsy }, { Clumsy, Nimble });
	const FStrategyTargetInfo NimbleInfo = FStrategyTargetActions::BuildTargetInfo(Mark, { Nimble }, { Clumsy, Nimble });
	const FTargetAction* ClumsyRow = SmoresTargetInfoTest_FindAction(ClumsyInfo, Pickpocket);
	const FTargetAction* NimbleRow = SmoresTargetInfoTest_FindAction(NimbleInfo, Pickpocket);

	if (!TestNotNull(TEXT("Pickpocket offered to the clumsy one"), ClumsyRow) || !TestNotNull(TEXT("Pickpocket offered to the nimble one"), NimbleRow))
	{
		return true;
	}

	TestEqual(TEXT("The clumsy squad member's row reads 50%"), ClumsyRow->Detail.ToString(), FString(TEXT("50% chance")));
	TestEqual(TEXT("The nimble one's reads 68%"), NimbleRow->Detail.ToString(), FString(TEXT("68% chance")));

	// clamped both ways: never certain, never hopeless
	FCharacterAttributes Superb;
	Superb.Agility = 100.0f;
	Superb.Strength = 100.0f;
	Nimble->SetAttributesForTest(Superb);

	TestEqual(TEXT("A huge edge is capped at 95%"), FStrategyTargetActions::GetPlaceholderSuccessChance(Pickpocket, Nimble, Mark), 0.95f, 0.001f);

	FCharacterAttributes Watchful;
	Watchful.Perception = 100.0f;
	Watchful.Endurance = 100.0f;
	Mark->SetAttributesForTest(Watchful);

	TestEqual(TEXT("A hopeless attempt is floored at 5%"), FStrategyTargetActions::GetPlaceholderSuccessChance(Pickpocket, Clumsy, Mark), 0.05f, 0.001f);

	// Kidnap and Knock out are Strength against Endurance, not Agility against Perception
	TestEqual(TEXT("Kidnap reads Strength against Endurance"), FStrategyTargetActions::GetPlaceholderSuccessChance(StrategyTargetAction::Kidnap(), Nimble, Mark), 0.5f, 0.001f);
	TestEqual(TEXT("...and so does Knock out"), FStrategyTargetActions::GetPlaceholderSuccessChance(StrategyTargetAction::KnockOut(), Nimble, Mark), 0.5f, 0.001f);
	TestTrue(TEXT("Knock out is a placeholder, like the others"), FStrategyTargetActions::IsPlaceholderAction(StrategyTargetAction::KnockOut()));

	TestTrue(TEXT("An action with no odds says so"), FStrategyTargetActions::GetPlaceholderSuccessChance(StrategyTargetAction::Talk(), Clumsy, Mark) < 0.0f);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresTargetInfoDistanceTest,
	"Smores.Strategy.TargetInfo.DistanceIsFromNearestSelectedUnit",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresTargetInfoDistanceTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	ATestStrategyContainer* Container = SmoresTargetInfoTest_Spawn<ATestStrategyContainer>(TestWorld, FVector::ZeroVector);
	ATestStrategyPlayerUnit* Near = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(400.0f, 0.0f, 0.0f));
	ATestStrategyPlayerUnit* Far = SmoresTargetInfoTest_Spawn<ATestStrategyPlayerUnit>(TestWorld, FVector(3000.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Container spawned"), Container)
		|| !TestNotNull(TEXT("Near pawn spawned"), Near)
		|| !TestNotNull(TEXT("Far pawn spawned"), Far))
	{
		return true;
	}

	// far one first, so a bug that takes the first unit rather than the nearest reads 30m
	const TArray<AStrategyUnit*> Selection = { Far, Near };

	const FStrategyTargetInfo Info = FStrategyTargetActions::BuildTargetInfo(Container, Selection, Selection);

	TestEqual(TEXT("The distance is to the nearest selected unit, not the first"), Info.DistanceMeters, 4.0f, 0.01f);

	// with nothing selected there is nothing to measure from, and the panel is told so rather than
	// being handed a zero it would print as "0m"
	const FStrategyTargetInfo Unselected = FStrategyTargetActions::BuildTargetInfo(Container, TArray<AStrategyUnit*>(), Selection);

	TestTrue(TEXT("With nothing selected the distance is unknown, not zero"), Unselected.DistanceMeters < 0.0f);
	TestTrue(TEXT("...and the target is still worth showing"), Unselected.HasTarget());

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
