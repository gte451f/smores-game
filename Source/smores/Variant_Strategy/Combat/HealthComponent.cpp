// Copyright Epic Games, Inc. All Rights Reserved.


#include "HealthComponent.h"
#include "TimerManager.h"
#include "DamageNumberActor.h"
#include "smores.h"

UHealthComponent::UHealthComponent()
{
	// health is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
}

void UHealthComponent::TakeDamage(float Amount, AActor* DamageInstigator)
{
	if (bIsDowned || Amount <= 0.0f)
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

void UHealthComponent::SpawnDamageNumber(float Amount) const
{
	if (!DamageNumberActorClass)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] SpawnDamageNumber: DamageNumberActorClass unset on %s's Health - skipping"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("(no owner)"));
		return;
	}

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	if (!Owner || !World)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] SpawnDamageNumber: missing Owner or World - skipping"));
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, DamageNumberSpawnHeight);

	ADamageNumberActor* NumberActor = World->SpawnActor<ADamageNumberActor>(DamageNumberActorClass, SpawnLocation, FRotator::ZeroRotator);

	if (NumberActor)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] SpawnDamageNumber: spawned %s above %s at %s for %.0f damage"),
			*NumberActor->GetName(), *Owner->GetName(), *SpawnLocation.ToString(), Amount);
		NumberActor->Initialize(Amount);
	}
	else
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] SpawnDamageNumber: SpawnActor FAILED for class %s"), *DamageNumberActorClass->GetName());
	}
}

void UHealthComponent::Downed()
{
	bIsDowned = true;

	OnDowned.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(RecoveryTimerHandle, this, &UHealthComponent::Recover, DownedDurationSeconds, false);
}

void UHealthComponent::Recover()
{
	Health = MaxHealth;
	bIsDowned = false;

	OnRecovered.Broadcast();
}
