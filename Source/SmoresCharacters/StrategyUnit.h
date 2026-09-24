// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "InventoryHolder.h"
#include "StrategyUnit.generated.h"

class USphereComponent;
class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;
class UInventoryComponent;
class UEquipmentComponent;
class UHealthComponent;
class UCombatComponent;
class UTexture2D;
class UCharacterDefinition;
class UCharacterRecordComponent;
struct FCharacterRecord;

/** Delegate to report that this unit has finished moving */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitMoveCompletedDelegate, AStrategyUnit*, Unit);

/** Behavior/targeting state driving whether a unit self-initiates combat */
UENUM(BlueprintType)
enum class EStrategyDisposition : uint8
{
	Passive,
	Aggressive
};

/**
 *  A simple strategy game unit
 *  Rather than react to inputs, it's controlled indirectly by the Strategy Player Controller
 */
UCLASS(abstract)
class SMORESCHARACTERS_API AStrategyUnit : public ACharacter, public IInventoryHolder
{
	GENERATED_BODY()

private:

	/** Interaction range sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

	/** Inventory carried by this unit. Present on NPC and player units alike. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> Inventory;

	/** Worn/equipped items of this unit - the paperdoll, distinct from the carried grid above.
	 *  Present on NPC and player units alike. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentComponent> Equipment;

	/** Health carried by this unit. Present on NPC and player units alike. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> Health;

	/** Attack-swing resolution (montage selection/playback, hit-frame damage) carried by this unit.
	 *  Present on NPC and player units alike - see SmoresCombat's UCombatComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatComponent> Combat;

	/**
	 *  Display name shown in the selection target UI (e.g. "Pawn 1", "NPC 3").
	 *
	 *  Authored per placed instance, where it names the character's record when that record is
	 *  created (see UCharacterRecordComponent::CreateRecord - a unique character ignores it). At
	 *  runtime it is the actor's working copy of FCharacterRecord::Name, set from the record and
	 *  replicated so every client shows the name the server chose.
	 */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Unit", meta = (AllowPrivateAccess = "true"))
	FText UnitDisplayName;

	/**
	 *  Face shown on this unit's portrait in the squad bar. Optional, and unset is the expected
	 *  state today: with no texture the portrait draws the unit's initials on a plain disc, which
	 *  is what the wireframe itself does.
	 *
	 *  Authored per Blueprint, or per placed instance where one unit should differ from the rest
	 *  of its class. A later characters pass may well move identity - name, face, biography - onto
	 *  a component of its own; one property here is the cheap version that doesn't block that.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> PortraitTexture;

protected:

	/**
	 *  What kind of character this unit stands in for. Authored per Blueprint (or per placed
	 *  instance). A new record is created from it - identity, attributes, faction, and the
	 *  DefaultLoadout placed into this unit's grid. A unit with none still gets a record, with a
	 *  warning, so nothing that predates definitions is left outside the record store.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterDefinition> CharacterDefinition;

	/**
	 *  The id of the record this placed unit stands in for - authored, never derived.
	 *
	 *  This is what stops a reloaded save standing the dead boss back up: the placed actor finds
	 *  its record by this key and adopts it, dead or alive, instead of creating a fresh one. It is
	 *  authored rather than taken from the actor's runtime name because that name is not stable
	 *  across edits to the level. Generated automatically when a unit is placed or pasted in the
	 *  editor; a unit spawned at runtime has none and gets a fresh record.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Character", AdvancedDisplay)
	FGuid PlacedRecordId;

	/** Sets UnitDisplayName as if a level designer had authored it - for code standing in for the level, like a test stand-in or a future spawner. Only meaningful before BeginPlay. */
	void SetAuthoredDisplayName(const FText& Name) { UnitDisplayName = Name; }

	/** Cast reference to the AI Controlling this unit */
	TObjectPtr<AAIController> AIController;

public:

	/** Constructor */
	AStrategyUnit();

protected:

	virtual void NotifyControllerChanged() override;

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostActorCreated() override;
#if WITH_EDITOR
	virtual void PostEditImport() override;
#endif
	//~ End AActor interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

public:

	/** Stops unit movement immediately */
	void StopMoving();

	/** Notifies this unit that it was selected */
	void UnitSelected();

	/** Notifies this unit that it was deselected */
	void UnitDeselected();

	/** Notifies this unit that it's been interacted with by another actor */
	void Interact(AStrategyUnit* Interactor);

