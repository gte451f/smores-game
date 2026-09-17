// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDPanelWidget.h"
#include "ResearchPanelWidget.generated.h"

/**
 *  The research panel - `U` on the nav rail.
 *
 *  A stub. A fake tech tree would be worse than an empty panel: it would teach the player a
 *  shape the real system may not take. `U` is the weakest key of the eight and is tracked as an
 *  open question in Docs/roadmaps/hud-roadmap.md - `R` is the conventional one, and is spoken
 *  for by the inventory's rotate key.
 */
UCLASS(abstract)
class SMORESUI_API UResearchPanelWidget : public UHUDPanelWidget
{
	GENERATED_BODY()

public:

	UResearchPanelWidget();

protected:

	//~ Begin UHUDPanelWidget interface
	virtual FText GetDefaultBodyText() const override;
	//~ End UHUDPanelWidget interface
};
