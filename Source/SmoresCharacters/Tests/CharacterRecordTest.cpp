// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "Tests/SmoresCharacterTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The record store on its own: creating a record from a definition, the unique rule, name
 *  generation, the one-actor-per-record binding, and authority gating. No unit is involved -
 *  the round trip between a unit and its record is the smores module's RecordSync group, which
 *  needs a concrete AStrategyUnit to spawn.
 *
 *  The load-bearing test is UniqueDefinitionHasOneRecord. A unique character is authored once
 *  and recorded once, alive or dead - if a second record can ever be made, a reload stands a
 *  dead named character back up next to his own corpse.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordCreateTest,
	"Smores.Characters.Records.CreateFromDefinition",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordCreateTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	UCharacterDefinition* Definition = MakeTestCharacterDefinition(TestWorld, TEXT("TestBandit"), false,
		{ FText::FromString(TEXT("Oskar")) });

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definition created"), Definition))
	{
		return true;
	}

	Definition->BaseAttributes.Strength = 14.0f;
	Definition->BaseAttributes.Charisma = 6.0f;

	const FGuid Requested = FGuid::NewGuid();
	const FGuid Created = Store->CreateRecord(Definition, Requested);

	TestTrue(TEXT("The record takes the requested id"), Created == Requested);

	const FCharacterRecord* Record = Store->FindRecord(Created);

	if (!TestNotNull(TEXT("The record can be found by its id"), Record))
	{
		return true;
	}

	TestEqual(TEXT("It names its definition by id"), Record->DefinitionId, FName(TEXT("TestBandit")));
	TestTrue(TEXT("It starts with the definition's attributes"), Record->Attributes == Definition->BaseAttributes);
	TestEqual(TEXT("It is named from the pool"), Record->Name.ToString(), FString(TEXT("Oskar")));
	TestTrue(TEXT("It starts Alive"), Record->LifeState == EHealthState::Alive);
	TestTrue(TEXT("Its condition half is empty until an actor writes back"), Record->Carried.Num() == 0 && Record->Equipped.Num() == 0);

	// a record is the truth, the definition only its starting point - editing one must never touch the other
	Store->EditRecord(Created)->Attributes.Strength = 20.0f;

	TestEqual(TEXT("Changing the record's attributes leaves the definition's alone"), Definition->BaseAttributes.Strength, 14.0f);

	const FGuid Minted = Store->CreateRecord(Definition, FGuid());

	TestTrue(TEXT("An invalid requested id mints a fresh one"), Minted.IsValid() && Minted != Created);

	AddExpectedMessagePlain(TEXT("already exists"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	TestFalse(TEXT("A second record with an existing id is refused"), Store->CreateRecord(Definition, Requested).IsValid());
	TestEqual(TEXT("...leaving two records"), Store->GetRecords().Num(), 2);

	// a unit authored before it had a definition still gets a record, named by the level designer
	const FGuid Bare = Store->CreateRecord(nullptr, FGuid(), FText::FromString(TEXT("NPC 7")));
	const FCharacterRecord* BareRecord = Store->FindRecord(Bare);

	if (TestNotNull(TEXT("A record with no definition is allowed"), BareRecord))
	{
		TestTrue(TEXT("...naming no definition"), BareRecord->DefinitionId.IsNone());
		TestEqual(TEXT("...and carrying the name it was given"), BareRecord->Name.ToString(), FString(TEXT("NPC 7")));
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordUniqueTest,
	"Smores.Characters.Records.UniqueDefinitionHasOneRecord",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordUniqueTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	UCharacterDefinition* Kess = MakeTestCharacterDefinition(TestWorld, TEXT("WarlordKess"), /*bUnique*/ true,
		{ FText::FromString(TEXT("Not Kess")) });

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definition created"), Kess))
	{
		return true;
	}

	Kess->DisplayName = FText::FromString(TEXT("Warlord Kess"));

	const FGuid First = Store->CreateRecord(Kess, FGuid(), FText::FromString(TEXT("A placed nickname")));
	const FCharacterRecord* Record = Store->FindRecord(First);

	if (!TestNotNull(TEXT("The first record of a unique definition is created"), Record))
	{
		return true;
	}

	TestEqual(TEXT("A unique character is named by the definition, ignoring the pool and any override"),
		Record->Name.ToString(), FString(TEXT("Warlord Kess")));
	TestTrue(TEXT("The store knows a record of him exists"), Store->HasRecordOfDefinition(TEXT("WarlordKess")));

	AddExpectedMessagePlain(TEXT("is unique and already has a record"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 2);

	TestFalse(TEXT("A second record of a unique definition is refused"), Store->CreateRecord(Kess, FGuid()).IsValid());

	// dead is still a record - uniqueness is about existing, not about being alive
	Store->EditRecord(First)->LifeState = EHealthState::Dead;

	TestFalse(TEXT("...and still refused once the first is dead"), Store->CreateRecord(Kess, FGuid()).IsValid());
	TestEqual(TEXT("Exactly one record exists"), Store->GetRecords().Num(), 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordNameTest,
	"Smores.Characters.Records.NamesAreRolledFromTheIdDeterministically",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordNameTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	UCharacterDefinition* Settler = MakeTestCharacterDefinition(TestWorld, TEXT("TestSettler"), false, {
		FText::FromString(TEXT("Ada")), FText::FromString(TEXT("Bram")), FText::FromString(TEXT("Cyra")), FText::FromString(TEXT("Dov")) });
	UCharacterDefinition* Nameless = MakeTestCharacterDefinition(TestWorld, TEXT("TestNameless"));

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definitions created"), Settler) || !Nameless)
	{
		return true;
	}

	const FGuid Id(0x11111111, 0x22222222, 0x33333333, 0x44444444);

	const FString Once = UCharacterRecordComponent::RollName(Settler, Id).ToString();

	// save-scumming rule: the same individual gets the same name, however many times it's asked
	for (int32 Attempt = 0; Attempt < 5; ++Attempt)
	{
		TestEqual(TEXT("The same id rolls the same name every time"), UCharacterRecordComponent::RollName(Settler, Id).ToString(), Once);
	}

	TestEqual(TEXT("A record created with that id carries that name"),
		Store->FindRecord(Store->CreateRecord(Settler, Id))->Name.ToString(), Once);

	// ...and the roll actually spreads across the pool rather than always landing on one entry
	TSet<FString> Seen;

	for (uint32 Index = 0; Index < 64; ++Index)
	{
		Seen.Add(UCharacterRecordComponent::RollName(Settler, FGuid(Index, Index * 7, Index * 13, Index * 31)).ToString());
	}

	TestTrue(TEXT("64 different ids roll more than one of the four names"), Seen.Num() > 1);

	TestEqual(TEXT("An empty pool falls back to the definition's DisplayName"),
		UCharacterRecordComponent::RollName(Nameless, Id).ToString(), FString(TEXT("TestNameless")));

	// a blank entry is an authoring slip - the sweep rejects it, and the roll mustn't hand out a blank name
	Settler->NamePool = { FText::GetEmpty() };

	TestEqual(TEXT("A blank pool entry falls back to DisplayName too"),
		UCharacterRecordComponent::RollName(Settler, Id).ToString(), FString(TEXT("TestSettler")));

	Settler->NamePool = { FText::FromString(TEXT("Ada")) };

	const FGuid Placed = Store->CreateRecord(Settler, FGuid(), FText::FromString(TEXT("Pawn 1")));

	TestEqual(TEXT("A placed unit's authored name wins over the pool for a non-unique character"),
		Store->FindRecord(Placed)->Name.ToString(), FString(TEXT("Pawn 1")));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordBindingTest,
	"Smores.Characters.Records.OneLiveActorPerRecord",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordBindingTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	AActor* First = TestWorld.SpawnOwner();
	AActor* Second = TestWorld.SpawnOwner();

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Actors spawned"), First) || !Second)
	{
		return true;
	}

	const FGuid Id = Store->CreateRecord(nullptr, FGuid());

	TestFalse(TEXT("An unknown record can't be bound"), Store->BindActor(FGuid::NewGuid(), First));
	TestTrue(TEXT("The first actor binds"), Store->BindActor(Id, First));
	TestTrue(TEXT("...and re-binding the same actor is fine"), Store->BindActor(Id, First));
	TestFalse(TEXT("A second live actor is refused - two puppets would overwrite each other"), Store->BindActor(Id, Second));
	TestTrue(TEXT("The first stays bound"), Store->GetBoundActor(Id) == First);

	Store->UnbindActor(Id, Second);

	TestTrue(TEXT("Unbinding with the wrong actor does nothing"), Store->GetBoundActor(Id) == First);

	Store->UnbindActor(Id, First);

	TestNull(TEXT("Unbinding with the right one clears it"), Store->GetBoundActor(Id));
	TestNotNull(TEXT("...and the record itself stays - it outlives its puppet"), Store->FindRecord(Id));
	TestTrue(TEXT("Once free, another actor can bind"), Store->BindActor(Id, Second));

	// a destroyed actor lets go by itself - the binding is weak
	Second->Destroy();

	TestNull(TEXT("A destroyed actor's binding reads as empty"), Store->GetBoundActor(Id));
	TestTrue(TEXT("...so a new actor can take its place"), Store->BindActor(Id, First));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordFactionTest,
	"Smores.Characters.Records.UnknownFactionIsKeptWithAWarning",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordFactionTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	UCharacterDefinition* Definition = MakeTestCharacterDefinition(TestWorld, TEXT("TestModded"), false, {},
		FName(TEXT("SmoresTest_NoSuchFaction")));

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Definition created"), Definition))
	{
		return true;
	}

	// a stripped mod's faction: the save must still load, so this warns rather than refusing
	AddExpectedMessagePlain(TEXT("which this world doesn't know"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	const FCharacterRecord* Record = Store->FindRecord(Store->CreateRecord(Definition, FGuid()));

	if (TestNotNull(TEXT("A record with an unknown faction is still created"), Record))
	{
		TestEqual(TEXT("...and keeps the id it was given"), Record->FactionId, FName(TEXT("SmoresTest_NoSuchFaction")));
	}

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresCharacterRecordAuthorityTest,
	"Smores.Characters.Records.MutatorsRefuseWithoutAuthority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresCharacterRecordAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// the authoritative half first, so the refusals below are proven against a store that works
	UCharacterRecordComponent* Store = TestWorld.SpawnComponent<UCharacterRecordComponent>();
	AActor* Actor = TestWorld.SpawnOwner();

	if (!TestNotNull(TEXT("Store created"), Store) || !TestNotNull(TEXT("Actor spawned"), Actor))
	{
		return true;
	}

	const FGuid Id = Store->CreateRecord(nullptr, FGuid());

	TestTrue(TEXT("With authority, a record is created"), Id.IsValid());
	TestNotNull(TEXT("...can be edited"), Store->EditRecord(Id));
	TestTrue(TEXT("...and can be bound"), Store->BindActor(Id, Actor));

	// testing.md's one sanctioned exception: an ownerless component has no authority. Not a
	// client, but it proves each gate refuses rather than merely that it exists.
	UCharacterRecordComponent* Ownerless = NewObject<UCharacterRecordComponent>(GetTransientPackage());
	TestWorld.KeepAlive(Ownerless);

	AddExpectedMessagePlain(TEXT("CreateRecord called on a non-authority machine"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	TestFalse(TEXT("Off-authority, CreateRecord refuses"), Ownerless->CreateRecord(nullptr, FGuid()).IsValid());
	TestEqual(TEXT("...creating nothing"), Ownerless->GetRecords().Num(), 0);
	TestNull(TEXT("Off-authority, EditRecord hands out nothing"), Ownerless->EditRecord(Id));
	TestFalse(TEXT("Off-authority, BindActor refuses"), Ownerless->BindActor(Id, Actor));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
