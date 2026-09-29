// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "GameFramework/PlayerController.h"
#include "StrategySelectionHost.h"
#include "StrategyCameraCommands.h"
#include "StrategyHUDCommands.h"
#include "InventoryMoveHost.h"
#include "DialogHost.h"
#include "DialogTypes.h"
#include "ActivityEntry.h"
#include "ActionOrderHost.h"
#include "StrategyPlayerController.generated.h"

class AStrategyPawn;
class UInputMappingContext;
class UNiagaraSystem;
struct FInputActionValue;
class AStrategyHUD;
class UInputAction;
struct FInputActionInstance;
class AStrategyUnit;
class AStrategyPlayerUnit;
class UStrategyTouchControls;
class UWindowWidget;
class UInventoryWidget;
class UEquipmentWidget;
class UHUDPanelWidget;
class AStrategyContainer;
class AWorldItem;
class UInventoryComponent;
class UEquipmentComponent;
class AStrategyPlayerState;
class UWalletComponent;
class UTraderComponent;
class UTimePaceComponent;
class UWorldFactionComponent;
class UPlayerStandingComponent;
class USmoresActivityLog;
class USquadActivityWatcher;
class IInventoryHolder;
class UConversationComponent;
class UConversationWidget;
class UExamineWidget;
class UMaterialInterface;
class UMeshComponent;
class AWorldDoor;

/**
 *  Player Controller for a top-down strategy game.
 *  Handles unit selection and commands.
 *  Implements both mouse and touch controls.
 */
UCLASS(abstract)
class AStrategyPlayerController : public APlayerController, public IStrategySelectionHost, public IStrategyCameraCommands, public IStrategyHUDCommands, public IInventoryMoveHost, public IDialogHost, public IActionOrderHost
{
	GENERATED_BODY()

protected:

	/** Strategy Pawn associated with this controller */
	TObjectPtr<AStrategyPawn> ControlledCameraPawn;

	/** Strategy HUD associated with this controller */
	TObjectPtr<AStrategyHUD> StrategyHUD;

	/** Input mapping context to use with mouse input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* MouseMappingContext;

	/** Input mapping context to use with touch input */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* TouchMappingContext;

	/** If true, the player is adding or removing units to the selected units list */
	bool bSelectionModifier = false;

	/** If true, double-tap touch select all mode is active */
	bool bDoubleTapActive = false;

	/** If true, allow the player to interact with game objects */
	bool bAllowInteraction = true;

	/** Input Action for moving the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveCameraAction;

	/** Input Action for zooming the camera */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ZoomCameraAction;

	/** Input Action for resetting the camera to its default position */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetCameraAction;

	/** Input Action for raising/lowering the camera height */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AdjustHeightAction;

	/** Input Action for select click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAction;

	/** Input Action for additive select click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectClickAdditiveAction;

	/** Input Action for select all double click */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectAllDoubleClickAction;

	/** Input Action for select press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectHoldAction;

	/** Input Action for click interaction */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractClickAction;

	/** Input Action for interaction press and hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractHoldAction;

	/** Input Action for modifying selection mode */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SelectionModifierAction;

	/** Input Action for cycling selection between player-controlled pawns */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CyclePawnAction;

	/** Input Action for toggling the inventory screen for the selected pawn */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleInventoryAction;

	/** Input Action for opening/closing a nearby container's inventory screen */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleContainerAction;

	/** Input Action for attacking the currently-selected NPC */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AttackAction;

	/** Input Action for talking/trading with the currently-selected NPC - the keyboard route to
	 *  the double-click interact, reading the same SelectedNPC the attack key does */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TalkAction;

	/** Input Action for rotating the item currently being dragged in an inventory window */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* RotateDraggedItemAction;

	/** Input Action for the squad roster panel */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SquadPanelAction;

	/** Input Action for the world map panel */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MapPanelAction;

	/** Input Action for the research panel */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResearchPanelAction;

	/** Input Action for the keybind/help panel */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HelpPanelAction;

	/** Input Action for toggling pause. Implemented by UTimePaceComponent - pause is the bottom
	 *  rung of the same dilation ladder as the speed keys, not a separate mechanism. */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TogglePauseAction;

	/** Input Action for stepping the pace ladder one tier slower (Slice 2) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PaceSlowerAction;

	/** Input Action for stepping the pace ladder one tier faster (Slice 2) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PaceFasterAction;

	/** Input Action for expanding/collapsing the activity feed (Slice 3) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleActivityFeedAction;

	/**
	 *  Mapping context added only while an inventory or container window is open, at a higher
	 *  priority than the gameplay context. Scoping it that way is what lets an inventory key
	 *  reuse a key that means something else in the world, and it keeps every inventory binding
	 *  player-rebindable alongside the rest of them.
	 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* InventoryMappingContext;

	/** Input Action for primary touch hold */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchPrimaryHoldAction;

	/** Input Action for secondary touch */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TouchSecondaryAction;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UStrategyTouchControls> MobileControlsWidget;

	/** Touch controls widget class to spawn on mobile platforms */
	UPROPERTY(EditAnywhere, Category="Input")
	TSubclassOf<UStrategyTouchControls> MobileControlsWidgetClass;

	/** If true, the PC will be initialized with touchscreen controls */
	UPROPERTY(EditAnywhere, Category="Input")
	bool bForceTouchControls = false;

	/** Max distance to look for nearby units when doing a click or touch interaction */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float SelectionRadius = 250.0f;

	/** Max distance to look for a nearby container (or door) on a single click or touch */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float ContainerSelectionRadius = 250.0f;

	/** Max distance to look for a loose world item on a single click. Deliberately tighter than
	 *  ContainerSelectionRadius: a dropped item is a small thing to aim at, and a generous radius
	 *  would let one lying near a chest swallow every click meant for the chest. (The double-click
	 *  and the right-click use the hover's exact resolver instead - see HoverPickRadius.) */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float WorldItemSelectionRadius = 100.0f;

	/**
	 *  How far from the point under the cursor the hover still catches something the cursor isn't
	 *  quite on - a body lying flat, a dropped item. Measured across the ground, ignoring height.
	 *
	 *  **Deliberately small**, and the whole reason the right-click menu and move orders don't
	 *  fight: the thing under the cursor is found exactly first (see ResolveInteractableUnderCursor),
	 *  and this is only the fallback. The double-click ladder used to use ContainerSelectionRadius
	 *  (250) here, which made the ground beside a person impossible to right-click.
	 */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float HoverPickRadius = 40.0f;

	/**
	 *  Drawn over whatever the cursor is on, so the player knows before clicking that a right-click
	 *  will open the menu rather than move the squad there. An overlay material, so it adds a rim on
	 *  top of the thing's own look - which must stay readably different from the *selected* look
	 *  (units' BP_UnitSelected, containers' dark-green material swap). Local and cosmetic: in co-op
	 *  every player lights their own. Unset draws nothing, and the menu still works.
	 */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UMaterialInterface> HoverOverlayMaterial;

	/** Cached starting position for camera drag scrolling */
	FVector2D StartingDragScrollPosition;

	/** Cached starting position for box select */
	FVector2D StartingBoxSelectionPosition;

	/** Last game time when a drag scroll was performed, so we can avoid spamming commands on drag scroll end */
	float LastTouchDragScrollTime = 0.0f;

