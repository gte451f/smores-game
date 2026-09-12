// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPlayerUnit.h"
#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

void AStrategyPlayerUnit::BeginPlay()
{
	Super::BeginPlay();

	// stock the inventory with the starting items
	if (UInventoryComponent* InventoryComp = GetInventory())
	{
		for (const FInventoryItem& Item : StartingItems)
		{
			InventoryComp->AddItem(Item);
		}
	}
}

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
