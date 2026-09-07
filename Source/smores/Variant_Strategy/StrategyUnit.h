// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "StrategyUnit.generated.h"

class USphereComponent;
class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;
class UInventoryComponent;
class UHealthComponent;
class UAnimMontage;

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
class AStrategyUnit : public ACharacter
{
	GENERATED_BODY()

private:

	/** Interaction range sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

	/** Inventory carried by this unit. Present on NPC and player units alike. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> Inventory;

	/** Health carried by this unit. Present on NPC and player units alike. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHealthComponent> Health;

	/** Display name shown in the selection target UI (e.g. "Pawn 1", "NPC 3") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit", meta = (AllowPrivateAccess = "true"))
	FText UnitDisplayName;

protected:

	/** Cast reference to the AI Controlling this unit */
	TObjectPtr<AAIController> AIController;

public:

	/** Constructor */
	AStrategyUnit();

protected:

	virtual void NotifyControllerChanged() override;

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface

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

	/** Returns this unit's display name */
	FText GetUnitDisplayName() const { return UnitDisplayName; }

	/** Returns this unit's health component */
	UHealthComponent* GetHealth() const { return Health; }

	/** True while this unit is Downed (at zero health, awaiting recovery) */
	bool IsDowned() const;

	/** True while this unit is Aggressive (self-hunting, or actively engaged via a player attack command) */
	bool IsAggressive() const { return Disposition == EStrategyDisposition::Aggressive; }

	/** Sets this unit's disposition. Turning Aggressive starts self-initiated hunting; turning Passive stops it and clears any attack target. */
	void SetAggressive(bool bAggressive);

	/** Returns true if the given unit is close enough to this one to loot or interact with it */
	bool IsUnitInRange(const AStrategyUnit* Unit) const;

	/** Engages the given target: attacks immediately if already in range, otherwise moves into range first */
	void AttackTarget(AStrategyUnit* Target);

	/** Applies this unit's attack damage to its current attack target. Called by UAnimNotify_AttackHit at the montage's hit frame. */
	void ApplyAttackDamage();

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

	/** Bound to Health->OnDamaged; auto-retaliates against the instigator if not already fighting someone else */
	UFUNCTION()
	void OnHealthDamaged(AActor* DamageInstigator);

	/** Bound to the anim instance's OnMontageEnded; continues the auto-attack loop */
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

private:

	/** Faces and swings at Target, playing a random attack montage */
	void PerformAttack(AStrategyUnit* Target);

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

	/** Montages to play (one chosen at random) when attacking. Expects the 3 wrapped MM_Attack_0X montages. */
	UPROPERTY(EditAnywhere, Category="Combat")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** Montage played while Downed: falls once, then holds a looping grounded pose until Recover() stops it */
	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<UAnimMontage> DownedMontage;

	/** Max distance to a target for an attack to land without needing to move closer first */
	UPROPERTY(EditAnywhere, Category="Combat", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float AttackRange = 150.0f;

	/** This unit's current behavior/targeting state */
	EStrategyDisposition Disposition = EStrategyDisposition::Passive;

	/** The unit currently being swung at. Set once in range; drives ApplyAttackDamage and the auto-attack loop. */
	TWeakObjectPtr<AStrategyUnit> CurrentAttackTarget;

	/** If true, this unit will attack PendingAttackTarget upon finishing movement */
	bool bAttackOnArrival = false;

	/** The unit to attack once movement into range finishes */
	TWeakObjectPtr<AStrategyUnit> PendingAttackTarget;

	/** Repeating timer driving TryEngageNearestPlayerPawn while Aggressive */
	FTimerHandle AggroRetargetTimerHandle;

public:

	FOnUnitMoveCompletedDelegate OnMoveCompleted;
};
