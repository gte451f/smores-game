// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "InventoryCellWidget.h"
#include "InventoryItemWidget.h"
#include "PricingProvider.h"
#include "InventoryWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UInventoryDragDropOperation;

/**
 *  Inventory screen for a single holder (a selected pawn, a world container, a loot target).
 *  Mirrors the UStrategyUI pattern: C++ owns the data, Blueprint builds the visuals.
 *
 *  The grid is drawn in two layers into a UGridPanel named SlotContainer: one
 *  UInventoryCellWidget per cell underneath, and one UInventoryItemWidget per placed entry on
 *  top, spanning that entry's footprint. A UUniformGridPanel cannot host the second layer at
 *  all (UUniformGridSlot has no span), so any other panel type degrades to a flat list of item
 *  widgets. The default visual, when no cell/item classes are set, is a single text block
 *  listing every placed entry.
 *
 *  Drag-and-drop is handled once here for the whole grid rather than per cell: with item
 *  widgets sitting on top of cell widgets, per-cell drop handlers get ambiguous about which
 *  cell was actually hit, so the drop bubbles up to this widget and the cell is recovered from
 *  the panel's geometry instead.
 */
UCLASS(abstract)
class SMORESUI_API UInventoryWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Inventory this widget is currently displaying */
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	/** Where a right-click-to-equip from this window sends the item, or null when this window's
	 *  holder has no paperdoll to send it to. Set by whoever opened the window, which is what
	 *  keeps right-click inert over a chest or a loot panel - see SetEquipmentTarget. */
	TWeakObjectPtr<UEquipmentComponent> EquipmentTarget;

	/** Who prices this window's contents while a trade is open, or null when this window isn't
	 *  part of one. Held weakly, and as the bare object rather than a TScriptInterface, so a
	 *  trader who walks away or dies can't be kept alive by a window still showing their stock. */
	TWeakObjectPtr<UObject> PricingSource;

	/** True when the items in this window belong to the *trader* (so the player would be buying
	 *  them), false when they are the player's own (so the player would be selling). Which side
	 *  of the counter a window is on is decided by whoever opened it, never inferred here - the
	 *  same rule that makes right-click mean "equip" in a pawn's window and nothing in a chest's. */
	bool bPricedAsTraderStock = false;

	/** Optional text block that lists the placed entries. Name it "SlotListText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotListText;

	/** Optional carried-weight readout ("Weight: 12.4 / 30.0"). Name it "WeightText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	/** Colour WeightText takes while the holder is within its capacity */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor WeightNormalColor = FLinearColor::White;

	/** Colour WeightText takes while carried weight exceeds capacity. Cosmetic only - being over capacity has no gameplay effect yet. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor WeightOverCapacityColor = FLinearColor(1.0f, 0.35f, 0.25f, 1.0f);

	/**
	 *  Container the grid is built into. A UGridPanel renders the real two-layer grid; any
	 *  other UPanelWidget (e.g. UVerticalBox) degrades to a flat list of item widgets. Name it
	 *  "SlotContainer" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	/** Widget class spawned once per grid cell, underneath the item widgets */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventoryCellWidget> CellWidgetClass;

	/** Widget class spawned once per placed entry, spanning that entry's footprint */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventoryItemWidget> ItemWidgetClass;

	/** Cell widgets spawned by the last RefreshDisplay, in row-major order */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryCellWidget>> CellWidgets;

	/** Item widgets spawned by the last RefreshDisplay, one per placed entry */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemWidget>> ItemWidgets;

	/** Drag currently hovering this grid, if any - held so a mid-drag rotate can redraw the preview */
	UPROPERTY(Transient)
	TObjectPtr<UInventoryDragDropOperation> HoveringDrag;

	/** Last pointer position a hovering drag reported, in screen space */
	FVector2D LastDragScreenPosition = FVector2D::ZeroVector;

	/** Subscription to HoveringDrag's OnRotated */
	FDelegateHandle HoveringDragRotatedHandle;

