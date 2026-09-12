// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StrategySelectionHost.h"
#include "StrategyCameraCommands.h"
#include "InventoryMoveHost.h"
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
class UInventoryWidget;
class AStrategyContainer;
class UInventoryComponent;

/**
 *  Player Controller for a top-down strategy game.
 *  Handles unit selection and commands.
 *  Implements both mouse and touch controls.
 */
UCLASS(abstract)
class AStrategyPlayerController : public APlayerController, public IStrategySelectionHost, public IStrategyCameraCommands, public IInventoryMoveHost
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

	/** Max distance to look for a nearby container when doing a click or touch interaction */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float ContainerSelectionRadius = 250.0f;

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

	/** Whichever pawn, NPC, or container was most recently selected/targeted. Drives the selection target UI label */
	TWeakObjectPtr<AActor> LastSelectionTarget;

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

	/** All player-controllable pawns in the level. Rebuilt on demand by RefreshPlayerPawns() */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStrategyPlayerUnit>> PlayerPawns;

	/** Index into PlayerPawns of the pawn most recently cycled to. INDEX_NONE until the first cycle */
	int32 CurrentPlayerPawnIndex = INDEX_NONE;

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

	/** Returns the label text for whichever pawn, NPC, or container was most recently selected, or empty if none */
	virtual FText GetSelectionTargetLabel() const override;

	//~ End IStrategySelectionHost interface

	//~ Begin IStrategyCameraCommands interface

	/** Returns the default camera zoom percentage value */
	virtual float GetDefaultZoomPercentage() const override;

	//~ End IStrategyCameraCommands interface (remaining members below, alongside the other camera commands)

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

	/** Replaces the current selection with the next player-controlled pawn */
	void CyclePawn(const FInputActionValue& Value);

	/** Toggles the inventory screen for the selected pawn (requires exactly one selected player pawn) */
	void ToggleInventory(const FInputActionValue& Value);

	/** Closes the inventory screen if one is open */
	void CloseInventory();

	/** Opens (or rebinds) the pawn inventory screen for the given pawn, spawning the widget on first use */
	void OpenInventoryForPawn(AStrategyPlayerUnit* PlayerUnit);

	/** Opens the inventory screen for a nearby container, or closes it if already open */
	void ToggleContainer(const FInputActionValue& Value);

	/** Closes the container screen if one is open */
	void CloseContainer();

	/** Opens the given container's inventory screen, spawning the widget on first use */
	void OpenContainer(AStrategyContainer* Container);

	/** Opens the given Downed NPC's inventory screen for looting, spawning the widget on first use */
	void OpenLoot(AStrategyUnit* LootTarget);

	/** Attacks the currently-selected NPC if it's Passive (flips it to Aggressive); no-op otherwise */
	void AttackKeyPressed(const FInputActionValue& Value);

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

	/** Interaction click input started */
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

public:

	//~ Begin IInventoryMoveHost interface

	/** Server-side entry point for an inventory drag-drop move (see UInventoryWidget::NativeOnDrop -
	 *  the widget can't mutate inventory contents directly, since UInventoryComponent's mutators are
	 *  authority-only). Just forwards to UInventoryComponent::MoveItem, which does all the validation. */
	UFUNCTION(Server, Reliable)
	virtual void Server_MoveInventoryItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity) override;

	//~ End IInventoryMoveHost interface

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
	 *  real authored footprints without needing an item-id lookup path that doesn't exist yet.
	 */
	UFUNCTION(Exec)
	void SmoresAddItem(int32 Count = 1);

protected:

	/** Server side of the inventory debug execs - optionally adds AddCount items, then logs the grid */
	UFUNCTION(Server, Reliable)
	void Server_DebugInventory(UInventoryComponent* Inventory, int32 AddCount);

public:

protected:

	/** Sorts all controlled units based on their distance to the provided world location */
	AStrategyUnit* GetClosestSelectedUnitToLocation(FVector TargetLocation);

	/** Returns the first container in the level with a selected unit within its InteractionRange, preferring SelectedContainer if it qualifies, or nullptr */
	AStrategyContainer* FindContainerInRange() const;

	/** Returns SelectedNPC if it's Downed and within range of a controlled unit (lootable), or nullptr. Mirrors FindContainerInRange's shape. */
	AStrategyUnit* FindLootableNPCInRange() const;

	/** Returns the container within click range of the given world location, or nullptr */
	AStrategyContainer* FindContainerAtLocation(const FVector& Location) const;

	/** Returns the nearest Downed NPC within click range of the given world location, or nullptr. A non-Downed (Passive or Aggressive) NPC never qualifies. Mirrors FindContainerAtLocation's shape. */
	AStrategyUnit* FindLootableNPCAtLocation(const FVector& Location) const;

	/** Returns whichever player-controlled pawn is closest to the given world location, or nullptr if none exist */
	AStrategyPlayerUnit* FindClosestPlayerPawn(const FVector& Location);

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
