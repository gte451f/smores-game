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
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UInventoryComponent();

	/** Number of item slots available on this inventory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0, ClampMax = 64))
	int32 NumSlots = 4;

protected:

	/** Items currently held, always exactly NumSlots entries; an empty FInventoryItem marks an empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryItem> Items;

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

public:

	/** Fired when the slot count or item list changes */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedDelegate OnInventoryChanged;

	/** Adds an item if there's a free slot. Returns false (and warns) when full. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& Item);

	/** Removes the item at the given index. Returns false if the index is invalid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(int32 Index);

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

	/** Resizes the inventory, dropping any items that no longer fit */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetNumSlots(int32 NewNumSlots);

	/** Read-only access to the held items */
	const TArray<FInventoryItem>& GetItems() const { return Items; }

	/** Blueprint-friendly copy of the held items */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryItem> GetItemsCopy() const { return Items; }
};
