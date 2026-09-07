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
 *  Inventory screen for a single selected pawn.
 *  Mirrors the UStrategyUI pattern: C++ owns the data, Blueprint builds the visuals.
 *  The default visual is a single text block listing every slot; if a SlotContainer and
 *  SlotWidgetClass are set, per-slot widgets are spawned into it instead (a UUniformGridPanel
 *  renders as a grid, any other UPanelWidget renders as a list).
 */
UCLASS(abstract)
class UInventoryWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Inventory this widget is currently displaying */
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	/** Optional text block that shows one line per slot. Name it "SlotListText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotListText;

	/**
	 *  Optional container for per-slot widgets. A UUniformGridPanel renders as a grid
	 *  (using GridColumns); any other UPanelWidget (e.g. UVerticalBox) renders as a list.
	 *  Name it "SlotContainer" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	/** Widget class spawned once per slot into SlotContainer. Must be set for slot widgets to appear. */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	/** Number of columns to wrap at when SlotContainer is a UUniformGridPanel. Ignored otherwise. */
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = 1))
	int32 GridColumns = 8;

	/** Slot widgets spawned by the last RefreshDisplay */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;

public:

	/** Binds this widget to an inventory and refreshes the display */
	void SetInventory(UInventoryComponent* InInventory);

	/** Unbinds this widget from its inventory */
	void ClearInventory();

	/** Number of slots on the bound inventory (0 if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetNumSlots() const;

	/** Items on the bound inventory (empty if none) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryItem> GetItems() const;

	/** Multi-line summary: one line per slot, "(empty)" for unfilled slots */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FText GetSlotSummary() const;

protected:

	/** Blueprint handler to rebuild custom slot visuals */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Inventory Updated"))
	void BP_InventoryUpdated();

	/** Bound to the inventory's OnInventoryChanged delegate */
	UFUNCTION()
	void HandleInventoryChanged();

	/** Pushes current inventory state to the default text block and the BP hook */
	void RefreshDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface
};
