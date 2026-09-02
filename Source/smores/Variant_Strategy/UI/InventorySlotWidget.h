// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryComponent.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;

/**
 *  One enumerated, index-addressable inventory slot. Placeholder visual is plain text;
 *  a designer wraps SlotText in a UBorder in the WBP for the "box around text" look.
 *  BP_SlotClicked is a hook for future drag-and-drop - no drag logic exists yet.
 */
UCLASS(abstract)
class UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional label text for this slot. Name it "SlotText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

	/** Index of this slot within its owning inventory (INDEX_NONE until set) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	/** Item currently occupying this slot; an empty FInventoryItem means the slot is empty */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem Item;

public:

	/** Sets slot index + item and refreshes SlotText. Called by the owning UInventoryWidget. */
	void SetSlot(int32 InSlotIndex, const FInventoryItem& InItem);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItem() const { return Item; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty() const { return Item.IsEmpty(); }

protected:

	/** Blueprint hook for future drag-and-drop / click interaction */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Slot Clicked"))
	void BP_SlotClicked();

	//~ Begin UUserWidget interface
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget interface
};
