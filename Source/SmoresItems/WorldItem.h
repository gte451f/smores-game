// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryComponent.h"
#include "InventoryHolder.h"
#include "WorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  A single loose item lying in the world, picked up by double-clicking it with a player pawn
 *  in range.
 *
 *  Deliberately *not* an AStrategyContainer. A container is a holder: it owns a whole
 *  UInventoryComponent grid and opens into a window the player drags items out of. A world
 *  pickup owns one FInventoryItem, has no grid, no window and no UI at all - the whole
 *  interaction is "double-click it and it's yours". It draws itself with the definition's
 *  WorldMesh (its 3D representation), never its 2D inventory icon.
 *
 *  Contents are authored per placed instance - or handed over at spawn time by SpawnWorldItem -
 *  rather than per Blueprint, so one Blueprint subclass serves every item type: the mesh follows
 *  whatever definition the instance happens to hold.
 */
UCLASS(abstract)
class SMORESITEMS_API AWorldItem : public AActor, public IInventoryHolder
{
	GENERATED_BODY()

private:

	/** Visual mesh, driven entirely by the held definition's WorldMesh - never authored directly */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	/** Interaction range sphere. A player pawn must be within this radius to pick the item up.
	 *  Same shape and default as AStrategyContainer's - the one proximity rule every transfer
	 *  context shares, reached through IInventoryHolder::IsInRangeOf. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionRange;

protected:

	/**
	 *  The single item instance lying here. Authored per placed instance (or set at spawn time),
	 *  and replicated so clients draw the right mesh and see a partial pickup shrink the stack
	 *  rather than leaving a phantom pile behind.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Item, Category = "World Item")
	FInventoryItem Item;

public:

	/** Constructor */
	AWorldItem();

protected:

	//~ Begin AActor interface
	virtual void BeginPlay() override;

	/** Refreshes the mesh from the held definition, so setting Item on a placed instance shows up
	 *  in the editor viewport immediately rather than only once the level runs */
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End AActor interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated item change - authority refreshes directly */
	UFUNCTION()
	void OnRep_Item();

	/** Pushes the held definition's WorldMesh onto ItemMesh, clearing it when this holds nothing */
	void RefreshMesh();

public:

	/** The item instance lying here */
	const FInventoryItem& GetItem() const { return Item; }

	//~ Begin IInventoryHolder interface

	/** Player-facing name of whatever is lying here, or empty if it holds nothing */
	virtual FText GetHolderDisplayName() const override { return Item.GetDisplayName(); }

	/** Returns true if the given actor is close enough to pick this item up */
	virtual bool IsInRangeOf(const AActor* Other) const override;

	//~ End IInventoryHolder interface

	/** Replaces what's lying here. Authority-only; a silent no-op elsewhere. */
	void SetItem(const FInventoryItem& NewItem);

	/**
	 *  Moves as much of this item as will fit into DestInventory, destroying this actor once the
	 *  whole stack is gone. Authority-only.
	 *
	 *  A pickup that only partly fits takes what fits and leaves the remainder on the ground -
	 *  UInventoryComponent::AddItem is already allowed to place part of a stack, so anything else
	 *  would quietly destroy the units it couldn't place. Returns true if anything at all moved;
	 *  false leaves this actor exactly as it was.
	 */
	bool TryPickUp(UInventoryComponent* DestInventory);

	/**
	 *  Spawns a world pickup holding Item at Location. Authority-only, because the actor is
	 *  replicated - a client-spawned copy would exist on that client and nowhere else.
	 *  Returns null if it couldn't be spawned.
	 */
	UFUNCTION(BlueprintCallable, Category = "World Item", meta = (WorldContext = "WorldContextObject"))
	static AWorldItem* SpawnWorldItem(const UObject* WorldContextObject, TSubclassOf<AWorldItem> WorldItemClass, const FInventoryItem& InItem, const FVector& Location, const FRotator& Rotation);
};
