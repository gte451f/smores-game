// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FactionDefinition.h"
#include "PlayerStandingComponent.h"
#include "WorldFactionComponent.h"
#include "Tests/SmoresTestDelegateListener.h"
#include "Tests/SmoresTestWorld.h"

/** A tier's name, so a failed assertion reads "Minor, expected Major" rather than comparing two bytes */
static FString SmoresFactionTest_TierName(EFactionTier Tier)
{
	return StaticEnum<EFactionTier>()->GetNameStringByValue(static_cast<int64>(Tier));
}

/**
 *  An in-memory faction definition. Never loaded from Content/ - a test that loads a real faction
 *  asset would break the day a designer retunes Ironclan's starting relations.
 *
 *  Uniquely prefixed statics rather than an anonymous namespace, because unity builds merge test
 *  files into one translation unit - see testing.md.
 */
static UFactionDefinition* SmoresFactionTest_MakeFaction(FSmoresTestWorld& TestWorld, const TCHAR* Id, EFactionTier StartingTier = EFactionTier::Minor)
{
	UFactionDefinition* Definition = TestWorld.NewKeptObject<UFactionDefinition>();
	Definition->DefinitionId = FName(Id);
	Definition->DisplayName = FText::FromString(Id);
	Definition->ShortName = FText::FromString(Id);
	Definition->StartingTier = StartingTier;

	return Definition;
}

static void SmoresFactionTest_AddRelation(UFactionDefinition* From, UFactionDefinition* To, int32 Standing)
{
	FFactionStartingRelation& Relation = From->StartingRelations.AddDefaulted_GetRef();
	Relation.Faction = To;
	Relation.Standing = Standing;
}

