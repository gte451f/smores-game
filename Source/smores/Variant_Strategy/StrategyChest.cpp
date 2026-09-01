// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyChest.h"

AStrategyChest::AStrategyChest()
{
	// seed the chest with a few default items for this first pass
	StartingItems.Add(FInventoryItem(TEXT("GoldCoin"), FText::FromString(TEXT("Gold Coin"))));
	StartingItems.Add(FInventoryItem(TEXT("HealthPotion"), FText::FromString(TEXT("Health Potion"))));
	StartingItems.Add(FInventoryItem(TEXT("IronSword"), FText::FromString(TEXT("Iron Sword"))));
}
