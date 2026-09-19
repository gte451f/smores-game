// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "EquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/Class.h"
#include "SmoresItems.h"

UEquipmentComponent::UEquipmentComponent()
{
	// equipment is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquipmentComponent, EquippedItems);
}

void UEquipmentComponent::OnRep_EquippedItems()
{
	// authority already broadcast this directly from whichever mutator it called
	if (HasOwnerAuthority())
	{
		return;
	}

	OnEquipmentChanged.Broadcast();
}

TArray<EEquipSlot> UEquipmentComponent::GetAllEquipSlots()
{
	// deliberately a short, fixed roster rather than an iteration over the enum - the paperdoll
	// draws these in this order, and EEquipSlot::None is "not wearable", not a slot
	return { EEquipSlot::MainHand, EEquipSlot::OffHand, EEquipSlot::Head, EEquipSlot::Body, EEquipSlot::Feet };
}

FText UEquipmentComponent::GetSlotDisplayName(EEquipSlot Slot)
{
	// the enum's UMETA(DisplayName) is already the player-facing label, so there's no second
	// name list to keep in step with it
	return StaticEnum<EEquipSlot>()->GetDisplayNameTextByValue(static_cast<int64>(Slot));
}

EEquipSlot UEquipmentComponent::GetSlotForItem(const FInventoryItem& Item)
{
	return Item.GetEquipSlot();
}

bool UEquipmentComponent::HasOwnerAuthority() const
{
	return GetOwner() != nullptr && GetOwner()->HasAuthority();
}

int32 UEquipmentComponent::IndexOfSlot(EEquipSlot Slot) const
{
	if (Slot == EEquipSlot::None)
	{
		return INDEX_NONE;
	}

	return EquippedItems.IndexOfByPredicate([Slot](const FEquippedItem& Equipped) { return Equipped.Slot == Slot; });
}

void UEquipmentComponent::SetSlotItem(EEquipSlot Slot, const FInventoryItem& Item)
{
	const int32 Index = IndexOfSlot(Slot);

	if (Item.IsEmpty())
	{
		// an empty slot isn't stored at all, so clearing one means dropping the entry
		if (Index != INDEX_NONE)
		{
			EquippedItems.RemoveAt(Index);
		}

		return;
	}

	if (Index != INDEX_NONE)
	{
		EquippedItems[Index].Item = Item;
		return;
	}

	EquippedItems.Emplace(Slot, Item);
}

FInventoryItem UEquipmentComponent::GetEquippedItem(EEquipSlot Slot) const
{
	const int32 Index = IndexOfSlot(Slot);

	return Index != INDEX_NONE ? EquippedItems[Index].Item : FInventoryItem();
}

bool UEquipmentComponent::IsSlotOccupied(EEquipSlot Slot) const
{
	return IndexOfSlot(Slot) != INDEX_NONE;
}

bool UEquipmentComponent::CanEquipItem(const FInventoryItem& Item, EEquipSlot Slot) const
{
	// an item that belongs in no slot can't be worn anywhere, and no item may be forced into a
	// slot that isn't its own - that single comparison is the entire rule
	return Slot != EEquipSlot::None && GetSlotForItem(Item) == Slot;
}

float UEquipmentComponent::GetTotalWeight() const
{
	float Total = 0.0f;

	for (const FEquippedItem& Equipped : EquippedItems)
	{
		Total += Equipped.Item.GetTotalWeight();
	}

	return Total;
}

UInventoryComponent* UEquipmentComponent::GetOwnerInventory() const
{
	const AActor* Owner = GetOwner();

	return Owner ? Owner->FindComponentByClass<UInventoryComponent>() : nullptr;
}

bool UEquipmentComponent::Equip(UInventoryComponent* FromInventory, int32 EntryId, EEquipSlot Slot)
{
	ESmoresRefusalReason UnusedReason = ESmoresRefusalReason::None;

	return EquipWithReason(FromInventory, EntryId, Slot, UnusedReason);
}