/** A world faction component on an authoritative owner, seeded with Alpha, Bravo and Charlie */
static UWorldFactionComponent* SmoresFactionTest_MakeWorld(FSmoresTestWorld& TestWorld)
{
	UWorldFactionComponent* WorldFactions = TestWorld.SpawnComponent<UWorldFactionComponent>();

	if (WorldFactions)
	{
		WorldFactions->InitializeFromDefinitions({
			SmoresFactionTest_MakeFaction(TestWorld, TEXT("Alpha")),
			SmoresFactionTest_MakeFaction(TestWorld, TEXT("Bravo")),
			SmoresFactionTest_MakeFaction(TestWorld, TEXT("Charlie")) });
	}

	return WorldFactions;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionStandingIsSymmetricTest,
	"Smores.Core.Factions.StandingBetweenIsSymmetric",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionStandingIsSymmetricTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UWorldFactionComponent* WorldFactions = SmoresFactionTest_MakeWorld(TestWorld);

	if (!TestNotNull(TEXT("World factions created"), WorldFactions))
	{
		return true;
	}

	const FName Alpha(TEXT("Alpha"));
	const FName Charlie(TEXT("Charlie"));

	// written one way round...
	TestTrue(TEXT("Charlie/Alpha accepts a standing"), WorldFactions->SetStandingBetween(Charlie, Alpha, 30));

	// ...read the other
	TestEqual(TEXT("Alpha/Charlie reads the same cell"), WorldFactions->GetStandingBetween(Alpha, Charlie), 30);
	TestEqual(TEXT("Charlie/Alpha reads it too"), WorldFactions->GetStandingBetween(Charlie, Alpha), 30);

	// and overwritten the other way round, which must not open a second cell
	TestTrue(TEXT("Alpha/Charlie accepts an overwrite"), WorldFactions->SetStandingBetween(Alpha, Charlie, -10));
	TestEqual(TEXT("Charlie/Alpha sees the overwrite"), WorldFactions->GetStandingBetween(Charlie, Alpha), -10);
	TestEqual(TEXT("One pair is one cell, whichever way it was written"), WorldFactions->GetStandingMatrix().Num(), 1);

	if (WorldFactions->GetStandingMatrix().Num() == 1)
	{
		// lexical order is the key, so a save reads back under the same pair on any machine
		TestEqual(TEXT("The cell is keyed lexically - Alpha first"), WorldFactions->GetStandingMatrix()[0].FirstId, Alpha);
		TestEqual(TEXT("...Charlie second"), WorldFactions->GetStandingMatrix()[0].SecondId, Charlie);
	}

	TestTrue(TEXT("Adjusting from either side moves the same cell"), WorldFactions->AdjustStandingBetween(Charlie, Alpha, 25));
	TestEqual(TEXT("-10 + 25"), WorldFactions->GetStandingBetween(Alpha, Charlie), 15);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionStandingClampsTest,
	"Smores.Core.Factions.StandingClamps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionStandingClampsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UWorldFactionComponent* WorldFactions = SmoresFactionTest_MakeWorld(TestWorld);
	UPlayerStandingComponent* PlayerStanding = TestWorld.SpawnComponent<UPlayerStandingComponent>();

	if (!TestNotNull(TEXT("World factions created"), WorldFactions) || !TestNotNull(TEXT("Player standing created"), PlayerStanding))
	{
		return true;
	}

	const FName Alpha(TEXT("Alpha"));
	const FName Bravo(TEXT("Bravo"));

	// clamping rather than refusing: a huge penalty on a hated faction should land at the floor,
	// not be thrown away and leave them where they were
	TestTrue(TEXT("An out-of-range pair standing is accepted..."), WorldFactions->SetStandingBetween(Alpha, Bravo, 500));
	TestEqual(TEXT("...and clamped to the ceiling"), WorldFactions->GetStandingBetween(Alpha, Bravo), SmoresStanding::Max);

	TestTrue(TEXT("Adjusting past the floor is accepted..."), WorldFactions->AdjustStandingBetween(Alpha, Bravo, -1000));
	TestEqual(TEXT("...and clamped to the floor"), WorldFactions->GetStandingBetween(Alpha, Bravo), SmoresStanding::Min);

	// the adjust widens before adding, so an extreme delta clamps instead of wrapping round
	TestTrue(TEXT("An extreme delta is accepted"), WorldFactions->AdjustStandingBetween(Alpha, Bravo, MAX_int32));
	TestEqual(TEXT("...and lands on the ceiling rather than wrapping"), WorldFactions->GetStandingBetween(Alpha, Bravo), SmoresStanding::Max);

	TestTrue(TEXT("The player half clamps the same way..."), PlayerStanding->SetStanding(Alpha, -250));
	TestEqual(TEXT("...to the floor"), PlayerStanding->GetStanding(Alpha), SmoresStanding::Min);

	TestTrue(TEXT("A player adjust is accepted"), PlayerStanding->AdjustStanding(Alpha, 130));
	TestEqual(TEXT("-100 + 130"), PlayerStanding->GetStanding(Alpha), 30);

	TestTrue(TEXT("A player adjust past the ceiling is accepted"), PlayerStanding->AdjustStanding(Alpha, MAX_int32));
	TestEqual(TEXT("...and clamped rather than wrapped"), PlayerStanding->GetStanding(Alpha), SmoresStanding::Max);

	// initial authored relations go through the same clamp - ClampMin/Max on the property only
	// guards the editor's details panel, not a value set any other way
	UFactionDefinition* Loud = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Loud"));
	UFactionDefinition* Quiet = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Quiet"));
	SmoresFactionTest_AddRelation(Loud, Quiet, -900);

	WorldFactions->InitializeFromDefinitions({ Loud, Quiet });

	TestEqual(TEXT("A starting relation authored out of range is clamped on the way in"),
		WorldFactions->GetStandingBetween(Loud->DefinitionId, Quiet->DefinitionId), SmoresStanding::Min);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionUnknownIsNeutralTest,
	"Smores.Core.Factions.UnknownFactionIsNeutral",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionUnknownIsNeutralTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UWorldFactionComponent* WorldFactions = SmoresFactionTest_MakeWorld(TestWorld);
	UPlayerStandingComponent* PlayerStanding = TestWorld.SpawnComponent<UPlayerStandingComponent>();

	if (!TestNotNull(TEXT("World factions created"), WorldFactions) || !TestNotNull(TEXT("Player standing created"), PlayerStanding))
	{
		return true;
	}

	const FName Alpha(TEXT("Alpha"));
	const FName Bravo(TEXT("Bravo"));
	const FName Stripped(TEXT("StrippedModFaction"));

	TestEqual(TEXT("Two known factions nobody has set start neutral"), WorldFactions->GetStandingBetween(Alpha, Bravo), SmoresStanding::Neutral);
	TestEqual(TEXT("An unknown faction is regarded neutrally"), WorldFactions->GetStandingBetween(Alpha, Stripped), SmoresStanding::Neutral);
	TestEqual(TEXT("...from either side"), WorldFactions->GetStandingBetween(Stripped, Alpha), SmoresStanding::Neutral);
	TestEqual(TEXT("None is neutral too"), WorldFactions->GetStandingBetween(NAME_None, Alpha), SmoresStanding::Neutral);
	TestEqual(TEXT("A known faction regards itself at the ceiling"), WorldFactions->GetStandingBetween(Alpha, Alpha), SmoresStanding::Max);
	TestEqual(TEXT("...but an unknown one is still just neutral"), WorldFactions->GetStandingBetween(Stripped, Stripped), SmoresStanding::Neutral);

	EFactionTier Tier = EFactionTier::Nomadic;
	TestFalse(TEXT("An unknown faction has no tier"), WorldFactions->GetFactionTier(Stripped, Tier));
	TestEqual(TEXT("...and the out-param is left alone"), SmoresFactionTest_TierName(Tier), SmoresFactionTest_TierName(EFactionTier::Nomadic));
	TestFalse(TEXT("An unknown faction has no record"), WorldFactions->FindRecord(Stripped) != nullptr);

	// writes naming an unknown faction are refused rather than inventing a cell for nothing
	TestFalse(TEXT("A pair standing with an unknown faction is refused"), WorldFactions->SetStandingBetween(Alpha, Stripped, 50));
	TestFalse(TEXT("A faction's standing with itself is refused"), WorldFactions->SetStandingBetween(Alpha, Alpha, 50));
	TestFalse(TEXT("An unknown faction's tier is refused"), WorldFactions->SetFactionTier(Stripped, EFactionTier::Major));
	TestEqual(TEXT("...and none of them wrote a cell"), WorldFactions->GetStandingMatrix().Num(), 0);
	TestEqual(TEXT("...or a record"), WorldFactions->GetRecords().Num(), 3);

	TestEqual(TEXT("A player starts neutral with a faction they've never dealt with"), PlayerStanding->GetStanding(Alpha), SmoresStanding::Neutral);
	TestEqual(TEXT("...and with one that doesn't exist"), PlayerStanding->GetStanding(Stripped), SmoresStanding::Neutral);
	TestFalse(TEXT("A player standing with None is refused"), PlayerStanding->SetStanding(NAME_None, 20));

	// deliberate: an id is allowed to name a faction that no longer resolves - see the component
	TestTrue(TEXT("A player standing with an unresolved id is stored anyway"), PlayerStanding->SetStanding(Stripped, 20));
	TestEqual(TEXT("...and answers its own number"), PlayerStanding->GetStanding(Stripped), 20);

	TestTrue(TEXT("A first write of neutral is accepted"), PlayerStanding->SetStanding(Bravo, SmoresStanding::Neutral));
	TestEqual(TEXT("...without spending an entry on it"), PlayerStanding->GetStandings().Num(), 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionRecordsSeedFromDefinitionsTest,
	"Smores.Core.Factions.RecordsSeedFromDefinitions",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionRecordsSeedFromDefinitionsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UWorldFactionComponent* WorldFactions = TestWorld.SpawnComponent<UWorldFactionComponent>();

	if (!TestNotNull(TEXT("World factions created"), WorldFactions))
	{
		return true;
	}

	UFactionDefinition* Zulu = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Zulu"), EFactionTier::Major);
	UFactionDefinition* Alpha = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Alpha"), EFactionTier::Nomadic);
	UFactionDefinition* Duplicate = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Alpha"), EFactionTier::Major);
	UFactionDefinition* Blank = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Blank"));
	Blank->DefinitionId = NAME_None;

	// authored on Zulu only - it has to land symmetrically anyway
	SmoresFactionTest_AddRelation(Zulu, Alpha, -40);

	AddExpectedMessagePlain(TEXT("share the id"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("no DefinitionId"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

	// the null and the blank are skipped, the duplicate id is skipped, and what's left is counted
	const int32 Created = WorldFactions->InitializeFromDefinitions({ Zulu, nullptr, Alpha, Duplicate, Blank });

	TestEqual(TEXT("Two records created - the null, the blank id and the duplicate id are all skipped"), Created, 2);
	TestEqual(TEXT("...and there are exactly that many"), WorldFactions->GetRecords().Num(), 2);

	if (WorldFactions->GetRecords().Num() == 2)
	{
		// id order, whatever order they were handed in
		TestEqual(TEXT("Records are in id order - Alpha first"), WorldFactions->GetRecords()[0].FactionId, FName(TEXT("Alpha")));
	}

	EFactionTier Tier = EFactionTier::Minor;
	TestTrue(TEXT("Alpha has a tier"), WorldFactions->GetFactionTier(TEXT("Alpha"), Tier));
	TestEqual(TEXT("...its first definition's starting tier, not the duplicate's"), SmoresFactionTest_TierName(Tier), SmoresFactionTest_TierName(EFactionTier::Nomadic));

	TestEqual(TEXT("A relation authored on one side reads from the other"),
		WorldFactions->GetStandingBetween(TEXT("Alpha"), TEXT("Zulu")), -40);

	// the record/definition split this slice exists to establish: a tier change is the record's
	TestTrue(TEXT("Zulu can be moved to another tier"), WorldFactions->SetFactionTier(TEXT("Zulu"), EFactionTier::Minor));
	TestTrue(TEXT("Zulu has a tier"), WorldFactions->GetFactionTier(TEXT("Zulu"), Tier));
	TestEqual(TEXT("...the record moved"), SmoresFactionTest_TierName(Tier), SmoresFactionTest_TierName(EFactionTier::Minor));
	TestEqual(TEXT("...and the definition didn't"), SmoresFactionTest_TierName(Zulu->StartingTier), SmoresFactionTest_TierName(EFactionTier::Major));

	// a second initialisation is a fresh campaign start, not an append - and Zulu's relation to
	// Alpha now names a faction that isn't there, which is skipped (and said so) rather than
	// inventing a cell for it
	AddExpectedMessagePlain(TEXT("which is not another known faction"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	TestEqual(TEXT("Re-initialising replaces rather than appends"), WorldFactions->InitializeFromDefinitions({ Zulu }), 1);
	TestEqual(TEXT("...one record"), WorldFactions->GetRecords().Num(), 1);
	TestEqual(TEXT("...and no pair naming the faction that went away"), WorldFactions->GetStandingMatrix().Num(), 0);
	TestTrue(TEXT("Zulu has a tier again"), WorldFactions->GetFactionTier(TEXT("Zulu"), Tier));
	TestEqual(TEXT("...back at its starting tier"), SmoresFactionTest_TierName(Tier), SmoresFactionTest_TierName(EFactionTier::Major));

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionBroadcastsTest,
	"Smores.Core.Factions.ChangesBroadcastOnlyWhenSomethingChanged",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionBroadcastsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UWorldFactionComponent* WorldFactions = SmoresFactionTest_MakeWorld(TestWorld);
	UPlayerStandingComponent* PlayerStanding = TestWorld.SpawnComponent<UPlayerStandingComponent>();

	USmoresTestDelegateListener* WorldListener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();
	USmoresTestDelegateListener* PlayerListener = TestWorld.NewKeptObject<USmoresTestDelegateListener>();

	if (!TestNotNull(TEXT("World factions created"), WorldFactions) || !TestNotNull(TEXT("Player standing created"), PlayerStanding))
	{
		return true;
	}

	WorldFactions->OnFactionsChanged.AddDynamic(WorldListener, &USmoresTestDelegateListener::OnChanged);
	PlayerStanding->OnStandingChanged.AddDynamic(PlayerListener, &USmoresTestDelegateListener::OnChanged);

	const FName Alpha(TEXT("Alpha"));
	const FName Bravo(TEXT("Bravo"));

	WorldFactions->SetStandingBetween(Alpha, Bravo, 20);
	TestEqual(TEXT("A pair change broadcasts once"), WorldListener->CallCount, 1);

	WorldFactions->SetStandingBetween(Bravo, Alpha, 20);
	TestEqual(TEXT("Setting the same value from the other side is not a change"), WorldListener->CallCount, 1);

	WorldFactions->SetStandingBetween(Alpha, TEXT("Nobody"), 20);
	TestEqual(TEXT("A refused write does not broadcast"), WorldListener->CallCount, 1);

	WorldFactions->SetFactionTier(Alpha, EFactionTier::Minor);
	TestEqual(TEXT("A tier set to the tier it already has does not broadcast"), WorldListener->CallCount, 1);

	WorldFactions->SetFactionTier(Alpha, EFactionTier::Major);
	TestEqual(TEXT("A real tier change does"), WorldListener->CallCount, 2);

	PlayerStanding->AdjustStanding(Alpha, 5);
	TestEqual(TEXT("A player standing change broadcasts once"), PlayerListener->CallCount, 1);

	PlayerStanding->SetStanding(Alpha, 5);
	TestEqual(TEXT("...and setting it to what it already is does not"), PlayerListener->CallCount, 1);

	PlayerStanding->SetStanding(NAME_None, 5);
	TestEqual(TEXT("...nor does a refused write"), PlayerListener->CallCount, 1);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionMutationsNeedAuthorityTest,
	"Smores.Core.Factions.MutationsNeedAuthority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionMutationsNeedAuthorityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// Ownerless ON PURPOSE, like SortWithoutAuthorityIsSilent: with no owner, HasOwnerAuthority()
	// is false, which is the only way to reach the refusing branch without a net driver. It is
	// not a client - it proves the gate refuses, not that replication works.
	UWorldFactionComponent* WorldFactions = TestWorld.NewKeptObject<UWorldFactionComponent>();
	UPlayerStandingComponent* PlayerStanding = TestWorld.NewKeptObject<UPlayerStandingComponent>();

	if (!TestNotNull(TEXT("World factions created"), WorldFactions) || !TestNotNull(TEXT("Player standing created"), PlayerStanding))
	{
		return true;
	}

	// every mutator warns when refused off-authority; asserting the warning proves each one
	// reached its gate rather than failing earlier for some other reason
	AddExpectedMessagePlain(TEXT("non-authority machine"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 6);

	UFactionDefinition* Alpha = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Alpha"));
	UFactionDefinition* Bravo = SmoresFactionTest_MakeFaction(TestWorld, TEXT("Bravo"));

	TestEqual(TEXT("Initialising off-authority is refused"), WorldFactions->InitializeFromDefinitions({ Alpha, Bravo }), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("...and created no records"), WorldFactions->GetRecords().Num(), 0);

	TestFalse(TEXT("SetStandingBetween is refused"), WorldFactions->SetStandingBetween(Alpha->DefinitionId, Bravo->DefinitionId, 50));
	TestFalse(TEXT("AdjustStandingBetween is refused"), WorldFactions->AdjustStandingBetween(Alpha->DefinitionId, Bravo->DefinitionId, 50));
	TestFalse(TEXT("SetFactionTier is refused"), WorldFactions->SetFactionTier(Alpha->DefinitionId, EFactionTier::Major));
	TestEqual(TEXT("...and the matrix is still empty"), WorldFactions->GetStandingMatrix().Num(), 0);

	TestFalse(TEXT("SetStanding is refused"), PlayerStanding->SetStanding(Alpha->DefinitionId, 50));
	TestFalse(TEXT("AdjustStanding is refused"), PlayerStanding->AdjustStanding(Alpha->DefinitionId, 50));
	TestEqual(TEXT("...and the player's standing is untouched"), PlayerStanding->GetStanding(Alpha->DefinitionId), SmoresStanding::Neutral);
	TestEqual(TEXT("...with no entry written"), PlayerStanding->GetStandings().Num(), 0);

	// the positive half of the pair: the same calls on an authoritative owner go through
	UPlayerStandingComponent* AuthoritativeStanding = TestWorld.SpawnComponent<UPlayerStandingComponent>();

	if (TestNotNull(TEXT("Authoritative player standing created"), AuthoritativeStanding))
	{
		TestTrue(TEXT("With authority, SetStanding goes through"), AuthoritativeStanding->SetStanding(Alpha->DefinitionId, 50));
		TestEqual(TEXT("...and stuck"), AuthoritativeStanding->GetStanding(Alpha->DefinitionId), 50);
	}

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
