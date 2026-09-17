// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "HUDRegionWidget.h"
#include "StrategyHUDCommands.h"
#include "GameFramework/PlayerController.h"

IStrategyHUDCommands* UHUDRegionWidget::GetHUDCommands() const
{
	return Cast<IStrategyHUDCommands>(GetOwningPlayer());
}

void UHUDRegionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bBlockWorldClicks)
	{
		// a region that doesn't hit-test never receives the press it is here to swallow, and
		// SelfHitTestInvisible is the designer default for a lot of panel roots
		SetVisibility(ESlateVisibility::Visible);
	}
}

FReply UHUDRegionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bBlockWorldClicks)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// Button-agnostic, and deliberately so: right-click means "move order" in the world, and a
	// right-click that lands on the HUD should mean nothing at all rather than ordering the squad
	// to walk to whatever is underneath the readout. Same reasoning as
	// UWindowWidget::NativeOnMouseButtonDown, which this mirrors - and like that one, the release
	// is left alone, since Enhanced Input never saw the press this ate.
	return FReply::Handled();
}

FReply UHUDRegionWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// The second click of a rapid pair arrives as its own Slate event, not as a press - claiming
	// only the press leaves a hole the second click falls through, where it reaches the viewport
	// and fires IA_Strategy_SelectAllDoubleClick. See input-and-keybinds.md.
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