bool UEquipmentComponent::EquipWithReason(UInventoryComponent* FromInventory, int32 EntryId, EEquipSlot Slot, ESmoresRefusalReason& OutReason)
{
	// the malformed-request failures below say nothing: a null inventory or a stale entry id is
	// a bug or a race, not something the player did wrong or can do anything about
	OutReason = ESmoresRefusalReason::None;

	if (!FromInventory)
	{
		return false;
	}

	// checked up front so a swap across the two components can never half-apply
	if (!HasOwnerAuthority() || !FromInventory->HasOwnerAuthority())
	{
		return false;
	}

	const FInventoryEntry SourceEntry = FromInventory->GetEntry(EntryId);

	if (!SourceEntry.IsValidEntry())
	{
		return false;
	}

	const EEquipSlot NaturalSlot = GetSlotForItem(SourceEntry.Item);

	// EEquipSlot::None from the caller means "wherever this belongs" (the right-click path);
	// a drop on a specific paperdoll slot names one, and still has to match
	const EEquipSlot TargetSlot = (Slot == EEquipSlot::None) ? NaturalSlot : Slot;

	if (!CanEquipItem(SourceEntry.Item, TargetSlot))
	{
		// covers both "this isn't wearable at all" and "not in that slot" - the player's next
		// move is the same either way, so one reason serves both
		OutReason = ESmoresRefusalReason::WrongSlot;

		return false;
	}

	// a pawn wears one of a thing, whatever the stack it came from held
	FInventoryItem WornItem = SourceEntry.Item;
	WornItem.Quantity = 1;

	const bool bConsumesWholeEntry = (SourceEntry.Item.Quantity <= 1);

	const FInventoryItem Displaced = GetEquippedItem(TargetSlot);

	FIntPoint DisplacedCell = FIntPoint::ZeroValue;
	bool bDisplacedRotated = false;

	if (!Displaced.IsEmpty())
	{
		// the displaced item needs somewhere to go *before* anything is mutated - it may reuse
		// the cells the equipped item is vacating, but only when the whole entry is leaving
		const int32 IgnoreEntryId = bConsumesWholeEntry ? EntryId : INDEX_NONE;

		if (!FromInventory->FindFreePlacement(Displaced, DisplacedCell, bDisplacedRotated, IgnoreEntryId))
		{
			UE_LOG(LogSmoresItems, Verbose, TEXT("Equip rejected on %s: no room to put down the %s it would replace."),
				*GetNameSafe(GetOwner()), *GetNameSafe(Displaced.Definition));

			OutReason = ESmoresRefusalReason::NoRoom;

			return false;
		}
	}

	// past this point nothing can fail, so the swap lands whole or not at all
	FromInventory->SetEntryQuantity(EntryId, SourceEntry.Item.Quantity - 1);

	SetSlotItem(TargetSlot, WornItem);

	if (!Displaced.IsEmpty())
	{
		FromInventory->AddItemAt(Displaced, DisplacedCell, bDisplacedRotated);
	}

	OnEquipmentChanged.Broadcast();

	return true;
}

bool UEquipmentComponent::Unequip(EEquipSlot Slot, UInventoryComponent* ToInventory)
{
	ESmoresRefusalReason UnusedReason = ESmoresRefusalReason::None;

	return UnequipWithReason(Slot, ToInventory, UnusedReason);
}

bool UEquipmentComponent::UnequipWithReason(EEquipSlot Slot, UInventoryComponent* ToInventory, ESmoresRefusalReason& OutReason)
{
	OutReason = ESmoresRefusalReason::None;

	if (!ToInventory)
	{
		return false;
	}

	if (!HasOwnerAuthority() || !ToInventory->HasOwnerAuthority())
	{
		return false;
	}

	const FInventoryItem Removed = GetEquippedItem(Slot);

	if (Removed.IsEmpty())
	{
		return false;
	}

	FIntPoint Cell = FIntPoint::ZeroValue;
	bool bRotated = false;

	// nowhere to put it means it stays worn - taking it off into nothing would destroy it
	if (!ToInventory->FindFreePlacement(Removed, Cell, bRotated))
	{
		UE_LOG(LogSmoresItems, Verbose, TEXT("Unequip rejected on %s: no room in the grid for the %s."),
			*GetNameSafe(GetOwner()), *GetNameSafe(Removed.Definition));

		OutReason = ESmoresRefusalReason::NoRoom;

		return false;
	}

	SetSlotItem(Slot, FInventoryItem());

	ToInventory->AddItemAt(Removed, Cell, bRotated);

	OnEquipmentChanged.Broadcast();

	return true;
}
