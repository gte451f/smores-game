// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackDamageDealer.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

/** Broadcast when AttackTarget is called on a target that's out of AttackRange - the owner is
 *  expected to move into range and call AttackTarget again on arrival; this component never
 *  moves its owner itself. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatTargetOutOfRangeDelegate, AActor*, Target);

/**
 *  Melee attack-swing resolution: choosing/playing an attack montage, applying damage at the
 *  hit frame, and looping the swing while a target remains in range. Owns none of the movement
 *  or targeting-policy decisions (moving into range, who to hunt while Aggressive) - those stay
 *  on the owning character, which calls into this component once a target/range decision has
 *  been made. See the game-systems skill's unreal-module-organization topic for why this split
 *  exists (SmoresCombat must not depend back on the character/Characters-module code).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESCOMBAT_API UCombatComponent : public UActorComponent, public IAttackDamageDealer
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCombatComponent();

	/** Engages Target: swings immediately if already in AttackRange of the owner, otherwise
	 *  broadcasts OnTargetOutOfRange and does nothing further - the owner is responsible for
	 *  moving closer and calling this again on arrival. Authority-only (no-op otherwise). */
	void AttackTarget(AActor* Target);

	/** Returns the target currently being swung at, if any */
	AActor* GetCurrentAttackTarget() const { return CurrentAttackTarget.Get(); }

	/** Clears the current attack target without playing DownedMontage - used when the owner
	 *  voluntarily disengages (e.g. turning Passive, or starting an unrelated move command)
	 *  rather than going Downed. */
	void ClearCurrentAttackTarget() { CurrentAttackTarget = nullptr; }

	/** Called by the owner's OnHealthDowned reaction: stops the swing loop and plays DownedMontage */
	void NotifyOwnerDowned();

	/** Called by the owner's OnHealthRecovered reaction: stops DownedMontage */
	void NotifyOwnerRecovered();

	//~ Begin IAttackDamageDealer interface
	/** Applies this component's owner's attack damage to CurrentAttackTarget. Called by
	 *  UAnimNotify_AttackHit at the montage's hit frame. */
	virtual void ApplyAttackDamage() override;
	//~ End IAttackDamageDealer interface

	/** Fired when AttackTarget is called on an out-of-range target; the owner should move closer
	 *  and call AttackTarget again on arrival. */
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatTargetOutOfRangeDelegate OnTargetOutOfRange;

protected:

	/** Faces and swings at Target, playing a random attack montage */
	void PerformAttack(AActor* Target);

	/** Plays Montage locally and (re)binds OnAttackMontageEnded. Multicast so the swing (and its
	 *  hit-frame AnimNotify) plays for every machine, not just the server - PerformAttack chooses
	 *  the montage once, authoritatively, and passes it here rather than each machine picking its
	 *  own (which would desync the swing shown to different observers). */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage(UAnimMontage* Montage);

	/** Bound to the anim instance's OnMontageEnded; continues the auto-attack loop */
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Montages to play (one chosen at random) when attacking. Expects the 3 wrapped MM_Attack_0X montages. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** Montage played while the owner is Downed: falls once, then holds a looping grounded pose
	 *  until NotifyOwnerRecovered stops it */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> DownedMontage;

	/** Max distance to a target for an attack to land without needing to move closer first */
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float AttackRange = 150.0f;

	/** The target currently being swung at. Set once in range; drives ApplyAttackDamage and the auto-attack loop. */
	TWeakObjectPtr<AActor> CurrentAttackTarget;
};