	/** Time the finger needs to be held down to initiate a drag scroll */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 1, Units = "s"))
	float TouchDragScrollHoldTime = 0.15f;

	/** Current camera zoom level */
	float CameraZoom = 0.0f;

	/** Default camera zoom level */
	float DefaultZoom = 1000.0f;

	/** Minimum allowed camera zoom level */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MinZoomLevel = 1000.0f;

	/** Maximum allowed camera zoom level */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MaxZoomLevel = 2500.0f;

	/** Scales zoom inputs by this value */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 1000))
	float ZoomScaling = 100.0f;

	/** Affects how fast the camera moves while dragging with the mouse */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float DragMultiplier = 0.1f;

	/** Minimum allowed camera pitch (steepest / closest to top-down) */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = -90, ClampMax = 0))
	float MinCameraPitch = -89.0f;

	/** Maximum allowed camera pitch (flattest / closest to horizontal) */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = -90, ClampMax = 0))
	float MaxCameraPitch = -5.0f;

	/** Default camera pitch, used on possess and by the camera reset command */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = -90, ClampMax = 0))
	float DefaultCameraPitch = -50.0f;

	/** Degrees of yaw applied per pixel of mouse delta while rotating the camera */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10))
	float CameraYawSpeed = 0.8f;

	/** Degrees of pitch applied per pixel of mouse delta while rotating the camera */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10))
	float CameraPitchSpeed = 0.8f;

	/** Default camera height, used on possess and by the camera reset command */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float DefaultCameraHeight = 1500.0f;

	/** Minimum allowed camera height */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MinCameraHeight = 800.0f;

	/** Maximum allowed camera height */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 10000))
	float MaxCameraHeight = 3000.0f;

	/** Scales height adjustment inputs by this value */
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = 0, ClampMax = 1000))
	float HeightScaling = 50.0f;

	/** Default camera yaw, captured from the pawn's resting orientation on possess and used by the camera reset command */
	float DefaultCameraYaw = 0.0f;

	/** Current camera height */
	float CameraHeight = 0.0f;

	/** True while MMB is held and PlayerTick should be sampling mouse delta to rotate the camera */
	bool bIsRotatingCamera = false;

	/** Number of ticks to discard mouse delta for at the start of every rotate drag, instead of
	 *  rotating, so the camera's perspective holds steady rather than jumping. Covers not just the
	 *  just-issued centering warp (which can be misread as real mouse movement before the OS/Slate
	 *  has caught up) but also the extra frame or two mouse-capture engagement itself can take on
	 *  the very first drag of a PIE session, which otherwise surfaces as one large spurious delta */
	static constexpr int32 RotateStartupSkipTicks = 3;

	/** Ticks remaining in the current rotate drag's startup skip window; see RotateStartupSkipTicks */
	int32 RotateStartupSkipTicksRemaining = 0;

	/** Trace channel to use for selection trace checks */
	UPROPERTY(EditAnywhere, Category = "Selection")
	TEnumAsByte<ETraceTypeQuery> SelectionTraceChannel;

	/** Currently selected unit list */
	TArray<AStrategyUnit*> ControlledUnits;

	/** The container the player has explicitly picked, used to disambiguate when several are in range */
	UPROPERTY()
	TObjectPtr<AStrategyContainer> SelectedContainer;

	/** The NPC the player has clicked to target. Highlight-only, like SelectedContainer - never added to ControlledUnits */
	UPROPERTY()
	TObjectPtr<AStrategyUnit> SelectedNPC;

	/** Whichever pawn, NPC, container, door or loose item was most recently selected/targeted. Drives the target panel. */
	TWeakObjectPtr<AActor> LastSelectionTarget;

	/** What the hover highlight is currently drawn on, if anything. Local only - see HoverOverlayMaterial. */
	TWeakObjectPtr<AActor> HoveredActor;

	/** The examine window's class - WBP_Examine. Without it Examine does nothing, and says so once. */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UExamineWidget> ExamineWidgetClass;

	/** The examine window, once spawned. Kept after it closes, like the panel windows. */
	UPROPERTY()
	TObjectPtr<UExamineWidget> ExamineWidget;

	/** Warned once when Examine was picked and ExamineWidgetClass was empty */
	bool bWarnedNoExamineWindow = false;

	/** Inventory screen widget class to spawn when the player opens an inventory */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	/** Active inventory screen widget, if one is open */
	UPROPERTY()
	TObjectPtr<UInventoryWidget> InventoryWidget;

	/** Screen widget class to spawn when the player opens a container (chest, barrel, etc.) */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UInventoryWidget> ContainerWidgetClass;

	/** Active container screen widget, if one is open */
	UPROPERTY()
	TObjectPtr<UInventoryWidget> ContainerWidget;

	/** Paperdoll screen widget class, spawned alongside a pawn's inventory window */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UEquipmentWidget> EquipmentWidgetClass;

	/** Active paperdoll widget, if one is open. Opens and closes with the pawn inventory window it belongs to. */
	UPROPERTY()
	TObjectPtr<UEquipmentWidget> EquipmentWidget;

	/** The conversation window's class - WBP_Conversation. Without it a conversation plays into the feed with no window, and says so once. */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UConversationWidget> ConversationWidgetClass;

	/** The conversation window, once spawned. Kept after it closes, like the panel windows. */
	UPROPERTY()
	TObjectPtr<UConversationWidget> ConversationWidget;

	/**
	 *  This player's conversations - the server runs them, the client shows them. On the controller
	 *  because a client can only send an RPC through an actor it owns. See UConversationComponent.
	 */
	UPROPERTY(VisibleAnywhere, Category="Dialog")
	TObjectPtr<UConversationComponent> Conversation;

	/** Warned once when a conversation opened and ConversationWidgetClass was empty */
	bool bWarnedNoConversationWindow = false;

	/**
	 *  Window class for each nav-rail panel. A map rather than one TSubclassOf per panel so that
	 *  adding a panel later is an entry here plus a value on EHUDPanel, not another pair of
	 *  properties to remember to wire.
	 *
	 *  EHUDPanel::Inventory is deliberately *not* in here - the inventory has its own window path
	 *  that predates the rail, and RequestPanel routes to it instead.
	 */
	UPROPERTY(EditAnywhere, Category="UI")
	TMap<EHUDPanel, TSubclassOf<UHUDPanelWidget>> PanelWidgetClasses;

	/** Panel windows spawned so far, keyed by panel. An entry survives the window being closed,
	 *  so re-opening a panel reuses it rather than building a fresh one each time. */
	UPROPERTY()
	TMap<EHUDPanel, TObjectPtr<UHUDPanelWidget>> PanelWidgets;

	/** World pickup class spawned when an item leaves a grid for the ground. Only the drop debug
	 *  exec uses it today; the drag-an-item-onto-the-world gesture is a later slice. */
	UPROPERTY(EditAnywhere, Category="World Item")
	TSubclassOf<AWorldItem> WorldItemClass;

	/** All player-controllable pawns in the level. Rebuilt on demand by RefreshPlayerPawns() */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStrategyPlayerUnit>> PlayerPawns;

	/** Index into PlayerPawns of the pawn most recently cycled to. INDEX_NONE until the first cycle */
	int32 CurrentPlayerPawnIndex = INDEX_NONE;

	/**
	 *  How often the squad bar's roster is rebuilt from the world.
	 *
	 *  The HUD asks for the roster every frame, and answering honestly means
	 *  GetAllActorsOfClass over the whole level plus a sort - which is fine a few times a second
	 *  and wasteful sixty times a second. A squad gains or loses a member rarely enough that half
	 *  a second of staleness is invisible.
	 */
	UPROPERTY(EditAnywhere, Category = "UI", meta = (ClampMin = 0, Units = "s"))
	float RosterRefreshIntervalSeconds = 0.5f;

	/** Real time the roster was last rebuilt for the squad bar. Real, not world, so a paused game
	 *  still keeps its roster current. */
	double LastRosterRefreshTime = -1.0;

	/**
	 *  One watcher per unit whose health this player's feed reports on, keyed by that unit.
	 *
	 *  A map rather than an array because the set is rebuilt against the world periodically and
	 *  the interesting question each time is "is this one already watched" - see
	 *  RefreshActivityWatchers. USquadActivityWatcher's own comment explains why there is one per
	 *  unit rather than four handlers here.
	 */
	UPROPERTY()
	TMap<TObjectPtr<AStrategyUnit>, TObjectPtr<USquadActivityWatcher>> ActivityWatchers;

	/**
	 *  How long the same refusal is treated as one event by the activity feed.
	 *
	 *  URefusalWidget does its own repeat suppression so that leaning on a key doesn't
	 *  machine-gun the refusal sound; the feed needs the same rule for a different reason. A
	 *  player holding `O` out of range would otherwise push every other line out of a
	 *  fixed-capacity record with sixty copies of "Too far away", which is the one way this feed
	 *  can actively lose information rather than merely repeat itself.
	 *
	 *  Two seconds, matching URefusalWidget::RefusalDisplaySeconds: while the same refusal is
	 *  still on screen, it is still the same refusal.
	 */
	UPROPERTY(EditAnywhere, Category = "UI", meta = (ClampMin = 0, Units = "s"))
	float FeedRefusalRepeatSeconds = 2.0f;

	/** The refusal last written to the feed, for the suppression above */
	ESmoresRefusalReason LastFeedRefusal = ESmoresRefusalReason::None;

	/** FPlatformTime::Seconds() when it was written. Negative until the first refusal. */
	double LastFeedRefusalTime = -1.0;

