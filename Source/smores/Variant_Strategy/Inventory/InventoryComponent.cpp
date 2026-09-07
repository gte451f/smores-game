// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryComponent.h"
#include "smores.h"

UInventoryComponent::UInventoryComponent()
{
	// inventory is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	Items.SetNum(NumSlots);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// a Blueprint subclass may have overridden NumSlots after the constructor ran;
	// re-sync before any StartingItems seeding (which happens after this in owning actors)
	if (Items.Num() != NumSlots)
	{
		Items.SetNum(NumSlots);
	}
}

bool UInventoryComponent::AddItem(const FInventoryItem& Item)
{
	int32 FreeIndex = INDEX_NONE;

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		if (Items[Index].IsEmpty())
		{
			FreeIndex = Index;
			break;
		}
	}

	if (FreeIndex == INDEX_NONE)
	{
		UE_LOG(Logsmores, Warning, TEXT("InventoryComponent on %s is full (%d/%d); can't add '%s'."),
			*GetNameSafe(GetOwner()), NumSlots - GetFreeSlotCount(), NumSlots, *Item.ItemId.ToString());

		return false;
	}

	Items[FreeIndex] = Item;

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::RemoveItemAt(int32 Index)
{
	// clear in place rather than shifting later items down, so slot indices stay stable
	return SetItemAt(Index, FInventoryItem());
}

bool UInventoryComponent::SetItemAt(int32 Index, const FInventoryItem& Item)
{
	if (!Items.IsValidIndex(Index))
	{
		return false;
	}

	Items[Index] = Item;

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::MoveItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* DestInventory, int32 DestIndex)
{
	if (!SourceInventory || !DestInventory)
	{
		return false;
	}

	if (SourceInventory == DestInventory && SourceIndex == DestIndex)
	{
		return false;
	}

	const FInventoryItem FromItem = SourceInventory->GetItemAt(SourceIndex);

	if (FromItem.IsEmpty())
	{
		return false;
	}

	const FInventoryItem ToItem = DestInventory->GetItemAt(DestIndex);

	if (!DestInventory->SetItemAt(DestIndex, FromItem))
	{
		return false;
	}

	SourceInventory->SetItemAt(SourceIndex, ToItem);

	return true;
}

int32 UInventoryComponent::GetFreeSlotCount() const
{
	int32 FreeCount = 0;

	for (const FInventoryItem& CurrentItem : Items)
	{
		if (CurrentItem.IsEmpty())
		{
			++FreeCount;
		}
	}

	return FreeCount;
}

bool UInventoryComponent::IsSlotEmpty(int32 Index) const
{
	return !Items.IsValidIndex(Index) || Items[Index].IsEmpty();
}

FInventoryItem UInventoryComponent::GetItemAt(int32 Index) const
{
	return Items.IsValidIndex(Index) ? Items[Index] : FInventoryItem();
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
