// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyUI.generated.h"

class UTextBlock;

/**
 *  Simple UI widget for the strategy game
 *	Keeps track of the number of units currently selected, and carries the quick-access
 *	resource readout (cash on hand)
 */
UCLASS(abstract)
class SMORESUI_API UStrategyUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Number of units currently selected */
	int32 SelectedUnitCount = 0;

	/** Text describing the currently targeted pawn, NPC, or container (e.g. "Pawn: Pawn 1") */
	FText SelectionTargetLabel;

	/** Owning player's current gold balance, pushed by the HUD */
	int32 Gold = 0;

	/** Optional at-a-glance gold readout. Name it "GoldText" in the WBP to auto-bind; C++ sets its text on every change. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

public:

	/** Sets the number of units selected */
	void SetSelectedUnitsCount(int32 Count);

	/** Sets the currently targeted pawn/NPC/container label */
	void SetSelectionTargetLabel(const FText& Label);

	/** Sets the owning player's gold balance, refreshing the readout if it actually changed */
	void SetGold(int32 NewGold);

	/** Blueprint handler to update unit count sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta = (DisplayName="Update Units Count"))
	void BP_UpdateUnitsCount();

	/** Blueprint handler to update gold sub-widgets beyond the bound GoldText block */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta = (DisplayName="Update Gold"))
	void BP_UpdateGold();

protected:

	/** Returns the number of units selected */
	UFUNCTION(BlueprintPure, Category="UI")
	int32 GetSelectedUnitsCount() { return SelectedUnitCount; }

	/** Returns the currently targeted pawn/NPC/container label */
	UFUNCTION(BlueprintPure, Category="UI")
	FText GetSelectionTargetLabel() { return SelectionTargetLabel; }

	/** Returns the owning player's gold balance */
	UFUNCTION(BlueprintPure, Category="UI")
	int32 GetGold() const { return Gold; }

	/** Returns the formatted gold readout (e.g. "Gold: 1,250") - what GoldText is filled with */
	UFUNCTION(BlueprintPure, Category="UI")
	FText GetGoldLabel() const;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	/** Pushes the current balance into GoldText and the BP hook */
	void RefreshGoldDisplay();
};