public:

	/** Constructor */
	AStrategyPlayerController();

	/** BeginPlay initialization */
	virtual void BeginPlay() override;

	/** Initialize input bindings */
	virtual void SetupInputComponent() override;

	/** Pawn initialization */
	virtual void OnPossess(APawn* InPawn);

	/** Per-frame update - drives camera rotation directly rather than relying on Enhanced
	 *  Input's own per-frame callback cadence for a held button, which isn't guaranteed to
	 *  fire every single tick */
	virtual void PlayerTick(float DeltaTime) override;

public:

	//~ Begin IStrategySelectionHost interface

	/** Updates selected units from the HUD's drag select box */
	virtual void DragSelectUnits(const TArray<AStrategyUnit*>& Units) override;

	/** Passes the list of selected units */
	virtual const TArray<AStrategyUnit*>& GetSelectedUnits() override;

	/** Everything the target panel draws about whichever pawn, NPC, or container was most recently
	 *  selected, or a struct with an empty name if none */
	virtual FStrategyTargetInfo GetSelectionTargetInfo() const override;

	/** The same, for any target - the right-click menu's, which is not necessarily the panel's */
	virtual FStrategyTargetInfo GetTargetInfoFor(const AActor* Target) const override;

	/** This player's own squad, in the Tab cycle's deterministic order. Drives the squad portrait bar. */
	virtual TArray<AStrategyUnit*> GetControlledPlayerUnits() override;

	//~ End IStrategySelectionHost interface

	/**
	 *  This player's whole squad, selected or not, read off the world rather than PlayerPawns - so it
	 *  answers correctly on the server, which never refreshes that list. What the action rules take
	 *  as "Squad" (see FStrategyTargetActions).
	 */
	void GetSquadUnits(TArray<AStrategyUnit*>& OutSquad) const;

	//~ Begin IStrategyCameraCommands interface

	/** Returns the default camera zoom percentage value */
	virtual float GetDefaultZoomPercentage() const override;

	//~ End IStrategyCameraCommands interface (remaining members below, alongside the other camera commands)

	//~ Begin IStrategyHUDCommands interface

	/** Opens the named panel, or closes it if it's already open. The nav rail button and the
	 *  panel's key both arrive here, so neither can drift from the other. */
	virtual void RequestPanel(EHUDPanel Panel) override;

	/** True while the named panel's window is on screen */
	virtual bool IsPanelOpen(EHUDPanel Panel) const override;

	/** Asks the server to run the simulation at the given tier. The pace strip's buttons and the
	 *  Space / - / = keys all arrive here. */
	virtual void RequestPace(EGamePace Pace) override;

	/** Selects the given unit alone, optionally cutting the camera to it. The squad portrait bar's
	 *  click and its focus gesture. */
	virtual void RequestSelectUnit(AStrategyUnit* Unit, bool bFocusCamera) override;

	/**
	 *  Runs an action on Target by id, re-checking the same rules that offered it. The target panel,
	 *  the right-click menu, the `T` and `O` keys and the double-click all end up here, so there is
	 *  one behaviour: Examine opens its window, Attack sends the squad, and everything else becomes
	 *  an order for one squad member to walk over and do it.
	 */
	virtual void RequestTargetAction(AActor* Target, FName ActionId) override;

	//~ End IStrategyHUDCommands interface

	/** This controller's player state, or null if it hasn't replicated in yet */
	AStrategyPlayerState* GetStrategyPlayerState() const;

	/** The session's simulation-pace component, or null if the GameState hasn't replicated in yet.
	 *  Shared world state living on the GameState - there is one of it per session, not per player. */
	UTimePaceComponent* GetTimePace() const;

	/** This controller's player's wallet, or null if the player state hasn't replicated in yet.
	 *  Keyed off this controller's own player state - there is no global balance. */
	UWalletComponent* GetWallet() const;

	/** The session's faction records, or null if the GameState hasn't replicated in yet. Session-wide, like the pace. */
	UWorldFactionComponent* GetWorldFactions() const;

	/** This controller's player's faction standing, or null if the player state hasn't replicated in yet */
	UPlayerStandingComponent* GetPlayerStanding() const;

protected:

	/** Returns true if the PC should run using touchscreen controls */
	bool ShouldUseTouchControls() const;

	// mouse + keyboard input

