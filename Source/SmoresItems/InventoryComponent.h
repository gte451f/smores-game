// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemDefinition.h"
#include "SmoresRefusalReason.h"
#include "InventoryComponent.generated.h"

class UTexture2D;

/**
 *  One carried item *instance*: a reference to the shared UItemDefinition that says what it
 *  is, plus only what actually varies copy-to-copy. Everything common (name, icon, weight,
 *  value, footprint, stack size) is read through Definition rather than duplicated here.
 *
 *  A default-constructed instance (no Definition) is "nothing" - it never appears as a placed
 *  grid entry, only as the empty result of a failed lookup.
 */
USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	/** What this item is. Null means "no item". Replicates by path, since definitions are stably-named assets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDefinition> Definition = nullptr;

	/** How many of the item this entry holds, capped by the definition's MaxStackSize x the holder's StackMultiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = 1))
	int32 Quantity = 1;

	/** Wear state, 0 (destroyed) to 1 (pristine). Placeholder only - no durability/upkeep mechanics read it yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float Condition = 1.0f;

	/** Set when this instance was taken by theft. Carried only - territory recognition and expiry wait on faction/NPC-awareness systems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bStolen = false;

	FInventoryItem() = default;

	explicit FInventoryItem(UItemDefinition* InDefinition, int32 InQuantity = 1)
		: Definition(InDefinition)
		, Quantity(InQuantity)
	{
	}

	/** True if this holds no item at all (no definition) */
	bool IsEmpty() const { return Definition == nullptr; }

	/** Stable item-type identifier, or NAME_None when empty */
	FName GetItemId() const { return Definition ? Definition->DefinitionId : NAME_None; }

	/** Player-facing name, or empty when this holds no item */
	FText GetDisplayName() const { return Definition ? Definition->DisplayName : FText::GetEmpty(); }

	/** Player-facing description, or empty when this holds no item */
	FText GetDescription() const { return Definition ? Definition->Description : FText::GetEmpty(); }

	/** Inventory icon, or null when this holds no item / the definition has no icon */
	UTexture2D* GetIcon() const { return Definition ? Definition->Icon : nullptr; }

	/** Combined weight of this entry (unit weight x quantity) */
	float GetTotalWeight() const { return Definition ? Definition->Weight * Quantity : 0.0f; }

	/** Combined base resale value of this entry, before any buy/sell markup */
	int32 GetTotalBaseValue() const { return Definition ? Definition->BaseValue * Quantity : 0; }

	/** Base (unmultiplied) stack cap from the definition; 0 when empty. The holder's StackMultiplier scales this - see UInventoryComponent::GetEffectiveMaxStack. */
	int32 GetBaseMaxStackSize() const { return Definition ? Definition->MaxStackSize : 0; }

	/**
	 *  Rectangular grid footprint in cells, from the definition. Rotation is the single
	 *  supported 90-degree turn, which just swaps width and height.
	 *  Zero when this holds no item.
	 */
	FIntPoint GetFootprint(bool bRotated = false) const
	{
		if (!Definition)
		{
			return FIntPoint::ZeroValue;
		}

		return bRotated
			? FIntPoint(Definition->FootprintHeight, Definition->FootprintWidth)
			: FIntPoint(Definition->FootprintWidth, Definition->FootprintHeight);
	}

	/** True if both entries hold the same item type */
	bool HasSameDefinitionAs(const FInventoryItem& Other) const { return Definition != nullptr && Definition == Other.Definition; }

	/**
	 *  True if these two entries may merge into one stack: same definition, the definition
	 *  allows stacking at all, and neither launders the other's stolen flag away. Condition
	 *  is deliberately *not* compared - stackable goods are bulk materials, and splitting a
	 *  stack per wear value would fragment it uselessly.
	 */
	bool CanStackWith(const FInventoryItem& Other) const
	{
		return HasSameDefinitionAs(Other) && Definition->IsStackable() && bStolen == Other.bStolen;
	}
};

/**
 *  One item instance *placed* in a holder's grid: the instance itself plus where it sits
 *  (anchor cell = its top-left corner) and whether it's turned 90 degrees.
 *
 *  EntryId is the stable handle callers use to refer to a placement - unlike an array index
 *  it survives other entries being added or removed, which matters because the UI holds a
 *  reference across a client->server round trip.
 */
