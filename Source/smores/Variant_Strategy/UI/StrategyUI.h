// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyUI.generated.h"

/**
 *  Simple UI widget for the strategy game
 *	Keeps track of the number of units currently selected
 */
UCLASS(abstract)
class UStrategyUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Number of units currently selected */
	int32 SelectedUnitCount = 0;

	/** Text describing the currently targeted pawn, NPC, or container (e.g. "Pawn: Pawn 1") */
	FText SelectionTargetLabel;

public:

	/** Sets the number of units selected */
	void SetSelectedUnitsCount(int32 Count);

	/** Sets the currently targeted pawn/NPC/container label */
	void SetSelectionTargetLabel(const FText& Label);

	/** Blueprint handler to update unit count sub-widgets */
	UFUNCTION(BlueprintImplementableEvent, Category="UI", meta = (DisplayName="Update Units Count"))
	void BP_UpdateUnitsCount();

protected:

	/** Returns the number of units selected */
	UFUNCTION(BlueprintPure, Category="UI")
	int32 GetSelectedUnitsCount() { return SelectedUnitCount; }

	/** Returns the currently targeted pawn/NPC/container label */
	UFUNCTION(BlueprintPure, Category="UI")
	FText GetSelectionTargetLabel() { return SelectionTargetLabel; }
};