protected:

	/** Moves the camera by the given input */
	void MoveCamera(const FInputActionValue& Value);

	/** Changes the camera zoom level by the given input */
	void ZoomCamera(const FInputActionValue& Value);

	/** Resets the camera to its initial value */
	void ResetCamera(const FInputActionValue& Value);

	/** Raises or lowers the camera by the given input */
	void AdjustHeight(const FInputActionValue& Value);

	/** Rebuilds PlayerPawns from the world with deterministic ordering */
	void RefreshPlayerPawns();

	/**
	 *  Rebuilds the set of units whose health events reach this player's activity feed - the whole
	 *  squad, plus every other unit in the level so a fight has two sides in the record.
	 *
	 *  Called from BeginPlay and from the roster's periodic refresh rather than on a subscription,
	 *  because a unit spawned mid-session announces itself to nobody. The cost is a map lookup per
	 *  unit on the frames the roster refreshes.
	 */
	void RefreshActivityWatchers();

	/** Moves the camera pawn over the given unit without changing its height or rotation. The
	 *  portrait bar's focus gesture; a hard cut, which is what player-interface.md asks for. */
	void FocusCameraOnUnit(const AStrategyUnit* Unit);

	/** Replaces the current selection with the next player-controlled pawn */
	void CyclePawn(const FInputActionValue& Value);

	/** Toggles the inventory screen for the selected pawn (requires exactly one selected player pawn) */
	void ToggleInventory(const FInputActionValue& Value);

	/** The inventory key's behaviour without the input plumbing, so the nav rail's Inventory
	 *  button can run exactly the same path rather than a lookalike of it. */
	void ToggleInventoryPanel();

	/** Opens the given nav-rail panel's window, spawning it on first use. RequestPanel decides
	 *  whether opening is what was meant; this just does it. */
	void OpenPanel(EHUDPanel Panel);

	/** Opens or closes the squad roster panel */
	void SquadPanelKeyPressed(const FInputActionValue& Value);

	/** Opens or closes the world map panel */
	void MapPanelKeyPressed(const FInputActionValue& Value);

	/** Opens or closes the research panel */
	void ResearchPanelKeyPressed(const FInputActionValue& Value);

	/** Opens or closes the keybind/help panel */
	void HelpPanelKeyPressed(const FInputActionValue& Value);

	/** Freezes the simulation, or returns it to whatever speed it was running at */
	void TogglePauseKeyPressed(const FInputActionValue& Value);

	/** Steps the pace ladder one tier slower */
	void PaceSlowerKeyPressed(const FInputActionValue& Value);

	/** Steps the pace ladder one tier faster */
	void PaceFasterKeyPressed(const FInputActionValue& Value);

	/** Asks for the tier Steps rungs from the current one. The shared body of the two keys above. */
	void RequestPaceStep(int32 Steps);

	/** Expands or collapses the activity feed */
	void ToggleActivityFeedKeyPressed(const FInputActionValue& Value);

	/** Closes the inventory screen if one is open */
	void CloseInventory();

	/**
	 *  Opens (or rebinds) the pawn inventory screen for the given pawn, spawning the widget on
	 *  first use. bOpenEquipment brings the paperdoll up alongside it, which only the inventory
	 *  key does - a pack opened as a transfer partner for a chest or a corpse comes on its own,
	 *  and closes any paperdoll already up, since this call may have just rebound the window to a
	 *  different pawn than the one that paperdoll belongs to.
	 */
	void OpenInventoryForPawn(AStrategyPlayerUnit* PlayerUnit, bool bOpenEquipment);

	/** Opens (or rebinds) the paperdoll window for the given pawn, spawning the widget on first use.
	 *  Reached only from the inventory key's OpenInventoryForPawn - see that method. */
	void OpenEquipmentForPawn(AStrategyPlayerUnit* PlayerUnit);

	/** Closes the paperdoll window if one is open */
	void CloseEquipment();

	/** Bound to every window this controller opens. A window's own close button removes only that
	 *  window, so this is what closes its companions and re-scopes the inventory input context. */
	UFUNCTION()
	void HandleWindowClosed(UWindowWidget* Window);

	/** Opens the inventory screen for a nearby container, or closes it if already open */
	void ToggleContainer(const FInputActionValue& Value);

	/** Closes the container screen if one is open */
	void CloseContainer();

	/**
	 *  Opens the given container's inventory screen, spawning the widget on first use, with
	 *  PackOwner's pack alongside - the squad member who walked over to open it. Null falls back to
	 *  whichever of this player's pawns is nearest.
	 */
	void OpenContainer(AStrategyContainer* Container, AStrategyPlayerUnit* PackOwner = nullptr);

	/** Opens the given lootable NPC's inventory screen, with PackOwner's pack alongside - see
	 *  OpenContainer. Downed and Dead bodies take the identical path - see IsLootableNPC. */
	void OpenLoot(AStrategyUnit* LootTarget, AStrategyPlayerUnit* PackOwner = nullptr);

	/** Opens the given trader's wares alongside PackOwner's pack (else the nearest pawn's), both
	 *  windows priced. Reuses the same ContainerWidget a chest and a corpse use - a shop shelf is a
	 *  grid like any other, and only the title and the prices differ. */
	void OpenTrade(AStrategyUnit* TraderUnit, UTraderComponent* Stock, AStrategyPlayerUnit* PackOwner = nullptr);

	/** Opens (or rebinds) the Examine window on Target - its name, what it is, and what the squad can see of it, in words. Local only. */
	void OpenExamine(AActor* Target);

	/** Opens or closes the conversation window to match what UConversationComponent's view says. Owning client. */
	void HandleConversationViewChanged();

	/** Attacks the currently-selected NPC if it's Passive (flips it to Aggressive); no-op otherwise */
	void AttackKeyPressed(const FInputActionValue& Value);

	/** Talks to the currently-targeted NPC - sends someone over if nobody is close. The same path as the panel's Talk. */
	void TalkKeyPressed(const FInputActionValue& Value);

	/** Rotates the inventory item currently being dragged, if there is one. Purely local UI state -
	 *  the orientation only reaches the server on drop, through Server_MoveInventoryItem. */
	void RotateDraggedItem(const FInputActionValue& Value);

	/** Adds InventoryMappingContext while any inventory window is open, and removes it once none are */
	void UpdateInventoryInputContext();

	/** Start a select and hold input */
	void SelectHoldStarted(const FInputActionValue& Value);
	
	/** Select and hold input triggered */
	void SelectHoldTriggered(const FInputActionValue& Value);

	/** Select and hold input completed */
	void SelectHoldCompleted(const FInputActionValue& Value);

	/** Select click action */
	void SelectClick(const FInputActionValue& Value);

	/** Select Click Additive Action */
	void SelectClickAdditive(const FInputActionValue& Value);

	/** Select All Double Click Action */
	void SelectAllDoubleClick(const FInputActionValue& Value);

	/** Starts an interaction hold input */
	void InteractHoldStarted(const FInputActionValue& Value);

	/** Interaction hold input completed or canceled */
	void InteractHoldCompleted(const FInputActionValue& Value);

	/** Right-click released: over something, target it and open the action menu; over empty ground, a move order */
	void InteractClick(const FInputActionValue& Value);

	/** Touch primary finger hold started */
	void TouchPrimaryHoldStarted(const FInputActionValue& Value);

	/** Touch primary finger hold triggered */
	void TouchPrimaryHoldTriggered(const FInputActionInstance& Instance);

	/** Touch primary finger hold completed */
	void TouchPrimaryHoldCompleted(const FInputActionValue& Value);

	/** Touch secondary finger triggered */
	void TouchSecondaryTriggered(const FInputActionValue& Value);

	/** Touch secondary finger completed */
	void TouchSecondaryCompleted(const FInputActionValue& Value);

	// commands

public:

	/** Attempt to select or deselect a unit near the given location. Supports additive selection modifier. */
	bool DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection);

	/** Attempts to select all units on screen */
	void DoSelectAllUnitsOnScreenCommand();

	/** Deselects any selected units */
	void DoDeselectAllUnitsCommand();

	/** Toggles between selecting all units on screen and deselecting units */
	virtual void DoToggleSelectAllUnitsCommand() override;

	/** Scrolls the camera based on a new screen coordinate */
	void DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition);

	/** Attempts to move all selected units to the given location. Client-side entry point - forwards to Server_MoveUnits. */
	void DoMoveUnitsCommand(const FVector& GoalLocation);

	/** Flips Target Aggressive (harmless if already Aggressive) and sends every controlled unit to engage it. Client-side entry point - forwards to Server_AttackCommand. */
	void DoAttackCommand(AStrategyUnit* Target);

	/** Applies a zoom change to the camera */
	void DoCameraModifyZoomCommand(float ZoomDelta);

	/** Resets the camera zoom to default */
	virtual void DoCameraResetZoomCommand() override;

	/** Sets the camera zoom to a percentage between min and max zoom */
	virtual void DoCameraSetZoomPercentageCommand(float Percentage) override;

	/** Rotates the camera by the given mouse delta, clamping pitch */
	void DoCameraRotateCommand(const FVector2D& MouseDelta);

	/** Applies a height change to the camera */
	void DoCameraModifyHeightCommand(float HeightDelta);

	/** Resets the camera height to default */
	void DoCameraResetHeightCommand();

	/** Resets the camera rotation to default */
	void DoCameraResetRotationCommand();

