// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPlayerUnit.h"
#include "Net/UnrealNetwork.h"

void AStrategyPlayerUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStrategyPlayerUnit, OwningController);
}

void AStrategyPlayerUnit::ClaimForController(APlayerController* NewOwningController)
{
	if (!HasAuthority() || OwningController)
	{
		return;
	}

	OwningController = NewOwningController;
	SetOwner(NewOwningController);
}
