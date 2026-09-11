// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryComponent;

/**
 *  Drag payload for moving/swapping an item between inventory slot widgets - identifies which
 *  inventory and slot the drag started from. Created by UInventorySlotWidget::NativeOnDragDetected
 *  and consumed by the target slot's NativeOnDrop.
 */
UCLASS()
class SMORESUI_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:

	/** Inventory the dragged item is coming from */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TWeakObjectPtr<UInventoryComponent> SourceInventory;

	/** Slot index within SourceInventory the dragged item is coming from */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SourceSlotIndex = INDEX_NONE;
};
