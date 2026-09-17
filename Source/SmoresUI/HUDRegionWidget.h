// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDRegionWidget.generated.h"

class IStrategyHUDCommands;

/**
 *  Base for one region of the always-on HUD - the nav rail, the resource strip, and (later) the
 *  pace strip, target panel, squad bar and activity feed. Two things every region needs, and
 *  neither comes for free:
 *
 *  **It swallows mouse presses that land on it.** The persistent HUD sits at Z-order 0, below
 *  every floating window, and unlike a window it gets none of UWindowWidget's click-eating
 *  behaviour. Without this, a click on the gap between two rail buttons falls straight through
 *  to the world and starts a drag-selection behind the HUD. Real UButtons inside a region
 *  already consume their own clicks (Slate's SButton handles both the press and the
 *  double-click), so this only ever catches what nothing inside the region claimed.
 *
 *  **It knows how to reach the player controller.** GetHUDCommands() is the one place the cast
 *  lives, so no region widget has to know that the controller is where requests go.
 *
 *  The shield only works if this widget actually hit-tests, which is why bBlockWorldClicks
 *  forces Visible on construct: a region left at the designer's SelfHitTestInvisible would
 *  silently pass every click through, and that failure is invisible until someone notices
 *  their squad running to a spot under the HUD.
 */
UCLASS(abstract)
class SMORESUI_API UHUDRegionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/**
	 *  If true, this region's whole rectangle stops mouse presses reaching the world behind it,
	 *  and the widget is forced to Visible on construct so that it can. Turn it off only for a
	 *  region that is deliberately see-through to clicks.
	 */
	UPROPERTY(EditAnywhere, Category = "HUD Region")
	bool bBlockWorldClicks = true;

public:

	/** The owning player controller as the HUD command interface, or null if it doesn't implement it */
	IStrategyHUDCommands* GetHUDCommands() const;

protected:

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget interface
};
