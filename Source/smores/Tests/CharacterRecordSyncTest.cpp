// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "EquipmentComponent.h"
#include "HealthComponent.h"
#include "InventoryComponent.h"
#include "Tests/SmoresCharacterTestFactory.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"
#include "Engine/World.h"

/**
 *  The soft split end to end: a real AStrategyUnit finding or creating its record at BeginPlay,
 *  writing every change back, and adopting an existing record instead of starting fresh.
 *
 *  Lives in the smores module rather than SmoresCharacters because AStrategyUnit is abstract and
 *  the concrete stand-in (ATestStrategyNPC) lives here - see SmoresStrategyTestActors.h.
 *
 *  The load-bearing test is PlacedUnitAdoptsItsRecordDeadOrAlive: kill a unit, take its actor
 *  away, stand a new one up with the same placed key, and the new one must be dead. That is the
 *  "reloading a save stands the boss back up" bug, played out inside one session.
 */

/**
 *  Every hit spawns a damage number no test has a class for, and the unit logs its own combat
 *  reactions at Warning - asserting them keeps the run green rather than yellow. Contains-match,
 *  one-or-more, since how many fire depends on the case.
 */
#define EXPECT_UNIT_COMBAT_LOGS() \
	AddExpectedMessagePlain(TEXT("[Combat]"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0)

/**
 *  Spawns the NPC stand-in authored the way a placed unit is - definition, record key, optional
 *  name - and lets it begin play. Uniquely prefixed rather than in an anonymous namespace, for
 *  the unity-build reason in testing.md.
 */
static ATestStrategyNPC* SmoresRecordSyncTest_SpawnUnit(FSmoresTestWorld& TestWorld, UCharacterDefinition* Definition,
	const FGuid& PlacedRecordId, const FVector& Location, const FText& PlacedName = FText::GetEmpty())
{
	UWorld* World = TestWorld.GetWorld();

	if (!World)
	{
		return nullptr;
	}

	const FTransform Transform(Location);

	ATestStrategyNPC* Unit = World->SpawnActorDeferred<ATestStrategyNPC>(ATestStrategyNPC::StaticClass(), Transform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Unit)
	{
		return nullptr;
	}

	Unit->SetCharacterForTest(Definition, PlacedRecordId, PlacedName);
	Unit->FinishSpawning(Transform);

	return Unit;
}

/** Begins play and puts a record store on the world's GameState, in the order MakeTestRecordStore wants */
static UCharacterRecordComponent* SmoresRecordSyncTest_BeginWithStore(FSmoresTestWorld& TestWorld)
{
	return TestWorld.BeginPlay() ? MakeTestRecordStore(TestWorld) : nullptr;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresRecordSyncCreateTest,
	"Smores.Characters.RecordSync.NewUnitCreatesItsRecord",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresRecordSyncCreateTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = SmoresRecordSyncTest_BeginWithStore(TestWorld);
	UCharacterDefinition* Definition = MakeTestCharacterDefinition(TestWorld, TEXT("TestSettler"), false, { FText::FromString(TEXT("Aldra")) });
	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), /*MaxStack*/ 5);

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definitions created"), Definition) || !Apple)
	{
		return true;
	}

	Definition->DefaultLoadout = { MakeTestItem(Apple, 3) };

	const FGuid Key = FGuid::NewGuid();
	ATestStrategyNPC* Unit = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, Key, FVector(100.0f, 200.0f, 0.0f));

	if (!TestNotNull(TEXT("Unit spawned"), Unit))
	{
		return true;
	}

	TestTrue(TEXT("The unit's record is keyed by its placed id"), Unit->GetRecordId() == Key);
	TestTrue(TEXT("...and the store knows this unit stands in for it"), Store->GetBoundActor(Key) == Unit);

	const FCharacterRecord* Record = Store->FindRecord(Key);

	if (!TestNotNull(TEXT("The record exists"), Record))
	{
		return true;
	}

	TestEqual(TEXT("It is named from the pool"), Record->Name.ToString(), FString(TEXT("Aldra")));
	TestEqual(TEXT("...and the unit shows that name"), Unit->GetHolderDisplayName().ToString(), FString(TEXT("Aldra")));

	TestEqual(TEXT("The loadout landed in the unit's grid"), GetTotalQuantity(Unit->GetInventory()), 3);
	TestEqual(TEXT("...and was written back to the record"), Record->Carried.Num(), 1);

	if (Record->Carried.Num() == 1)
	{
		TestEqual(TEXT("...with the same quantity"), Record->Carried[0].Item.Quantity, 3);
	}

	TestEqual(TEXT("The record holds the unit's full health"), Record->Health, Unit->GetHealth()->MaxHealth);
	TestTrue(TEXT("...Alive"), Record->LifeState == EHealthState::Alive);
	TestTrue(TEXT("...where it was spawned"), Record->LastKnownLocation.Equals(Unit->GetActorLocation()));

	// a level designer's name wins over the pool for a non-unique character
	ATestStrategyNPC* Named = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, FGuid::NewGuid(), FVector::ZeroVector, FText::FromString(TEXT("NPC 3")));

	if (TestNotNull(TEXT("Named unit spawned"), Named))
	{
		TestEqual(TEXT("A placed unit's authored name names its record"), Named->GetRecord()->Name.ToString(), FString(TEXT("NPC 3")));
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresRecordSyncWriteBackTest,
	"Smores.Characters.RecordSync.WriteBackRoundTrip",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresRecordSyncWriteBackTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = SmoresRecordSyncTest_BeginWithStore(TestWorld);
	UCharacterDefinition* Definition = MakeTestCharacterDefinition(TestWorld, TEXT("TestBandit"));
	UItemDefinition* Sword = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), 1, 0.0f, 0, EEquipSlot::MainHand);
	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definitions created"), Definition) || !Sword || !Apple)
	{
		return true;
	}

	ATestStrategyNPC* Unit = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, FGuid::NewGuid(), FVector::ZeroVector);

	if (!TestNotNull(TEXT("Unit spawned"), Unit) || !TestNotNull(TEXT("...with a record"), Unit->GetRecord()))
	{
		return true;
	}

	EXPECT_UNIT_COMBAT_LOGS();

	const FGuid Id = Unit->GetRecordId();

	// read fresh after every change, and never through a null - a missing record fails the
	// assertions below instead of taking the whole run down
	auto Current = [Store, &Id]() -> FCharacterRecord
	{
		const FCharacterRecord* Found = Store->FindRecord(Id);

		return Found ? *Found : FCharacterRecord();
	};

	// actor -> record: every kind of change reaches the record without anyone asking
	Unit->GetHealth()->TakeDamage(30.0f);

	TestEqual(TEXT("Damage is written back"), Current().Health, Unit->GetHealth()->MaxHealth - 30.0f);

	TestTrue(TEXT("A sword was picked up"), Unit->GetInventory()->AddItem(MakeTestItem(Sword)));
	TestEqual(TEXT("A pickup is written back"), Current().Carried.Num(), 1);

	TestTrue(TEXT("The sword was equipped"), Unit->GetEquipment()->Equip(Unit->GetInventory(), GetOnlyEntryId(Unit->GetInventory())));
	TestEqual(TEXT("Equipping is written back - off the grid..."), Current().Carried.Num(), 0);
	TestEqual(TEXT("...and onto the paperdoll"), Current().Equipped.Num(), 1);

	Unit->GetHealth()->TakeDamage(1000.0f);

	TestTrue(TEXT("Going Downed is written back"), Current().LifeState == EHealthState::Downed);
	TestEqual(TEXT("...at zero health"), Current().Health, 0.0f);

	Unit->GetHealth()->Kill();

	TestTrue(TEXT("Dying is written back"), Current().LifeState == EHealthState::Dead);

	// record -> actor: the other direction, on a second unit that is still alive
	ATestStrategyNPC* Other = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, FGuid::NewGuid(), FVector::ZeroVector);

	if (!TestNotNull(TEXT("Second unit spawned"), Other))
	{
		return true;
	}

	FCharacterRecord* OtherRecord = Store->EditRecord(Other->GetRecordId());

	OtherRecord->Health = 40.0f;
	OtherRecord->Carried = { FInventoryEntry(5, MakeTestItem(Apple, 2), FIntPoint(3, 3), false) };
	OtherRecord->LastKnownLocation = FVector(500.0f, 0.0f, 0.0f);

	Other->ApplyRecordToActor();

	TestEqual(TEXT("Applying the record sets the unit's health"), Other->GetHealth()->GetHealth(), 40.0f);
	TestTrue(TEXT("...its grid, entry id and anchor included"), Other->GetInventory()->GetEntry(5).AnchorCell == FIntPoint(3, 3));
	TestTrue(TEXT("...and its position"), Other->GetActorLocation().Equals(FVector(500.0f, 0.0f, 0.0f)));

	// applying mustn't round-trip back into the record half-way through and clobber what it's applying
	TestEqual(TEXT("The record still says what it said"), Store->FindRecord(Other->GetRecordId())->Health, 40.0f);
	TestEqual(TEXT("...carrying exactly the one entry"), Store->FindRecord(Other->GetRecordId())->Carried.Num(), 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresRecordSyncAdoptTest,
	"Smores.Characters.RecordSync.PlacedUnitAdoptsItsRecordDeadOrAlive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresRecordSyncAdoptTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = SmoresRecordSyncTest_BeginWithStore(TestWorld);
	UCharacterDefinition* Definition = MakeTestCharacterDefinition(TestWorld, TEXT("TestBoss"));
	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), 5);

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definitions created"), Definition) || !Apple)
	{
		return true;
	}

	Definition->DefaultLoadout = { MakeTestItem(Apple, 2) };

	EXPECT_UNIT_COMBAT_LOGS();

	const FGuid Key = FGuid::NewGuid();
	ATestStrategyNPC* First = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, Key, FVector::ZeroVector);

	if (!TestNotNull(TEXT("First unit spawned"), First))
	{
		return true;
	}

	First->GetHealth()->Kill();
	First->Destroy();

	TestNotNull(TEXT("The record outlives its actor"), Store->FindRecord(Key));
	TestNull(TEXT("...and is no longer bound to anything"), Store->GetBoundActor(Key));

	// the reload: the level stands a new actor up with the same authored key
	ATestStrategyNPC* Second = SmoresRecordSyncTest_SpawnUnit(TestWorld, Definition, Key, FVector(300.0f, 0.0f, 0.0f));

	if (!TestNotNull(TEXT("Second unit spawned"), Second))
	{
		return true;
	}

	TestTrue(TEXT("The new actor adopts the existing record"), Second->GetRecordId() == Key);
	TestEqual(TEXT("...rather than creating another"), Store->GetRecords().Num(), 1);
	TestTrue(TEXT("The boss stays dead"), Second->IsDead());
	TestEqual(TEXT("...holding what he died with, and the loadout isn't handed out twice"), GetTotalQuantity(Second->GetInventory()), 2);
	TestTrue(TEXT("...where he fell, not where the new actor was placed"), Second->GetActorLocation().Equals(FVector::ZeroVector));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresRecordSyncCollisionTest,
	"Smores.Characters.RecordSync.SharedKeyAndSecondUniqueAreRefused",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresRecordSyncCollisionTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = SmoresRecordSyncTest_BeginWithStore(TestWorld);
	UCharacterDefinition* Bandit = MakeTestCharacterDefinition(TestWorld, TEXT("TestBandit"));
	UCharacterDefinition* Kess = MakeTestCharacterDefinition(TestWorld, TEXT("TestKess"), /*bUnique*/ true);

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definitions created"), Bandit) || !Kess)
	{
		return true;
	}

	// two placed actors sharing a key - a copy made outside the editor's paste path
	AddExpectedMessagePlain(TEXT("is already bound to"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);

	const FGuid Key = FGuid::NewGuid();
	ATestStrategyNPC* First = SmoresRecordSyncTest_SpawnUnit(TestWorld, Bandit, Key, FVector::ZeroVector);
	ATestStrategyNPC* Copy = SmoresRecordSyncTest_SpawnUnit(TestWorld, Bandit, Key, FVector::ZeroVector);

	if (!TestNotNull(TEXT("Both units spawned"), First) || !Copy)
	{
		return true;
	}

	TestTrue(TEXT("The first keeps the key"), First->GetRecordId() == Key);
	TestTrue(TEXT("The copy gets a record of its own instead of sharing"), Copy->GetRecordId().IsValid() && Copy->GetRecordId() != Key);
	TestEqual(TEXT("...so there are two records"), Store->GetRecords().Num(), 2);

	// two placed Kesses - the second must not become a second Kess
	AddExpectedMessagePlain(TEXT("is unique and already has a record"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("could not be given a record"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);

	ATestStrategyNPC* RealKess = SmoresRecordSyncTest_SpawnUnit(TestWorld, Kess, FGuid::NewGuid(), FVector::ZeroVector);
	ATestStrategyNPC* SecondKess = SmoresRecordSyncTest_SpawnUnit(TestWorld, Kess, FGuid::NewGuid(), FVector::ZeroVector);

	if (!TestNotNull(TEXT("Both Kesses spawned"), RealKess) || !SecondKess)
	{
		return true;
	}

	TestTrue(TEXT("The first Kess has a record"), RealKess->GetRecordId().IsValid());
	TestFalse(TEXT("The second is refused one"), SecondKess->GetRecordId().IsValid());
	TestEqual(TEXT("Three records in all"), Store->GetRecords().Num(), 3);

	TestWorld.ForwardErrors(this);

	return true;
}

#undef EXPECT_UNIT_COMBAT_LOGS

#endif // WITH_DEV_AUTOMATION_TESTS
