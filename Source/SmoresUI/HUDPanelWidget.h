// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "HUDPanel.h"
#include "HUDPanelWidget.generated.h"

class UTextBlock;

/**
 *  Base for every floating panel the nav rail opens. A UWindowWidget subclass, so drag, resize,
 *  the title bar and the close button all come for free and behave the way the inventory windows
 *  already do - including swallowing the press of any mouse button that lands on it.
 *
 *  What it adds is the two things the controller and the rail need from all of them: an
 *  EHUDPanel id, so a panel can be tracked without a cast per class, and a single body text
 *  block filled from GetBodyText(). Three of the four panels built this round are honest stubs
 *  whose body says what will live there and which design topic owns it; the help panel overrides
 *  the same hook with a real keybind list. That is the whole difference between them, which is
 *  why they are thin subclasses rather than four copies of this.
 *
 *  **A HUD panel must never touch AStrategyPlayerController::UpdateInventoryInputContext.** That
 *  context is the inventory's business; a stub panel that added it would quietly give `R` a
 *  second meaning while an empty research window was open.
 */
UCLASS(abstract)
class SMORESUI_API UHUDPanelWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Which panel this window is. Set as a class default on the C++ subclass; overridable per WBP. */
	UPROPERTY(EditAnywhere, Category = "Panel")
	EHUDPanel PanelId = EHUDPanel::None;

	/** The panel's body copy. Name it "BodyText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	/**
	 *  Optional per-WBP replacement for the C++ body copy. Left empty (the default) the panel
	 *  shows GetDefaultBodyText(), which is where the real wording lives.
	 */
	UPROPERTY(EditAnywhere, Category = "Panel", meta = (MultiLine = true))
	FText BodyOverride;

public:

	/** Which panel this window is */
	EHUDPanel GetPanelId() const { return PanelId; }

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface

protected:

	/** The copy actually shown: the WBP's override if one was authored, otherwise GetDefaultBodyText() */
	UFUNCTION(BlueprintPure, Category = "Panel")
	FText GetBodyText() const;

	/**
	 *  The panel's own body copy. Subclasses override this rather than setting text in a WBP, so
	 *  a stub's "what will live here, and what owns it" line is versioned with the code and shows
	 *  up in a grep.
	 */
	virtual FText GetDefaultBodyText() const;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
