// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryCellWidget.generated.h"

class UBorder;

/** How a grid cell is currently being marked up by a drag hovering over the grid */
UENUM(BlueprintType)
enum class EInventoryCellHighlight : uint8
{
	/** Nothing hovering this cell */
	None,
	/** A hovering drag would land here, and the drop would be accepted */
	Valid,
	/** A hovering drag would land here, but the drop would be rejected */
	Invalid
};

/**
 *  One cell of a holder's inventory grid - background only. It draws no item and handles no
 *  mouse input: items are drawn by UInventoryItemWidget over the top of these, and the drop is
 *  handled once for the whole grid by the owning UInventoryWidget.
 *
 *  Its one interactive job is being the drag preview surface: while a drag hovers the grid, the
 *  cells its footprint would claim are marked Valid or Invalid, which is the only feedback that
 *  tells the player whether the drop will take and whether the rotate key did anything.
 */
UCLASS(abstract)
class SMORESUI_API UInventoryCellWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional background border this widget tints per highlight state. Name it "CellBorder" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CellBorder;

	/** This cell's grid coordinate (X = column, Y = row) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint CellCoord = FIntPoint::ZeroValue;

	/** How this cell is currently marked up by a hovering drag */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EInventoryCellHighlight Highlight = EInventoryCellHighlight::None;

	/** CellBorder tint with nothing hovering */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor EmptyCellColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.08f);

	/** CellBorder tint for a cell a hovering drag would legally land on */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor ValidHighlightColor = FLinearColor(0.15f, 0.8f, 0.25f, 0.5f);

	/** CellBorder tint for a cell a hovering drag covers but can't be dropped onto */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FLinearColor InvalidHighlightColor = FLinearColor(0.9f, 0.15f, 0.1f, 0.5f);

public:

	/** Binds this widget to a grid coordinate. Called by the owning UInventoryWidget as it builds the grid. */
	void SetCell(FIntPoint InCellCoord);

	/** Marks this cell up for a hovering drag (or clears the markup with None) */
	void SetHighlight(EInventoryCellHighlight InHighlight);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetCellCoord() const { return CellCoord; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	EInventoryCellHighlight GetHighlight() const { return Highlight; }

protected:

	/** Blueprint hook for a richer highlight look than the flat CellBorder tint */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory", meta = (DisplayName = "Cell Highlight Changed"))
	void BP_HighlightChanged(EInventoryCellHighlight NewHighlight);

	/** Pushes the current highlight state to CellBorder and the BP hook */
	void RefreshVisuals();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
