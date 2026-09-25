// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "InventoryHolder.h"
#include "StrategyContainer.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class ULootTableDefinition;

/**
 *  Base class for a world container holding its own inventory (a chest, barrel, bag, etc.).
 *  A selected unit must be within InteractionRange to open one. Concrete container types
 *  (e.g. AStrategyChest) derive from this to add any type-specific behavior; this class only
 *  holds what every container type needs.
 *
 *  Contents come from two places, which coexist: StartingItems, authored item by item for one
 *  specific chest, and an optional LootTable rolled on top of them at BeginPlay. The table is not
 *  a replacement for authoring a particular chest - it's what lets fifty chests hold plausible,
 *  different things without anyone authoring fifty chests.
 */
UCLASS(abstract)
class SMORESITEMS_API AStrategyContainer : public AActor, public IInventoryHolder
{
	GENERATED_BODY()

private:

	/** Visual mesh for this container */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	/** Interaction range sphere. A unit must be within this radius to open the container. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionRange;

	/** Inventory held by this container */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> Inventory;

protected:

	/** Items this container starts with, authored per-Blueprint as references to UItemDefinition assets */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryItem> StartingItems;

	/**
	 *  A loot table rolled into this container once, at BeginPlay, on top of StartingItems.
	 *  Optional - leave it empty for a chest whose contents are authored in full.
	 *
	 *  The roll is **seeded, not random**: from the world seed plus PlacedContainerId, so the same
	 *  chest in the same campaign always holds the same thing and reloading to re-roll it gets
	 *  nothing (characters-and-squads.md's save-scumming rule). It is also **all-or-nothing**: a
	 *  roll that doesn't fit the grid is refused whole with a warning, leaving only StartingItems,
	 *  rather than keeping whichever items happened to fit first.
	 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TObjectPtr<ULootTableDefinition> LootTable;

	/**
	 *  The stable key this placed container's loot roll is seeded from - authored, never derived.
	 *
	 *  Authored rather than taken from the actor's runtime name because that name isn't stable
	 *  across edits to the level: a key that changed whenever someone moved a rock would re-roll
	 *  the chest. Generated automatically when a container is placed or pasted in the editor, the
	 *  same way AStrategyUnit's PlacedRecordId is. A container with a LootTable and no key still
	 *  rolls, from its actor name, and warns.
	 */
	UPROPERTY(EditInstanceOnly, Category = "Inventory", AdvancedDisplay)
	FGuid PlacedContainerId;

	/** Material applied to ContainerMesh when this container is not the selected one */
	UPROPERTY(EditAnywhere, Category = "Container")
	TObjectPtr<UMaterialInterface> NormalMaterial;

	/** Material applied to ContainerMesh when this container is the selected one */
	UPROPERTY(EditAnywhere, Category = "Container")
	TObjectPtr<UMaterialInterface> SelectedMaterial;

	/** Display name shown in the selection target UI (e.g. "Chest 1") */
	UPROPERTY(EditAnywhere, Category = "Container")
	FText ContainerDisplayName;

public:

	/** Constructor */
	AStrategyContainer();

protected:

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	virtual void PostActorCreated() override;
#if WITH_EDITOR
	virtual void PostEditImport() override;
#endif
	//~ End AActor interface

	/** Rolls LootTable into the inventory, all-or-nothing - see LootTable. Authority only; BeginPlay calls it once. */
	void RollLootTable();

public:

	/** Returns this container's inventory component */
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** The table this container rolls at BeginPlay, or null for one whose contents are authored in full */
	ULootTableDefinition* GetLootTable() const { return LootTable; }

	/** The key this container's roll is seeded from; invalid when none was authored */
	const FGuid& GetPlacedContainerId() const { return PlacedContainerId; }

	//~ Begin IInventoryHolder interface

	/** Returns this container's display name */
	virtual FText GetHolderDisplayName() const override { return ContainerDisplayName; }

	/** Returns true if the given actor is close enough to open this container */
	virtual bool IsInRangeOf(const AActor* Other) const override;

	//~ End IInventoryHolder interface

	/** Notifies this container that it has been opened, so Blueprint can play cosmetic feedback */
	void NotifyOpened();

	/** Updates this container's material to reflect whether it is the player's current selected container */
	void SetSelected(bool bSelected);

protected:

	/** Blueprint handler for cosmetic/audio response when this container is opened */
	UFUNCTION(BlueprintImplementableEvent, Category = "Container", meta = (DisplayName = "Container Opened"))
	void BP_ContainerOpened();
};