private:

	/** Server-side implementation of DoMoveUnitsCommand - ControlledUnits only exists locally on the
	 *  owning client's PlayerController instance, so the units/goal/closest-unit have to travel
	 *  explicitly over the RPC rather than being re-read from this instance server-side. */
	UFUNCTION(Server, Reliable)
	void Server_MoveUnits(const TArray<AStrategyUnit*>& Units, const FVector& GoalLocation, AStrategyUnit* ClosestUnit);

	/** Server-side implementation of DoAttackCommand - see Server_MoveUnits for why Units travels explicitly */
	UFUNCTION(Server, Reliable)
	void Server_AttackCommand(const TArray<AStrategyUnit*>& Units, AStrategyUnit* Target);

	/**
	 *  Server-side implementation of RequestPace.
	 *
	 *  The pace lives on the GameState, which a client cannot RPC because it doesn't own it - but
	 *  it does own this controller, so the ask travels here and the server hands it to
	 *  UTimePaceComponent::SetPace. Nothing is gated here: SetPace owns the one rule, which is that
	 *  a networked session stays at 1x whoever asks (game-systems/hud-and-panels.md).
	 */
	UFUNCTION(Server, Reliable)
	void Server_RequestPace(EGamePace Pace);

public:

	//~ Begin IInventoryMoveHost interface

	/** Server-side entry point for an inventory drag-drop move (see UInventoryWidget::NativeOnDrop -
	 *  the widget can't mutate inventory contents directly, since UInventoryComponent's mutators are
	 *  authority-only). Just forwards to UInventoryComponent::MoveItem, which does all the validation. */
	UFUNCTION(Server, Reliable)
	virtual void Server_MoveInventoryItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity) override;

	/** Server-side entry point for wearing a carried item (right-click on an item widget, or a drop on a
	 *  paperdoll slot). Forwards to UEquipmentComponent::Equip, which does all the validation. */
	UFUNCTION(Server, Reliable)
	virtual void Server_EquipItem(UInventoryComponent* SourceInventory, int32 EntryId, UEquipmentComponent* Equipment, EEquipSlot Slot) override;

	/** Server-side entry point for taking a worn item off (right-click on a paperdoll slot) */
	UFUNCTION(Server, Reliable)
	virtual void Server_UnequipItem(UEquipmentComponent* Equipment, EEquipSlot Slot, UInventoryComponent* DestInventory) override;

	/**
	 *  Server-side entry point for repacking one holder's grid (an inventory window's sort
	 *  buttons). Forwards to UInventoryComponent::SortEntries, which does all the work.
	 *
	 *  Deliberately ungated beyond authority, unlike PickUpWorldItem and TryTradeItem: a
	 *  sort can only ever rearrange one holder's own contents, so there is nothing for a bad
	 *  request to take. Proximity is already the gate on the window being open at all.
	 */
	UFUNCTION(Server, Reliable)
	virtual void Server_SortInventory(UInventoryComponent* Inventory, EInventorySortCriterion Criterion) override;

	//~ End IInventoryMoveHost interface

	/**
	 *  Puts a refusal on this player's own HUD ("Too far away", "Not enough gold").
	 *
	 *  Local and client-side: call it directly from anything that decides on this machine, which
	 *  is most refusals - reach, hostility and a full grid are all things the client can work out
	 *  before it asks the server anything. On a listen server this is also what
	 *  Client_NotifyRefusal ends up calling.
	 *
	 *  Does nothing without a HUD, which is the correct behaviour on a dedicated server: server
	 *  code must route through Client_NotifyRefusal instead, or the player is never told.
	 */
	void NotifyRefusal(ESmoresRefusalReason Reason);

	/**
	 *  Server -> owning client: why a server-side check refused something.
	 *
	 *  Only for refusals the client could not have predicted. Most can be: the drop preview
	 *  already turns cells red before the player lets go, and that is a better answer than this
	 *  one, because the gesture never completes. This carries the rest - a price the client
	 *  hasn't priced, a range re-check against a pawn that has moved, a repack that couldn't fit.
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyRefusal(ESmoresRefusalReason Reason);

	/**
	 *  Records a line in this player's activity feed.
	 *
	 *  Local and client-side, exactly like NotifyRefusal, and safe to call from anywhere: it does
	 *  nothing on a controller with no local player, which is the correct behaviour on a
	 *  dedicated server and for any other player's controller.
	 */
	void PostActivity(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source = FText::GetEmpty());

	/**
	 *  Server -> owning client: a line for the feed that only the server knew about.
	 *
	 *  The counterpart to Client_NotifyRefusal, and needed for the same reason: a purchase and a
	 *  pickup are both resolved on the server, so the client has no way of knowing what actually
	 *  moved or what it cost. The text is worded server-side because that is where the numbers
	 *  are; FText replicates.
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyActivity(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source);

	//~ Begin IDialogHost interface

	/** True if any unit this controller owns is within Range of Location. Server-side. */
	virtual bool IsSquadMemberWithin(const FVector& Location, float Range) const override;

	/** A bark this player's squad could hear - forwards the id and the speaker to Client_NotifyBark */
	virtual void DeliverBark(FName LineId, AActor* Speaker, const FText& SpeakerName) override;

	/** The OpenTrade effect: re-checks Trader keeps a shop and may be dealt with, then Client_OpenTrade and a TradeOpened bark. Server-side. */
	virtual bool OpenTradeWith(AActor* Trader) override;

	/** A conversation's effect was refused - forwards to Client_NotifyRefusal */
	virtual void NotifyDialogRefusal(ESmoresRefusalReason Reason) override;

	//~ End IDialogHost interface

	/** The conversation component. Never null - it's a default subobject. */
	UConversationComponent* GetConversation() const { return Conversation; }

	/** Server -> owning client: open this trader's shop, with PackOwner's pack alongside - the server
	 *  decided Talk means trade, a conversation ran OpenTrade, or a squad member walked over to Trade */
	UFUNCTION(Client, Reliable)
	void Client_OpenTrade(AStrategyUnit* TraderUnit, AStrategyPlayerUnit* PackOwner);

	/**
	 *  Server -> owning client: a squad member has walked up to Holder to loot it - open its window,
	 *  with that squad member's own pack alongside rather than whichever pawn happens to be nearest.
	 *  A container or a body; each opens the window it always did.
	 */
	UFUNCTION(Client, Reliable)
	void Client_OpenHolder(AActor* Holder, AStrategyPlayerUnit* PackOwner);

	/**
	 *  Client -> server: send Actor over to do ActionId to Target. The server re-checks that Actor is
	 *  this player's own and able, and that the action is on offer for them, rather than trusting
	 *  the menu - the same stance RequestSelectUnit takes - then hands it to the unit's
	 *  UActionOrderComponent. A refused request says why (Client_NotifyRefusal).
	 */
	UFUNCTION(Server, Reliable)
	void Server_RequestActionOrder(AStrategyUnit* Actor, AActor* Target, FName ActionId);

	//~ Begin IActionOrderHost interface

	/** The arrival re-check, through FStrategyTargetActions::FindActionFor - the rules the menu drew from */
	virtual bool CanPerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId, ESmoresRefusalReason& OutReason) override;

	/** Carries out an action a squad member has walked over to do. Server-side; windows open on the owning client. */
	virtual void PerformAction(AStrategyUnit* Actor, AActor* Target, FName ActionId) override;

	/** Tells this player why an order ended without acting - a refusal, or a line in the feed */
	virtual void HandleActionOrderEnded(AStrategyUnit* Actor, AActor* Target, const FText& TargetName, FName ActionId, EActionOrderEnd Why, ESmoresRefusalReason Reason) override;

	//~ End IActionOrderHost interface

	/**
	 *  Server -> owning client: somebody within earshot of this player's squad said a line.
	 *
	 *  **The line travels as its id, never its text** - the dialog roadmap's settled rule. The
	 *  client looks it up in its own loaded, translated library, so in co-op each player reads the
	 *  same bark in their own language. A client missing the id (host and client running different
	 *  dialog) logs it and posts nothing. Reliable, like Client_NotifyActivity: the director's quiet
	 *  time keeps the rate low, and NothingToSay is the only answer a player gets to talking to
	 *  someone.
	 *
	 *  The line goes to the feed (the record) and floats over Speaker (the event - see
	 *  UBarkBubbleLayerWidget). A speaker this client can't resolve - not relevant to it, so it
	 *  arrives null - gets the feed line and no bubble.
	 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyBark(FName LineId, AActor* Speaker, const FText& SpeakerName);

protected:

	/**
	 *  What talking to NPC means, with Listener doing the talking - the Talk action's arrival.
	 *  Server-side. Re-checks that NPC may be dealt with, then: an eligible greeting opens a
	 *  conversation; else a trader opens trade and says a TradeOpened line; else the NPC says a
	 *  NothingToSay line. The server decides - selection reads standing and the squad's dialog
	 *  memory, which only the server holds.
	 */
	void StartTalk(AStrategyUnit* NPC, AStrategyUnit* Listener);

	/**
	 *  Opens Trader's shop for this player with Listener at the counter, and plays the TradeOpened
	 *  bark addressed to them. Server-side. The body of OpenTradeWith - kept apart from it so the
	 *  Trade action's arrival can name its own listener without changing IDialogHost's signature,
	 *  which conversations' OpenTrade effect also calls.
	 */
	bool OpenTradeFor(AStrategyUnit* Trader, AStrategyUnit* Listener);

	/**
	 *  Collects a loose world item into DestInventory - the Pick up action's arrival. Server-side.
	 *  Re-checks proximity and that the destination really is a player pawn's own pack, since range
	 *  is the whole gate on a pickup.
	 */
	void PickUpWorldItem(AWorldItem* WorldItem, UInventoryComponent* DestInventory);

