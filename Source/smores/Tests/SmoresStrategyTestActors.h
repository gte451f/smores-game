// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "StrategyContainer.h"
#include "WorldItem.h"
#include "WorldDoor.h"
#include "CharacterDefinition.h"
#include "LootTableDefinition.h"
#include "ActionOrderHost.h"
#include "SmoresStrategyTestActors.generated.h"

/**
 *  Concrete stand-ins for the three abstract actor classes a target panel can describe.
 *
 *  **Why these have to exist.** AStrategyUnit, AStrategyPlayerUnit and AStrategyContainer are all
 *  UCLASS(abstract) - the project's standing rule, since a Blueprint subclass supplies the
 *  meshes - and an abstract class cannot be spawned at all. A test that wants to ask "what would
 *  the panel offer on a chest?" therefore needs something concrete to point at, and it must not
 *  be a real Blueprint out of Content/: testing.md's rule is that a test loading an authored
 *  asset is testing that asset too, so a designer retuning a chest would break an unrelated
 *  panel test and the failure would point at the wrong place.
 *
 *  **Why they aren't wrapped in WITH_DEV_AUTOMATION_TESTS**, unlike every test .cpp. UHT parses
 *  every header regardless of preprocessor conditions it doesn't know about, so guarding these
 *  would generate reflection code for classes the compiler had been told to skip. Same reasoning
 *  as USmoresTestDelegateListener - see testing.md. The cost in a packaged build is three class
 *  registrations of classes nothing ever spawns.
 *
 *  Each one disables AutoPossessAI, which the real classes turn on. A unit test has no navmesh
 *  and wants no AI controller; the panel's rules don't involve either.
 */
UCLASS(NotPlaceable, Hidden)
class ATestStrategyNPC : public AStrategyUnit
{
	GENERATED_BODY()

public:

	ATestStrategyNPC();

	/**
	 *  Turns this unit hostile, and nothing else.
	 *
	 *  AStrategyUnit::SetAggressive is deliberately *not* used, and the reason is a real piece of
	 *  behaviour worth knowing: going Aggressive immediately runs TryEngageNearestPlayerPawn,
	 *  which stands the unit straight back down when there is no player pawn to hunt - so a test
	 *  that turns an NPC hostile with no squad in the world finds it Passive a moment later. Put
	 *  a pawn in the world instead and the unit starts a real fight: a montage on a skeletal mesh
	 *  the test has no assets for, or an EQS move with no navmesh and no query asset.
	 *
	 *  Neither is what a test about *what the target panel offers* is asking about. This sets the
	 *  one piece of state the panel actually reads.
	 */
	void MakeHostileForTest() { Disposition = EStrategyDisposition::Aggressive; }

	/**
	 *  Authors what a placed unit would carry in the level - a character definition, a record key
	 *  and optionally a name - onto a unit that hasn't begun play yet. Call between
	 *  SpawnActorDeferred and FinishSpawning, since the unit finds or creates its record at
	 *  BeginPlay and reads all three there.
	 */
	void SetCharacterForTest(UCharacterDefinition* Definition, const FGuid& InPlacedRecordId, const FText& PlacedName = FText::GetEmpty())
	{
		CharacterDefinition = Definition;
		PlacedRecordId = InPlacedRecordId;
		SetAuthoredDisplayName(PlacedName);
	}

	/** Gives this unit a definition after spawning - for what the definition decides at any time (a person or a creature), not for records */
	void SetDefinitionForTest(UCharacterDefinition* Definition) { CharacterDefinition = Definition; }

	/** Sets the replicated attributes directly, as a record would - the action menu's odds read these */
	void SetAttributesForTest(const FCharacterAttributes& InAttributes) { Attributes = InAttributes; }

	/** Names the unit, as a level designer or its record would */
	void SetNameForTest(const FText& Name) { SetAuthoredDisplayName(Name); }
};

/** The player-pawn stand-in - the "one of your own squad" case. See ATestStrategyNPC. */
UCLASS(NotPlaceable, Hidden)
class ATestStrategyPlayerUnit : public AStrategyPlayerUnit
{
	GENERATED_BODY()

public:

	ATestStrategyPlayerUnit();

	/** Sets the replicated attributes directly, as a record would - see ATestStrategyNPC */
	void SetAttributesForTest(const FCharacterAttributes& InAttributes) { Attributes = InAttributes; }

	/** Names the unit, as a level designer or its record would */
	void SetNameForTest(const FText& Name) { SetAuthoredDisplayName(Name); }
};

/** The loose-item stand-in. See ATestStrategyNPC. */
UCLASS(NotPlaceable, Hidden)
class ATestWorldItem : public AWorldItem
{
	GENERATED_BODY()
};

/** The door stand-in. See ATestStrategyNPC. */
UCLASS(NotPlaceable, Hidden)
class ATestWorldDoor : public AWorldDoor
{
	GENERATED_BODY()
};

/**
 *  Stands in for the player controller as a unit's IActionOrderHost, counting what the order asks
 *  of it. Answers CanPerformAction through the real rules (FStrategyTargetActions::FindActionFor)
 *  against its own Squad, so an order test exercises the same re-check on arrival the controller
 *  does - without a controller, which a test world can't usefully have.
 */
UCLASS(NotPlaceable, Hidden)
class UTestActionOrderHost : public UObject, public IActionOrderHost
{
	GENERATED_BODY()

public:

	/**
	 *  The squad the rules treat as this player's own. Weak and deliberately **not** a UPROPERTY:
	 *  the host is kept alive for the whole test, so holding the units strongly would keep alive the
	 *  world they were spawned into, and the teardown then fails with "Previously active world not
	 *  cleaned up" - the same trap as USmoresTestDelegateListener::LastActor (testing.md).
	 */
	TArray<TWeakObjectPtr<AStrategyUnit>> Squad;

	int32 CanPerformCount = 0;
	int32 PerformCount = 0;
	int32 EndedCount = 0;

	FName LastPerformedAction;
	EActionOrderEnd LastEnd = EActionOrderEnd::TargetGone;
	ESmoresRefusalReason LastReason = ESmoresRefusalReason::None;

	/** Reset between steps of one test */
	void ResetCounts()
	{
		CanPerformCount = 0;
		PerformCount = 0;
		EndedCount = 0;
		LastPerformedAction = NAME_None;
		LastReason = ESmoresRefusalReason::None;
	}

	//~ Begin IActionOrderHost interface
	virtual bool CanPerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId, ESmoresRefusalReason& OutReason) override;
	virtual void PerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId) override;
	virtual void HandleActionOrderEnded(AStrategyUnit* Actor, AActor* Target, const FText& TargetName, FName ActionId, EActionOrderEnd Why, ESmoresRefusalReason Reason) override;
	//~ End IActionOrderHost interface
};

/** The container stand-in. See ATestStrategyNPC. */
UCLASS(NotPlaceable, Hidden)
class ATestStrategyContainer : public AStrategyContainer
{
	GENERATED_BODY()

public:

	/**
	 *  Authors what a placed chest would carry in the level - a loot table, a roll key and any
	 *  hand-placed items - onto a container that hasn't begun play yet. Call between
	 *  SpawnActorDeferred and FinishSpawning, since the container stocks itself at BeginPlay.
	 */
	void SetContentsForTest(ULootTableDefinition* Table, const FGuid& Key, const TArray<FInventoryItem>& Items = TArray<FInventoryItem>())
	{
		LootTable = Table;
		PlacedContainerId = Key;
		StartingItems = Items;
	}
};
