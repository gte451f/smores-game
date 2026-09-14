// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WalletComponent.h"
#include "SmoresEconomy.h"
#include "Net/UnrealNetwork.h"

UWalletComponent::UWalletComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// the balance is shared state, so every machine needs a copy of it
	SetIsReplicatedByDefault(true);
}

void UWalletComponent::BeginPlay()
{
	Super::BeginPlay();

	// the starting balance is authored content, so only the authoritative copy seeds it -
	// clients receive it through replication like any other change
	if (HasOwnerAuthority() && StartingGold > 0)
	{
		AddGold(StartingGold);
	}
}

void UWalletComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWalletComponent, Gold);
}

bool UWalletComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();

	return Owner && Owner->HasAuthority();
}

void UWalletComponent::OnRep_Gold()
{
	OnGoldChanged.Broadcast(Gold);
}

void UWalletComponent::AddGold(int32 Amount)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresEconomy, Warning, TEXT("AddGold called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return;
	}

	if (Amount <= 0)
	{
		// debiting goes through TrySpendGold, which can actually fail; this path only credits
		return;
	}

	Gold += Amount;

	// authority doesn't run OnRep on itself, so broadcast directly
	OnGoldChanged.Broadcast(Gold);
}

bool UWalletComponent::TrySpendGold(int32 Amount)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresEconomy, Warning, TEXT("TrySpendGold called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return false;
	}

	if (Amount < 0)
	{
		UE_LOG(LogSmoresEconomy, Warning, TEXT("TrySpendGold called with a negative amount (%d) - use AddGold to credit."), Amount);
		return false;
	}

	// nothing to pay: succeed without touching the balance, so a free transaction still goes through
	if (Amount == 0)
	{
		return true;
	}

	if (!CanAfford(Amount))
	{
		return false;
	}

	Gold -= Amount;

	OnGoldChanged.Broadcast(Gold);

	return true;
}
