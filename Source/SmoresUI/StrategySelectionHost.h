// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StrategyTargetInfo.h"
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

	/**
	 *  Everything the target panel draws about whichever pawn, NPC, or container was most recently
	 *  selected - name, what it is, how far away, its health, and what the player may do to it.
	 *  Returns a struct with an empty DisplayName when nothing is targeted.
	 *
	 *  This **replaced** GetSelectionTargetLabel rather than joining it. One path, not two: a
	 *  label built separately from the panel would eventually name a different target than the
	 *  action row acted on.
	 */
	virtual FStrategyTargetInfo GetSelectionTargetInfo() const = 0;

	/**
	 *  The player's own squad, in the same deterministic order the Tab cycle already walks, so
	 *  the portrait bar doesn't reshuffle itself between frames.
	 *
	 *  Returned by value rather than as a reference to the controller's own list: the controller
	 *  stores AStrategyPlayerUnit and everything on this interface speaks AStrategyUnit, and a
	 *  squad-sized array copied once a frame costs nothing next to keeping two lists in step.
	 *
	 *  Non-const for the same reason GetSelectedUnits() is - answering it may mean rebuilding the
	 *  roster from the world.
	 */
	virtual TArray<AStrategyUnit*> GetControlledPlayerUnits() = 0;
};
