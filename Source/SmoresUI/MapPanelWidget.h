// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDPanelWidget.h"
#include "MapPanelWidget.generated.h"

/**
 *  The world map panel - `M` on the nav rail.
 *
 *  A stub, and it will stay one until there is a world map to draw. `M` has been reserved for
 *  exactly this since the keybind list was written, so taking it now costs nothing and stops
 *  anything else claiming it.
 */
UCLASS(abstract)
class SMORESUI_API UMapPanelWidget : public UHUDPanelWidget
{
	GENERATED_BODY()

public:

	UMapPanelWidget();

protected:

	//~ Begin UHUDPanelWidget interface
	virtual FText GetDefaultBodyText() const override;
	//~ End UHUDPanelWidget interface
};
