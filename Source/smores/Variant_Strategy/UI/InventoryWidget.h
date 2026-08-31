// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryComponent.h"
#include "InventoryWidget.generated.h"

class UTextBlock;

/**
 *  Inventory screen for a single selected pawn.
 *  Mirrors the UStrategyUI pattern: C++ owns the data, Blueprint builds the visuals.
 *  The default visual is a single text block listing every slot; a designer can hide it
 *  and build a richer layout off the BP_InventoryUpdated hook instead.
 */
UCLASS(abstract)
class UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Inventory this widget is currently displaying */
	TWeakObjectPtr<UInventoryComponent> BoundInventory;

	/** Optional text block that shows one line per slot. Name it "SlotListText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotListText;

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
};
