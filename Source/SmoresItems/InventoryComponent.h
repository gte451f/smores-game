// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UTexture2D;

/**
 *  A single carried item.
 *  Deliberately minimal for this first pass - just enough for a designer to dress up later.
 */
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	/** Stable identifier for this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemId;

	/** Player-facing name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FText DisplayName;

	/** Player-facing description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FText Description;

	/** Optional icon, wired by a designer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

	FInventoryItem() = default;

	FInventoryItem(FName InItemId, const FText& InDisplayName, const FText& InDescription = FText::GetEmpty())
		: ItemId(InItemId)
		, DisplayName(InDisplayName)
		, Description(InDescription)
	{
	}

	/** True if this represents an empty slot (no item placed) */
	bool IsEmpty() const { return ItemId.IsNone(); }
};

/** Broadcast whenever the slot count or item list changes */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedDelegate);

/**
 *  Simple inventory carried by every strategy pawn (NPC and player alike).
 *  Holds a dynamic number of slots (default 4) and the items filling them.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESITEMS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UInventoryComponent();

	/** Number of item slots available on this inventory. Replicated (see Items) since it's shared gameplay state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory", meta = (ClampMin = 0, ClampMax = 64))
	int32 NumSlots = 64;

protected:

	/** Items currently held, always exactly NumSlots entries; an empty FInventoryItem marks an empty slot.
	 *  Replicated so every machine sees the same contents (a container's inventory is visible to whichever
	 *  player has it open, a unit's inventory to whoever's looting/trading with it). */
	UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated item-list change - authority already broadcast
	 *  OnInventoryChanged directly from AddItem/SetItemAt/SetNumSlots */
	UFUNCTION()
	void OnRep_Items();

public:

	/** Fired when the slot count or item list changes */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedDelegate OnInventoryChanged;

	/** Adds an item if there's a free slot. Authority-only (no-ops on a non-authority machine). Returns false (and warns) when full. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& Item);

	/** Removes the item at the given index. Authority-only (see SetItemAt). Returns false if the index is invalid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(int32 Index);

	/** Sets the item at the given index (an empty FInventoryItem clears the slot). Authority-only (no-ops on a non-authority machine). Returns false if the index is invalid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetItemAt(int32 Index, const FInventoryItem& Item);

	/**
	 *  Moves the item at SourceIndex (on SourceInventory) to DestIndex (on DestInventory), swapping
	 *  with whatever already occupies DestIndex. Works for reordering within one inventory
	 *  (SourceInventory == DestInventory) and for transferring between two different inventories.
	 *  Authority-only (enforced by the underlying SetItemAt calls). No-ops (returns false) if either
	 *  component is null, either index is invalid, the source slot is empty, or the source and
	 *  destination are the same slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool MoveItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* DestInventory, int32 DestIndex);

	/** Number of empty slots remaining */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetFreeSlotCount() const;

	/** True if the slot at Index holds no item (or Index is out of range) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty(int32 Index) const;

	/** Item at the given slot index; an empty item if Index is invalid or the slot is empty */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItemAt(int32 Index) const;

	/** Current number of slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetNumSlots() const { return NumSlots; }

	/** Resizes the inventory, dropping any items that no longer fit. Authority-only (no-ops on a non-authority machine). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetNumSlots(int32 NewNumSlots);

	/** Read-only access to the held items */
	const TArray<FInventoryItem>& GetItems() const { return Items; }

	/** Blueprint-friendly copy of the held items */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryItem> GetItemsCopy() const { return Items; }
};
