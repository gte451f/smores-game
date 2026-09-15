// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SmoresRefusalReason.h"
#include "RefusalWidget.generated.h"

class UTextBlock;
class USoundBase;
class APlayerController;

/**
 *  The one line that says why something was refused ("Too far away", "Not enough gold").
 *
 *  **Its own viewport widget, deliberately, rather than a text block inside UI_Strategy.** Slate
 *  paints viewport widgets in Z-order and then in the order they were added, and every window in
 *  this game goes up at Z-order 0 *after* the HUD does - so a refusal living in the HUD renders
 *  behind the very inventory window the player was working in, which is exactly where they were
 *  looking. Splitting it out is what lets this sit above everything while the HUD's persistent
 *  readouts stay *below* a dragged window, which is what they should do.
 *
 *  That is the general shape: a transient message that answers what the player just did belongs
 *  on its own top-most layer, not in the readout HUD. Anything else needing to sit over open
 *  windows (a confirmation prompt, a tooltip that must escape its panel) wants this layer, or one
 *  like it, rather than a higher Z-order on UI_Strategy.
 *
 *  Never modal, never focusable, and hit-test-transparent all the way down - a refusal that ate
 *  the player's next click would cost more than the refusal did.
 */
UCLASS(abstract)
class SMORESUI_API URefusalWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/**
	 *  The refusal line itself. Name it "RefusalText" in the WBP to auto-bind; C++ fills it,
	 *  positions it and hides it again, so it needs no Blueprint graph work.
	 *
	 *  Put it in a Canvas Panel to get the at-the-cursor behaviour - it is moved to the mouse on
	 *  every refusal. In any other panel it simply stays wherever it was placed, which is a
	 *  perfectly good fixed-position readout, just a less immediate one.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RefusalText;

	/** How long a refusal stays on screen before clearing itself */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refusal")
	float RefusalDisplaySeconds = 2.0f;

	/** Offset from the cursor, in widget-space pixels, so the line sits clear of the pointer rather than under it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refusal")
	FVector2D RefusalCursorOffset = FVector2D(20.0f, 20.0f);

	/**
	 *  Repeats of the *same* reason inside this window only extend the line's time on screen -
	 *  they don't replay the sound or re-fire the BP hook. Without it, leaning on a key or
	 *  double-clicking a chest you can't reach machine-guns the refusal sound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refusal")
	float RefusalRepeatSeconds = 0.4f;

	/** Optional "denied" sound, played once per refusal. Unset is fine - the line still shows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refusal")
	TObjectPtr<USoundBase> RefusalSound;

	/** The reason currently on screen, or None when nothing is showing */
	ESmoresRefusalReason CurrentRefusal = ESmoresRefusalReason::None;

	/** World time the current refusal was last raised, for the repeat suppression above */
	double LastRefusalTime = 0.0;

	/** Clears the line once RefusalDisplaySeconds is up */
	FTimerHandle RefusalClearTimer;

public:

	/**
	 *  The words for a refusal code. The single place any of them is phrased - a refusal
	 *  travels as an ESmoresRefusalReason and becomes English only here.
	 */
	UFUNCTION(BlueprintPure, Category = "Refusal")
	static FText GetRefusalText(ESmoresRefusalReason Reason);

	/**
	 *  Raises a refusal on a player's own HUD without needing a reference to this widget.
	 *
	 *  The single place HUD -> refusal widget -> ShowRefusal is resolved, so anything that
	 *  decides a rule for itself can report it. Safe with a null controller, a player with no HUD
	 *  (a dedicated server), or a HUD whose widget hasn't been created yet - it does nothing
	 *  rather than making every caller check.
	 */
	static void RaiseRefusal(const APlayerController* OwningPlayer, ESmoresRefusalReason Reason);

	/** Shows a refusal at the cursor for RefusalDisplaySeconds, replacing anything already showing. A reason of None does nothing. */
	UFUNCTION(BlueprintCallable, Category = "Refusal")
	void ShowRefusal(ESmoresRefusalReason Reason);

	/** Hides the line early - a successful action doesn't need the last refusal still on screen */
	UFUNCTION(BlueprintCallable, Category = "Refusal")
	void ClearRefusal();

	/** Blueprint hook for cosmetic response to a refusal - a shake, a flash, a bespoke sound */
	UFUNCTION(BlueprintImplementableEvent, Category = "Refusal", meta = (DisplayName = "Refusal Shown"))
	void BP_RefusalShown(ESmoresRefusalReason Reason);

	/** Blueprint hook for the line going away again */
	UFUNCTION(BlueprintImplementableEvent, Category = "Refusal", meta = (DisplayName = "Refusal Cleared"))
	void BP_RefusalCleared();

protected:

	/** The reason currently on screen, or None when the line is hidden */
	UFUNCTION(BlueprintPure, Category = "Refusal")
	ESmoresRefusalReason GetCurrentRefusal() const { return CurrentRefusal; }

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** Moves the line to the mouse cursor. No-op unless RefusalText sits in a Canvas Panel. */
	void PositionRefusalAtCursor();
};
