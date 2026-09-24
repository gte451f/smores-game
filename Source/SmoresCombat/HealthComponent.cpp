// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "HealthComponent.h"
#include "TimerManager.h"
#include "DamageNumberActor.h"
#include "Net/UnrealNetwork.h"
#include "SmoresCombat.h"

UHealthComponent::UHealthComponent()
{
	// health is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, HealthState);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
}

void UHealthComponent::TakeDamage(float Amount, AActor* DamageInstigator)
{
	// shared gameplay state - only the server may mutate it
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// a Downed owner is already out of the fight and a Dead one is gone for good - neither takes
	// further damage
	if (IsIncapacitated() || Amount <= 0.0f)
	{
		return;
	}

	Health = FMath::Max(Health - Amount, 0.0f);

	// shown regardless of whether this hit goes Downed - unlike OnDamaged below (retaliation-only),
	// a lethal hit should still visibly show its damage
	SpawnDamageNumber(Amount);

	if (Health <= 0.0f)
	{
		Downed();
	}
	else
	{
		// only broadcast if this hit didn't go Downed - a lethal hit's retaliation would just be
		// cleared again immediately by Downed() resetting the target's own attack state
		OnDamaged.Broadcast(DamageInstigator);
	}
}

void UHealthComponent::Kill()
{
	// shared gameplay state - only the server may mutate it
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	// a kill can land on a unit that is already Downed, so cancel the recovery that was in
	// flight - otherwise the timer fires a few seconds later and stands the corpse back up
	GetWorld()->GetTimerManager().ClearTimer(RecoveryTimerHandle);

	Health = 0.0f;
	HealthState = EHealthState::Dead;

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s died"), GetOwner() ? *GetOwner()->GetName() : TEXT("(no owner)"));

	OnDied.Broadcast();
}

bool UHealthComponent::RestoreState(float NewHealth, EHealthState NewState)
{
	// shared gameplay state - only the server may mutate it
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const EHealthState OldState = HealthState;

	// whatever was pending belonged to the state being replaced
	GetWorld()->GetTimerManager().ClearTimer(RecoveryTimerHandle);

	Health = (NewState == EHealthState::Alive) ? FMath::Clamp(NewHealth, 0.0f, MaxHealth) : 0.0f;
	HealthState = NewState;

	if (NewState == EHealthState::Downed)
	{
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UHealthComponent::Recover, DownedDurationSeconds, false);
	}

	if (NewState != OldState)
	{
		switch (NewState)
		{
		case EHealthState::Downed:
			OnDowned.Broadcast();
			break;

		case EHealthState::Dead:
			OnDied.Broadcast();
			break;

		case EHealthState::Alive:
			OnRecovered.Broadcast();
			break;
		}
	}

	return true;
}

void UHealthComponent::SpawnDamageNumber(float Amount) const
{
	if (!DamageNumberActorClass)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] SpawnDamageNumber: DamageNumberActorClass unset on %s's Health - skipping"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("(no owner)"));
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!Owner || !World)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] SpawnDamageNumber: missing Owner or World - skipping"));
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, DamageNumberSpawnHeight);

	ADamageNumberActor* NumberActor = World->SpawnActor<ADamageNumberActor>(DamageNumberActorClass, SpawnLocation, FRotator::ZeroRotator);

	if (NumberActor)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] SpawnDamageNumber: spawned %s above %s at %s for %.0f damage"),
			*NumberActor->GetName(), *Owner->GetName(), *SpawnLocation.ToString(), Amount);
		NumberActor->Initialize(Amount);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] SpawnDamageNumber: SpawnActor FAILED for class %s"), *DamageNumberActorClass->GetName());
	}
}

void UHealthComponent::Downed()
{
	HealthState = EHealthState::Downed;

	OnDowned.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UHealthComponent::Recover, DownedDurationSeconds, false);
}

void UHealthComponent::Recover()
{
	// Kill() clears this timer, so this is belt-and-braces against a recovery already queued on
	// the timer manager when the kill landed - nothing brings a dead unit back
	if (IsDead())
	{
		return;
	}

	Health = MaxHealth;
	HealthState = EHealthState::Alive;

	OnRecovered.Broadcast();
}

void UHealthComponent::OnRep_Health(float OldHealth)
{
	// authority already ran SpawnDamageNumber/OnDamaged synchronously in TakeDamage - this is
	// only for the non-authority machines that just received the replicated change
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return;
	}

	const float Amount = OldHealth - Health;

	if (Amount > 0.0f)
	{
		SpawnDamageNumber(Amount);

		// HealthState has already been applied by the time RepNotifies run, even though
		// OnRep_HealthState may fire before or after this callback
		if (!IsIncapacitated())
		{
			OnDamaged.Broadcast(nullptr);
		}
	}
}

void UHealthComponent::OnRep_HealthState(EHealthState OldHealthState)
{
	// authority already broadcast these directly from Downed()/Recover()/Kill()
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return;
	}

	if (HealthState == OldHealthState)
	{
		return;
	}

	switch (HealthState)
	{
	case EHealthState::Downed:
		OnDowned.Broadcast();
		break;

	case EHealthState::Dead:
		// a kill on an already-Downed owner replicates as Downed -> Dead. The owner is inert
		// either way and has already reacted to OnDowned, so only announce the death itself.
		OnDied.Broadcast();
		break;

	case EHealthState::Alive:
		OnRecovered.Broadcast();
		break;
	}
}