public:

protected:

	/**
	 *  One purchase or sale, applied all-or-nothing on the server. Reached from
	 *  Server_MoveInventoryItem whenever exactly one side of a move is a trader's stock, so a
	 *  trade is the ordinary drag-and-drop move with gating layered in *front* of MoveItem
	 *  rather than a fourth resolution inside it.
	 *
	 *  Re-checks proximity and hostility here rather than trusting the client, for the same
	 *  reason PickUpWorldItem does: MoveItem itself has no idea how far away the asking
	 *  pawn was, or whether the counterparty is currently trying to kill it.
	 *
	 *  A purchase is priced against the *whole* requested quantity before anything moves. That
	 *  is what makes "insufficient gold changes nothing" true: the debit afterwards is for what
	 *  actually moved, which can only be less, so it can never fail once the item has already
	 *  changed hands.
	 */
	bool TryTradeItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity);

public:

	/**
	 *  Debug exec: dumps the selected pawn's inventory grid to the log as an ASCII occupancy map
	 *  plus a per-entry list. The authoritative grid only exists on the server, so this hops there
	 *  via Server_DebugInventory rather than reading the local replicated copy.
	 */
	UFUNCTION(Exec)
	void SmoresDumpInventory();

	/**
	 *  Debug exec: adds Count more of whatever item already sits in the selected pawn's first grid
	 *  entry, then dumps the grid - exercising stack-merge, auto-placement and rotation against
	 *  real authored footprints.
	 *
	 *  ModifierId optionally names an item modifier (SmoresDumpDefinitions lists the ids) to put
	 *  on the copies being added - `SmoresAddItem 3 Bronze`. That is what makes the modifier model
	 *  testable from a running game: the added copies won't merge with the unmodified ones already
	 *  in the grid, and their name, weight and price all read differently.
	 */
	UFUNCTION(Exec)
	void SmoresAddItem(int32 Count = 1, FName ModifierId = NAME_None);

	/**
	 *  Debug exec: repacks the selected pawn's grid by Criterion (0 = weight, 1 = value,
	 *  2 = quantity), then dumps it - so the occupancy map before and after shows the repack
	 *  without needing the window open. Runs the same SortEntries the buttons do.
	 */
	UFUNCTION(Exec)
	void SmoresSortInventory(int32 Criterion = 0);

	/** Debug exec: wears the selected pawn's EntryIndex'th placed grid entry, then dumps the paperdoll. */
	UFUNCTION(Exec)
	void SmoresEquipItem(int32 EntryIndex = 0);

	/** Debug exec: takes off whatever is in the SlotIndex'th paperdoll slot (see UEquipmentComponent::GetAllEquipSlots), then dumps. */
	UFUNCTION(Exec)
	void SmoresUnequipItem(int32 SlotIndex = 0);

	/** Debug exec: logs the selected pawn's worn slots. Server-side, like the grid dump. */
	UFUNCTION(Exec)
	void SmoresDumpEquipment();

	/**
	 *  Debug exec: logs the targeted NPC's wares with their buy and sell prices, plus this
	 *  player's balance. Click an NPC to target it first, the same as SmoresKillNPC.
	 */
	UFUNCTION(Exec)
	void SmoresDumpTrader();

	/** Debug exec: buys the targeted trader's EntryIndex'th stocked entry into the selected pawn's pack, then dumps. */
	UFUNCTION(Exec)
	void SmoresBuyItem(int32 EntryIndex = 0);

	/** Debug exec: sells the selected pawn's EntryIndex'th grid entry to the targeted trader, then dumps. */
	UFUNCTION(Exec)
	void SmoresSellItem(int32 EntryIndex = 0);

	/**
	 *  Debug exec: kills the currently-targeted NPC outright, so the Dead state has a way to be
	 *  reached without a combat rule that decides when a unit should actually die (see
	 *  UHealthComponent::Kill). A killed NPC is lootable exactly like a Downed one, except that
	 *  it never gets back up.
	 */
	UFUNCTION(Exec)
	void SmoresKillNPC();

	/**
	 *  Debug exec: drops the selected pawn's EntryIndex'th placed grid entry on the ground in front
	 *  of it as an AWorldItem, so the pickup path has something to pick up without hand-placing
	 *  actors in the level. The player-facing drop gesture is a later slice; this exercises the
	 *  same authority-side spawn it will use.
	 */
	UFUNCTION(Exec)
	void SmoresDropItem(int32 EntryIndex = 0);

	/**
	 *  Debug exec: lists every definition the Asset Manager found, grouped by definition type -
	 *  the direct check that a type's PrimaryAssetTypesToScan entry in Config/DefaultGame.ini is
	 *  right and that its ids actually resolve. Not a server hop: the definition registry is
	 *  authored content and is identical on every machine.
	 */
	UFUNCTION(Exec)
	void SmoresDumpDefinitions();

	/**
	 *  Debug exec: logs every faction's record (current tier beside its authored starting tier),
	 *  the faction-to-faction standing matrix, and this player's own standing with each faction.
	 *  Hops to the server, since the records and this player's standing are server-owned.
	 */
	UFUNCTION(Exec)
	void SmoresDumpFactions();

	/**
	 *  Debug exec: moves this player's standing with FactionId by Delta (clamped to -100..100),
	 *  then dumps - the manual check that standing is per-player and survives a round trip.
	 *  SmoresDumpDefinitions lists the faction ids.
	 */
	UFUNCTION(Exec)
	void SmoresAdjustStanding(FName FactionId, int32 Delta = 10);

	/**
	 *  Debug exec: logs a unit's character record beside the live state of its components, field
	 *  by field, flagging any that disagree - the manual check that the actor and its record are
	 *  in sync. Reads the targeted NPC if one is clicked, otherwise the first selected unit. Hops
	 *  to the server, since records exist only there.
	 */
	UFUNCTION(Exec)
	void SmoresDumpRecord();

	/**
	 *  Debug exec: rolls the loot table whose id is TableId Count times, from seeds Seed, Seed + 1,
	 *  ..., and prints each roll - every item with the table it came from - followed by a tally
	 *  across all of them. Touches nothing: no container, no inventory, not even the world seed,
	 *  so it is the way to judge a table's spread without opening a single chest. Not a server hop,
	 *  for the same reason as SmoresDumpDefinitions - tables are authored content, identical on
	 *  every machine. SmoresDumpDefinitions lists the table ids.
	 */
	UFUNCTION(Exec)
	void SmoresRollTable(FName TableId, int32 Seed = 0, int32 Count = 1);

	/**
	 *  Debug exec: re-reads every dialog package from disk - Content/Dialog/core and Mods/ - and
	 *  logs the summary. A writer edits a file, types this, and hears the change without
	 *  restarting. This machine only: each machine loads its own copy of the files.
	 */
	UFUNCTION(Exec)
	void SmoresReloadDialog();

	/** Debug exec: logs every dialog package, its counts and every load problem. "SmoresDialogReport facts" adds the writers' reference - every fact a condition may use, and who each event carries. */
	UFUNCTION(Exec)
	void SmoresDialogReport(const FString& Detail = TEXT(""));

	/**
	 *  Debug exec: fires a bark event on a unit and logs every line considered and why it did or
	 *  didn't win - the answer to "why did that line play?" - then the winning line as this machine
	 *  shows it, in the current culture. The unit is the one whose name contains TargetName
	 *  ("SmoresTestBark TradeOpened Ada"), or with no name the targeted NPC, else the first selected
	 *  squad member. Delivers the line exactly as play would, including cooldowns; only the
	 *  speaker's quiet time is skipped, so lines can be tried back to back. For WitnessedDeath the
	 *  unit is treated as the one who died, and its allies nearby are the ones who react. Hops to
	 *  the server, where the director lives.
	 */
	UFUNCTION(Exec)
	void SmoresTestBark(const FString& EventName, const FString& TargetName = TEXT(""));

	/**
	 *  Debug exec: shows game text in another culture - "SmoresSetCulture fr", then "en" to return.
	 *  In the editor it previews the game's text without touching the editor's own menus. The proof
	 *  that runtime-loaded dialog translates; see dialog.md's localization section.
	 */
	UFUNCTION(Exec)
	void SmoresSetCulture(const FString& Culture);

	/**
	 *  Debug exec: opens a conversation without walking up - "SmoresTestConversation
	 *  example.Shakedown Bandit". A Greeting or Topic opens its window on the NPC whose name
	 *  contains TargetName or whose character definition it names (else the clicked NPC),
	 *  eligible or not, however far away, with the
	 *  nearest squad member doing the talking; an Ambient one plays among the squad now. With no
	 *  id, runs Talk's own selection on that NPC and logs why each greeting did or didn't win.
	 *  Hops to the server.
	 */
	UFUNCTION(Exec)
	void SmoresTestConversation(const FString& ConversationId = TEXT(""), const FString& TargetName = TEXT(""));

	/**
	 *  Debug exec: logs this squad's dialog memory - every flag set and every conversation seen.
	 *  "SmoresDialogMemory set <flag>" sets a flag and "clear <flag>" clears one, to try a
	 *  conversation that waits on one. Hops to the server, which owns the memory.
	 */
	UFUNCTION(Exec)
	void SmoresDialogMemory(const FString& Action = TEXT(""), const FString& Flag = TEXT(""));