USTRUCT(BlueprintType)
struct FInventoryEntry
{
	GENERATED_BODY()

	/** Stable per-holder identifier, assigned on placement. INDEX_NONE on a default-constructed (invalid) entry. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 EntryId = INDEX_NONE;

	/** The carried instance */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem Item;

	/** Top-left cell of this entry's footprint. X is the column, Y the row. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint AnchorCell = FIntPoint::ZeroValue;

	/** True when the footprint is turned 90 degrees (width and height swapped) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bRotated = false;

	FInventoryEntry() = default;

	FInventoryEntry(int32 InEntryId, const FInventoryItem& InItem, FIntPoint InAnchorCell, bool bInRotated)
		: EntryId(InEntryId)
		, Item(InItem)
		, AnchorCell(InAnchorCell)
		, bRotated(bInRotated)
	{
	}

	/** True if this is a real placement rather than a "not found" result */
	bool IsValidEntry() const { return EntryId != INDEX_NONE && !Item.IsEmpty(); }

	/** Footprint in cells, already accounting for rotation */
	FIntPoint GetFootprint() const { return Item.GetFootprint(bRotated); }

	/** True if this entry's footprint covers the given cell */
	bool CoversCell(FIntPoint Cell) const
	{
		const FIntPoint Size = GetFootprint();

		return Cell.X >= AnchorCell.X && Cell.X < AnchorCell.X + Size.X
			&& Cell.Y >= AnchorCell.Y && Cell.Y < AnchorCell.Y + Size.Y;
	}
};

/**
 *  What a repack orders a holder's contents by. Every criterion sorts *descending* - the
 *  biggest figure lands top-left - because every one of them answers a "what is taking up my
 *  pack?" question, and the answer wants to be the first thing read.
 *
 *  Deliberately small: these are the three figures an item carries that a player compares
 *  between items. Category is not one of them - that's what the filter is for.
 */
UENUM(BlueprintType)
enum class EInventorySortCriterion : uint8
{
	/** Heaviest stack first (unit weight x quantity) */
	Weight		UMETA(DisplayName = "Weight"),
	/** Most valuable stack first (base value x quantity), before any buy/sell markup */
	Value		UMETA(DisplayName = "Value"),
	/** Biggest stack first */
	Quantity	UMETA(DisplayName = "Quantity")
};

/** Broadcast whenever the grid size or placed entries change */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedDelegate);

