// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDPanelWidget.h"
#include "HelpPanelWidget.generated.h"

/**
 *  The keybind list - `F1` on the nav rail. The one panel in this round that is not a stub.
 *
 *  It is deliberately static text rather than anything generated: the bindings are authored by
 *  hand in IMC_Strategy_Mouse, so there is nothing to read them back from that would be more
 *  truthful than this list, and a keybind settings screen (which *would* be generated) is its own
 *  piece of work. That makes this a thing to keep honest by hand.
 *
 *  **It lists only keys that actually do something.** A help screen naming a key that does
 *  nothing is worse than one that is incomplete - the player tries it, nothing happens, and they
 *  stop trusting the list. Slices 2 and 3 of Docs/roadmaps/hud-roadmap.md add their keys here in
 *  the same change that makes them work, and game-systems/input-and-keybinds.md stays the
 *  authoritative version of this table.
 */
UCLASS(abstract)
class SMORESUI_API UHelpPanelWidget : public UHUDPanelWidget
{
	GENERATED_BODY()

public:

	UHelpPanelWidget();

protected:

	//~ Begin UHUDPanelWidget interface
	virtual FText GetDefaultBodyText() const override;
	//~ End UHUDPanelWidget interface
};
