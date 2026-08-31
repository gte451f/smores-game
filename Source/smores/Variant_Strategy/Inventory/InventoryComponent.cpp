// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryComponent.h"
#include "smores.h"

UInventoryComponent::UInventoryComponent()
{
	// inventory is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;
}

bool UInventoryComponent::AddItem(const FInventoryItem& Item)
{
	// reject if there's no room
	if (Items.Num() >= NumSlots)
	{
		UE_LOG(Logsmores, Warning, TEXT("InventoryComponent on %s is full (%d/%d); can't add '%s'."),
			*GetNameSafe(GetOwner()), Items.Num(), NumSlots, *Item.ItemId.ToString());

		return false;
	}

	Items.Add(Item);

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::RemoveItemAt(int32 Index)
{
	if (!Items.IsValidIndex(Index))
	{
		return false;
	}

	Items.RemoveAt(Index);

	OnInventoryChanged.Broadcast();

	return true;
}

int32 UInventoryComponent::GetFreeSlotCount() const
{
	return FMath::Max(0, NumSlots - Items.Num());
}

void UInventoryComponent::SetNumSlots(int32 NewNumSlots)
{
	NewNumSlots = FMath::Clamp(NewNumSlots, 0, 64);

	if (NewNumSlots == NumSlots)
	{
		return;
	}

	NumSlots = NewNumSlots;

	// drop any items that no longer fit
	if (Items.Num() > NumSlots)
	{
		Items.SetNum(NumSlots);
	}

	OnInventoryChanged.Broadcast();
}
