// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class ADamageNumberActor;

/**
 *  Whether this component's owner is on its feet, temporarily out of the fight, or gone.
 *
 *  Downed and Dead are deliberately one enum rather than two bools: two bools can encode four
 *  combinations, one of which (Downed *and* Dead) is meaningless, and every caller would have
 *  to remember to check them in the right order. Both of the non-Alive states are inert - see
 *  IsIncapacitated(), which is what nearly every gameplay check actually wants. What separates
 *  them is only that Downed has a recovery timer running and Dead is terminal.
 */
UENUM(BlueprintType)
enum class EHealthState : uint8
{
	/** On its feet and able to act */
	Alive,

	/** At zero health, inert, awaiting the automatic recovery timer */
	Downed,

	/** Killed. Inert permanently - no recovery timer, and lootable like a Downed unit. */
	Dead
};

/** Broadcast when this component's health drops to zero and it goes Downed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDownedDelegate);

/** Broadcast when this component recovers from Downed back to full health */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthRecoveredDelegate);

/** Broadcast when this component is killed. Terminal - no OnRecovered ever follows it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDiedDelegate);

/** Broadcast when this component takes nonzero damage that doesn't go Downed. Passed the actor that dealt the damage, if known. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDamagedDelegate, AActor*, DamageInstigator);

/**
 *  Simple health carried by every strategy unit (NPC and player alike).
 *  Goes Downed at zero health and auto-recovers to full after a delay, or is killed outright
 *  via Kill(), which is terminal.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESCOMBAT_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UHealthComponent();

	/** Maximum (and starting) health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.0f;

	/** How long this component stays Downed before auto-recovering to full health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 0, Units = "s"))
	float DownedDurationSeconds = 15.0f;

	/** Actor spawned above the owner to show a floating damage number on every hit. If unset, no number is shown. */
	UPROPERTY(EditAnywhere, Category = "Health")
	TSubclassOf<ADamageNumberActor> DamageNumberActorClass;

	/** Height above the owner's actor location at which the damage number actor is spawned */
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = 0, Units = "cm"))
	float DamageNumberSpawnHeight = 180.0f;

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

public:

	/** Fired when health drops to zero and this component goes Downed */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDownedDelegate OnDowned;

	/** Fired when this component recovers from Downed back to full health */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthRecoveredDelegate OnRecovered;

	/** Fired when this component takes nonzero damage that doesn't go Downed (i.e. survives the hit) */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDamagedDelegate OnDamaged;

	/** Fired when this component is killed. Nothing ever follows it - a dead owner stays dead. */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDiedDelegate OnDied;

	/** Applies damage. Authority-only (no-ops on a non-authority machine); no-ops while already Downed or Dead, or if Amount isn't positive. DamageInstigator (if known) is passed via OnDamaged for auto-retaliation. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void TakeDamage(float Amount, AActor* DamageInstigator = nullptr);

	/**
	 *  Kills this component's owner outright. Authority-only; a no-op if already Dead.
	 *
	 *  Reachable from Alive or from Downed, cancels any pending recovery, and is terminal -
	 *  nothing here ever brings a dead owner back. Deliberately NOT wired into the damage path:
	 *  a lethal hit still goes Downed, exactly as it always has. What should actually kill a
	 *  unit in play - bleeding out while Downed, a finishing blow, a hit taken past some
	 *  threshold - is a combat/characters design decision (see the game-design skill's
	 *  character-death-and-permadeath.md, still a placeholder), not an inventory one. This is
	 *  the state and the transition; whoever decides that rule calls this.
	 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Kill();

	/** Current state: on its feet, Downed, or Dead */
	UFUNCTION(BlueprintPure, Category = "Health")
	EHealthState GetHealthState() const { return HealthState; }

	/** True while health is at zero and the recovery timer is running. False once Dead. */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDowned() const { return HealthState == EHealthState::Downed; }

	/** True once killed. Terminal. */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return HealthState == EHealthState::Dead; }

	/** True while Downed or Dead - inert either way. This is the check nearly all gameplay wants;
	 *  reach for IsDowned()/IsDead() only when the two genuinely need different treatment. */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsIncapacitated() const { return HealthState != EHealthState::Alive; }

	/** Current health */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

protected:

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated health change (damage number, OnDamaged) - authority already handled these synchronously in TakeDamage */
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	/** Reacts on non-authority machines to a replicated state change (OnDowned/OnRecovered/OnDied) - authority already handled these synchronously in Downed()/Recover()/Kill() */
	UFUNCTION()
	void OnRep_HealthState(EHealthState OldHealthState);

private:

	/** Current health. Replicated so every machine can react to damage/downs (e.g. play the Downed pose). */
	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 100.0f;

	/** Alive / Downed / Dead. Replicated for the same reason as Health - every machine has to be
	 *  able to react (Downed pose, lootability) without waiting to be told separately. */
	UPROPERTY(ReplicatedUsing = OnRep_HealthState)
	EHealthState HealthState = EHealthState::Alive;

	/** Handle for the pending Recover() call while Downed */
	FTimerHandle RecoveryTimerHandle;

	/** Spawns a floating damage number above the owner, if DamageNumberActorClass is set */
	void SpawnDamageNumber(float Amount) const;

	/** Goes Downed: broadcasts OnDowned and starts the recovery timer */
	void Downed();

	/** Recovers from Downed: restores full health and broadcasts OnRecovered. Refuses to run
	 *  while Dead, so a kill that lands on an already-Downed owner can't be undone by the
	 *  recovery timer that was already in flight. */
	void Recover();
};
