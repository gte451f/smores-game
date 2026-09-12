// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "InventoryComponent.h"
#include "InventorySlotWidget.h"
#include "InventoryWidget.generated.h"

class UTextBlock;
class UPanelWidget;

/**
 *  Inventory screen for a single holder (a selected pawn, a world container, a loot target).
 *  Mirrors the UStrategyUI pattern: C++ owns the data, Blueprint builds the visuals.
 *
 *  The default visual is a single text block listing every placed entry; if a SlotContainer and
 *  SlotWidgetClass are set, one cell widget per grid cell is spawned into it instead (a
 *  UUniformGridPanel lays them out as the actual GridWidth x GridHeight grid, any other
 *  UPanelWidget as a flat list).
 */
UCLASS(abstract)
class SMORESUI_API UInventoryWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Inventory this widget is currently displaying */
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	/** Optional text block that lists the placed entries. Name it "SlotListText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotListText;

	/**
	 *  Optional container for per-cell widgets. A UUniformGridPanel renders the real grid
	 *  (wrapping at the bound inventory's GridWidth); any other UPanelWidget (e.g. UVerticalBox)
	 *  renders a flat list. Name it "SlotContainer" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	/** Widget class spawned once per grid cell into SlotContainer. Must be set for cell widgets to appear. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	/** Cell widgets spawned by the last RefreshDisplay, in row-major order */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;

public:

	/** Binds this widget to an inventory and refreshes the display */
	void SetInventory(UInventoryComponent* InInventory);

	/** Unbinds this widget from its inventory */
	void ClearInventory();

	/** Grid dimensions of the bound inventory (zero if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetGridSize() const;

	/** Placed entries on the bound inventory (empty if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryEntry> GetEntries() const;

	/** Multi-line summary: one line per placed entry with its quantity, cell and orientation */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FText GetContentsSummary() const;

	/** Player-facing label for one carried item - its definition's display name, plus "xN" for a real stack.
	 *  Shared by the summary text and the per-cell widgets so both read the definition the same way. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static FText GetItemLabel(const FInventoryItem& Item);

protected:

	/** Blueprint handler to rebuild custom slot visuals */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Inventory Updated"))
	void BP_InventoryUpdated();

	/** Bound to the inventory's OnInventoryChanged delegate */
	UFUNCTION()
	void HandleInventoryChanged();

	/** Pushes current inventory state to the default text block, the cell widgets and the BP hook */
	void RefreshDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface
};
