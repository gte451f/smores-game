// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HUDPanel.h"
#include "StrategyHUDCommands.generated.h"

class AStrategyUnit;

UINTERFACE(MinimalAPI)
class UStrategyHUDCommands : public UInterface
{
	GENERATED_BODY()
};

/**
 *  The things the HUD needs the player controller to *do*.
 *
 *  SmoresUI cannot include anything from `smores`, so a widget that wants the controller to act
 *  reaches it through this, the same way drag-select and the camera buttons already reach it
 *  through IStrategySelectionHost and IStrategyCameraCommands. AStrategyPlayerController
 *  implements it alongside those.
 *
 *  **This is one interface for behaviour, not four.** Per CLAUDE.md, a component is the better
 *  shape when the UI wants per-player *state*; everything here is a request that only the
 *  controller can service. Data the HUD only needs to *read* goes the other way - through
 *  AStrategyHUD::DrawHUD's per-frame push, or through a component looked up the way
 *  AStrategyHUD::GetWallet() looks up the wallet.
 */
class SMORESUI_API IStrategyHUDCommands
{
	GENERATED_BODY()

public:

	/**
	 *  Opens the named panel, or closes it if it is already open. The nav rail button and the
	 *  panel's key both come through here, so a refusal comes out the same way either way.
	 */
	virtual void RequestPanel(EHUDPanel Panel) = 0;

	/** True while the named panel's window is on screen. Drives the rail's "this one is active" state. */
	virtual bool IsPanelOpen(EHUDPanel Panel) const = 0;

	/**
	 *  Selects the given unit, optionally cutting the camera to it. The squad portrait bar's
	 *  click and double-click.
	 *
	 *  Stubbed until the portrait bar exists - see Docs/roadmaps/hud-roadmap.md, Slice 3.
	 */
	virtual void RequestSelectUnit(AStrategyUnit* Unit, bool bFocusCamera) = 0;

	/**
	 *  Runs the action the target panel offered under this id (open a container, talk, attack,
	 *  loot). The panel only ever offers what the controller's own gating helpers already
	 *  permit, so this re-checks rather than trusts.
	 *
	 *  Stubbed until the target panel exists - see Docs/roadmaps/hud-roadmap.md, Slice 2.
	 */
	virtual void RequestTargetAction(FName ActionId) = 0;
};
