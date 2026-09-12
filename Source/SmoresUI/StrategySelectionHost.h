// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategySelectionHost.generated.h"

class AStrategyUnit;

UINTERFACE(MinimalAPI)
class UStrategySelectionHost : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by whichever PlayerController owns the strategy HUD's drag-select box and unit selection. */
class SMORESUI_API IStrategySelectionHost
{
	GENERATED_BODY()

public:

	/** Updates selected units from the HUD's drag select box */
	virtual void DragSelectUnits(const TArray<AStrategyUnit*>& Units) = 0;

	/** Returns the currently selected units */
	virtual const TArray<AStrategyUnit*>& GetSelectedUnits() = 0;

	/** Returns the label text for whichever pawn, NPC, or container was most recently selected, or empty if none */
	virtual FText GetSelectionTargetLabel() const = 0;
};