protected:

	/**
	 *  Server side of the inventory debug execs - optionally adds AddCount items carrying the
	 *  modifier named by ModifierId, then logs the grid.
	 *
	 *  The modifier crosses as an id rather than an object pointer so the server resolves it
	 *  itself through USmoresDefinitionLibrary - the same look-up a record or a loot table will
	 *  use, and the first caller of it outside SmoresDumpDefinitions.
	 */
	UFUNCTION(Server, Reliable)
	void Server_DebugInventory(UInventoryComponent* Inventory, int32 AddCount, FName ModifierId);

	/** Server side of the sort debug exec - repacks the grid, then logs it the same way */
	UFUNCTION(Server, Reliable)
	void Server_DebugSortInventory(UInventoryComponent* Inventory, EInventorySortCriterion Criterion);

	/** Shared body of the inventory debug execs: logs one grid as an ASCII occupancy map plus a per-entry list */
	static void LogInventoryGrid(UInventoryComponent* Inventory);

	/** Client side of the equipment debug execs - resolves the selected pawn locally, then hops to the server */
	void DebugEquipmentForSelection(int32 EquipEntryIndex, int32 UnequipSlotIndex);

	/** Server side of the equipment debug execs - optionally equips one grid entry and/or unequips one slot
	 *  (either index negative to skip it), then logs the paperdoll */
	UFUNCTION(Server, Reliable)
	void Server_DebugEquipment(UEquipmentComponent* Equipment, UInventoryComponent* Inventory, int32 EquipEntryIndex, int32 UnequipSlotIndex);

	/** Server side of the drop debug exec - removes one grid entry and spawns it as a world pickup
	 *  at the pawn's feet, since both the grid and the spawned actor are server-owned */
	UFUNCTION(Server, Reliable)
	void Server_DebugDropItem(APawn* DroppingPawn, UInventoryComponent* Inventory, int32 EntryIndex);

	/** Server side of the kill debug exec - health state is server-owned, like everything else here */
	UFUNCTION(Server, Reliable)
	void Server_DebugKill(AStrategyUnit* Target);

	/** Client side of the trade debug execs - resolves the targeted trader and the selected pawn locally, then hops to the server */
	void DebugTradeForSelection(int32 BuyEntryIndex, int32 SellEntryIndex);

	/** Server side of the trade debug execs - optionally buys one stocked entry and/or sells one
	 *  carried entry (either index negative to skip it), then logs the stock, the prices and the balance */
	UFUNCTION(Server, Reliable)
	void Server_DebugTrade(AStrategyUnit* TraderUnit, UInventoryComponent* PawnInventory, int32 BuyEntryIndex, int32 SellEntryIndex);

	/** Shared body of the two trade execs: resolves EntryIndex to an entry id, finds it room in
	 *  the destination grid, and runs the ordinary transaction. Auto-placement is a debug-only
	 *  convenience - the real path always carries the cell the player dropped on. */
	bool DebugTradeEntry(UInventoryComponent* SourceInventory, UInventoryComponent* DestInventory, int32 EntryIndex);

public:

	/** Debug exec: credits Amount gold to this player. Hops to the server, since the balance is server-owned. */
	UFUNCTION(Exec)
	void SmoresAddGold(int32 Amount = 100);

	/**
	 *  Debug exec: attempts to debit Amount gold from this player, exercising TrySpendGold's
	 *  insufficient-funds rejection (which leaves the balance untouched).
	 */
	UFUNCTION(Exec)
	void SmoresSpendGold(int32 Amount = 100);

