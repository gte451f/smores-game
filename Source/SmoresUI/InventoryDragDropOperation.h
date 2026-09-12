// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryComponent;

/**
 *  Drag payload for moving a placed item between inventory grids - identifies which inventory
 *  and which placed entry the drag started from, plus the orientation the item is currently
 *  being carried at. Created by UInventorySlotWidget::NativeOnDragDetected and consumed by the
 *  target cell's NativeOnDrop, which supplies the destination cell.
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

	/** Orientation the item will be dropped at. Seeded from the entry's current rotation; a rotate key flips it mid-drag (Slice 3). */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bRotated = false;
};
