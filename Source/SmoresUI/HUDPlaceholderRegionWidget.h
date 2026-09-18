// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "HUDPlaceholderRegionWidget.generated.h"

class UTextBlock;

/**
 *  A labelled empty box standing in for a HUD region that hasn't been built yet.
 *
 *  It exists so the whole frame can be judged at once rather than a corner at a time - but the
 *  reason it is a **widget class** rather than a `UBorder` dropped into `UI_Strategy` is a bug
 *  that shipped and was caught in PIE: a plain `UBorder` on the HUD does not consume the mouse
 *  press that lands on it. Slate bubbles the unhandled event up past the canvas to the game
 *  viewport, so a right-click on the placeholder also issued a move order to the selected squad.
 *  Only a `UHUDRegionWidget` swallows the press, so a placeholder has to be one.
 *
 *  The general rule this is an instance of: **anything drawn on the HUD that the player can see
 *  as a panel must be a `UHUDRegionWidget`,** placeholder or not. See `hud-and-panels.md`.
 *
 *  All six regions of the HUD are now real, so nothing in UI_Strategy uses this class today. It
 *  is kept for the next region that gets laid out before it is built: swapping a placeholder for
 *  the real widget keeps the same base class, so the click shield survives the swap.
 */
UCLASS(abstract)
class SMORESUI_API UHUDPlaceholderRegionWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/**
	 *  What this region will hold, and which slice builds it. Set per instance in the parent
	 *  widget's details panel - one WBP serves all four regions, which is why this is a property
	 *  rather than an override like the panel windows' body copy.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Region")
	FText RegionLabel;

	/** The label. Name it "LabelText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

public:

	/** Sets the label at runtime (no-op if LabelText isn't bound) */
	UFUNCTION(BlueprintCallable, Category = "HUD Region")
	void SetRegionLabel(const FText& InLabel);

protected:

	//~ Begin UWidget interface
	// Pushed through SynchronizeProperties as well as NativeConstruct so the label shows in the
	// UMG designer, not just in PIE - the whole point of these boxes is judging layout, and half
	// of that happens in the designer.
	virtual void SynchronizeProperties() override;
	//~ End UWidget interface

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

private:

	/** Pushes RegionLabel into LabelText */
	void RefreshLabel();
};
