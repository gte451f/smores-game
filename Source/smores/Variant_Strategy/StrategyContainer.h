// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryComponent.h"
#include "StrategyContainer.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class AStrategyUnit;

/**
 *  Base class for a world container holding its own inventory (a chest, barrel, bag, etc.).
 *  A selected unit must be within InteractionRange to open one. Concrete container types
 *  (e.g. AStrategyChest) derive from this to supply their own default StartingItems and
 *  any type-specific behavior; this class only holds what every container type needs.
 */
UCLASS(abstract)
class AStrategyContainer : public AActor
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

	/** Items this container starts with. Empty by default here; concrete types populate it in their constructor. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryItem> StartingItems;

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
	//~ End AActor interface

public:

	/** Returns this container's inventory component */
	UInventoryComponent* GetInventory() const { return Inventory; }

	/** Returns this container's display name */
	FText GetContainerDisplayName() const { return ContainerDisplayName; }

	/** Returns true if the given unit is close enough to open this container */
	bool IsUnitInRange(const AStrategyUnit* Unit) const;

	/** Notifies this container that it has been opened, so Blueprint can play cosmetic feedback */
	void NotifyOpened();

	/** Updates this container's material to reflect whether it is the player's current selected container */
	void SetSelected(bool bSelected);

protected:

	/** Blueprint handler for cosmetic/audio response when this container is opened */
	UFUNCTION(BlueprintImplementableEvent, Category = "Container", meta = (DisplayName = "Container Opened"))
	void BP_ContainerOpened();
};
