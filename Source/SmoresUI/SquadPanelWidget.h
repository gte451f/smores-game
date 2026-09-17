// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDPanelWidget.h"
#include "SquadPanelWidget.generated.h"

/**
 *  The squad roster panel - `P` on the nav rail.
 *
 *  A stub for now: the roster it will hold is the same list the bottom-left portrait bar shows,
 *  at full size and with the per-unit detail a portrait has no room for. It exists this early
 *  because the frame is the thing that has to be lived with, and an empty panel the player can
 *  open is better than a HUD that grows a new corner when the roster ships.
 */
UCLASS(abstract)
class SMORESUI_API USquadPanelWidget : public UHUDPanelWidget
{
	GENERATED_BODY()

public:

	USquadPanelWidget();

protected:

	//~ Begin UHUDPanelWidget interface
	virtual FText GetDefaultBodyText() const override;
	//~ End UHUDPanelWidget interface
};
