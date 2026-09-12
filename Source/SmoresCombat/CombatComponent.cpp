// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "CombatComponent.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "SmoresCombat.h"

UCombatComponent::UCombatComponent()
{
	// combat resolution is pure state/RPC driven - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UCombatComponent::AttackTarget(AActor* Target)
{
	AActor* Owner = GetOwner();

	// drives shared combat state (CurrentAttackTarget, the attack montage) - only the server may mutate it
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const UHealthComponent* OwnerHealth = Owner->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* TargetHealth = IsValid(Target) ? Target->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bOwnerDowned = OwnerHealth && OwnerHealth->IsDowned();
	const bool bTargetDowned = TargetHealth && TargetHealth->IsDowned();

	// a Downed owner can't initiate or continue an attack - without this, a still-running
	// aggro/retarget loop on the owner could keep calling back in here forever, replaying an
	// attack montage on top of the Downed pose on the same anim slot
	if (bOwnerDowned || !IsValid(Target) || bTargetDowned)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s) bailed early: %s"),
			*Owner->GetName(), Target ? *Target->GetName() : TEXT("null"),
			bOwnerDowned ? TEXT("attacker is Downed") : (!IsValid(Target) ? TEXT("Target invalid") : TEXT("Target already Downed")));
		return;
	}

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

	if (Dist <= AttackRange)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s): in range (%.0f <= %.0f), swinging now"),
			*Owner->GetName(), *Target->GetName(), Dist, AttackRange);
		PerformAttack(Target);
		return;
	}

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s): out of range (%.0f > %.0f), requesting a move into range"),
		*Owner->GetName(), *Target->GetName(), Dist, AttackRange);

	// out of range - the owner is responsible for moving closer and calling AttackTarget again on arrival
	OnTargetOutOfRange.Broadcast(Target);
}

void UCombatComponent::PerformAttack(AActor* Target)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	// rotate towards the target, same helper the owner's Interact() uses
	Owner->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Owner->GetActorLocation(), Target->GetActorLocation()));

	CurrentAttackTarget = Target;

	if (AttackMontages.Num() == 0)
	{
		return;
	}

	// chosen once, authoritatively (PerformAttack only ever runs server-side - see AttackTarget's
	// HasAuthority guard), and replicated to every machine so the swing plays in lock-step
	// everywhere rather than each machine picking its own random montage
	if (UAnimMontage* ChosenMontage = AttackMontages[FMath::RandHelper(AttackMontages.Num())])
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s PerformAttack: calling Multicast_PlayAttackMontage(%s)"),
			*Owner->GetName(), *ChosenMontage->GetName());
		Multicast_PlayAttackMontage(ChosenMontage);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s PerformAttack: AttackMontages[chosen index] was null"), *Owner->GetName());
	}
}

void UCombatComponent::Multicast_PlayAttackMontage_Implementation(UAnimMontage* Montage)
{
	AActor* Owner = GetOwner();
	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation(%s): OwnerCharacter=%s, Mesh=%s"),
		Owner ? *Owner->GetName() : TEXT("?"), Montage ? *Montage->GetName() : TEXT("null"),
		OwnerCharacter ? TEXT("valid") : TEXT("NULL - Cast<ACharacter> failed"),
		(OwnerCharacter && OwnerCharacter->GetMesh()) ? TEXT("valid") : TEXT("NULL"));

	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation: AnimInstance=%s, calling Montage_Play"),
			*Owner->GetName(), *AnimInstance->GetClass()->GetName());

		AnimInstance->Montage_Play(Montage);

		// bind fresh every swing rather than relying on a persistent AnimInstance::OnMontageEnded
		// subscription, which an AnimInstance recreation (e.g. an Animation Mode switch elsewhere)
		// would silently orphan
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UCombatComponent::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation: GetAnimInstance() returned null"), *Owner->GetName());
	}
}

void UCombatComponent::ApplyAttackDamage()
{
	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: notify fired"), GetOwner() ? *GetOwner()->GetName() : TEXT("?"));

	AActor* Owner = GetOwner();

	// the anim notify fires on every machine simulating this montage (attacker + all observing
	// clients) - only the server may actually apply damage
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	AActor* Target = CurrentAttackTarget.Get();

	// re-check range at the hit frame, not the swing-start frame, so a target that fled
	// mid-swing doesn't take a phantom hit
	if (!IsValid(Target))
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: no valid CurrentAttackTarget - hit whiffed"), *Owner->GetName());
		return;
	}

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

	if (Dist > AttackRange)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: %s moved out of range at hit frame (%.0f > %.0f) - hit whiffed"),
			*Owner->GetName(), *Target->GetName(), Dist, AttackRange);
		return;
	}

	UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();

	if (!TargetHealth)
	{
		return;
	}

	TargetHealth->TakeDamage(25.0f, Owner);

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: hit %s for 25, health now %.0f, IsDowned=%s"),
		*Owner->GetName(), *Target->GetName(), TargetHealth->GetHealth(), TargetHealth->IsDowned() ? TEXT("true") : TEXT("false"));
}

void UCombatComponent::NotifyOwnerDowned()
{
	// stop being anyone's live attack target and stop this owner's own attack loop
	CurrentAttackTarget = nullptr;

	if (!DownedMontage)
	{
		return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(DownedMontage);
			}
		}
	}
}

void UCombatComponent::NotifyOwnerRecovered()
{
	// blend back to locomotion - accepted small pop, no "get up" anim exists
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.25f, DownedMontage);
			}
		}
	}
}

void UCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// ignore montages that aren't one of ours (e.g. DownedMontage ending)
	if (!AttackMontages.Contains(Montage))
	{
		return;
	}

	// keep re-swinging the same target automatically until it goes Downed, this attacker is
	// invalid (CurrentAttackTarget is cleared by NotifyOwnerDowned), or this attacker is given a
	// new command - symmetric for player-issued attacks and NPC self-attacks alike
	AActor* Target = CurrentAttackTarget.Get();
	const UHealthComponent* TargetHealth = IsValid(Target) ? Target->FindComponentByClass<UHealthComponent>() : nullptr;

	if (IsValid(Target) && TargetHealth && !TargetHealth->IsDowned())
	{
		AttackTarget(Target);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s OnAttackMontageEnded: stopping the auto-attack loop (Target %s)"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), !IsValid(Target) ? TEXT("invalid") : TEXT("Downed"));
	}
}
