// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;

/**
 *  One enumerated, index-addressable inventory slot. Placeholder visual is plain text;
 *  a designer wraps SlotText in a UBorder in the WBP for the "box around text" look.
 *  BP_SlotClicked is a hook for cosmetic click feedback. A non-empty slot is also a drag
 *  source (NativeOnDragDetected) and every slot is a drop target (NativeOnDrop), moving/
 *  swapping items via UInventoryComponent::MoveItem - within one panel or between the two
 *  paired panels (pawn <-> container), since OwningInventory is just whatever component
 *  this widget is currently bound to.
 */
UCLASS(abstract)
class SMORESUI_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional label text for this slot. Name it "SlotText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

	/** Inventory this slot is currently bound to. Set alongside SlotIndex/Item by SetSlot. */
	TWeakObjectPtr<UInventoryComponent> OwningInventory;

	/** Index of this slot within its owning inventory (INDEX_NONE until set) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	/** Item currently occupying this slot; an empty FInventoryItem means the slot is empty */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem Item;

public:

	/** Sets owning inventory + slot index + item and refreshes SlotText. Called by the owning UInventoryWidget. */
	void SetSlot(UInventoryComponent* InOwningInventory, int32 InSlotIndex, const FInventoryItem& InItem);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotIndex() const { return SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItem() const { return Item; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty() const { return Item.IsEmpty(); }

protected:

	/** Blueprint hook for cosmetic click feedback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Slot Clicked"))
	void BP_SlotClicked();

	//~ Begin UUserWidget interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget interface
};