/**
 *  Grid inventory carried by every holder (strategy pawn, world container, and later
 *  storefronts and world pickups).
 *
 *  Storage is a GridWidth x GridHeight cell grid rather than a flat slot list: an item
 *  occupies a rectangular footprint of cells taken from its definition, optionally rotated
 *  90 degrees, and no two footprints may overlap. That footprint is the game's stand-in for
 *  bulk/volume, deliberately independent of weight.
 *
 *  Stacking is capped at the definition's MaxStackSize x this holder's StackMultiplier, so a
 *  storefront shelf stacks deeper than a pawn's backpack without any per-transfer special case.
 *
 *  All mutation is authority-only and every mutator broadcasts OnInventoryChanged; Entries
 *  replicates, and OnRep re-broadcasts on non-authority machines.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESITEMS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UInventoryComponent();

	/** Grid width in cells. Sized per holder type - a pawn's pack is meaningfully smaller than a warehouse chest's. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory", meta = (ClampMin = 1, ClampMax = 32))
	int32 GridWidth = 8;

	/** Grid height in cells. Sized per holder type (see GridWidth). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory", meta = (ClampMin = 1, ClampMax = 32))
	int32 GridHeight = 8;

	/**
	 *  Scales every item definition's MaxStackSize into this holder's effective stack cap.
	 *  1.0 for a pawn's pack; storefronts and warehouse chests set it higher.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory", meta = (ClampMin = 0.01))
	float StackMultiplier = 1.0f;

	/**
	 *  How much weight this holder is meant to carry, the denominator of the inventory window's
	 *  "carried / capacity" readout. Zero means no limit at all, which is what a static holder
	 *  like a chest wants - weight is a carried-density figure, and nothing static carries.
	 *
	 *  Deliberately inert: exceeding it is reported but never blocks a placement, a transfer or a
	 *  pickup. The movement-speed and stealth-noise penalties this figure eventually feeds belong
	 *  to a later characters/combat pass, which owns those effects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Inventory", meta = (ClampMin = 0.0))
	float WeightCapacity = 30.0f;

protected:

	/** Items currently placed in the grid, in no particular order. Replicated so every machine sees the same
	 *  contents (a container's inventory is visible to whichever player has it open, a unit's inventory to
	 *  whoever's looting/trading with it). */
	UPROPERTY(ReplicatedUsing = OnRep_Entries, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventoryEntry> Entries;

	/** Server-side counter handing out stable EntryIds. Not replicated - clients read ids off Entries. */
	int32 NextEntryId = 0;

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated entry-list change - authority already broadcast
	 *  OnInventoryChanged directly from the mutator it called */
	UFUNCTION()
	void OnRep_Entries();

	/** Index into Entries for the given id, or INDEX_NONE */
	int32 IndexOfEntry(int32 EntryId) const;

	/**
	 *  Body of CanPlaceAt, tested against an arbitrary set of placements rather than this
	 *  holder's own Entries. A repack builds its new arrangement in a scratch array and only
	 *  commits it if everything fits, so it has to ask "would this fit?" about an array that
	 *  isn't the live one yet.
	 */
	bool CanPlaceAgainst(const TArray<FInventoryEntry>& Placements, const FInventoryItem& Item, FIntPoint Cell, bool bRotated, int32 IgnoreEntryId) const;

	/** Body of FindFreePlacement, scanning against an arbitrary set of placements (see CanPlaceAgainst) */
	bool FindFreePlacementAgainst(const TArray<FInventoryEntry>& Placements, const FInventoryItem& Item, FIntPoint& OutCell, bool& bOutRotated, int32 IgnoreEntryId) const;

public:

	/** Fired when the grid size or the placed entries change */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedDelegate OnInventoryChanged;

	//~ Queries

	/** True when this machine may mutate the inventory (it owns the authoritative copy) */
	bool HasOwnerAuthority() const;

	/** Grid dimensions in cells */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetGridSize() const { return FIntPoint(GridWidth, GridHeight); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridWidth() const { return GridWidth; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridHeight() const { return GridHeight; }

	/** True if the cell coordinate lies inside the grid */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsCellInBounds(FIntPoint Cell) const;

	/** Number of cells not covered by any placed entry */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetFreeCellCount() const;

	/**
	 *  Combined weight of everything placed here (each entry's unit weight x its quantity).
	 *  Deliberately unrelated to how many cells those entries occupy - a bundle of cloth is bulky
	 *  and light, an ingot small and heavy, and the two measures are meant to disagree.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	/** This holder's carry capacity; zero or less means unlimited (see HasWeightLimit) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetWeightCapacity() const { return WeightCapacity; }

	/** True if this holder has a meaningful capacity at all - a chest normally doesn't */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasWeightLimit() const { return WeightCapacity > 0.0f; }

	/** True if carried weight exceeds capacity. Reported for display only - nothing acts on it yet. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverWeightCapacity() const;

	/** This holder's stack cap for the given definition: its base MaxStackSize x StackMultiplier, never below 1. Zero for a null definition. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEffectiveMaxStack(const UItemDefinition* Definition) const;

	/** Read-only access to the placed entries */
	const TArray<FInventoryEntry>& GetEntries() const { return Entries; }

	/** Blueprint-friendly copy of the placed entries */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryEntry> GetEntriesCopy() const { return Entries; }

	/** The entry with the given id, or a default (invalid) entry if there's none */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryEntry GetEntry(int32 EntryId) const;

	/** Id of the entry whose footprint covers the given cell, or INDEX_NONE if the cell is free/out of bounds */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEntryIdAtCell(FIntPoint Cell) const;

	/**
	 *  True if Item's footprint would fit with its top-left corner at Cell: inside the grid and
	 *  overlapping no other entry. IgnoreEntryId excludes one existing entry from the overlap
	 *  test, which is what lets an entry be re-anchored onto cells it already occupies itself.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanPlaceAt(const FInventoryItem& Item, FIntPoint Cell, bool bRotated, int32 IgnoreEntryId = -1) const;

	/**
	 *  Scans for somewhere Item fits, trying the natural orientation across the whole grid first
	 *  and only then the rotated one (so items don't get turned sideways when they didn't need to be).
	 *  Returns false and leaves the outputs untouched when nothing fits.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindFreePlacement(const FInventoryItem& Item, FIntPoint& OutCell, bool& bOutRotated, int32 IgnoreEntryId = -1) const;

	//~ Mutators - all authority-only, all silent no-ops on a non-authority machine

	/**
	 *  Adds an item: merges into existing stacks of the same type first (up to this holder's
	 *  effective cap), then auto-places whatever's left over, splitting into as many entries as
	 *  the cap requires. Returns true only if the entire quantity was taken; a partial add still
	 *  keeps what fit and warns about the rest.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& Item);

	/**
	 *  AddItem, additionally reporting how many units actually landed. A caller that still holds
	 *  the source copy - a world pickup, a storefront purchase - needs the count rather than the
	 *  bool: a partial add keeps what fit, so destroying the source on anything short of the full
	 *  quantity would silently delete the units that didn't make it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemCounted(const FInventoryItem& Item, int32& OutQuantityAdded);

	/** Places an item at an explicit cell/rotation, without any stack merging. Returns false if it doesn't fit. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemAt(const FInventoryItem& Item, FIntPoint Cell, bool bRotated);

	/** Removes a placed entry outright. Returns false if there's no such entry. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveEntry(int32 EntryId);

	/** Sets an entry's quantity, clamped to this holder's effective stack cap; a new quantity of 0 or less removes the entry. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetEntryQuantity(int32 EntryId, int32 NewQuantity);

	/** Moves an existing entry to a new cell/rotation within this same grid. Returns false if it wouldn't fit there. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RepositionEntry(int32 EntryId, FIntPoint NewCell, bool bRotated);

	/** Resizes the grid, dropping any entry that no longer fits inside it. Returns false if the size was already that. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetGridSize(int32 NewWidth, int32 NewHeight);

	/**
	 *  Repacks the whole grid: merges every stack that can merge, orders what's left by
	 *  Criterion (descending), and re-places it from the top-left with no gaps between items.
	 *  Rotation is re-picked per item the way auto-placement does, so a repack can leave an item
	 *  turned the other way from how the player last held it - that is what "repack" means.
	 *
	 *  All-or-nothing. First-fit packing in criterion order can strand an item that the previous
	 *  arrangement had room for, so the new arrangement is built in a scratch array and committed
	 *  only once every entry has landed; a repack that can't place something changes nothing at
	 *  all rather than dropping it.
	 *
	 *  Returns false when nothing changed - no authority, an empty grid, or a failed repack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SortEntries(EInventorySortCriterion Criterion);

	/**
	 *  SortEntries, additionally reporting *why* it changed nothing - the same split as AddItem
	 *  vs. AddItemCounted, for the same reason: the bool collapses two outcomes a caller has to
	 *  tell apart.
	 *
	 *  "Already in this order" and "couldn't fit everything back in" both return false, and only
	 *  the second is worth interrupting the player over. An OutReason of None means the grid was
	 *  already sorted (or empty, or this isn't the authority) and nothing needs saying.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SortEntriesWithReason(EInventorySortCriterion Criterion, ESmoresRefusalReason& OutReason);

	/**
	 *  The single move/transfer entry point, used for repositioning within one grid
	 *  (SourceInventory == DestInventory) and for transferring between two.
	 *
	 *  Quantity <= 0 moves the whole entry; a smaller quantity splits the stack. The move either
	 *  merges into a stackable entry already anchored at DestCell, or places the item there
	 *  outright - it never swaps, so a drop onto something that can't stack is rejected whole and
	 *  the UI just snaps back. All validation happens before anything is mutated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool MoveItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity = 0);

	/**
	 *  MoveItem plus the quantity that actually changed hands.
	 *
	 *  The bool alone is not enough for a caller that has to do something *proportional* to the
	 *  move - a purchase charging for it, say. A merge into an existing stack only takes what
	 *  fits under that stack's cap and still reports success, so "moved" can mean three of the
	 *  eight the caller asked for. Same trap, and same fix, as AddItem vs. AddItemCounted.
	 *
	 *  A pure reposition within one grid reports the whole entry as moved: nothing changed
	 *  hands, but nothing was left behind either.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static bool MoveItemCounted(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity, int32& OutQuantityMoved);
};
