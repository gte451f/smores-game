// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "InventoryHolder.h"
#include "CharacterRecord.h"
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
class UActionOrderComponent;
struct FCharacterRecord;
enum class EActionApproachResult : uint8;

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

	/** This unit's walk-over-to-act order, if it has one. Server-side state - see UActionOrderComponent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UActionOrderComponent> ActionOrder;

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

	/** Turns this unit to face Other, yaw only - a squad member facing what it acts on, an NPC facing whoever came to talk. Does nothing while Downed or Dead. */
	void FaceToward(const AActor* Other);

	/**
	 *  Attempts to move this unit to the passed location. bLeadUnit is the move order's lead - the
	 *  selected unit nearest the goal - which goes to the best point; the rest spread out around it.
	 *  Cancels any pending action order, silently - every caller means "this unit is now doing
	 *  something else".
	 */
	void MoveToLocation(const FVector& Location, bool bLeadUnit);

	/**
	 *  Walks toward Goal and keeps following it if it moves - the walk behind an action order.
	 *
	 *  A goal-actor move rather than MoveToLocation's EQS-picked point: EQS hands non-lead units a
	 *  random point in the best 25%, which can end outside reach, and a fixed point wouldn't follow
	 *  a target that wanders off. Same guards as MoveToLocation (authority, incapacitated) and
	 *  clears attack state the same way. Only the action order calls this; the walk it starts is
	 *  the one OnMoveFinished reports back to the order.
	 */
	EActionApproachResult MoveToActor(AActor* Goal, float AcceptanceRadius);

	/** Stops the action order's walk, if that is the walk running. Anything else this unit is doing is left alone. */
	void StopActionApproach();

	/**
	 *  Drops whatever this unit was doing - a pending move query, a walk, an attack - so an action
	 *  order can take over. Authority only. Leaves the order itself alone; the order calls this.
	 */
	void TakeOverForActionOrder();

	/** Returns the last cached movement goal location */
	FVector GetMovementGoal() const;

	/** Returns this unit's inventory component */
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** Returns this unit's equipment (worn slots) component */
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	/** Returns this unit's action order component. Never null - it's a default subobject. */
	UActionOrderComponent* GetActionOrder() const { return ActionOrder; }

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
	 *  This unit's attributes - a replicated copy of its record's, so a client can work out what the
	 *  squad's own numbers make likely (the action menu's odds). 10 across the board until a record
	 *  or a definition says otherwise.
	 */
	const FCharacterAttributes& GetAttributes() const { return Attributes; }

	/** True for a person, false for a creature - see ECharacterKind. A unit with no definition counts as a person. */
	bool IsPerson() const;

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

	/**
	 *  What the squad can see of this unit: its definition's description, a person's backstory,
	 *  and its condition in words (unhurt, hurt, down, dead). **Never a number** - no attributes,
	 *  no skills, no health figure, even for your own squad, whose numbers belong on the character
	 *  sheet. See player-interface.md's known-vs-hidden rule.
	 */
	virtual FText GetExamineText() const override;

	//~ End IInventoryHolder interface

	/** This unit's condition as the squad would describe it - "Unhurt.", "Badly hurt.", "Dead." Words only. */
	FText GetConditionText() const;

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

	/**
	 *  Called by the AI controller when a move request ends - arrived, blocked, or aborted. It fires
	 *  for aborts too, and synchronously inside whatever call replaced the move, so the action
	 *  order only hears about the one walk it started (ActionMoveRequestId), never about another.
	 */
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** Unbinds and forgets a pending MoveToLocation query, so its late answer can't send this unit somewhere it's no longer going */
	void DropPendingMoveQuery();

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

protected:

	/**
	 *  EnvQuery the lead unit of a move order uses to pick its spot - the single best point near the
	 *  goal. The name is the Strategy template's, from when the lead unit also played an interaction
	 *  on arrival; kept because it is what the unit Blueprints have assigned.
	 */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> InteractionQuery;

	/** EnvQuery every other unit of a move order uses - a random pick from the best quarter, so a group spreads out */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> NoInteractionQuery;

	/** How close we should get to the movement goal to consider ourselves as having reached it */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float MovementAcceptanceRadius = 100.0f;

	/** EQS instance running the movement query for this unit */
	TObjectPtr<UEnvQueryInstanceBlueprintWrapper> EnvQueryInstance;

	/** Cached movement goal for this unit */
	FVector CurrentMovementGoal;

	/** True while this unit is its move order's lead - see MoveToLocation */
	bool bLeadUnit = false;

	/**
	 *  This unit's current behavior/targeting state. Replicated: the target panel and the
	 *  right-click menu read it on every machine to decide whether Talk, Trade and Pickpocket are
	 *  on offer, and a remote client would otherwise offer all three against someone attacking it.
	 */
	UPROPERTY(Replicated)
	EStrategyDisposition Disposition = EStrategyDisposition::Passive;

	/** The move request behind the action order's walk, or invalid when none is running. See OnMoveFinished. */
	FAIRequestID ActionMoveRequestId;

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

	/** Replicated copy of the record's Attributes, set wherever FactionId is. See GetAttributes. */
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "Character")
	FCharacterAttributes Attributes;

	/** True while ApplyRecordToActor is pushing record state into the components, so their change broadcasts don't write it straight back */
	bool bApplyingRecord = false;

public:

	FOnUnitMoveCompletedDelegate OnMoveCompleted;
};
