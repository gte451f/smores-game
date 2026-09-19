// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresDefinition.h"
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
 *  Worn/equipped slot an item can occupy, if any. EEquipSlot::None means the item isn't
 *  wearable at all. Read by UEquipmentComponent, whose only equip-time gate is matching this
 *  against the slot being filled.
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
 *  common (icon, weight, value, grid footprint, stack size, plus the id/name/description it
 *  inherits from USmoresDefinition). One asset per item type under Content/Items/; a carried
 *  FInventoryItem only references one of these plus whatever actually varies copy-to-copy.
 *
 *  Everything below is item-specific and stays here on purpose - Icon in particular, because a
 *  character portrait and a faction crest are not the same kind of picture and a shared field
 *  could never be *required* of any of them. See USmoresDefinition's comment.
 *
 *  A UPrimaryDataAsset rather than a UDataTable row because each definition directly
 *  references other assets (icon texture, world mesh), is Blueprint- and MCP-friendly to
 *  author, and is naturally moddable as content.
 *
 *  One field below (world mesh) is authored now but not read by anything yet - the world
 *  pickup actor that consumes it is a later slice of the inventory roadmap.
 */
UCLASS(BlueprintType)
class SMORESITEMS_API UItemDefinition : public USmoresDefinition
{
	GENERATED_BODY()

public:

	/** The Asset Manager type items are registered under - see Config/DefaultGame.ini's PrimaryAssetTypesToScan */
	static const FPrimaryAssetType DefinitionType;

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

	/** Width in grid cells of this item's rectangular footprint - the game's stand-in for bulk, independent of Weight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1, ClampMax = 16))
	int32 FootprintWidth = 1;

	/** Height in grid cells of this item's rectangular footprint (see FootprintWidth). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1, ClampMax = 16))
	int32 FootprintHeight = 1;

	/** Base maximum stack size. A holder's StackMultiplier scales this into an effective cap - see UInventoryComponent::GetEffectiveMaxStack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Storage", meta = (ClampMin = 1))
	int32 MaxStackSize = 1;

	/** Which worn slot this item equips into, if any. None means it can't be worn - see UEquipmentComponent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	EEquipSlot EquipSlot = EEquipSlot::None;

	/** True if more than one of this item may share a single entry */
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsStackable() const { return MaxStackSize > 1; }

	//~ Begin USmoresDefinition interface
	virtual FPrimaryAssetType GetDefinitionType() const override { return DefinitionType; }
	//~ End USmoresDefinition interface
};
