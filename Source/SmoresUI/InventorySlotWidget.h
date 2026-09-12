// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "InventorySlotWidget.generated.h"

class UTextBlock;

/**
 *  One cell of a holder's inventory grid. Placeholder visual is plain text; a designer wraps
 *  SlotText in a UBorder in the WBP for the "box around text" look. BP_CellClicked is a hook
 *  for cosmetic click feedback.
 *
 *  A cell covered by a placed entry is a drag source (NativeOnDragDetected, carrying that
 *  entry's id), and every cell is a drop target (NativeOnDrop, supplying its own coordinate as
 *  the destination) - within one grid or between the two paired panels (pawn <-> container),
 *  since OwningInventory is just whatever component this widget is currently bound to.
 *
 *  Interim rendering: one widget per cell, with the item's label drawn only on its anchor cell,
 *  so a multi-cell footprint reads as a label followed by blank cells. Slice 3 of the inventory
 *  roadmap replaces this with item widgets that span their footprint.
 */
UCLASS(abstract)
class SMORESUI_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional label text for this cell. Name it "SlotText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotText;

	/** Inventory this cell belongs to. Set alongside CellCoord/Entry by SetCell. */
	TWeakObjectPtr<UInventoryComponent> OwningInventory;

	/** This cell's grid coordinate (X = column, Y = row) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint CellCoord = FIntPoint::ZeroValue;

	/** The placed entry whose footprint covers this cell; an invalid entry means the cell is free */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryEntry Entry;

public:

	/** Sets owning inventory + cell coordinate + covering entry and refreshes SlotText. Called by the owning UInventoryWidget. */
	void SetCell(UInventoryComponent* InOwningInventory, FIntPoint InCellCoord, const FInventoryEntry& InEntry);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetCellCoord() const { return CellCoord; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryEntry GetEntry() const { return Entry; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FInventoryItem GetItem() const { return Entry.Item; }

	/** True if no placed entry covers this cell */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsCellEmpty() const { return !Entry.IsValidEntry(); }

	/** True if this is the top-left cell of the covering entry's footprint - the one that draws its label */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsAnchorCell() const { return Entry.IsValidEntry() && Entry.AnchorCell == CellCoord; }

protected:

	/** Blueprint hook for cosmetic click feedback */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Cell Clicked"))
	void BP_CellClicked();

	//~ Begin UUserWidget interface
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget interface
};
