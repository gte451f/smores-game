// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SmoresStrategyTestActors.h"
#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "DialogCondition.h"
#include "DialogFacts.h"
#include "HealthComponent.h"
#include "PlayerStandingComponent.h"
#include "GameFramework/PlayerState.h"
#include "Tests/SmoresCharacterTestFactory.h"
#include "Tests/SmoresTestWorld.h"
#include "Engine/World.h"

/**
 *  The built-in dialog facts, answered by real units - the one dialog test that needs a world.
 *
 *  Every other dialog test answers facts from a map (SmoresDialog's own Tests/). This one proves the
 *  real answers come from the right places: definition and role from the character definition,
 *  faction and name from the record the unit stands in for, health and life state from its health
 *  component, standing from the player's standing component. Lives in the smores module because the
 *  concrete unit stand-in does (SmoresStrategyTestActors.h).
 */

/** Spawns the NPC stand-in authored the way a placed unit is, and lets it begin play */
static ATestStrategyNPC* SmoresDialogFactsTest_SpawnUnit(FSmoresTestWorld& TestWorld, UCharacterDefinition* Definition, const FVector& Location, const FText& PlacedName = FText::GetEmpty())
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

	Unit->SetCharacterForTest(Definition, FGuid::NewGuid(), PlacedName);
	Unit->FinishSpawning(Transform);

	return Unit;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogFactsFromUnitsTest,
	"Smores.Dialog.Facts.AnswerFromRealUnits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogFactsFromUnitsTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UCharacterRecordComponent* Store = TestWorld.BeginPlay() ? MakeTestRecordStore(TestWorld) : nullptr;
	UWorld* World = TestWorld.GetWorld();

	// a faction the world doesn't know, on purpose: it keeps the test off the real faction assets,
	// and the record store keeps an unknown id with a warning - which is asserted, not suppressed
	UCharacterDefinition* Guard = MakeTestCharacterDefinition(TestWorld, TEXT("TestGuard"), false, TArray<FText>(), TEXT("TestRaiders"));
	UCharacterDefinition* Settler = MakeTestCharacterDefinition(TestWorld, TEXT("TestSettler"));

	if (!TestNotNull(TEXT("Store created"), Store) || !World || !Guard || !Settler)
	{
		return true;
	}

	Guard->RoleId = TEXT("guard");

	AddExpectedMessagePlain(TEXT("which this world doesn't know"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	ATestStrategyNPC* Speaker = SmoresDialogFactsTest_SpawnUnit(TestWorld, Guard, FVector::ZeroVector, FText::FromString(TEXT("Merchant Ada")));
	ATestStrategyNPC* Listener = SmoresDialogFactsTest_SpawnUnit(TestWorld, Settler, FVector(200.0f, 0.0f, 0.0f));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APlayerState* Player = World->SpawnActor<APlayerState>(APlayerState::StaticClass(), FTransform::Identity, SpawnParameters);
	UPlayerStandingComponent* Standing = TestWorld.AddComponent<UPlayerStandingComponent>(Player);

	if (!TestNotNull(TEXT("Speaker spawned"), Speaker) || !TestNotNull(TEXT("Listener spawned"), Listener) || !TestNotNull(TEXT("Standing created"), Standing))
	{
		return true;
	}

	Standing->SetStanding(TEXT("TestRaiders"), -35);

	FDialogContext Context;
	Context.Event = EBarkEvent::TradeOpened;
	Context.Speaker = Speaker;
	Context.Listener = Listener;
	Context.Player = Player;

	const FDialogFactRegistry Facts = FDialogFactRegistry::MakeBuiltIn();

	auto Ask = [&Facts, &Context](const TCHAR* Fact)
	{
		return Facts.Ask(FName(Fact), Context).ToString();
	};

	TestEqual(TEXT("Speaker.Definition is the character definition's id"), Ask(TEXT("Speaker.Definition")), FString(TEXT("TestGuard")));
	TestEqual(TEXT("Speaker.Role is the definition's role"), Ask(TEXT("Speaker.Role")), FString(TEXT("guard")));
	TestEqual(TEXT("Speaker.Faction is the record's faction"), Ask(TEXT("Speaker.Faction")), FString(TEXT("TestRaiders")));
	TestEqual(TEXT("Speaker.Name is the name the game shows"), Ask(TEXT("Speaker.Name")), FString(TEXT("Merchant Ada")));
	TestEqual(TEXT("Speaker.LifeState starts Alive"), Ask(TEXT("Speaker.LifeState")), FString(TEXT("Alive")));
	TestEqual(TEXT("Speaker.Health starts full"), Ask(TEXT("Speaker.Health")), FString(TEXT("1.0")));

	TestEqual(TEXT("Listener.Definition reads the other unit"), Ask(TEXT("Listener.Definition")), FString(TEXT("TestSettler")));
	TestEqual(TEXT("An unaffiliated listener's faction is None"), Ask(TEXT("Listener.Faction")), FString(TEXT("None")));
	TestEqual(TEXT("...and with no role authored, so is its role"), Ask(TEXT("Listener.Role")), FString(TEXT("None")));

	TestEqual(TEXT("StandingWithSpeaker is this player's standing with the speaker's faction"), Ask(TEXT("StandingWithSpeaker")), FString(TEXT("-35.0")));
	TestEqual(TEXT("A subject the moment doesn't have answers unset"), Ask(TEXT("Event.Victim.Definition")), FString(TEXT("(none)")));

	// a real condition, compiled for a real event, against the same moment
	FDialogCondition Condition;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	const bool bCompiled = SmoresDialog::CompileCondition(TEXT("Speaker.Role == guard; StandingWithSpeaker <= -20; Listener.Definition == TestSettler"),
		Facts, SmoresDialog::GetEventSubjects(EBarkEvent::TradeOpened), nullptr, Condition, Errors, Warnings);

	if (TestTrue(FString::Printf(TEXT("A three-clause condition compiles (%s)"), *FString::Join(Errors, TEXT(" | "))), bCompiled))
	{
		TestTrue(TEXT("...and holds for this moment"), Condition.Evaluate(Context, Facts));
	}

	// every hit spawns a damage number no test has a class for, and dying logs too - asserted, so
	// the run stays green rather than yellow
	AddExpectedMessagePlain(TEXT("[Combat]"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);

	Speaker->GetHealth()->TakeDamage(Speaker->GetHealth()->MaxHealth * 0.25f);

	TestEqual(TEXT("Speaker.Health follows the health component, as a fraction"), Ask(TEXT("Speaker.Health")), FString(TEXT("0.75")));

	Speaker->GetHealth()->Kill();

	TestEqual(TEXT("Speaker.LifeState follows the health state"), Ask(TEXT("Speaker.LifeState")), FString(TEXT("Dead")));
	TestEqual(TEXT("...and a dead speaker's health is 0"), Ask(TEXT("Speaker.Health")), FString(TEXT("0.0")));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
