// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "ResourceStripWidget.generated.h"

class UTextBlock;

/**
 *  The top-right resource strip: the at-a-glance figures the player should never have to open a
 *  panel to read. Gold is the only one today, and it moved here out of UStrategyUI so that the
 *  HUD root hosts regions rather than holding readouts itself - and so the next such figure
 *  (food, water, squad weight) has an obvious home instead of being wedged in beside a number
 *  that was already there.
 *
 *  The balance still arrives the same way it always did: AStrategyHUD resolves UWalletComponent
 *  off its own player state and pushes it every frame through UStrategyUI::SetGold. Only the
 *  display moved.
 */
UCLASS(abstract)
class SMORESUI_API UResourceStripWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** Owning player's current gold balance, pushed by the HUD through UStrategyUI */
	int32 Gold = 0;

	/** The gold readout. Name it "GoldText" in the WBP to auto-bind; C++ sets its text on every change. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

public:

	/** Sets the owning player's gold balance, refreshing the readout if it actually changed */
	void SetGold(int32 NewGold);

	/** Blueprint handler to update gold sub-widgets beyond the bound GoldText block */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Gold"))
	void BP_UpdateGold();

protected:

	/** Returns the owning player's gold balance */
	UFUNCTION(BlueprintPure, Category = "UI")
	int32 GetGold() const { return Gold; }

	/** Returns the formatted gold readout (e.g. "Gold: 1,250") - what GoldText is filled with */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetGoldLabel() const;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	/** Pushes the current balance into GoldText and the BP hook */
	void RefreshGoldDisplay();
};
