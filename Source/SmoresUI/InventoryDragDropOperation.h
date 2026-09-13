// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryComponent.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryItemWidget;

/** Broadcast when the rotate key flips a drag's orientation, so a hovering grid can redraw its preview */
DECLARE_MULTICAST_DELEGATE(FOnInventoryDragRotated);

/**
 *  Drag payload for moving a placed item between inventory grids: which inventory and which
 *  placed entry the drag started from, the orientation it is being held at, and which cell of
 *  the footprint the pointer grabbed.
 *
 *  Created by UInventoryItemWidget::NativeOnDragDetected and consumed by
 *  UInventoryWidget::NativeOnDrop, which converts the pointer position into the destination
 *  cell. Rotation is driven from outside: the rotate key is a normal Enhanced Input action on
 *  the player controller, which reaches the in-flight drag through GetActiveDrag.
 */
UCLASS()
class SMORESUI_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:

	/** Inventory the dragged item is coming from */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TWeakObjectPtr<UInventoryComponent> SourceInventory;

	/** Stable id of the placed entry being dragged (see FInventoryEntry::EntryId) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SourceEntryId = INDEX_NONE;

	/** Copy of the dragged instance, so a hovering grid can test the drop without reaching back into the source inventory */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem DraggedItem;

	/** Orientation the item will be dropped at. Seeded from the entry's current rotation; the rotate key flips it mid-drag. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bRotated = false;

	/** Which cell of the footprint the pointer grabbed, in the current orientation. The drop anchor is the hovered cell minus this. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint GrabOffset = FIntPoint::ZeroValue;

	/** Pixel size of one cell in the grid the drag started from, used to size the decorator */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FVector2D CellSize = FVector2D::ZeroVector;

	/** Fired when ToggleRotation actually changes the orientation */
	FOnInventoryDragRotated OnRotated;

	/** Footprint in cells at the orientation the item is currently being held at */
	FIntPoint GetFootprint() const { return DraggedItem.GetFootprint(bRotated); }

	/**
	 *  The inventory drag currently in flight for the local player, or null if there isn't one.
	 *
	 *  Slate owns the in-flight drag - no widget and no player controller holds a reference to
	 *  it - so this is the only handle a keybound action has on the thing it needs to rotate.
	 *  Keeping the Slate/UMG drag plumbing here means the caller is just three lines.
	 */
	static UInventoryDragDropOperation* GetActiveDrag();

	/** Flips the held orientation, transposing the grab offset with it. No-op for a square footprint. */
	void ToggleRotation();

	/** Re-points the decorator at the current orientation and grab offset. Call after changing either. */
	void ApplyPreview();

private:

	/** The decorator widget, when it is one of ours */
	UInventoryItemWidget* GetDecoratorItem() const;
};