protected:

	/** Server side of the gold debug execs - credits or debits, then logs the resulting balance */
	UFUNCTION(Server, Reliable)
	void Server_DebugGold(int32 Amount, bool bSpend);

	/** Server side of the faction debug execs - optionally adjusts this player's standing, then logs everything */
	UFUNCTION(Server, Reliable)
	void Server_DebugFactions(FName AdjustFactionId, int32 Delta);

	/** Server side of the record debug exec */
	UFUNCTION(Server, Reliable)
	void Server_DebugRecord(AStrategyUnit* Unit);

	/** Server side of SmoresTestBark - the director and everything selection reads are server-owned */
	UFUNCTION(Server, Reliable)
	void Server_DebugBark(AStrategyUnit* Target, EBarkEvent Event);

	/** Server side of SmoresTestConversation */
	UFUNCTION(Server, Reliable)
	void Server_DebugConversation(FName ConversationId, AStrategyUnit* Target);

	/** Server side of SmoresDialogMemory. Action is "", "set" or "clear". */
	UFUNCTION(Server, Reliable)
	void Server_DebugDialogMemory(const FString& Action, FName Flag);

public:

protected:

	/** Sorts all controlled units based on their distance to the provided world location */
	AStrategyUnit* GetClosestSelectedUnitToLocation(FVector TargetLocation);

	/**
	 *  Shared body of every Find*AtLocation below: the nearest actor of HolderClass within Radius
	 *  of Location that also passes Filter, or nullptr.
	 *
	 *  Nearest rather than first-found because actor iteration order isn't guaranteed, so two
	 *  holders close together would otherwise be picked between arbitrarily.
	 *
	 *  Radius is a parameter rather than one shared field because how precisely the player has to
	 *  aim is per-gesture: a chest is a big thing to click (ContainerSelectionRadius) and a
	 *  dropped item is a small one (the tighter WorldItemSelectionRadius). Sharing one radius let
	 *  an item lying near a chest swallow every double-click meant for the chest.
	 *
	 *  Filter is a parameter rather than something this could infer because each holder type
	 *  qualifies on a rule of its own - an NPC has to be lootable, a world item has to actually
	 *  hold something, a container always qualifies.
	 */
	AActor* FindHolderActorAtLocation(TSubclassOf<AActor> HolderClass, const FVector& Location, float Radius, TFunctionRef<bool(const AActor*)> Filter) const;

	/** True if any currently selected unit is within HolderActor's own reach. Real reach (the
	 *  thing's own interaction sphere), not click precision - FStrategyTargetActions::IsInRangeOfUnits
	 *  over this controller's selection. */
	bool IsHolderInRangeOfSelection(const AActor* HolderActor) const;

	/** Returns the first container in the level with a selected unit within its InteractionRange, preferring SelectedContainer if it qualifies, or nullptr */
	AStrategyContainer* FindContainerInRange() const;

	/** Returns SelectedNPC if it's lootable and within range of a controlled unit, or nullptr. Deliberately
	 *  never sweeps the level the way FindContainerInRange does - a body is only ever looted by key press if
	 *  the player has actually targeted it. */
	AStrategyUnit* FindLootableNPCInRange() const;

	/** Returns the container within click range of the given world location, or nullptr */
	AStrategyContainer* FindContainerAtLocation(const FVector& Location) const;

	/** Returns whichever player-controlled pawn is closest to the given world location, or nullptr if none
	 *  exist. Finds a *collector*, not a holder - there's no proximity gate here at all, so it answers
	 *  "who opens this panel" rather than "who may touch this". */
	AStrategyPlayerUnit* FindClosestPlayerPawn(const FVector& Location);

	/** Returns the loose world item within WorldItemSelectionRadius of the given world location, or nullptr */
	AWorldItem* FindWorldItemAtLocation(const FVector& Location) const;

	/** Returns the door within ContainerSelectionRadius of the given world location, or nullptr - a single click's door lookup */
	AWorldDoor* FindDoorAtLocation(const FVector& Location) const;

	/**
	 *  The thing the cursor is on, or null over empty ground - what the hover lights, what a
	 *  right-click opens the menu on, and what a double-click acts on. One answer for all three, so
	 *  what is lit is what any of those clicks will act on.
	 *
	 *  Exact first: whatever the cursor's ray passes through - a unit's body, a chest, a door's
	 *  leaf, an item - tested against each thing's own mesh bounds, so it works whatever each one's
	 *  collision is set to, and nothing behind a wall counts. Only then the small HoverPickRadius
	 *  fallback around the point on the ground, in the double-click ladder's order (item, container,
	 *  door, unit). A single click keeps its own generous sweep (DoSelectCommand) - picking your own
	 *  moving pawns is a different job.
	 */
	AActor* ResolveInteractableUnderCursor();

	/** True if Actor is a thing the hover and the menu treat as a target - an item with something in it, a container, a door, any unit */
	static bool IsHoverable(const AActor* Actor);

	/** The meshes the hover draws on (and aims at) for Actor - a door's leaf only, so an open doorway is still ground; otherwise every mesh it has */
	static void GetHoverMeshes(AActor* Actor, TArray<UMeshComponent*>& OutMeshes);

	/** Moves the hover highlight to NewHovered (null clears it) */
	void SetHoveredActor(AActor* NewHovered);

	/** Per-frame: what the cursor is over, or nothing while it's over the HUD or the camera is turning. The menu's target stays lit while the menu is open. */
	void UpdateHover();

	/** Targets Actor the way a click on it would - the container or NPC highlight, or just the target panel for things without one */
	void TargetActor(AActor* Actor);

	/** Makes Actor the target panel's subject without any highlight of its own - loose items and doors. Null clears it if it's one of those. */
	void SetTargetedActor(AActor* Actor);

	/** What a double-click on Actor does, or None if it does nothing to that kind of thing */
	static FName GetDoubleClickAction(const AActor* Actor);

	/** Returns the nearest player-controlled pawn close enough to transfer items with Holder, or nullptr.
	 *  Unlike FindClosestPlayerPawn this applies the holder's own reach, since proximity is a gate here
	 *  rather than a tiebreak. Checks every player pawn rather than just ControlledUnits - a holder may be
	 *  reached by double-click, and the plain select click that fires alongside that gesture may have just
	 *  cleared the selection. */
	AStrategyPlayerUnit* FindPlayerPawnInRangeOfHolder(const AActor* HolderActor);

	/** Updates SelectedContainer, toggling the old and new container's highlight material to match */
	void SetSelectedContainer(AStrategyContainer* NewContainer);

	/** Updates SelectedNPC, toggling the old and new NPC's selection state to match */
	void SetSelectedNPC(AStrategyUnit* NewNPC);

	/** Calculates and returns the current mouse location */
	FVector2D GetMouseLocationForPlayer();

	/** Attempts to get the world location under the cursor, returns true if successful */
	bool GetLocationUnderCursor(FVector& Location);

	/** Attempts to get the world location under the Touch 1 finger, returns true if successful */
	bool GetLocationUnderFinger(FVector& Location);

	/** Projects the current touch location into world space */
	FVector ProjectTouchPointToWorldSpace();

protected:

	/** Spawns the positive cursor effect */
	UFUNCTION(BlueprintImplementableEvent, Category="Cursor", meta = (DisplayName="Cursor Feedback"))
	void BP_CursorFeedback(FVector Location, bool bPositive);
};
