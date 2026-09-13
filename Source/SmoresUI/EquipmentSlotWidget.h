// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EquipmentComponent.h"
#include "InventoryCellWidget.h"
#include "EquipmentSlotWidget.generated.h"

class UTextBlock;
class UBorder;
class UInventoryDragDropOperation;

/**
 *  One slot of a pawn's paperdoll: a named place that holds exactly one item of a matching
 *  type. It draws whatever is worn there, accepts a drag from an inventory grid, and
 *  right-clicks off whatever it's holding.
 *
 *  Unlike the inventory grid - where item widgets overlap cell widgets and a per-cell drop
 *  handler can't tell which cell was hit - paperdoll slots never overlap, so each one handles
 *  its own drop. Don't generalise the grid's single-drop-target rule onto this.
 */
UCLASS(abstract)
class SMORESUI_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional slot name ("Main Hand"). Name it "SlotLabel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotLabel;

	/** Optional worn-item label, empty while the slot is. Name it "ItemLabel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemLabel;

	/** Optional background border this widget tints per state. Name it "SlotBorder" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotBorder;

	/** SlotBorder tint while the slot is empty and nothing is hovering it */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FLinearColor EmptySlotColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.08f);

	/** SlotBorder tint while something is worn here */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FLinearColor FilledSlotColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.2f);

	/** SlotBorder tint for a hovering drag this slot would accept */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FLinearColor ValidHighlightColor = FLinearColor(0.15f, 0.8f, 0.25f, 0.5f);

	/** SlotBorder tint for a hovering drag this slot would reject (wrong item type, or no room for what it would displace) */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	FLinearColor InvalidHighlightColor = FLinearColor(0.9f, 0.15f, 0.1f, 0.5f);

	/** Equipment component this slot reads and writes */
	TWeakObjectPtr<UEquipmentComponent> BoundEquipment;

	/** Which worn slot this widget represents. Not named "Slot" - UWidget already has one (its layout slot). */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	EEquipSlot EquipSlot = EEquipSlot::None;

	/** How this slot is currently marked up by a hovering drag. Reuses the grid cells' states - it's the same yes/no question. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	EInventoryCellHighlight Highlight = EInventoryCellHighlight::None;

public:

	/** Binds this widget to one slot of one equipment component and refreshes its visuals */
	void SetSlot(UEquipmentComponent* InEquipment, EEquipSlot InSlot);

	/** Pushes the currently worn item (and the slot's colour) to the bound widgets and the BP hook */
	void RefreshVisuals();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipSlot GetEquipSlot() const { return EquipSlot; }

	/** The item worn here, or an empty instance */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FInventoryItem GetEquippedItem() const;

protected:

	/** Blueprint hook for a richer slot look than the flat SlotBorder tint */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "Slot Updated"))
	void BP_SlotUpdated();

	/** Marks this slot up for a hovering drag (or clears the markup with None) */
	void SetHighlight(EInventoryCellHighlight InHighlight);

	/** True if dropping the held item here would be accepted - mirrors UEquipmentComponent::Equip's checks */
	bool WouldAcceptDrop(const UInventoryDragDropOperation* DragOperation) const;

	/** Sends the worn item back to the pawn's own grid through the move host */
	void TryUnequip();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget interface
};