public:

	/** Binds this widget to an inventory and refreshes the display */
	void SetInventory(UInventoryComponent* InInventory);

	/** Unbinds this widget from its inventory, and from any equipment target with it */
	void ClearInventory();

	/**
	 *  Points right-click-to-equip from this window at a pawn's worn slots. Call it *after*
	 *  SetInventory, which clears the target along with the previous binding.
	 *
	 *  A window opened against a chest or a Downed NPC deliberately leaves this null: right-click
	 *  there does nothing rather than dressing the corpse. The paperdoll itself is a separate
	 *  window (UEquipmentWidget); this is only the routing.
	 */
	void SetEquipmentTarget(UEquipmentComponent* InEquipment);

	/** Worn slots a right-click in this window equips into, or null if this window has none */
	UEquipmentComponent* GetEquipmentTarget() const { return EquipmentTarget.Get(); }

	/**
	 *  Puts this window on one side of a trade counter, so its items can quote a price. Call it
	 *  *after* SetInventory, which clears it along with the previous binding.
	 *
	 *  bItemsAreTraderStock says which side: true for the trader's own window (the player would
	 *  be buying what it shows), false for the player's pack opened alongside it (the player
	 *  would be selling). A window opened against a chest, a corpse or a plain pack leaves this
	 *  unset and quotes nothing.
	 */
	void SetPricing(const TScriptInterface<IPricingProvider>& InPricing, bool bItemsAreTraderStock);

	/** Takes this window back off the trade counter, so its items stop quoting prices. A window
	 *  rebound to a different holder clears this on its own; this is for the case where the
	 *  holder stays and only the trade ends. */
	void ClearPricing();

	/** Prices for this window's contents, or null when it isn't part of a trade */
	const IPricingProvider* GetPricing() const;

	/**
	 *  Hover text quoting what one entry in this window is worth, or empty when this window
	 *  isn't part of a trade. The one place a price is formatted for the player.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FText GetItemPriceTooltip(const FInventoryItem& Item) const;

	/** Grid dimensions of the bound inventory (zero if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetGridSize() const;

	/** Placed entries on the bound inventory (empty if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryEntry> GetEntries() const;

	/** Multi-line summary: one line per placed entry with its quantity, cell and orientation */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FText GetContentsSummary() const;

	/**
	 *  Carried weight against this holder's capacity, as "Weight: 12.4 / 30.0" - or just
	 *  "Weight: 12.4" for a holder with no capacity authored (a chest doesn't carry anything
	 *  anywhere, so a limit on it would be meaningless).
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FText GetWeightSummary() const;

	/** True while the bound holder is over its capacity. Drives WeightText's colour and nothing else. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverWeightCapacity() const;

	/** Player-facing label for one carried item - its definition's display name, plus "xN" for a real stack.
	 *  Shared by the summary text and the item widgets so both read the definition the same way. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static FText GetItemLabel(const FInventoryItem& Item);

protected:

	/** Blueprint handler to rebuild custom slot visuals */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Inventory Updated"))
	void BP_InventoryUpdated();

	/** Bound to the inventory's OnInventoryChanged delegate */
	UFUNCTION()
	void HandleInventoryChanged();

	/** Bound to a hovering drag's OnDrop/OnDragCancelled so the preview can never outlive the drag */
	UFUNCTION()
	void HandleDragEnded(UDragDropOperation* Operation);

	/** Pushes current inventory state to the default text block, the grid and the BP hook */
	void RefreshDisplay();

	/** Rebuilds the cell layer and the item layer inside SlotContainer */
	void RebuildGrid();

	/**
	 *  Converts a screen-space pointer position to a grid cell using SlotContainer's own
	 *  geometry. The returned cell may be outside the grid, which callers treat as an invalid
	 *  drop rather than clamping. False only when there is no panel or no grid to measure.
	 */
	bool ScreenPositionToCell(const FVector2D& ScreenPosition, FIntPoint& OutCell) const;

	/** Cell the dragged item's top-left corner would land on for a pointer at ScreenPosition */
	bool GetDropAnchorCell(const UInventoryDragDropOperation* DragOperation, const FVector2D& ScreenPosition, FIntPoint& OutAnchorCell) const;

	/** True if dropping the held item at AnchorCell would be accepted - mirrors UInventoryComponent::MoveItem's resolution */
	bool WouldAcceptDrop(const UInventoryDragDropOperation* DragOperation, FIntPoint AnchorCell) const;

	/** Starts tracking a drag hovering this grid */
	void SetHoveringDrag(UInventoryDragDropOperation* DragOperation);

	/** Re-marks the cells the hovering drag would claim */
	void UpdateDragPreview();

	/** Stops tracking the hovering drag and clears every cell highlight */
	void ClearDragPreview();

	//~ Begin UUserWidget interface
	virtual void NativeDestruct() override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	//~ End UUserWidget interface

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface
};
