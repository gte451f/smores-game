// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class ADamageNumberActor;

/** Broadcast when this component's health drops to zero and it goes Downed */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDownedDelegate);

/** Broadcast when this component recovers from Downed back to full health */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthRecoveredDelegate);

/** Broadcast when this component takes nonzero damage that doesn't go Downed. Passed the actor that dealt the damage, if known. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDamagedDelegate, AActor*, DamageInstigator);

/**
 *  Simple health carried by every strategy unit (NPC and player alike).
 *  Goes Downed at zero health and auto-recovers to full after a delay.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UHealthComponent : public UActorComponent
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

	/** Applies damage. No-ops while already Downed or if Amount isn't positive. DamageInstigator (if known) is passed via OnDamaged for auto-retaliation. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void TakeDamage(float Amount, AActor* DamageInstigator = nullptr);

	/** True while health is at zero, awaiting the recovery timer */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDowned() const { return bIsDowned; }

	/** Current health */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

private:

	/** Current health */
	float Health = 100.0f;

	/** True while health is at zero, awaiting the recovery timer */
	bool bIsDowned = false;

	/** Handle for the pending Recover() call while Downed */
	FTimerHandle RecoveryTimerHandle;

	/** Spawns a floating damage number above the owner, if DamageNumberActorClass is set */
	void SpawnDamageNumber(float Amount) const;

	/** Goes Downed: broadcasts OnDowned and starts the recovery timer */
	void Downed();

	/** Recovers from Downed: restores full health and broadcasts OnRecovered */
	void Recover();
};
