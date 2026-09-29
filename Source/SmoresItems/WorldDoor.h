// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmoresInteractable.h"
#include "WorldDoor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 *  A door: opens and closes, and blocks walking paths while shut.
 *
 *  **How it blocks paths.** The leaf's own collision affects navigation, and the project's navmesh
 *  is rebuilt at runtime (RuntimeGeneration=Dynamic in DefaultEngine.ini) - so swinging the leaf
 *  out of the doorway re-opens the path through it, and swinging it back cuts the path again. A
 *  shut door is a wall to pathfinding until someone is ordered to open it; units never open
 *  doors on their own.
 *
 *  **Opened and closed only by the server** (SetOpen), and replicated as one bool. The swing is
 *  instant: sound and animation belong to the Blueprint's BP_DoorOpened / BP_DoorClosed hooks.
 *
 *  **Lives in SmoresItems for now**, beside AStrategyContainer, because that is where the world
 *  objects the squad handles live today. It isn't an item, though - move it when a world or
 *  building module is cut (unreal-module-organization.md).
 *
 *  Not an inventory holder, which is exactly why ISmoresInteractable exists: reach and the
 *  display name had to live somewhere a door could share with a chest.
 */
UCLASS(abstract)
class SMORESITEMS_API AWorldDoor : public AActor, public ISmoresInteractable
{
	GENERATED_BODY()

private:

	/** The door's origin - the middle of the doorway, on the floor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DoorRoot;

	/** Optional frame around the doorway. Leave its mesh empty for a door set into a wall. Never swings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	/** The hinge the leaf swings on. Place it at one edge of the doorway; the leaf hangs off it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HingePivot;

	/** The part that swings, and the part that blocks - its collision is what cuts the navmesh while shut */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeafMesh;

	/** How close a squad member has to be to open or close it */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionRange;

protected:

	/** Open or shut. Authored per placed door for its starting state; replicated after that. */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Open, Category = "Door")
	bool bOpen = false;

	/** How far the leaf swings open, in degrees about the hinge. Negative swings it the other way. */
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = -180, ClampMax = 180, Units = "Degrees"))
	float OpenYaw = 90.0f;

	/** Name shown on the target panel and the menu ("Storeroom Door") */
	UPROPERTY(EditAnywhere, Category = "Door")
	FText DoorDisplayName;

	/** What the Examine window says about this door, in words. Never its lock's difficulty as a number, once locks exist. */
	UPROPERTY(EditAnywhere, Category = "Door", meta = (MultiLine = "true"))
	FText ExamineText;

public:

	AWorldDoor();

	/** True while the door stands open */
	bool IsOpen() const { return bOpen; }

	/** Opens or shuts the door. Authority only - a silent no-op elsewhere. Does nothing if it's already that way. */
	void SetOpen(bool bNewOpen);

	/** The part that swings - the thing to light up when the door is hovered, and to aim at */
	UStaticMeshComponent* GetLeafMesh() const { return LeafMesh; }

	//~ Begin ISmoresInteractable interface
	virtual FText GetInteractionDisplayName() const override { return DoorDisplayName; }
	virtual bool IsInRangeOf(const AActor* Other) const override;
	virtual FText GetExamineText() const override { return ExamineText; }
	//~ End ISmoresInteractable interface

protected:

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End AActor interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Clients: the server opened or shut the door */
	UFUNCTION()
	void OnRep_Open();

	/** Swings the leaf to match bOpen. bAnnounce fires the BP hook - never for the starting pose. */
	void ApplyOpenState(bool bAnnounce);

	/** Blueprint handler for the door opening - a creak, an animation */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door", meta = (DisplayName = "Door Opened"))
	void BP_DoorOpened();

	/** Blueprint handler for the door shutting */
	UFUNCTION(BlueprintImplementableEvent, Category = "Door", meta = (DisplayName = "Door Closed"))
	void BP_DoorClosed();
};
