// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyTargetInfo.h"
#include "ActionMenuWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UTargetActionWidget;

/**
 *  The right-click menu: what the squad can do to the thing that was right-clicked, at the cursor.
 *
 *  **Not a second list.** Every row comes from the same FStrategyTargetInfo the target panel
 *  draws, built by the same rules and pushed every frame by AStrategyHUD::DrawHUD - so the menu
 *  and the panel can never disagree about what a thing offers, and a row greys out the moment its
 *  action stops being possible. The menu has **its own target**, though, not the panel's: `Tab`
 *  can retarget the panel while the menu is open.
 *
 *  **Two parts.** The whole widget is a full-screen click catcher: while the menu is open, the
 *  press (and the double-click - see input-and-keybinds.md) of any mouse button anywhere outside
 *  the panel closes the menu and does nothing else, so a click-away never also moves or selects.
 *  The release is left alone, the same rule the windows follow. The panel itself sits at the
 *  cursor, clamped to the screen, with the target's name, what it is, who would go, and one row
 *  per action - rows reuse UTargetActionWidget, and so WBP_TargetAction.
 *
 *  Its own layer, owned by AStrategyHUD at Z-order 50: above every window (a menu opened next to an
 *  open inventory must not draw behind it) and below the refusal line at 100. The game keeps
 *  running while it is open - notifications-and-alerts.md's "never changes the simulation on the
 *  player's behalf" applies to menus too.
 */
UCLASS(abstract)
class SMORESUI_API UActionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/**
	 *  The visible menu - everything below lives inside it. Name it "MenuPanel" in the WBP to
	 *  auto-bind, and put it in a Canvas Panel with Size To Content on: it is moved to the cursor
	 *  and clamped to the screen by its canvas slot. A click inside it never closes the menu.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MenuPanel;

	/** The target's name. Name it "NameText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** What it is and how far ("PERSON - HOSTILE - 12m"). Name it "ClassificationText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassificationText;

	/** Who would go ("Hana would go"). Name it "ActorText" to auto-bind. Hidden when nobody would - the rows say why. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActorText;

	/** Holds the rows. Name it "EntryBox" to auto-bind - a vertical box. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EntryBox;

	/** The widget each row is - WBP_TargetAction, the same one the target panel uses */
	UPROPERTY(EditAnywhere, Category = "Action Menu")
	TSubclassOf<UTargetActionWidget> EntryWidgetClass;

	/** Where the menu's corner sits relative to the cursor, in widget-space pixels, so it doesn't open under the pointer */
	UPROPERTY(EditAnywhere, Category = "Action Menu")
	FVector2D CursorOffset = FVector2D(12.0f, 4.0f);

	/** What the menu is about. Its own, not the target panel's. */
	TWeakObjectPtr<AActor> MenuTarget;

	/** What the HUD last pushed in */
	FStrategyTargetInfo Info;

	/** The rows currently in EntryBox, in order - reused between frames like the panel's */
	UPROPERTY()
	TArray<TObjectPtr<UTargetActionWidget>> EntryWidgets;

	/** Where the menu was opened, in widget space, before clamping - kept so clamping never walks it away from the cursor */
	FVector2D OpenPosition = FVector2D::ZeroVector;

	/** True while the menu is showing */
	bool bOpen = false;

	/** Warned once when a row was wanted and EntryWidgetClass was empty */
	bool bWarnedNoEntryClass = false;

public:

	/** Opens the menu on Target at the cursor, replacing whatever it was showing. The first push fills it. */
	void OpenAt(AActor* Target);

	/** Hides the menu. Harmless when it isn't open. */
	void Close();

	/** True while the menu is showing */
	bool IsOpen() const { return bOpen; }

	/** What the menu is about, or null when it's closed */
	AActor* GetMenuTarget() const { return bOpen ? MenuTarget.Get() : nullptr; }

	/**
	 *  The per-frame push: the rows for the menu's target, rebuilt by the controller. Redraws only if
	 *  something visible changed, and closes the menu if its target has stopped existing.
	 */
	void SetTargetInfo(const FStrategyTargetInfo& NewInfo);

	/** Blueprint handler for anything beyond the bound header and rows */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Action Menu"))
	void BP_UpdateActionMenu();

protected:

	/** A row was clicked: straight to the controller with the menu's own target, then the menu closes */
	void HandleEntryClicked(FName ActionId);

	/** Grows or shrinks EntryWidgets to match the action count */
	void ResizeEntries(int32 EntryCount);

	/** Pushes Info into every bound widget and the BP hook */
	void RefreshMenuDisplay();

	/** Keeps the whole panel on screen: flips it to the other side of the cursor rather than letting it run off an edge */
	void PlaceAtOpenPosition();

	/** True if a screen position falls on the panel rather than on the click catcher around it */
	bool IsOverMenuPanel(const FVector2D& ScreenPosition) const;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget interface
};
