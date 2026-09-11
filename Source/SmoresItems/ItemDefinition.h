// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 *  Broad item classification. Drives equip-slot matching, UI filtering, and (later) pricing
 *  and crafting-input lookups. Deliberately coarse - the definition asset carries the detail.
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	None		UMETA(DisplayName = "None"),
	Weapon		UMETA(DisplayName = "Weapon"),
	Armor		UMETA(DisplayName = "Armor"),
	Consumable	UMETA(DisplayName = "Consumable"),
	Material	UMETA(DisplayName = "Material/Component"),
	Tool		UMETA(DisplayName = "Tool"),
	Valuable	UMETA(DisplayName = "Valuable"),
	Misc		UMETA(DisplayName = "Misc")
};

/**
 *  Worn/equipped slot an item can occupy, if any. Nothing reads this yet - the equipment
 *  component and paperdoll that consume it are a later slice.
 */
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
	None		UMETA(DisplayName = "None"),
	MainHand	UMETA(DisplayName = "Main Hand"),
	OffHand		UMETA(DisplayName = "Off Hand"),
	Head		UMETA(DisplayName = "Head"),
	Body		UMETA(DisplayName = "Body"),
	Feet		UMETA(DisplayName = "Feet")
};

/**
 *  Shared, immutable definition of one item type - the data every copy of that item has in
 *  common (name, icon, weight, value, grid footprint, stack size). One asset per item type
 *  under Content/Items/; a carried FInventoryItem only references one of these plus whatever
 *  actually varies copy-to-copy.
 *
 *  A UPrimaryDataAsset rather than a UDataTable row because each definition directly
 *  references other assets (icon texture, world mesh), is Blueprint- and MCP-friendly to
 *  author, and is naturally moddable as content.
 *
 *  Several fields below (footprint, stack size, equip slot, world mesh) are authored now but
 *  not read by anything yet - the grid, stacking, and equipment systems that consume them
 *  are later slices of the inventory roadmap.
 */
UCLASS(BlueprintType)
class SMORESITEMS_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Stable identifier for this item type, independent of the asset's name/path */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemId;

	/** Player-facing name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FText DisplayName;

	/** Player-facing description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity", meta = (MultiLine = "true"))
	FText Description;

	/** Broad classification - drives equip-slot matching, UI filtering and (later) crafting lookups */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	EItemCategory Category = EItemCategory::Misc;

	/** 2D inventory/UI representation. Deliberately a separate asset from WorldMesh, not the same art reused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Presentation")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** 3D representation, used for the world pickup actor and (later) the equipped visual */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Presentation")
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	/** Weight of a single unit. Accumulated into a holder's carried weight; deliberately independent of grid footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = 0.0))
	float Weight = 0.0f;

	/** Base resale value of a single unit, in the placeholder "gold" currency. Buy/sell markups are applied on top by the pricing layer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = 0))
	int32 BaseValue = 0;

	/** Width in grid cells of this item's rectangular footprint. Not read yet (grid storage is a later slice). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1, ClampMax = 16))
	int32 FootprintWidth = 1;

	/** Height in grid cells of this item's rectangular footprint. Not read yet (grid storage is a later slice). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1, ClampMax = 16))
	int32 FootprintHeight = 1;

	/** Base maximum stack size. A holder's StackMultiplier scales this into an effective cap. Not read yet (stacking is a later slice). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1))
	int32 MaxStackSize = 1;

	/** Which worn slot this item equips into, if any. Not read yet (equipment is a later slice). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	EEquipSlot EquipSlot = EEquipSlot::None;

	/** True if more than one of this item may share a single entry */
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsStackable() const { return MaxStackSize > 1; }

	//~ Begin UPrimaryDataAsset interface
	/** Uses the authored ItemId so the asset can be renamed/moved without breaking references by ID */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset interface
};
