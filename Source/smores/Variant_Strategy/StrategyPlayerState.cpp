// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPlayerState.h"
#include "smores.h"
#include "Net/UnrealNetwork.h"

void AStrategyPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// the starting balance is authored content, so only the authoritative copy seeds it -
	// clients receive it through replication like any other change
	if (HasAuthority() && StartingGold > 0)
	{
		AddGold(StartingGold);
	}
}

void AStrategyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStrategyPlayerState, Gold);
}

void AStrategyPlayerState::OnRep_Gold()
{
	OnGoldChanged.Broadcast(Gold);
}

void AStrategyPlayerState::AddGold(int32 Amount)
{
	if (!HasAuthority())
	{
		UE_LOG(Logsmores, Warning, TEXT("AddGold called on a non-authority machine for %s - ignored."), *GetPlayerName());
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

bool AStrategyPlayerState::TrySpendGold(int32 Amount)
{
	if (!HasAuthority())
	{
		UE_LOG(Logsmores, Warning, TEXT("TrySpendGold called on a non-authority machine for %s - ignored."), *GetPlayerName());
		return false;
	}

	if (Amount < 0)
	{
		UE_LOG(Logsmores, Warning, TEXT("TrySpendGold called with a negative amount (%d) - use AddGold to credit."), Amount);
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
