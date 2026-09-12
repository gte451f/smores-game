// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InputCoreTypes.h"
#include "InventoryComponent.h"
#include "InventoryDragDropOperation.generated.h"

class FInventoryDragRotateProcessor;
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
 *  cell. The operation also owns the rotate-key hook for the duration of the drag - see
 *  BeginRotateInput for why that cannot just be a widget key handler.
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
	 *  Starts watching for the rotate key for the rest of this drag.
	 *
	 *  A drag captures the pointer but not keyboard focus, and Slate routes key events along
	 *  the *focus* path - which during play is the game viewport, not the inventory window. A
	 *  Slate input pre-processor is the one hook that sees the key regardless of who holds
	 *  focus; it is registered here and torn down in Drop/DragCancelled so it lives exactly as
	 *  long as the drag does.
	 */
	void BeginRotateInput(const FKey& InRotateKey);

	/** Flips the held orientation, transposing the grab offset with it. No-op for a square footprint. */
	void ToggleRotation();

	/** Re-points the decorator at the current orientation and grab offset. Call after changing either. */
	void ApplyPreview();

	//~ Begin UDragDropOperation interface
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	//~ End UDragDropOperation interface

	//~ Begin UObject interface
	virtual void BeginDestroy() override;
	//~ End UObject interface

private:

	/** Stops watching for the rotate key. Safe to call more than once. */
	void EndRotateInput();

	/** The decorator widget, when it is one of ours */
	UInventoryItemWidget* GetDecoratorItem() const;

	/** Registered with Slate for the lifetime of the drag; null otherwise */
	TSharedPtr<FInventoryDragRotateProcessor> RotateProcessor;
};
