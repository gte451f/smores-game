// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "InventoryItemWidget.generated.h"

class UTextBlock;
class USizeBox;

/**
 *  One placed item, drawn as a single widget spanning its whole footprint rather than once per
 *  cell - which is what gives a multi-cell item one border and one centred label instead of a
 *  name stranded in its top-left corner.
 *
 *  It's also the drag source: a drag carries the entry id, the orientation it's being held at,
 *  and which cell of the footprint the pointer grabbed, so the item lands where its ghost sits
 *  rather than under the cursor. The same class doubles as that ghost - the drag decorator is
 *  another instance of it, resized to the footprint and re-drawn whenever the rotate key flips
 *  the orientation mid-flight. The rotate key itself belongs to the player controller (an
 *  Enhanced Input action, so it is player-rebindable); this widget only supplies the drag.
 *
 *  Right-clicking one wears it, when the window it sits in has a paperdoll to wear it into.
 *  That's a modifier-free mouse interaction inside a widget, so it deliberately isn't a
 *  UInputAction - the click event is all the state it needs.
 */
UCLASS(abstract)
class SMORESUI_API UInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional item label, centred over the footprint. Name it "ItemLabel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemLabel;

	/**
	 *  Optional size box wrapping this widget's content. Unused in the grid (the grid slot's
	 *  spans size the widget), but required on a drag decorator, which has no slot to stretch
	 *  it - see SetPreviewOrientation. Name it "ItemSizeBox" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> ItemSizeBox;

	/** Inventory this item is placed in. Set alongside Entry by SetEntry. */
	TWeakObjectPtr<UInventoryComponent> OwningInventory;

	/** The placed entry this widget draws */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryEntry Entry;

	/** Orientation this widget is drawn at. Matches the entry in the grid; follows the drag while acting as a decorator. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bDrawRotated = false;

public:

	/** Binds this widget to a placed entry and refreshes its visuals. Called by the owning UInventoryWidget. */
	void SetEntry(UInventoryComponent* InOwningInventory, const FInventoryEntry& InEntry);

	/**
	 *  Re-draws at the given orientation and, when a size box is bound, resizes to the
	 *  footprint x CellSize. Only the drag decorator needs this - a grid instance is sized by
	 *  its grid slot's row/column spans instead.
	 */
	void SetPreviewOrientation(bool bInRotated, const FVector2D& CellSize);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryEntry GetEntry() const { return Entry; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItem() const { return Entry.Item; }

	/** Footprint in cells at the orientation this widget is currently drawn at */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetDrawnFootprint() const { return Entry.Item.GetFootprint(bDrawRotated); }

protected:

	/** Blueprint hook for cosmetic click feedback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Item Clicked"))
	void BP_ItemClicked();

	/** Blueprint hook to rebuild custom item visuals after a bind or a mid-drag rotate */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Item Updated"))
	void BP_ItemUpdated();

	/** Pushes the current entry and orientation to ItemLabel and the BP hook */
	void RefreshVisuals();

	/** Pushes this item's price, if the window it sits in is part of a trade, onto its hover tooltip */
	void RefreshPriceTooltip();

	/** Sends this entry to the owning window's equipment target, if it has one and the item is wearable */
	void TryEquip();

	//~ Begin UUserWidget interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	//~ End UUserWidget interface
};
