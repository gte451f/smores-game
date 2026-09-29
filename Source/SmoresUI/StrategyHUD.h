// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StrategyHUD.generated.h"

class UStrategyUI;
class URefusalWidget;
class UActionMenuWidget;
class UWalletComponent;
class UTimePaceComponent;

/**
 *  Simple strategy game HUD
 *  Draws the selection box and unit selected overlays
 */
UCLASS(abstract)
class SMORESUI_API AStrategyHUD : public AHUD
{
	GENERATED_BODY()
	
protected:

	/** Pointer to the UI user widget */
	UPROPERTY()
	TObjectPtr<UStrategyUI> UIWidget;

	/** Type of UI Widget to spawn */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UStrategyUI> UIWidgetClass;

	/** The top-most refusal line (see URefusalWidget). Null until BeginPlay, and stays null if no class is set. */
	UPROPERTY()
	TObjectPtr<URefusalWidget> RefusalWidget;

	/** Type of refusal widget to spawn. Leaving this unset costs the refusal messages and nothing else. */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<URefusalWidget> RefusalWidgetClass;

	/**
	 *  Z-order for the refusal layer. Every window in the game goes up at 0, so this has to beat
	 *  that - and it is deliberately far clear of it rather than 1, since anything else added
	 *  later (a confirmation prompt, a tooltip layer) should still be able to sit between the
	 *  windows and this.
	 */
	UPROPERTY(EditAnywhere, Category="UI")
	int32 RefusalZOrder = 100;

	/** The right-click action menu's layer (see UActionMenuWidget). Null until BeginPlay, and stays null if no class is set. */
	UPROPERTY()
	TObjectPtr<UActionMenuWidget> ActionMenuWidget;

	/** Type of action menu widget to spawn - WBP_ActionMenu. Leaving this unset costs the right-click menu and nothing else. */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UActionMenuWidget> ActionMenuWidgetClass;

	/**
	 *  Z-order for the action menu's layer: above every window at 0, so a menu opened beside an
	 *  open inventory doesn't draw behind it, and below the refusal line at 100, which left room for
	 *  exactly this.
	 */
	UPROPERTY(EditAnywhere, Category="UI")
	int32 ActionMenuZOrder = 50;

	/** Warned once when a right-click wanted the menu and ActionMenuWidgetClass was empty */
	bool bWarnedNoActionMenu = false;

	/** If true, the HUD will draw the selection box */
	bool bDrawBox = false;

	/** Starting coords of the selection box */
	FVector2D BoxStart;

	/** Width and height of the selection box */
	FVector2D BoxSize;

	/** Current position of the selection box */
	FVector2D BoxCurrentPosition;

	/** Color of the selection box */
	UPROPERTY(EditAnywhere, Category="UI")
	FLinearColor SelectionBoxColor;

	/** The owning player's wallet, resolved once and held. DrawHUD reads the balance every frame,
	 *  and the player state it hangs off can replicate in well after this HUD exists, so the
	 *  lookup re-runs only while the pointer is still null. */
	TWeakObjectPtr<UWalletComponent> CachedWallet;

	/** The session's pace component, resolved once and held. Same shape as CachedWallet and for
	 *  the same reason: the GameState replicates in late on a client, so a miss early on is
	 *  normal and simply retries next frame rather than being cached as a negative. */
	TWeakObjectPtr<UTimePaceComponent> CachedTimePace;

public:

	/** Initialization */
	virtual void BeginPlay() override;

	/** Updates the drag selection box */
	void DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw);

	/**
	 *  The top-most refusal layer, or null before BeginPlay has created it (and on a dedicated
	 *  server, which never creates one at all).
	 *
	 *  Exposed so anything that refuses something can reach it - a refusal is raised wherever the
	 *  rule lives, which is rarely the HUD. `URefusalWidget::RaiseRefusal` is the front door;
	 *  this accessor is how it gets here.
	 */
	URefusalWidget* GetRefusalWidget() const { return RefusalWidget; }

	/**
	 *  Expands or collapses the activity feed.
	 *
	 *  The `L` key is bound on the player controller, and the feed is a region inside this HUD's
	 *  widget - so the controller asks the HUD, which forwards to the root, which forwards to the
	 *  region. No new interface: `smores` already depends on SmoresUI, and the traffic runs that
	 *  way round. IStrategyHUDCommands exists for the opposite direction.
	 */
	void ToggleActivityFeed();

	/**
	 *  Floats a bark over the person who said it.
	 *
	 *  Called by the player controller's Client_NotifyBark with the line already looked up in this
	 *  machine's language - the same controller-to-HUD direction as ToggleActivityFeed, so no
	 *  interface. The feed gets the line separately; this is only the moment of it.
	 */
	void ShowBarkBubble(const AActor* Speaker, const FText& Line);

	/** Opens the right-click menu on Target at the cursor. The controller's right-click comes here; the rows arrive with DrawHUD's next push. */
	void OpenActionMenu(AActor* Target);

	/** Closes the right-click menu if it is open */
	void CloseActionMenu();

	/** True while the right-click menu is showing */
	bool IsActionMenuOpen() const;

	/** What the right-click menu is about, or null when it's closed */
	AActor* GetActionMenuTarget() const;

	/**
	 *  True while the cursor is over something on screen that would take a click for itself - a
	 *  HUD region, a window, the open menu - rather than letting it reach the world.
	 *
	 *  The hover highlight asks this so it never lights up something a click couldn't reach. UMG has
	 *  no one-call answer, so this asks Slate for the widgets under the cursor and looks for one of
	 *  the kinds that swallow clicks. The refusal line and the bark bubbles are hit-test invisible,
	 *  so they correctly never count.
	 */
	bool IsCursorOverHUD() const;

protected:

	/** Draws the HUD */
	virtual void DrawHUD() override;

	/** The owning player's wallet component, looked up through its PlayerState and cached */
	UWalletComponent* GetWallet();

	/**
	 *  The session's simulation-pace component, looked up on the GameState and cached.
	 *
	 *  Found by component class rather than by casting to AStrategyGameState, because SmoresUI
	 *  cannot include anything from `smores` - and doesn't need to: AGameStateBase is an engine
	 *  type, exactly like the APlayerState the wallet hangs off.
	 */
	UTimePaceComponent* GetTimePace();
};
