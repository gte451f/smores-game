// Copyright Epic Games, Inc. All Rights Reserved.


#include "HealthComponent.h"
#include "TimerManager.h"

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