	/** Attempts to move this unit to the passed location, and optionally signals it to interact on arrival */
	void MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList);

	/** Returns the last cached movement goal location */
	FVector GetMovementGoal() const;

	/** Returns this unit's inventory component */
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** Returns this unit's equipment (worn slots) component */
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	/** This unit's portrait face: its own PortraitTexture, else its character definition's Portrait, else null */
	UTexture2D* GetPortraitTexture() const;

	//~ Character record (the soft split - see game-data.md)

	/** The kind of character this unit stands in for, or null if none was authored */
	UCharacterDefinition* GetCharacterDefinition() const { return CharacterDefinition; }

	/** The id of this unit's record. Invalid on clients, and on a unit that found no record store. */
	const FGuid& GetRecordId() const { return RecordId; }

	/** This unit's record, or null on a client or with no record store. Server-side reads only. */
	const FCharacterRecord* GetRecord() const;

	/** This unit's faction id - a replicated copy of its record's, None when unaffiliated. Nothing reads it for behaviour yet. */
	FName GetFactionId() const { return FactionId; }

	/**
	 *  Copies this unit's current condition - health, life state, location, carried grid, worn
	 *  slots - into its record. Authority only. Called on every change to any of them, so there is
	 *  normally no reason to call it by hand; it is public for the debug exec and tests.
	 */
	void WriteBackToRecord();

	/**
	 *  Puts this unit's components into the state its record holds - the other direction from
	 *  WriteBackToRecord. Authority only. Run when a unit adopts an existing record at BeginPlay;
	 *  public so a test can prove the round trip.
	 */
	void ApplyRecordToActor();

	//~ Begin IInventoryHolder interface

	/** Returns this unit's display name */
	virtual FText GetHolderDisplayName() const override { return UnitDisplayName; }

	/** Returns true if the given actor is close enough to this one to loot or interact with it */
	virtual bool IsInRangeOf(const AActor* Other) const override;

	//~ End IInventoryHolder interface

	/** Returns this unit's health component */
	UHealthComponent* GetHealth() const { return Health; }

	/** True while this unit is Downed (at zero health, awaiting recovery) - but not while Dead */
	bool IsDowned() const;

	/** True once this unit has been killed. Terminal: unlike Downed, it never recovers. */
	bool IsDead() const;

	/** True while this unit is Downed or Dead - i.e. inert. This, not IsDowned(), is what almost
	 *  every gameplay check wants: a unit that can't move, fight, be fought, or stand up on its
	 *  own, and that can be looted. */
	bool IsIncapacitated() const;

	/** True while this unit is Aggressive (self-hunting, or actively engaged via a player attack command) */
	bool IsAggressive() const { return Disposition == EStrategyDisposition::Aggressive; }

	/** Sets this unit's disposition. Turning Aggressive starts self-initiated hunting; turning Passive stops it and clears any attack target. */
	void SetAggressive(bool bAggressive);

	/** Engages the given target: attacks immediately if already in range, otherwise moves into range first */
	void AttackTarget(AStrategyUnit* Target);

	/** Returns this unit's combat component */
	UCombatComponent* GetCombat() const { return Combat; }

protected:

	/** Called by EQS when the movement destination query has finished */
	UFUNCTION()
	void OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);

	/** Called by the AI controller when this unit has finished moving */
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** Wraps up movement logic */
	void HandleMoveFinished();

	/** Periodically finds and engages the nearest non-Downed player-controlled pawn while Aggressive */
	void TryEngageNearestPlayerPawn();

	/** Bound to Health->OnDowned */
	UFUNCTION()
	void OnHealthDowned();

	/** Bound to Health->OnRecovered */
	UFUNCTION()
	void OnHealthRecovered();

	/** Bound to Health->OnDied. Leaves this unit inert exactly as OnHealthDowned does - the
	 *  difference is entirely that no recovery is coming. */
	UFUNCTION()
	void OnHealthDied();

	/** Bound to Health->OnDamaged; auto-retaliates against the instigator if not already fighting someone else */
	UFUNCTION()
	void OnHealthDamaged(AActor* DamageInstigator);

	/** Bound to every no-argument "my state changed" delegate on this unit's components - writes the change back to the record */
	UFUNCTION()
	void OnWorkingCopyChanged();

	/** Finds or creates this unit's record at BeginPlay and binds to it. Authority only. */
	void RegisterWithRecordStore();

	/** Bound to Combat->OnTargetOutOfRange; moves into range and re-issues the attack on arrival */
	UFUNCTION()
	void OnCombatTargetOutOfRange(AActor* Target);

protected:

	/** Blueprint handler for strategy game selection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Selected"))
	void BP_UnitSelected();

	/** Blueprint handler for strategy game deselection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Deselected"))
	void BP_UnitDeselected();

	/** Blueprint handler to stop the unit's interaction animation */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="NPC", meta = (DisplayName="Stop Animation"))
	void BP_StopAnimation();

	/** Blueprint handler for strategy game interactions */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Interaction Behavior"))
	void BP_InteractionBehavior(AStrategyUnit* Interactor);

protected:

	/** EnvQuery to use when this unit interacts after movement */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> InteractionQuery;

	/** EnvQuery to use when this unit does not interact after movement */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> NoInteractionQuery;

	/** How close we should get to the movement goal to consider ourselves as having reached it */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float MovementAcceptanceRadius = 100.0f;

	/** Max distance to look for nearby units when doing an interaction check */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float InteractionRadius = 250.0f;

	/** EQS instance running the movement query for this unit */
	TObjectPtr<UEnvQueryInstanceBlueprintWrapper> EnvQueryInstance;

	/** Cached movement goal for this unit */
	FVector CurrentMovementGoal;

	/** If true, this unit will attempt to interact with a nearby unit upon finishing movement */
	bool bInteractOnArrival = false;

	/** List of actors to ignore when searching for units to interact with */
	TArray<AStrategyUnit*> InteractIgnoreList;

	/** This unit's current behavior/targeting state */
	EStrategyDisposition Disposition = EStrategyDisposition::Passive;

	/** If true, this unit will attack PendingAttackTarget upon finishing movement */
	bool bAttackOnArrival = false;

	/** The unit to attack once movement into range finishes */
	TWeakObjectPtr<AStrategyUnit> PendingAttackTarget;

	/** Repeating timer driving TryEngageNearestPlayerPawn while Aggressive */
	FTimerHandle AggroRetargetTimerHandle;

	/** The id of the record this unit is bound to. Server-side; invalid until RegisterWithRecordStore succeeds. */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Character")
	FGuid RecordId;

	/** The store RecordId lives in - cached at registration */
	TWeakObjectPtr<UCharacterRecordComponent> RecordStore;

	/** Replicated copy of the record's FactionId */
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Character")
	FName FactionId;

	/** True while ApplyRecordToActor is pushing record state into the components, so their change broadcasts don't write it straight back */
	bool bApplyingRecord = false;

public:

	FOnUnitMoveCompletedDelegate OnMoveCompleted;
};
