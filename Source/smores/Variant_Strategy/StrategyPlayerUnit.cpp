// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPlayerUnit.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

AStrategyPlayerUnit::AStrategyPlayerUnit()
{
	// every player unit starts with a couple of fake items for this first pass
	StartingItems.Add(FInventoryItem(TEXT("Apple"), FText::FromString(TEXT("Apple"))));
	StartingItems.Add(FInventoryItem(TEXT("PocketKnife"), FText::FromString(TEXT("Pocket Knife"))));
}

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
