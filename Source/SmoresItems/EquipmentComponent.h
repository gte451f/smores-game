// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.generated.h"

/**
 *  One worn item: the slot it occupies plus the instance occupying it. Only occupied slots
 *  are stored, so an absent slot is an empty one - there is no "empty slot" object, the same
 *  way FInventoryEntry only exists for cells that actually hold something.
 *
 *  Unlike an FInventoryEntry a worn item has no anchor cell or rotation: a paperdoll slot is a
 *  named place, not a region of a grid.
 */
USTRUCT(BlueprintType)
struct FEquippedItem
{
	GENERATED_BODY()

	/** Which worn slot this item occupies. Never EEquipSlot::None for a stored entry. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	EEquipSlot Slot = EEquipSlot::None;

	/** The worn instance. Always quantity 1 - a pawn wears one of a thing, whatever the stack it came from held. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FInventoryItem Item;

	FEquippedItem() = default;

	FEquippedItem(EEquipSlot InSlot, const FInventoryItem& InItem)
		: Slot(InSlot)
		, Item(InItem)
	{
	}
};

/** Broadcast whenever the worn items change */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChangedDelegate);

/**
 *  The worn/equipped slots of a pawn - a paperdoll, deliberately separate from the carried
 *  grid rather than a reserved region of it. A slot is a named place that accepts exactly one
 *  item of a matching type; it has no footprint, no packing, and no rotation.
 *
 *  Slot-type matching is this system's *only* gate. Any pawn may wear any weapon or armour
 *  regardless of training - the performance consequences of an untrained equip belong entirely
 *  to combat/skill resolution, never to an inventory-side restriction.
 *
 *  Equip and Unequip are all-or-nothing: a swap that has nowhere to put the item it displaces
 *  fails outright rather than half-applying, so nothing is ever destroyed by running out of
 *  grid room. Both are authority-only and broadcast OnEquipmentChanged; EquippedItems
 *  replicates, and OnRep re-broadcasts on non-authority machines - the same split
 *  UInventoryComponent uses.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESITEMS_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UEquipmentComponent();

protected:

	/** Worn items, one per occupied slot, in no particular order. Replicated so every machine sees what a pawn
	 *  is wearing - a looter's panel and (later) a combat resolution both read it off a non-authority copy. */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquippedItem> EquippedItems;

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated change - authority already broadcast
	 *  OnEquipmentChanged directly from the mutator it called */
	UFUNCTION()
	void OnRep_EquippedItems();

	/** Index into EquippedItems for the given slot, or INDEX_NONE if the slot is empty */
	int32 IndexOfSlot(EEquipSlot Slot) const;

	/** Writes a slot's contents without broadcasting - an empty item clears the slot outright.
	 *  Callers broadcast once, after the whole swap has landed. */
	void SetSlotItem(EEquipSlot Slot, const FInventoryItem& Item);

public:

	/** Fired when the worn items change */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChangedDelegate OnEquipmentChanged;

	//~ Queries

	/** Every real worn slot, in paperdoll display order. Excludes EEquipSlot::None, which means "not wearable". */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	static TArray<EEquipSlot> GetAllEquipSlots();

	/** Player-facing name of a slot ("Main Hand"), taken from the enum's authored display name */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	static FText GetSlotDisplayName(EEquipSlot Slot);

	/** The slot this item is meant to be worn in, or EEquipSlot::None when it isn't wearable at all */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	static EEquipSlot GetSlotForItem(const FInventoryItem& Item);

	/** True when this machine may mutate the equipment (it owns the authoritative copy) */
	bool HasOwnerAuthority() const;

	/** The item worn in the given slot, or an empty instance when the slot is free */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FInventoryItem GetEquippedItem(EEquipSlot Slot) const;

	/** True if something is worn in the given slot */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(EEquipSlot Slot) const;

	/** Read-only access to the worn items */
	const TArray<FEquippedItem>& GetEquippedItems() const { return EquippedItems; }

	/** Blueprint-friendly copy of the worn items */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<FEquippedItem> GetEquippedItemsCopy() const { return EquippedItems; }

	/**
	 *  True if this item may be worn in this slot. Slot-type matching is the whole test -
	 *  no skill, attribute or condition gating belongs here.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool CanEquipItem(const FInventoryItem& Item, EEquipSlot Slot) const;

	/** Combined weight of everything worn. Reported separately from the carried grid's weight; nothing folds the two together yet. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetTotalWeight() const;

	/** The inventory on this component's own owner - where an unequipped item goes back to, and where an equip takes from */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UInventoryComponent* GetOwnerInventory() const;

	//~ Mutators - authority-only, silent no-ops on a non-authority machine

	/**
	 *  Wears one unit of a placed grid entry, swapping out whatever already occupies the slot.
	 *
	 *  A Slot of EEquipSlot::None means "wherever this item belongs", which is what the
	 *  right-click path sends; naming a slot explicitly (a drop on a paperdoll slot) still has
	 *  to match the item's own slot. The displaced item returns to FromInventory via
	 *  FindFreePlacement, and the whole operation fails - mutating nothing - if it doesn't fit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Equip(UInventoryComponent* FromInventory, int32 EntryId, EEquipSlot Slot = EEquipSlot::None);

	/**
	 *  Takes a worn item off and places it back into ToInventory. Fails - leaving the item worn -
	 *  if the grid has no room for it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Unequip(EEquipSlot Slot, UInventoryComponent* ToInventory);
};
