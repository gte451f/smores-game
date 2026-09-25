// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPlayerController.h"
#include "StrategyPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "StrategyPawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "StrategyHUD.h"
#include "RefusalWidget.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "NavigationSystem.h"
#include "Engine/OverlapResult.h"
#include "InputAction.h"
#include "StrategyTouchControls.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "InventoryWidget.h"
#include "EquipmentWidget.h"
#include "WindowWidget.h"
#include "HUDPanelWidget.h"
#include "InventoryDragDropOperation.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "StrategyContainer.h"
#include "WorldItem.h"
#include "InventoryHolder.h"
#include "HealthComponent.h"
#include "SmoresActivityLog.h"
#include "SquadActivityWatcher.h"
#include "WalletComponent.h"
#include "TraderComponent.h"
#include "ItemDefinition.h"
#include "ItemModifierDefinition.h"
#include "LootTableDefinition.h"
#include "SmoresDefinition.h"
#include "SmoresDefinitionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "StrategyGameState.h"
#include "TimePaceComponent.h"
#include "WorldFactionComponent.h"
#include "PlayerStandingComponent.h"
#include "FactionDefinition.h"
#include "CharacterRecordComponent.h"
#include "CharacterDefinition.h"
#include "StrategyTargetInfo.h"
#include "smores.h"

#define LOCTEXT_NAMESPACE "StrategyPlayerController"

AStrategyPlayerController::AStrategyPlayerController()
{
	// mouse cursor should always be shown
	bShowMouseCursor = true;
}

void AStrategyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UStrategyTouchControls>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

			// set the PC pointer on the mobile controls widget
			MobileControlsWidget->SetPlayerController(this);

		} else {

			UE_LOG(Logsmores, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// claim any not-yet-owned player unit for this controller - placeholder squad-assignment
	// policy until a real per-connection spawn/login flow exists; see
	// AStrategyPlayerUnit::ClaimForController
	if (HasAuthority())
	{
		TArray<AActor*> FoundUnits;
		UGameplayStatics::GetAllActorsOfClass(this, AStrategyPlayerUnit::StaticClass(), FoundUnits);

		for (AActor* CurrentActor : FoundUnits)
		{
			if (AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentActor))
			{
				CurrentUnit->ClaimForController(this);
			}
		}
	}

	// warm the player pawn list (CyclePawn refreshes again on use, so this is best-effort)
	RefreshPlayerPawns();

	// and start listening for anything worth a line in the feed
	RefreshActivityWatchers();

}

void AStrategyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only set up input on local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// choose the context based on the input mode
			UInputMappingContext* ChosenContext = nullptr;

			if (ShouldUseTouchControls())
			{
				ChosenContext = TouchMappingContext;
			}
			else
			{
				ChosenContext = MouseMappingContext;
			}

			Subsystem->AddMappingContext(ChosenContext, 0);
		}

		// bind the input mappings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Camera
			EnhancedInputComponent->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::MoveCamera);
			EnhancedInputComponent->BindAction(ZoomCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ZoomCamera);
			EnhancedInputComponent->BindAction(ResetCameraAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::ResetCamera);

			// Height (desktop only; not mapped in the touch IMC)
			if (AdjustHeightAction)
			{
				EnhancedInputComponent->BindAction(AdjustHeightAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::AdjustHeight);
			}

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClickAdditive);

			EnhancedInputComponent->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectAllDoubleClick);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractHoldCompleted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::InteractHoldCompleted);

			EnhancedInputComponent->BindAction(InteractClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::InteractClick);

			// Pawn cycling (desktop only; not mapped in the touch IMC)
			if (CyclePawnAction)
			{
				EnhancedInputComponent->BindAction(CyclePawnAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::CyclePawn);
			}

			// Inventory toggle (desktop only; not mapped in the touch IMC)
			if (ToggleInventoryAction)
			{
				EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::ToggleInventory);
			}

			// Container toggle (desktop only; not mapped in the touch IMC)
			if (ToggleContainerAction)
			{
				EnhancedInputComponent->BindAction(ToggleContainerAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::ToggleContainer);
			}

			// Attack (desktop only; not mapped in the touch IMC)
			if (AttackAction)
			{
				EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::AttackKeyPressed);
			}

			// Talk/trade with the targeted NPC - the keyboard route to the double-click interact,
			// and the accessible alternative to it (desktop only; not mapped in the touch IMC)
			if (TalkAction)
			{
				EnhancedInputComponent->BindAction(TalkAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TalkKeyPressed);
			}

			// Drag rotate. Bound here but only *mapped* while an inventory window is open (see
			// UpdateInventoryInputContext). Started rather than Completed so the item turns on the
			// key press instead of the release - and never Triggered, which for a held key would
			// spin the item once per frame.
			if (RotateDraggedItemAction)
			{
				EnhancedInputComponent->BindAction(RotateDraggedItemAction, ETriggerEvent::Started, this, &AStrategyPlayerController::RotateDraggedItem);
			}

			// HUD panels. Each mirrors a nav-rail button exactly - both routes run RequestPanel,
			// so the key and the button can't drift apart (desktop only; not mapped in the touch IMC)
			if (SquadPanelAction)
			{
				EnhancedInputComponent->BindAction(SquadPanelAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SquadPanelKeyPressed);
			}

			if (MapPanelAction)
			{
				EnhancedInputComponent->BindAction(MapPanelAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::MapPanelKeyPressed);
			}

			if (ResearchPanelAction)
			{
				EnhancedInputComponent->BindAction(ResearchPanelAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::ResearchPanelKeyPressed);
			}

			if (HelpPanelAction)
			{
				EnhancedInputComponent->BindAction(HelpPanelAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::HelpPanelKeyPressed);
			}

			// Time pace and the activity feed. Bound here so the eight HUD key mappings could all
			// be authored and verified in one editor pass, ahead of the features behind them -
			// see game-systems/input-and-keybinds.md for the discipline that goes with mapping a
			// key early. All eight do something now.
			if (TogglePauseAction)
			{
				EnhancedInputComponent->BindAction(TogglePauseAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TogglePauseKeyPressed);
			}

			if (PaceSlowerAction)
			{
				EnhancedInputComponent->BindAction(PaceSlowerAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::PaceSlowerKeyPressed);
			}

			if (PaceFasterAction)
			{
				EnhancedInputComponent->BindAction(PaceFasterAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::PaceFasterKeyPressed);
			}

			if (ToggleActivityFeedAction)
			{
				EnhancedInputComponent->BindAction(ToggleActivityFeedAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::ToggleActivityFeedKeyPressed);
			}

			// Touch Interaction
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::TouchPrimaryHoldStarted);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchPrimaryHoldTriggered);
			EnhancedInputComponent->BindAction(TouchPrimaryHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchPrimaryHoldCompleted);

			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::TouchSecondaryTriggered);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::TouchSecondaryCompleted);
			EnhancedInputComponent->BindAction(TouchSecondaryAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::TouchSecondaryCompleted);

		}
	}
}

void AStrategyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// ensure we have the right pawn type
	ControlledCameraPawn = Cast<AStrategyPawn>(InPawn);
	check(ControlledCameraPawn);

	// capture the pawn's resting yaw so a camera reset restores the level's designed facing rather than fighting it
	DefaultCameraYaw = ControlledCameraPawn->GetCamera()->GetRelativeRotation().Yaw;
	DoCameraResetRotationCommand();

	// push the default zoom and height onto the pawn
	CameraZoom = DefaultZoom;
	ControlledCameraPawn->SetZoomModifier(CameraZoom);

	CameraHeight = DefaultCameraHeight;
	ControlledCameraPawn->SetHeight(CameraHeight);

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());

	// if we have a touch controls widget, sync the camera zoom
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
}

void AStrategyPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bIsRotatingCamera)
	{
		return;
	}

	// sample the raw mouse delta every tick - Enhanced Input's Triggered callback for a held
	// digital button isn't guaranteed to fire every single frame, which would silently drop
	// most of the drag's movement if we sampled from there instead
	float DeltaX = 0.0f, DeltaY = 0.0f;
	GetInputMouseDelta(DeltaX, DeltaY);

	if (RotateStartupSkipTicksRemaining > 0)
	{
		// discard this tick's delta - it can include a spurious readback of the centering warp,
		// or of mouse capture itself still engaging, before the OS/Slate has caught up
		--RotateStartupSkipTicksRemaining;
	}
	else
	{
		DoCameraRotateCommand(FVector2D(DeltaX, DeltaY));
	}

	// re-center the cursor every tick so it never reaches a screen edge and saturates
	int32 ViewportSizeX = 0, ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);
	SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
}

void AStrategyPlayerController::DragSelectUnits(const TArray<AStrategyUnit*>& Units)
{
	// do we have units in the list?
	if (Units.Num() > 0)
	{
		// ensure any previous units are deselected
		DoDeselectAllUnitsCommand();

		// select each new unit
		for (AStrategyUnit* CurrentUnit : Units)
		{
			// add the unit to the selection list
			ControlledUnits.Add(CurrentUnit);

			// select the unit
			CurrentUnit->UnitSelected();
		}

		// treat the first boxed unit as the most recently targeted, for the selection label
		LastSelectionTarget = Units[0];
	}
	else
	{

		// release any currently selected units since nothing is on the box
		if (ControlledUnits.Num() > 0)
		{
			DoDeselectAllUnitsCommand();
		}

	}
}

const TArray<AStrategyUnit*>& AStrategyPlayerController::GetSelectedUnits()
{
	return ControlledUnits;
}

float AStrategyPlayerController::GetDefaultZoomPercentage() const
{
	float ZoomPct = (DefaultZoom - MinZoomLevel) / (MaxZoomLevel - MinZoomLevel);
	return FMath::Clamp(ZoomPct, 0.0f, 1.0f);
}

bool AStrategyPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AStrategyPlayerController::MoveCamera(const FInputActionValue& Value)
{
	if (!ControlledCameraPawn)
	{
		return;
	}

	FVector2D InputVector = Value.Get<FVector2D>();

	// derive movement axes from the camera's own current relative rotation rather than
	// GetControlRotation() - the engine can reset ControlRotation independently of the camera's
	// actual orientation (see DoCameraRotateCommand's matching note), so movement direction would
	// silently desync from the visible facing if it were sourced from ControlRotation instead
	FRotator ForwardRot = ControlledCameraPawn->GetCamera()->GetRelativeRotation();
	ForwardRot.Pitch = 0.0f;

	FRotator RightRot = ForwardRot;
	RightRot.Roll = 0.0f;

	ControlledCameraPawn->AddMovementInput(ForwardRot.RotateVector(FVector::ForwardVector), InputVector.X);

	// add the right input (negated - this IMC's Y axis reads +1 from A / -1 from D)
	ControlledCameraPawn->AddMovementInput(RightRot.RotateVector(FVector::RightVector), -InputVector.Y);
}

void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	// negated - scrolling up should move the camera closer (zoom in), not further away
	DoCameraModifyZoomCommand(-Value.Get<float>() * ZoomScaling);
}

void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
	DoCameraResetHeightCommand();
	DoCameraResetRotationCommand();
}

void AStrategyPlayerController::AdjustHeight(const FInputActionValue& Value)
{
	DoCameraModifyHeightCommand(Value.Get<float>() * HeightScaling);
}

void AStrategyPlayerController::RefreshPlayerPawns()
{
	PlayerPawns.Reset();

	// gather every player-controllable pawn currently in the level
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategyPlayerUnit::StaticClass(), FoundActors);

	for (AActor* CurrentActor : FoundActors)
	{
		if (AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentActor))
		{
			// only this controller's own squad - see AStrategyPlayerUnit::ClaimForController
			if (CurrentUnit->GetOwningController() == this)
			{
				PlayerPawns.Add(CurrentUnit);
			}
		}
	}

	// GetAllActorsOfClass order isn't stable across runs or streaming, so impose a
	// deterministic order for the Tab cycle. Placed actors keep a stable object name.
	PlayerPawns.Sort([](const AStrategyPlayerUnit& A, const AStrategyPlayerUnit& B)
	{
		return A.GetName() < B.GetName();
	});
}

void AStrategyPlayerController::RefreshActivityWatchers()
{
	USmoresActivityLog* ActivityLog = USmoresActivityLog::Get(this);

	if (!ActivityLog)
	{
		// no local player means no feed to report into - a remote controller, or a dedicated
		// server. Correct, and the reason this is checked here rather than in every handler.
		return;
	}

	TArray<AActor*> FoundUnits;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategyUnit::StaticClass(), FoundUnits);

	// drop watchers whose unit has gone, so the map doesn't grow across a long session
	for (auto It = ActivityWatchers.CreateIterator(); It; ++It)
	{
		AStrategyUnit* WatchedUnit = It.Key().Get();

		if (!IsValid(WatchedUnit) || !FoundUnits.Contains(WatchedUnit))
		{
			if (USquadActivityWatcher* Watcher = It.Value())
			{
				Watcher->Unwatch();
			}

			It.RemoveCurrent();
		}
	}

	for (AActor* CurrentActor : FoundUnits)
	{
		AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor);

		if (!IsValid(CurrentUnit) || ActivityWatchers.Contains(CurrentUnit))
		{
			continue;
		}

		// Both sides of a fight are watched, not just the squad: "Pawn 1 hit Bandit" and "Bandit
		// is down" are the half of the record that says the fight was going the player's way.
		// Which wording a unit gets is decided once, here, by whether it is one of ours.
		const AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit);
		const bool bOwnSquad = PlayerUnit && PlayerUnit->GetOwningController() == this;

		USquadActivityWatcher* Watcher = NewObject<USquadActivityWatcher>(this);

		Watcher->Watch(CurrentUnit, ActivityLog, bOwnSquad);

		ActivityWatchers.Add(CurrentUnit, Watcher);
	}
}

void AStrategyPlayerController::CyclePawn(const FInputActionValue& Value)
{
	// always work from a fresh list (handles units streamed in/out or destroyed at runtime)
	RefreshPlayerPawns();

	if (PlayerPawns.Num() == 0)
	{
		return;
	}

	// if a player pawn is currently the primary selection, resume cycling from its position
	if (ControlledUnits.Num() > 0)
	{
		if (AStrategyPlayerUnit* CurrentlySelected = Cast<AStrategyPlayerUnit>(ControlledUnits[0]))
		{
			const int32 FoundIndex = PlayerPawns.IndexOfByKey(CurrentlySelected);

			if (FoundIndex != INDEX_NONE)
			{
				CurrentPlayerPawnIndex = FoundIndex;
			}
		}
	}

	// advance with wrap-around. On the first press CurrentPlayerPawnIndex is INDEX_NONE (-1),
	// so (-1 + 1) % N == 0 and we select the first pawn.
	CurrentPlayerPawnIndex = (CurrentPlayerPawnIndex + 1) % PlayerPawns.Num();

	AStrategyPlayerUnit* NextPawn = PlayerPawns[CurrentPlayerPawnIndex].Get();

	if (!IsValid(NextPawn))
	{
		return;
	}

	// replace the selection using the existing deselect/select path
	DoDeselectAllUnitsCommand();

	ControlledUnits.Add(NextPawn);
	NextPawn->UnitSelected();

	// the newly cycled-to pawn is now the most recently targeted, for the selection label
	LastSelectionTarget = NextPawn;

	// NOTE: deliberately does not touch ControlledCameraPawn - the camera must not move on cycle

	// the previous pawn's inventory (if shown) is now stale
	CloseInventory();
}

void AStrategyPlayerController::ToggleInventory(const FInputActionValue& Value)
{
	ToggleInventoryPanel();
}

void AStrategyPlayerController::ToggleInventoryPanel()
{
	// if a screen is already open, pressing again closes it
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		CloseInventory();
		return;
	}

	// require exactly one selected player-controlled pawn
	AStrategyPlayerUnit* SinglePlayerUnit = nullptr;

	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			if (SinglePlayerUnit)
			{
				// more than one player pawn selected - do nothing
				return;
			}

			SinglePlayerUnit = PlayerUnit;
		}
	}

	if (!SinglePlayerUnit)
	{
		return;
	}

	OpenInventoryForPawn(SinglePlayerUnit, /*bOpenEquipment =*/ true);
}

void AStrategyPlayerController::OpenInventoryForPawn(AStrategyPlayerUnit* PlayerUnit, bool bOpenEquipment)
{
	if (!PlayerUnit)
	{
		return;
	}

	// spawn the widget on first use
	if (!InventoryWidget)
	{
		if (!InventoryWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no InventoryWidgetClass set; can't open the inventory screen."));
			return;
		}

		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);

		if (InventoryWidget)
		{
			InventoryWidget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (InventoryWidget)
	{
		InventoryWidget->SetWindowTitle(FText::Format(LOCTEXT("PawnInventoryTitle", "{0} Inventory"), PlayerUnit->GetHolderDisplayName()));
		InventoryWidget->SetInventory(PlayerUnit->GetInventory());

		// after SetInventory, which clears the previous binding's target along with it. This is
		// what makes right-click-to-equip work in a pawn's own window and nowhere else
		InventoryWidget->SetEquipmentTarget(PlayerUnit->GetEquipment());

		InventoryWidget->AddToViewport(0);
	}

	if (bOpenEquipment)
	{
		// its own floating window rather than part of the inventory panel, so the two can be moved
		// and sized independently and either can be the drop target for the other
		OpenEquipmentForPawn(PlayerUnit);
	}
	else
	{
		// a pack opened as a transfer partner doesn't bring a paperdoll - and any paperdoll
		// already up may belong to a different pawn than the one this window just rebound to,
		// which is the same staleness the cycle-pawn path closes the inventory to avoid
		CloseEquipment();
	}

	UpdateInventoryInputContext();
}

void AStrategyPlayerController::OpenEquipmentForPawn(AStrategyPlayerUnit* PlayerUnit)
{
	if (!PlayerUnit)
	{
		return;
	}

	// spawn the widget on first use
	if (!EquipmentWidget)
	{
		if (!EquipmentWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no EquipmentWidgetClass set; can't open the equipment screen."));
			return;
		}

		EquipmentWidget = CreateWidget<UEquipmentWidget>(this, EquipmentWidgetClass);

		if (EquipmentWidget)
		{
			EquipmentWidget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (EquipmentWidget)
	{
		EquipmentWidget->SetWindowTitle(FText::Format(LOCTEXT("PawnEquipmentTitle", "{0} Equipment"), PlayerUnit->GetHolderDisplayName()));
		EquipmentWidget->SetEquipment(PlayerUnit->GetEquipment());
		EquipmentWidget->AddToViewport(0);
	}
}

void AStrategyPlayerController::CloseEquipment()
{
	if (EquipmentWidget)
	{
		EquipmentWidget->ClearEquipment();

		if (EquipmentWidget->IsInViewport())
		{
			EquipmentWidget->RemoveFromParent();
		}
	}
}

void AStrategyPlayerController::HandleWindowClosed(UWindowWidget* Window)
{
	// A nav-rail panel closes alone and brings nothing with it. It returns before the inventory
	// bookkeeping below on purpose: the inventory input context is the inventory's business, and
	// a research window that re-scoped it would quietly give `R` a second meaning. The entry stays
	// in PanelWidgets so re-opening reuses the window.
	if (Cast<UHUDPanelWidget>(Window))
	{
		return;
	}

	// the window has already removed itself by the time this fires; what it can't do on its own
	// is take its companions with it, or drop the input context scoped to a window being open
	if (Window == InventoryWidget)
	{
		// the paperdoll belongs to the pack it opened with
		CloseInventory();
	}
	else if (Window == EquipmentWidget)
	{
		// closing just the paperdoll leaves the pack open, which is what the player asked for
		CloseEquipment();
	}
	else if (Window == ContainerWidget)
	{
		CloseContainer();
	}

	// CloseEquipment is the one path above that doesn't already do this
	UpdateInventoryInputContext();
}

void AStrategyPlayerController::CloseInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->ClearInventory();

		if (InventoryWidget->IsInViewport())
		{
			InventoryWidget->RemoveFromParent();
		}
	}

	// the paperdoll belongs to the same pawn as the grid it opened with, so it goes with it -
	// a stale paperdoll for a pawn that's no longer selected is exactly the failure the
	// cycle-pawn path closes the inventory to avoid
	CloseEquipment();

	UpdateInventoryInputContext();
}

void AStrategyPlayerController::RotateDraggedItem(const FInputActionValue& Value)
{
	// a window can be open with nothing in flight, so this is a no-op more often than not.
	// Slate owns the drag, not this controller - UInventoryDragDropOperation::GetActiveDrag is
	// what bridges the two.
	if (UInventoryDragDropOperation* ActiveDrag = UInventoryDragDropOperation::GetActiveDrag())
	{
		ActiveDrag->ToggleRotation();
	}
}

void AStrategyPlayerController::UpdateInventoryInputContext()
{
	if (!InventoryMappingContext || !IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (!Subsystem)
	{
		return;
	}

	const bool bAnyInventoryWindowOpen =
		(InventoryWidget && InventoryWidget->IsInViewport()) ||
		(ContainerWidget && ContainerWidget->IsInViewport()) ||
		(EquipmentWidget && EquipmentWidget->IsInViewport());

	if (bAnyInventoryWindowOpen)
	{
		// priority 1 beats the gameplay context at 0, so an inventory key wins over whatever the
		// same key means in the world for as long as a window is up
		Subsystem->AddMappingContext(InventoryMappingContext, 1);
	}
	else
	{
		Subsystem->RemoveMappingContext(InventoryMappingContext);
	}
}

void AStrategyPlayerController::RequestPanel(EHUDPanel Panel)
{
	// Purely local UI. Nothing here is shared state, so none of it is authority-gated or
	// replicated - which panels this player has open is nobody else's business, and in co-op
	// every player's rail answers only for their own windows.
	if (Panel == EHUDPanel::Inventory)
	{
		// the rail button and the `I` key are the same path, refusal and all
		ToggleInventoryPanel();
		return;
	}

	if (Panel == EHUDPanel::None)
	{
		return;
	}

	if (IsPanelOpen(Panel))
	{
		if (TObjectPtr<UHUDPanelWidget>* Existing = PanelWidgets.Find(Panel))
		{
			// straight through the window's own close path, so it broadcasts OnWindowClosed
			// exactly as it would have if the player had clicked its X
			(*Existing)->RequestClose();
		}

		return;
	}

	OpenPanel(Panel);
}

bool AStrategyPlayerController::IsPanelOpen(EHUDPanel Panel) const
{
	if (Panel == EHUDPanel::Inventory)
	{
		return InventoryWidget && InventoryWidget->IsInViewport();
	}

	const TObjectPtr<UHUDPanelWidget>* Found = PanelWidgets.Find(Panel);

	return Found && *Found && (*Found)->IsInViewport();
}

void AStrategyPlayerController::RequestPace(EGamePace Pace)
{
	// Unlike RequestPanel, this is *not* local UI state - it changes the simulation for everyone
	// in the session. The client can't touch the GameState that owns it, so the ask goes to the
	// server through the one actor this client does own.
	Server_RequestPace(Pace);
}

void AStrategyPlayerController::Server_RequestPace_Implementation(EGamePace Pace)
{
	if (UTimePaceComponent* TimePace = GetTimePace())
	{
		TimePace->SetPace(Pace);
	}
}

void AStrategyPlayerController::OpenPanel(EHUDPanel Panel)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	const TSubclassOf<UHUDPanelWidget>* PanelClass = PanelWidgetClasses.Find(Panel);

	if (!PanelClass || !*PanelClass)
	{
		// an unwired panel is a missing entry in PanelWidgetClasses on BP_StrategyPlayerController,
		// not a bug here - say which one so it's a one-line fix
		UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no panel widget class for %s; can't open it."),
			*UEnum::GetValueAsString(Panel));
		return;
	}

	TObjectPtr<UHUDPanelWidget>& Widget = PanelWidgets.FindOrAdd(Panel);

	// spawn on first use, then keep it - re-opening a panel shouldn't rebuild it
	if (!Widget)
	{
		Widget = CreateWidget<UHUDPanelWidget>(this, *PanelClass);

		if (Widget)
		{
			Widget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (Widget && !Widget->IsInViewport())
	{
		// Z-order 0, same as every other floating window, so the refusal line still beats it
		Widget->AddToViewport(0);
	}
}

void AStrategyPlayerController::RequestSelectUnit(AStrategyUnit* Unit, bool bFocusCamera)
{
	if (!IsValid(Unit))
	{
		return;
	}

	// Only this player's own squad, checked here rather than trusted from the widget: the bar is
	// built from GetControlledPlayerUnits() and so can only offer this player's own pawns, but
	// "the UI only ever asks for legal things" is not a rule this method should depend on.
	const AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(Unit);

	if (!PlayerUnit || PlayerUnit->GetOwningController() != this)
	{
		return;
	}

	// Replacing the selection rather than adding to it, and through the same deselect/select path
	// the Tab cycle uses - a portrait click is the mouse's version of that gesture, so the two had
	// better mean the same thing. Additive selection has a modifier in the world and does not need
	// a second, different answer here.
	DoDeselectAllUnitsCommand();

	ControlledUnits.Add(Unit);
	Unit->UnitSelected();

	// the newly selected pawn is now the most recently targeted, for the target panel
	LastSelectionTarget = Unit;

	// keep the Tab cycle resuming from where the click left off, rather than from wherever it had
	// got to before the player reached for the mouse
	const int32 FoundIndex = PlayerPawns.IndexOfByKey(PlayerUnit);

	if (FoundIndex != INDEX_NONE)
	{
		CurrentPlayerPawnIndex = FoundIndex;
	}

	if (bFocusCamera)
	{
		FocusCameraOnUnit(Unit);
	}
}

void AStrategyPlayerController::FocusCameraOnUnit(const AStrategyUnit* Unit)
{
	if (!ControlledCameraPawn || !IsValid(Unit))
	{
		return;
	}

	// A hard cut, deliberately: player-interface.md specifies exactly that for switching between
	// divisions, and this is the same gesture arriving early on a single roster. Height and
	// rotation are left alone - the player set those, and a focus that also reset the camera
	// would cost them their framing every time they clicked a portrait.
	const FVector UnitLocation = Unit->GetActorLocation();
	const float CameraZ = ControlledCameraPawn->GetHeight();

	// AStrategyPawn::UpdateCameraDollyOffset keeps the camera exactly DollyDistance *behind the
	// root* along its own look direction, so the root is always the point at the centre of the
	// screen - and zoom only slides the camera along that same ray, so none of this depends on
	// the zoom level.
	//
	// That is also why moving the root to the unit's own X and Y is wrong, which is how this first
	// shipped: the root has to stay up at camera height, so screen centre landed on a point in
	// mid-air *above* the unit, and with the camera pitched down the ground beneath it sits well
	// below and behind that. The symptom is the camera arriving in the right place while looking
	// out over the pawn, which reads as the focus having missed rather than as a framing error.
	//
	// So solve for it instead: find how far along the look direction the unit's height lies, and
	// put the root that far back from the unit. Screen centre then lands on the unit itself.
	const FVector Forward = ControlledCameraPawn->GetCamera()->GetComponentRotation().Vector();

	FVector NewRootLocation(UnitLocation.X, UnitLocation.Y, CameraZ);

	// A near-horizontal camera never crosses the unit's height, so there is no answer to solve
	// for - fall back to the unit's own X and Y, which is at least the right place on the map.
	// Unreachable with the current height/pitch limits; here so a later camera change degrades
	// rather than divides by ~zero and throws the pawn to the far side of the level.
	if (FMath::Abs(Forward.Z) > UE_KINDA_SMALL_NUMBER)
	{
		const float DistanceToUnitHeight = (UnitLocation.Z - CameraZ) / Forward.Z;

		NewRootLocation = UnitLocation - Forward * DistanceToUnitHeight;

		// the plane constraint in AStrategyPawn::SetHeight owns the height; the solve above should
		// already land on it, and this makes that exact rather than nearly so
		NewRootLocation.Z = CameraZ;
	}

	ControlledCameraPawn->SetActorLocation(NewRootLocation);
}

void AStrategyPlayerController::RequestTargetAction(FName ActionId)
{
	AActor* Target = LastSelectionTarget.Get();

	if (!IsValid(Target))
	{
		return;
	}

	// Rebuild the row and look the action up in it, rather than trusting the button that sent it.
	// The panel the player clicked is a frame old, and a frame is long enough for the squad to
	// have walked out of range - so the gate that *offered* the action is the gate that decides
	// it, by construction rather than by two checks that agree today.
	const FStrategyTargetInfo Info = BuildTargetInfo(Target, ControlledUnits);

	const FTargetAction* Action = Info.Actions.FindByPredicate([ActionId](const FTargetAction& Candidate)
	{
		return Candidate.Id == ActionId;
	});

	if (!Action)
	{
		// no longer on offer at all - the target changed state between the draw and the click
		return;
	}

	if (!Action->bEnabled)
	{
		// the button was greyed out and the player clicked it anyway (or it went grey in between).
		// Saying why beats doing nothing - same line the key press would have raised.
		NotifyRefusal(Action->DisabledReason);
		return;
	}

	// each branch runs exactly what the equivalent key runs. Nothing here re-implements an
	// action; the panel is a second route to the same behaviour, never a lookalike of it.
	if (ActionId == StrategyTargetAction::Open())
	{
		if (AStrategyContainer* Container = Cast<AStrategyContainer>(Target))
		{
			// opening a container always leaves it highlighted, same as the `O` key's path
			SetSelectedContainer(Container);

			OpenContainer(Container);
		}
	}
	else if (ActionId == StrategyTargetAction::Loot())
	{
		if (AStrategyUnit* Body = Cast<AStrategyUnit>(Target))
		{
			OpenLoot(Body);
		}
	}
	else if (ActionId == StrategyTargetAction::Talk())
	{
		if (AStrategyUnit* NPC = Cast<AStrategyUnit>(Target))
		{
			InteractWithNPC(NPC);
		}
	}
	else if (ActionId == StrategyTargetAction::Attack())
	{
		if (AStrategyUnit* NPC = Cast<AStrategyUnit>(Target))
		{
			DoAttackCommand(NPC);
		}
	}
}

void AStrategyPlayerController::SquadPanelKeyPressed(const FInputActionValue& Value)
{
	RequestPanel(EHUDPanel::Squad);
}

void AStrategyPlayerController::MapPanelKeyPressed(const FInputActionValue& Value)
{
	RequestPanel(EHUDPanel::Map);
}

void AStrategyPlayerController::ResearchPanelKeyPressed(const FInputActionValue& Value)
{
	RequestPanel(EHUDPanel::Research);
}

void AStrategyPlayerController::HelpPanelKeyPressed(const FInputActionValue& Value)
{
	RequestPanel(EHUDPanel::Help);
}

void AStrategyPlayerController::TogglePauseKeyPressed(const FInputActionValue& Value)
{
	const UTimePaceComponent* TimePace = GetTimePace();

	if (!TimePace)
	{
		return;
	}

	// The tier to come back to is read off the component, not remembered here: in co-op one
	// player can pause and another unpause, and they have to arrive at the same speed. Both
	// values are replicated, so this reads correctly on a client.
	RequestPace(TimePace->IsPaused() ? TimePace->GetResumePace() : EGamePace::Paused);
}

void AStrategyPlayerController::PaceSlowerKeyPressed(const FInputActionValue& Value)
{
	RequestPaceStep(-1);
}

void AStrategyPlayerController::PaceFasterKeyPressed(const FInputActionValue& Value)
{
	RequestPaceStep(1);
}

void AStrategyPlayerController::RequestPaceStep(int32 Steps)
{
	if (const UTimePaceComponent* TimePace = GetTimePace())
	{
		// stepping from the replicated current tier rather than from a local guess, so holding the
		// key can't run the client's idea of the pace ahead of the server's
		RequestPace(UTimePaceComponent::StepPace(TimePace->GetPace(), Steps));
	}
}

void AStrategyPlayerController::ToggleActivityFeedKeyPressed(const FInputActionValue& Value)
{
	// The feed is a region inside the HUD's widget, and `smores` already depends on SmoresUI - so
	// this goes controller -> HUD -> root -> region rather than through IStrategyHUDCommands,
	// which exists for requests travelling the other way.
	if (StrategyHUD)
	{
		StrategyHUD->ToggleActivityFeed();
	}
}

void AStrategyPlayerController::ToggleContainer(const FInputActionValue& Value)
{
	// if a screen is already open, pressing again closes it
	if (ContainerWidget && ContainerWidget->IsInViewport())
	{
		CloseContainer();
		return;
	}

	// require a container within range of at least one selected unit
	AStrategyContainer* NearbyContainer = FindContainerInRange();

	if (NearbyContainer)
	{
		// opening a container always leaves it highlighted, even via the no-ambiguity auto-fallback
		SetSelectedContainer(NearbyContainer);

		OpenContainer(NearbyContainer);
		return;
	}

	// no container in range - try a Downed NPC instead, reusing the same widget/proximity rules
	if (AStrategyUnit* LootableNPC = FindLootableNPCInRange())
	{
		OpenLoot(LootableNPC);
		return;
	}

	// nothing to open. A key that does nothing reads as a broken keybind, which is the one thing
	// it isn't - the pawn is just standing too far from whatever the player meant
	NotifyRefusal(ESmoresRefusalReason::TooFar);
}

void AStrategyPlayerController::CloseContainer()
{
	if (ContainerWidget)
	{
		ContainerWidget->ClearInventory();

		if (ContainerWidget->IsInViewport())
		{
			ContainerWidget->RemoveFromParent();
		}
	}

	// the pawn's own pack stays open, but it was only quoting sell prices because a trader was
	// on the other side of it - with the counter gone, so is the offer
	if (InventoryWidget)
	{
		InventoryWidget->ClearPricing();
	}

	UpdateInventoryInputContext();
}

void AStrategyPlayerController::OpenContainer(AStrategyContainer* Container)
{
	if (!Container)
	{
		return;
	}

	// spawn the widget on first use
	if (!ContainerWidget)
	{
		if (!ContainerWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no ContainerWidgetClass set; can't open the container screen."));
			return;
		}

		ContainerWidget = CreateWidget<UInventoryWidget>(this, ContainerWidgetClass);

		if (ContainerWidget)
		{
			ContainerWidget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (ContainerWidget)
	{
		ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("ContainerInventoryTitle", "{0} Contents"), Container->GetHolderDisplayName()));
		ContainerWidget->SetInventory(Container->GetInventory());
		ContainerWidget->AddToViewport(0);

		Container->NotifyOpened();
	}

	UpdateInventoryInputContext();

	// also open the inventory of whichever player-controlled pawn is closest to this container,
	// regardless of current selection, so the two panels can be used together to transfer items
	if (AStrategyPlayerUnit* ClosestPawn = FindClosestPlayerPawn(Container->GetActorLocation()))
	{
		OpenInventoryForPawn(ClosestPawn, /*bOpenEquipment =*/ false);
	}
}

void AStrategyPlayerController::OpenLoot(AStrategyUnit* LootTarget)
{
	if (!LootTarget)
	{
		return;
	}

	// spawn the widget on first use
	if (!ContainerWidget)
	{
		if (!ContainerWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no ContainerWidgetClass set; can't open the loot screen."));
			return;
		}

		ContainerWidget = CreateWidget<UInventoryWidget>(this, ContainerWidgetClass);

		if (ContainerWidget)
		{
			ContainerWidget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (ContainerWidget)
	{
		// the two states loot identically, but the player should still be able to tell a body
		// that will get up again from one that won't
		const FText StateLabel = LootTarget->IsDead()
			? LOCTEXT("LootStateDead", "Dead")
			: LOCTEXT("LootStateDowned", "Downed");

		ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("LootInventoryTitle", "{0} ({1})"), LootTarget->GetHolderDisplayName(), StateLabel));
		ContainerWidget->SetInventory(LootTarget->GetInventory());
		ContainerWidget->AddToViewport(0);
	}

	UpdateInventoryInputContext();

	// also open the inventory of whichever player-controlled pawn is closest to this NPC,
	// regardless of current selection, so the two panels can be used together to transfer items
	if (AStrategyPlayerUnit* ClosestPawn = FindClosestPlayerPawn(LootTarget->GetActorLocation()))
	{
		OpenInventoryForPawn(ClosestPawn, /*bOpenEquipment =*/ false);
	}
}

void AStrategyPlayerController::OpenTrade(AStrategyUnit* TraderUnit, UTraderComponent* Stock)
{
	if (!TraderUnit || !Stock)
	{
		return;
	}

	// spawn the widget on first use
	if (!ContainerWidget)
	{
		if (!ContainerWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no ContainerWidgetClass set; can't open the trade screen."));
			return;
		}

		ContainerWidget = CreateWidget<UInventoryWidget>(this, ContainerWidgetClass);

		if (ContainerWidget)
		{
			ContainerWidget->OnWindowClosed.AddUniqueDynamic(this, &AStrategyPlayerController::HandleWindowClosed);
		}
	}

	if (ContainerWidget)
	{
		ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("TraderInventoryTitle", "{0} - Wares"), TraderUnit->GetHolderDisplayName()));
		ContainerWidget->SetInventory(Stock);

		// this window holds the trader's stock, so its items quote what the player would *pay*
		ContainerWidget->SetPricing(Stock, /*bItemsAreTraderStock =*/ true);
		ContainerWidget->AddToViewport(0);
	}

	UpdateInventoryInputContext();

	// the pawn's own pack opens alongside, exactly as it does for a chest or a body - a trade is
	// the same two-panel drag-and-drop transfer with prices attached
	if (AStrategyPlayerUnit* ClosestPawn = FindClosestPlayerPawn(TraderUnit->GetActorLocation()))
	{
		OpenInventoryForPawn(ClosestPawn, /*bOpenEquipment =*/ false);

		// ...and it is the other side of the same counter, so its items quote what the trader
		// would pay for them. Set after OpenInventoryForPawn, which rebinds and so clears this.
		if (InventoryWidget)
		{
			InventoryWidget->SetPricing(Stock, /*bItemsAreTraderStock =*/ false);
		}
	}
}

void AStrategyPlayerController::InteractWithNPC(AStrategyUnit* NPC)
{
	// a hostile NPC is filtered out by IsInteractableNPC, not by a check here - the rule that
	// the player never trades with (or, later, talks to) someone currently trying to kill them
	// lives in one predicate, so dialog inherits it rather than re-deriving it
	if (!IsInteractableNPC(NPC))
	{
		// by the time a double-click reaches here the incapacitated cases have already gone down
		// the loot branch, so this is someone on their feet who is currently hostile
		NotifyRefusal(ESmoresRefusalReason::NotInteractable);

		return;
	}

	UTraderComponent* Stock = GetTraderStock(NPC);

	// no wares means no shop. This is where dialog goes when it exists, and there is
	// deliberately no stub for it here: an empty hook nobody implements against is clutter, and
	// the settled *ordering* is what lets dialog drop in later with no rework.
	//
	// Deliberately no refusal either, for the same reason. Every other silence in this file is a
	// rule the player ran into; this one is a feature that isn't built, and "they have nothing to
	// say" would be a lie the day dialog lands.
	if (!Stock)
	{
		return;
	}

	// proximity is the same gate every transfer context shares - a trader implements
	// IInventoryHolder through AStrategyUnit, so this needed no new distance code
	if (!FindPlayerPawnInRangeOfHolder(NPC))
	{
		NotifyRefusal(ESmoresRefusalReason::TooFar);

		return;
	}

	OpenTrade(NPC, Stock);
}

void AStrategyPlayerController::AttackKeyPressed(const FInputActionValue& Value)
{
	// no effect on a selected container or player pawn - neither ever populates SelectedNPC
	if (SelectedNPC && !SelectedNPC->IsAggressive())
	{
		DoAttackCommand(SelectedNPC);
	}
}

void AStrategyPlayerController::TalkKeyPressed(const FInputActionValue& Value)
{
	// if a trade (or container, or loot) screen is already open, pressing again closes it -
	// same toggle shape as the container key
	if (ContainerWidget && ContainerWidget->IsInViewport())
	{
		CloseContainer();
		return;
	}

	if (AStrategyUnit* Target = FindInteractableNPCInRange())
	{
		InteractWithNPC(Target);
	}
}

void AStrategyPlayerController::SelectHoldStarted(const FInputActionValue& Value)
{
	// save the box selection start position
	StartingBoxSelectionPosition = GetMouseLocationForPlayer();

}

void AStrategyPlayerController::SelectHoldTriggered(const FInputActionValue& Value)
{
	// get the current mouse position
	FVector2D SelectionPosition = GetMouseLocationForPlayer();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}	
}

void AStrategyPlayerController::SelectHoldCompleted(const FInputActionValue& Value)
{
	// reset the drag box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

void AStrategyPlayerController::SelectClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// select at the cursor
		DoSelectCommand(CursorLocation, false);
	}
}

void AStrategyPlayerController::SelectClickAdditive(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// additive select at the cursor
		DoSelectCommand(CursorLocation, true);
	}
}

void AStrategyPlayerController::SelectAllDoubleClick(const FInputActionValue& Value)
{
	// if the double-click landed on a container, select + open it instead of the usual select-all gesture
	FVector CursorLocation;

	if (GetLocationUnderCursor(CursorLocation))
	{
		// loose world items are checked first, on their own tighter radius: a pickup is the most
		// precisely-aimed of the three gestures, and unlike a container or an NPC the item carries
		// no selection state, so finding one either collects it or does nothing at all
		if (AWorldItem* ClickedItem = FindWorldItemAtLocation(CursorLocation))
		{
			// collect it with whichever player pawn is nearest and close enough. Checked against
			// every player pawn (not just ControlledUnits) for the same reason the container branch
			// below is: the plain SelectClickAction fires alongside this gesture and, being
			// non-additive, may have just cleared the current selection.
			if (AStrategyPlayerUnit* Collector = FindPlayerPawnInRangeOfHolder(ClickedItem))
			{
				Server_PickUpWorldItem(ClickedItem, Collector->GetInventory());
			}
			else
			{
				// decided here rather than on the server: the client already knows where every
				// pawn is, so there is no reason to ask and wait to be told no
				NotifyRefusal(ESmoresRefusalReason::TooFar);
			}

			// out of range collects nothing, and still swallows the select-all - same as an
			// out-of-range container or corpse, where the gesture means "that thing", not "everyone"
			return;
		}

		if (AStrategyContainer* Clicked = FindContainerAtLocation(CursorLocation))
		{
			// highlight it dark green, same as a single click, regardless of range
			SetSelectedContainer(Clicked);

			// open it if any player-controlled pawn is close enough - see
			// FindPlayerPawnInRangeOfHolder for why that's every pawn rather than the selection
			if (FindPlayerPawnInRangeOfHolder(Clicked))
			{
				OpenContainer(Clicked);
			}
			else
			{
				// it still highlights, so the player can see they picked the right chest - the
				// only thing missing is somebody standing near it
				NotifyRefusal(ESmoresRefusalReason::TooFar);
			}

			return;
		}

		// no container at this location - try an NPC instead. A body and a living NPC are the same
		// actor type differing only by health state, so this is one lookup that then branches
		// rather than two sweeps that would have to agree with each other about which is nearer.
		if (AStrategyUnit* Clicked = FindNPCAtLocation(CursorLocation))
		{
			// highlight it, same as a single click, regardless of range
			SetSelectedNPC(Clicked);

			if (IsLootableNPC(Clicked))
			{
				// Downed and Dead are indistinguishable here (see IsLootableNPC) - open it if any
				// player-controlled pawn is close enough, same as the container branch
				if (FindPlayerPawnInRangeOfHolder(Clicked))
				{
					OpenLoot(Clicked);
				}
				else
				{
					NotifyRefusal(ESmoresRefusalReason::TooFar);
				}
			}
			else
			{
				// on its feet: double-click is the "interact with this person" verb. A trader
				// opens trade; a non-trader is where dialog will go; a hostile gets neither.
				InteractWithNPC(Clicked);
			}

			// swallowed either way, in range or not, trader or not - the gesture meant *that
			// person*, not "select everyone". Double-clicking empty ground still selects all.
			return;
		}
	}

	DoSelectAllUnitsOnScreenCommand();
}

void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{
	bIsRotatingCamera = true;
	RotateStartupSkipTicksRemaining = RotateStartupSkipTicks;

	// hide the cursor while rotating
	// NOTE: deliberately not calling SetInputMode here - doing so while a mouse button is
	// actively captured causes Slate to synthesize a release+repress of that same button,
	// which re-triggers this Hold action in a loop (visible as MouseLockMode thrashing in the log)
	bShowMouseCursor = false;

	// re-center the cursor so PlayerTick's own re-centering (below) has a consistent starting
	// point instead of wherever the cursor happened to be when MMB was pressed
	int32 ViewportSizeX = 0, ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);
	SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);

	// discard the delta generated by that warp, plus any stale leftover from the click itself
	// giving the viewport input focus, before rotation starts
	float FlushX = 0.0f, FlushY = 0.0f;
	GetInputMouseDelta(FlushX, FlushY);
}

void AStrategyPlayerController::InteractHoldCompleted(const FInputActionValue& Value)
{
	bIsRotatingCamera = false;

	// restore the cursor
	bShowMouseCursor = true;
}

void AStrategyPlayerController::InteractClick(const FInputActionValue& Value)
{
	// get the cursor location
	FVector CursorLocation;

	// do we have a valid interaction location under the cursor?
	if (GetLocationUnderCursor(CursorLocation))
	{
		// move the selected units to the target location
		DoMoveUnitsCommand(CursorLocation);
	}
}

void AStrategyPlayerController::TouchPrimaryHoldStarted(const FInputActionValue& Value)
{
	// save the camera drag screen coords
	StartingDragScrollPosition = Value.Get<FVector2D>();

	
}

void AStrategyPlayerController::TouchPrimaryHoldTriggered(const FInputActionInstance& Instance)
{
	FVector2D InputVector = Instance.GetValue().Get<FVector2D>();

	// update the box select start position
	StartingBoxSelectionPosition = InputVector;

	if (Instance.GetElapsedTime() > TouchDragScrollHoldTime)
	{
		DoCameraDragScrollCommand(InputVector);

		// save the game time
		LastTouchDragScrollTime = GetWorld()->GetTimeSeconds();
	}
}

void AStrategyPlayerController::TouchPrimaryHoldCompleted(const FInputActionValue& Value)
{
	// ensure we don't trigger a tap input right after we finish a drag scroll
	if (GetWorld()->GetTimeSeconds() - LastTouchDragScrollTime > 0.1f)
	{
		// get the touch location in world space
		FVector TouchLocation = ProjectTouchPointToWorldSpace();

		// try to do a select command
		if (!DoSelectCommand(TouchLocation, true))
		{
			// if nothing was selected, do a move units command instead
			DoMoveUnitsCommand(TouchLocation);
		}
	}
}

void AStrategyPlayerController::TouchSecondaryTriggered(const FInputActionValue& Value)
{
	// get the touch 2 screen coords
	FVector2D SelectionPosition = Value.Get<FVector2D>();

	// calculate the size of the selection box
	FVector2D SelectionSize = SelectionPosition - StartingBoxSelectionPosition;

	// update the selection box on the HUD
	if (StrategyHUD)
	{
		StrategyHUD->DragSelectUpdate(StartingBoxSelectionPosition, SelectionSize, SelectionPosition, true);
	}
		
}

void AStrategyPlayerController::TouchSecondaryCompleted(const FInputActionValue& Value)
{
	if (StrategyHUD)
	{
		// hide the selection box
		StrategyHUD->DragSelectUpdate(FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, false);
	}
}

bool AStrategyPlayerController::DoSelectCommand(const FVector& SelectLocation, bool bAdditiveSelection)
{
	// NOTE: deselecting ControlledUnits happens per-branch below, not unconditionally up front -
	// targeting an NPC or a container must not clear the squad currently selected to command it
	// (otherwise the very click that sets SelectedNPC for an attack command would empty
	// ControlledUnits before DoAttackCommand ever runs). Only a click landing on a player pawn,
	// or one landing on nothing at all, still clears the squad.

	// do an overlap test at the cursor location
	TArray<FOverlapResult> OutOverlaps;

	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SelectionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;

	GetWorld()->OverlapMultiByObjectType(OutOverlaps, SelectLocation, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams);

	// OverlapMultiByObjectType doesn't return results ordered by distance - when two units
	// are close enough together that both fall inside the selection sphere, sort by distance
	// to the click so the nearest one under the cursor is picked, not an arbitrary further one
	OutOverlaps.Sort([&SelectLocation](const FOverlapResult& A, const FOverlapResult& B)
	{
		const AActor* ActorA = A.GetActor();
		const AActor* ActorB = B.GetActor();

		const float DistA = ActorA ? FVector::DistSquared(ActorA->GetActorLocation(), SelectLocation) : TNumericLimits<float>::Max();
		const float DistB = ActorB ? FVector::DistSquared(ActorB->GetActorLocation(), SelectLocation) : TNumericLimits<float>::Max();

		return DistA < DistB;
	});

	// nearest overlapping unit, if any (AStrategyPlayerUnit derives from AStrategyUnit, so this
	// picks up both - which subtype it is gets sorted out below)
	AStrategyUnit* NearestUnit = nullptr;

	for (const FOverlapResult& CurrentOverlap : OutOverlaps)
	{
		if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
		{
			NearestUnit = CurrentUnit;
			break;
		}
	}

	// nearest container within range, if any, so the player can disambiguate which one they
	// mean when several are nearby
	AStrategyContainer* NearestContainer = FindContainerAtLocation(SelectLocation);

	// a unit and a container can both be within range of the same click - a unit was
	// previously always preferred even when the container was visibly closer to the cursor,
	// since the container check only ran as a fallback when no unit overlap was found at all.
	// Compare distances instead so whichever is actually closer to the click wins.
	if (NearestUnit && NearestContainer)
	{
		const float UnitDistSq = FVector::DistSquared(NearestUnit->GetActorLocation(), SelectLocation);
		const float ContainerDistSq = FVector::DistSquared(NearestContainer->GetActorLocation(), SelectLocation);

		if (ContainerDistSq < UnitDistSq)
		{
			NearestUnit = nullptr;
		}
		else
		{
			NearestContainer = nullptr;
		}
	}

	if (NearestUnit)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] DoSelectCommand: click resolved to %s (%s), ControlledUnits.Num()=%d"),
			*NearestUnit->GetName(), Cast<AStrategyPlayerUnit>(NearestUnit) ? TEXT("player pawn") : TEXT("NPC"), ControlledUnits.Num());

		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(NearestUnit))
		{
			// only this controller's own squad is selectable/commandable - another player's unit
			// is inert to click (still counts as "found a unit" so this doesn't fall through to
			// the empty-ground branch and clear this player's own selection)
			if (PlayerUnit->GetOwningController() != this)
			{
				return true;
			}

			// deselect any previously selected units unless this is an additive selection -
			// scoped to this branch since only a player-pawn click should ever clear the squad
			if (!bAdditiveSelection)
			{
				DoDeselectAllUnitsCommand();
			}

			// is this unit already selected?
			if (ControlledUnits.Contains(PlayerUnit))
			{
				// deselect the unit
				ControlledUnits.Remove(PlayerUnit);

				PlayerUnit->UnitDeselected();

				if (LastSelectionTarget.Get() == PlayerUnit)
				{
					LastSelectionTarget = nullptr;
				}
			}
			else
			{
				// select the unit
				ControlledUnits.Add(PlayerUnit);

				PlayerUnit->UnitSelected();

				LastSelectionTarget = PlayerUnit;
			}
		}
		else
		{
			// NPCs are targetable (highlighted, shown in the selection label) but never commandable.
			// Targeting is *all* a click does: it used to also launch an attack when the NPC was
			// already Aggressive, which made one click mean two different things depending on the
			// target's mood, and made double-clicking a hostile trader open their shop and start a
			// fight at once. H attacks the target; this only picks it.
			SetSelectedNPC(NearestUnit);
		}

		return true;
	}

	if (NearestContainer)
	{
		// same reasoning as the NPC branch above - picking a container as a target must not
		// clear the squad currently selected to send there
		SetSelectedContainer(NearestContainer);
		return true;
	}
	else if (!bAdditiveSelection)
	{
		// clicked empty ground - this is the one remaining case that still clears the squad,
		// same as it clears the container/NPC pick
		DoDeselectAllUnitsCommand();

		if (SelectedContainer)
		{
			SetSelectedContainer(nullptr);
		}

		if (SelectedNPC)
		{
			SetSelectedNPC(nullptr);
		}
	}

	// didn't find a unit
	return false;
}

void AStrategyPlayerController::DoSelectAllUnitsOnScreenCommand()
{
	// get all player-controlled units on the level (NPC units are not selectable)
	TArray<AActor*> Units;

	UGameplayStatics::GetAllActorsOfClass(this, AStrategyPlayerUnit::StaticClass(), Units);

	// process each unit
	AStrategyPlayerUnit* LastAdded = nullptr;

	for (AActor* CurrentActor : Units)
	{
		if (AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentActor))
		{
			// only this controller's own squad is selectable - see AStrategyPlayerUnit::ClaimForController
			if (CurrentUnit->GetOwningController() != this)
			{
				continue;
			}

			// is the unit is not already selected, and is on screen?
			if (!ControlledUnits.Contains(CurrentUnit) && CurrentUnit->WasRecentlyRendered(0.2f))
			{
				// select the unit
				ControlledUnits.Add(CurrentUnit);

				CurrentUnit->UnitSelected();

				LastAdded = CurrentUnit;
			}
		}

	}

	// the last unit newly added is the most recently targeted, for the selection label
	if (LastAdded)
	{
		LastSelectionTarget = LastAdded;
	}
}

void AStrategyPlayerController::DoDeselectAllUnitsCommand()
{
	// deselect each unit
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			CurrentUnit->UnitDeselected();
		}
	}

	// clear the selection list
	ControlledUnits.Empty();

	// if a pawn was the most recently targeted, it's no longer selected - clear the label
	if (Cast<AStrategyPlayerUnit>(LastSelectionTarget.Get()))
	{
		LastSelectionTarget = nullptr;
	}

	// nothing is selected, so any open inventory screen is now stale
	CloseInventory();
}

void AStrategyPlayerController::DoToggleSelectAllUnitsCommand()
{
	// do we have units selected?
	if (ControlledUnits.Num() > 0)
	{
		// deselect all units
		DoDeselectAllUnitsCommand();
	}
	else
	{
		// select all units on screen
		DoSelectAllUnitsOnScreenCommand();
	}
}

void AStrategyPlayerController::DoCameraDragScrollCommand(const FVector2D& CurrentCursorPosition)
{
	// subtract the starting position from the cursor to find the on-screen movement delta
	FVector2D MoveDelta = StartingDragScrollPosition - CurrentCursorPosition;

	// rotate the movement delta to match the isometric perspective
	const FRotator IsoRotation(0.0f, -45.0f, 0.0f);

	FVector RotatedDelta = IsoRotation.RotateVector(FVector(MoveDelta.X, MoveDelta.Y, 0.0f));

	// apply drag
	RotatedDelta *= DragMultiplier;

	// apply the offset to the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddActorWorldOffset(RotatedDelta);
	}
}

void AStrategyPlayerController::DoMoveUnitsCommand(const FVector& GoalLocation)
{
	if (ControlledUnits.Num() > 0)
	{
		// find the closest unit to the goal
		AStrategyUnit* ClosestUnit = GetClosestSelectedUnitToLocation(GoalLocation);

		// the actual move is shared-world state, owned by the server - ControlledUnits only
		// exists locally on this (the owning client's) PlayerController instance, so it has to
		// travel explicitly rather than being re-read server-side
		Server_MoveUnits(ControlledUnits, GoalLocation, ClosestUnit);

		// cosmetic/local-only feedback - shown immediately rather than waiting on the round trip
		BP_CursorFeedback(GoalLocation, true);

	}
	else
	{
		// no units selected, so just show negative cursor feedback
		BP_CursorFeedback(GoalLocation, false);
	}
}

void AStrategyPlayerController::DoAttackCommand(AStrategyUnit* Target)
{
	if (!IsValid(Target) || Target->IsIncapacitated())
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] DoAttackCommand bailed early: Target %s"),
			!IsValid(Target) ? TEXT("invalid") : TEXT("already down"));
		return;
	}

	UE_LOG(Logsmores, Warning, TEXT("[Combat] DoAttackCommand(%s): ControlledUnits.Num()=%d"),
		*Target->GetName(), ControlledUnits.Num());

	// see Server_MoveUnits for why ControlledUnits has to travel explicitly rather than being
	// re-read server-side
	Server_AttackCommand(ControlledUnits, Target);
}

void AStrategyPlayerController::Server_MoveUnits_Implementation(const TArray<AStrategyUnit*>& Units, const FVector& GoalLocation, AStrategyUnit* ClosestUnit)
{
	// tell each unit to move to the location
	for (AStrategyUnit* CurrentUnit : Units)
	{
		if (IsValid(CurrentUnit))
		{
			CurrentUnit->MoveToLocation(GoalLocation, CurrentUnit == ClosestUnit, Units);
		}
	}
}

void AStrategyPlayerController::NotifyRefusal(ESmoresRefusalReason Reason)
{
	if (Reason == ESmoresRefusalReason::None)
	{
		return;
	}

	// one shared resolver in SmoresUI, so a widget that decides a rule for itself reaches the
	// same line by the same route - see UInventoryItemWidget::TryEquip. Does nothing without a
	// HUD, which is the correct behaviour on a dedicated server and exactly why server-side code
	// has to come through Client_NotifyRefusal rather than calling this.
	URefusalWidget::RaiseRefusal(this, Reason);

	// A refused action does both: the line at the cursor answers it now, and the feed remembers it
	// for the player who was looking somewhere else. The wording comes from the same resolver, so
	// the two can never disagree about what was refused.
	//
	// Repeats of the same reason inside the window are one event, the same way URefusalWidget
	// treats them as one line - see FeedRefusalRepeatSeconds for why the feed needs the rule more
	// than the line does.
	const double Now = FPlatformTime::Seconds();

	if (LastFeedRefusal == Reason && LastFeedRefusalTime >= 0.0 && (Now - LastFeedRefusalTime) < FeedRefusalRepeatSeconds)
	{
		return;
	}

	LastFeedRefusal = Reason;
	LastFeedRefusalTime = Now;

	PostActivity(EActivityCategory::Squad, EActivitySeverity::Warning, URefusalWidget::GetRefusalText(Reason));
}

void AStrategyPlayerController::Client_NotifyRefusal_Implementation(ESmoresRefusalReason Reason)
{
	NotifyRefusal(Reason);
}

void AStrategyPlayerController::PostActivity(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source)
{
	// null on a controller with no local player, which is every remote player's controller and
	// every controller on a dedicated server - so this does nothing rather than making each
	// caller check. Server-side code that wants a line reaches Client_NotifyActivity instead.
	if (USmoresActivityLog* ActivityLog = USmoresActivityLog::Get(this))
	{
		ActivityLog->Post(Category, Severity, Text, Source);
	}
}

void AStrategyPlayerController::Client_NotifyActivity_Implementation(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source)
{
	PostActivity(Category, Severity, Text, Source);
}

void AStrategyPlayerController::Server_MoveInventoryItem_Implementation(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity)
{
	// A trader's stock is a UInventoryComponent like any other, so a drag in or out of a trade
	// window arrives here as an ordinary move and the UI needed no new gesture at all. What makes
	// it a transaction is recognised on this side, where the gold lives: exactly one end being a
	// trader means money has to change hands too.
	//
	// Source == Dest is excluded deliberately - that is the player repacking one grid, whoever
	// owns it, and nothing is bought or sold by moving an item within a single shelf.
	const bool bEitherSideIsStock = SourceInventory && DestInventory && SourceInventory != DestInventory
		&& (SourceInventory->IsA<UTraderComponent>() || DestInventory->IsA<UTraderComponent>());

	if (bEitherSideIsStock)
	{
		TryTradeItem(SourceInventory, EntryId, DestInventory, DestCell, bRotated, Quantity);
		return;
	}

	// The drop preview has already refused every placement the client could work out for itself,
	// so reaching here with a failure means the client predicted wrong - the grid changed under
	// it between the preview and the drop. Rare, and invisible without this: the item just snaps
	// back onto cells that still look free.
	if (!UInventoryComponent::MoveItem(SourceInventory, EntryId, DestInventory, DestCell, bRotated, Quantity))
	{
		Client_NotifyRefusal(ESmoresRefusalReason::NoRoom);
	}
}

void AStrategyPlayerController::Server_SortInventory_Implementation(UInventoryComponent* Inventory, EInventorySortCriterion Criterion)
{
	if (!Inventory)
	{
		return;
	}

	// nothing to gate beyond authority, which SortEntries checks itself: a repack can only ever
	// rearrange one holder's own contents, so unlike a pickup or a trade there is nothing here
	// for a bad request to take. It works on a trader's shelf and a corpse's pack for the same
	// reason - tidying either one costs nobody anything
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	// an already-sorted grid reports None and says nothing; only an abandoned repack speaks up,
	// because that one is indistinguishable on screen from a grid that was already tidy
	if (!Inventory->SortEntriesWithReason(Criterion, Reason))
	{
		Client_NotifyRefusal(Reason);
	}
}

bool AStrategyPlayerController::TryTradeItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity)
{
	if (!HasAuthority() || !SourceInventory || !DestInventory)
	{
		return false;
	}

	UTraderComponent* SourceStock = Cast<UTraderComponent>(SourceInventory);
	UTraderComponent* DestStock = Cast<UTraderComponent>(DestInventory);

	// one counter, two sides. Two traders would be a transaction between two NPCs, which is not
	// something the player is standing in the middle of - and nobody's wallet would pay for it.
	if ((SourceStock != nullptr) == (DestStock != nullptr))
	{
		return false;
	}

	UTraderComponent* Stock = SourceStock ? SourceStock : DestStock;
	const bool bBuying = (SourceStock != nullptr);

	AStrategyUnit* TraderUnit = Cast<AStrategyUnit>(Stock->GetOwner());

	// re-checked here rather than trusted from the client, for the same reason
	// Server_PickUpWorldItem re-checks its own: MoveItem has no idea how far away the asking pawn
	// was, or what the counterparty thinks of it
	if (!IsInteractableNPC(TraderUnit))
	{
		UE_LOG(Logsmores, Warning, TEXT("[Trade] Refused: %s is not an interactable trader."), *GetNameSafe(TraderUnit));

		Client_NotifyRefusal(ESmoresRefusalReason::NotInteractable);

		return false;
	}

	// split from the check above only so the two can be told apart on screen - "they won't deal
	// with you" and "walk closer" ask for completely different things from the player
	if (!FindPlayerPawnInRangeOfHolder(TraderUnit))
	{
		UE_LOG(Logsmores, Warning, TEXT("[Trade] Refused: no pawn in range of %s."), *GetNameSafe(TraderUnit));

		Client_NotifyRefusal(ESmoresRefusalReason::TooFar);

		return false;
	}

	UWalletComponent* Wallet = GetWallet();

	if (!Wallet)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Trade] Refused: no wallet - is the game mode's PlayerStateClass set to a BP_StrategyPlayerState?"));
		return false;
	}

	const FInventoryEntry SourceEntry = SourceInventory->GetEntry(EntryId);

	if (!SourceEntry.IsValidEntry())
	{
		return false;
	}

	const int32 RequestedQuantity = (Quantity <= 0) ? SourceEntry.Item.Quantity : FMath::Min(Quantity, SourceEntry.Item.Quantity);

	if (bBuying)
	{
		// priced against everything the player asked for, before anything moves. MoveItem may
		// well take less than that (a destination stack cap), and the debit below is for what
		// actually moved - so checking the larger figure first is what guarantees the debit can
		// never fail after the goods have already changed hands. A player who can't cover the
		// whole stack is refused outright rather than quietly sold a smaller pile, since there is
		// no way yet for them to ask for one.
		const int32 MaximumPrice = Stock->GetBuyPrice(SourceEntry.Item, RequestedQuantity);

		if (!Wallet->CanAfford(MaximumPrice))
		{
			UE_LOG(Logsmores, Warning, TEXT("[Trade] REJECTED: %s costs %d, balance %d."),
				*SourceEntry.Item.GetDisplayName().ToString(), MaximumPrice, Wallet->GetGold());

			// the case this whole mechanism was built for: the cells were fine, so there is no
			// red preview to explain it, and the item snapping back looks exactly like a bad drop
			Client_NotifyRefusal(ESmoresRefusalReason::CannotAfford);

			return false;
		}
	}

	int32 QuantityMoved = 0;

	// the goods half. Rejected for any of MoveItem's own reasons (no room, an unstackable
	// collision) it mutates nothing, and neither does the money half below
	if (!UInventoryComponent::MoveItemCounted(SourceInventory, EntryId, DestInventory, DestCell, bRotated, Quantity, QuantityMoved) || QuantityMoved <= 0)
	{
		Client_NotifyRefusal(ESmoresRefusalReason::NoRoom);

		return false;
	}

	if (bBuying)
	{
		const int32 Price = Stock->GetBuyPrice(SourceEntry.Item, QuantityMoved);

		// can only fail if something mutated the balance between the check above and here, which
		// nothing can inside one server call - logged rather than silently ignored because the
		// failure would mean goods handed over for free
		if (!Wallet->TrySpendGold(Price))
		{
			UE_LOG(Logsmores, Error, TEXT("[Trade] Balance moved mid-transaction: %d x %s handed over unpaid."),
				QuantityMoved, *SourceEntry.Item.GetDisplayName().ToString());
		}

		UE_LOG(Logsmores, Warning, TEXT("[Trade] Bought %d x %s for %d, balance %d."),
			QuantityMoved, *SourceEntry.Item.GetDisplayName().ToString(), Price, Wallet->GetGold());

		// COMMS rather than SQUAD: a trade is something that happened with somebody else, and the
		// tab is what lets a player find the price they paid without reading past a fight
		Client_NotifyActivity(EActivityCategory::Comms, EActivitySeverity::Normal,
			FText::Format(NSLOCTEXT("StrategyPlayerController", "ActivityBought", "Bought {0} x {1} for {2} gold"),
				FText::AsNumber(QuantityMoved), SourceEntry.Item.GetDisplayName(), FText::AsNumber(Price)),
			TraderUnit ? TraderUnit->GetHolderDisplayName() : FText::GetEmpty());

		return true;
	}

	const int32 Payment = Stock->GetSellPrice(SourceEntry.Item, QuantityMoved);

	Wallet->AddGold(Payment);

	UE_LOG(Logsmores, Warning, TEXT("[Trade] Sold %d x %s for %d, balance %d."),
		QuantityMoved, *SourceEntry.Item.GetDisplayName().ToString(), Payment, Wallet->GetGold());

	Client_NotifyActivity(EActivityCategory::Comms, EActivitySeverity::Normal,
		FText::Format(NSLOCTEXT("StrategyPlayerController", "ActivitySold", "Sold {0} x {1} for {2} gold"),
			FText::AsNumber(QuantityMoved), SourceEntry.Item.GetDisplayName(), FText::AsNumber(Payment)),
		TraderUnit ? TraderUnit->GetHolderDisplayName() : FText::GetEmpty());

	return true;
}

void AStrategyPlayerController::Server_EquipItem_Implementation(UInventoryComponent* SourceInventory, int32 EntryId, UEquipmentComponent* Equipment, EEquipSlot Slot)
{
	if (!Equipment)
	{
		return;
	}

	// no validation of its own, same as the move RPC - UEquipmentComponent::Equip checks
	// authority, slot matching and room for the displaced item, and mutates nothing if any fails
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	// right-click-to-equip has no preview to refuse it in advance the way a drag does, so this is
	// the only thing standing between "that item isn't armour" and nothing happening at all
	if (!Equipment->EquipWithReason(SourceInventory, EntryId, Slot, Reason))
	{
		Client_NotifyRefusal(Reason);
	}
}

void AStrategyPlayerController::Server_PickUpWorldItem_Implementation(AWorldItem* WorldItem, UInventoryComponent* DestInventory)
{
	if (!IsValid(WorldItem) || !IsValid(DestInventory))
	{
		return;
	}

	// the destination has to be a player pawn's own pack - nothing else is a legal pickup target,
	// and the client picked it
	const AStrategyPlayerUnit* DestPawn = Cast<AStrategyPlayerUnit>(DestInventory->GetOwner());

	if (!DestPawn)
	{
		return;
	}

	// proximity is the whole gate on a pickup, so it gets re-checked here rather than being left
	// to the requesting client, which may have moved (or lied) since
	if (!WorldItem->IsInRangeOf(DestInventory->GetOwner()))
	{
		Client_NotifyRefusal(ESmoresRefusalReason::TooFar);

		return;
	}

	// what was picked up has to be read before TryPickUp, which empties the world item on success
	const FText ItemName = WorldItem->GetItem().GetDisplayName();

	// range was the only thing the client checked, so a full pack is the server's news to break
	if (!WorldItem->TryPickUp(DestInventory))
	{
		Client_NotifyRefusal(ESmoresRefusalReason::NoRoom);

		return;
	}

	// worded here because this is where the item's name is known, and carried to the owning
	// client, which is the only machine with a feed to put it in
	Client_NotifyActivity(EActivityCategory::Squad, EActivitySeverity::Normal,
		FText::Format(NSLOCTEXT("StrategyPlayerController", "ActivityPickedUp", "Picked up {0}"), ItemName),
		DestPawn->GetHolderDisplayName());
}

void AStrategyPlayerController::Server_UnequipItem_Implementation(UEquipmentComponent* Equipment, EEquipSlot Slot, UInventoryComponent* DestInventory)
{
	if (!Equipment)
	{
		return;
	}

	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	// the item stays worn when the grid is full, which on screen is the paperdoll simply
	// ignoring the click
	if (!Equipment->UnequipWithReason(Slot, DestInventory, Reason))
	{
		Client_NotifyRefusal(Reason);
	}
}

AStrategyPlayerState* AStrategyPlayerController::GetStrategyPlayerState() const
{
	// keyed off this controller's own player state - there is no single "the" player in a co-op session
	return GetPlayerState<AStrategyPlayerState>();
}

UWalletComponent* AStrategyPlayerController::GetWallet() const
{
	const AStrategyPlayerState* StrategyPlayerState = GetStrategyPlayerState();

	return StrategyPlayerState ? StrategyPlayerState->GetWallet() : nullptr;
}

UTimePaceComponent* AStrategyPlayerController::GetTimePace() const
{
	// one per session, on the GameState - never a per-player copy, and never a singleton lookup
	// that assumes there is only one of anything else
	AStrategyGameState* StrategyGameState = GetWorld() ? GetWorld()->GetGameState<AStrategyGameState>() : nullptr;

	return StrategyGameState ? StrategyGameState->GetTimePace() : nullptr;
}

UWorldFactionComponent* AStrategyPlayerController::GetWorldFactions() const
{
	// one per session, on the GameState, exactly like the pace
	AStrategyGameState* StrategyGameState = GetWorld() ? GetWorld()->GetGameState<AStrategyGameState>() : nullptr;

	return StrategyGameState ? StrategyGameState->GetWorldFactions() : nullptr;
}

UPlayerStandingComponent* AStrategyPlayerController::GetPlayerStanding() const
{
	// keyed off this controller's own player state - each player has their own reputation
	const AStrategyPlayerState* StrategyPlayerState = GetStrategyPlayerState();

	return StrategyPlayerState ? StrategyPlayerState->GetStanding() : nullptr;
}

void AStrategyPlayerController::SmoresDumpFactions()
{
	Server_DebugFactions(NAME_None, 0);
}

void AStrategyPlayerController::SmoresAdjustStanding(FName FactionId, int32 Delta)
{
	Server_DebugFactions(FactionId, Delta);
}

void AStrategyPlayerController::Server_DebugFactions_Implementation(FName AdjustFactionId, int32 Delta)
{
	UWorldFactionComponent* WorldFactions = GetWorldFactions();
	UPlayerStandingComponent* PlayerStanding = GetPlayerStanding();

	if (!WorldFactions)
	{
		UE_LOG(Logsmores, Warning, TEXT("[FactionDebug] No world faction component - is the game mode's GameStateClass set to a BP_StrategyGameState?"));
		return;
	}

	if (!PlayerStanding)
	{
		UE_LOG(Logsmores, Warning, TEXT("[FactionDebug] No player standing component - is the game mode's PlayerStateClass set to a BP_StrategyPlayerState?"));
		return;
	}

	if (!AdjustFactionId.IsNone())
	{
		const int32 Before = PlayerStanding->GetStanding(AdjustFactionId);
		const bool bAdjusted = PlayerStanding->AdjustStanding(AdjustFactionId, Delta);

		// player standing accepts ids the world doesn't know, on purpose (see the component), so
		// flag a typo here rather than letting it quietly create an entry for nothing
		UE_LOG(Logsmores, Warning, TEXT("[FactionDebug] AdjustStanding(%s, %d) -> %s, %d -> %d%s"),
			*AdjustFactionId.ToString(), Delta, bAdjusted ? TEXT("ok") : TEXT("REJECTED"),
			Before, PlayerStanding->GetStanding(AdjustFactionId),
			WorldFactions->IsKnownFaction(AdjustFactionId) ? TEXT("") : TEXT(" (not a faction this world knows - typo?)"));
	}

	const TArray<FFactionRecord>& Records = WorldFactions->GetRecords();

	UE_LOG(Logsmores, Warning, TEXT("[FactionDebug] %d faction(s):"), Records.Num());

	for (const FFactionRecord& Record : Records)
	{
		const UFactionDefinition* Definition = Cast<UFactionDefinition>(
			USmoresDefinitionLibrary::FindDefinition(UFactionDefinition::DefinitionType, Record.FactionId));

		// current tier beside the authored one, so a faction that has moved reads as having moved
		UE_LOG(Logsmores, Warning, TEXT("[FactionDebug]   %s \"%s\" tier %s (started %s), lineage %s, your standing %d"),
			*Record.FactionId.ToString(),
			Definition ? *Definition->DisplayName.ToString() : TEXT("?"),
			*UEnum::GetDisplayValueAsText(Record.Tier).ToString(),
			Definition ? *UEnum::GetDisplayValueAsText(Definition->StartingTier).ToString() : TEXT("?"),
			Definition ? *UEnum::GetDisplayValueAsText(Definition->LineageStance).ToString() : TEXT("?"),
			PlayerStanding->GetStanding(Record.FactionId));
	}

	const TArray<FFactionPairStanding>& Matrix = WorldFactions->GetStandingMatrix();

	UE_LOG(Logsmores, Warning, TEXT("[FactionDebug] %d stored pair(s) - any pair not listed is 0:"), Matrix.Num());

	for (const FFactionPairStanding& Pair : Matrix)
	{
		UE_LOG(Logsmores, Warning, TEXT("[FactionDebug]   %s <-> %s: %d"), *Pair.FirstId.ToString(), *Pair.SecondId.ToString(), Pair.Standing);
	}

	// entries for ids the world doesn't know would otherwise never be printed by the loop above
	for (const FPlayerFactionStanding& Entry : PlayerStanding->GetStandings())
	{
		if (!WorldFactions->IsKnownFaction(Entry.FactionId))
		{
			UE_LOG(Logsmores, Warning, TEXT("[FactionDebug]   your standing with unknown faction %s: %d"), *Entry.FactionId.ToString(), Entry.Standing);
		}
	}
}

void AStrategyPlayerController::SmoresDumpRecord()
{
	// SelectedNPC and ControlledUnits are client-side input state, so resolve the unit here and
	// hop to the server with the actor - the same shape as the kill exec
	AStrategyUnit* Unit = IsValid(SelectedNPC) ? SelectedNPC.Get() : nullptr;

	if (!Unit)
	{
		for (AStrategyUnit* CurrentUnit : ControlledUnits)
		{
			if (IsValid(CurrentUnit))
			{
				Unit = CurrentUnit;
				break;
			}
		}
	}

	if (!Unit)
	{
		UE_LOG(Logsmores, Warning, TEXT("[RecordDebug] No unit - target an NPC or select a pawn first."));
		return;
	}

	Server_DebugRecord(Unit);
}

/** One "record vs live" line for the record dump, flagged when the two disagree */
static void SmoresLogRecordField(const TCHAR* Field, const FString& RecordValue, const FString& LiveValue)
{
	const bool bMatch = RecordValue == LiveValue;

	UE_LOG(Logsmores, Warning, TEXT("[RecordDebug]   %-10s record %-28s live %s%s"),
		Field, *RecordValue, *LiveValue, bMatch ? TEXT("") : TEXT("   <-- MISMATCH"));
}

/** A grid or paperdoll as one comparable line: "id@x,y name xN" per entry, in stored order */
static FString SmoresDescribeCarried(const TArray<FInventoryEntry>& Entries)
{
	TArray<FString> Parts;

	for (const FInventoryEntry& Entry : Entries)
	{
		Parts.Add(FString::Printf(TEXT("%d@%d,%d%s %s x%d"), Entry.EntryId, Entry.AnchorCell.X, Entry.AnchorCell.Y,
			Entry.bRotated ? TEXT("r") : TEXT(""), *Entry.Item.GetDisplayName().ToString(), Entry.Item.Quantity));
	}

	return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : FString(TEXT("(empty)"));
}

static FString SmoresDescribeEquipped(const TArray<FEquippedItem>& Items)
{
	TArray<FString> Parts;

	for (const FEquippedItem& Worn : Items)
	{
		Parts.Add(FString::Printf(TEXT("%s: %s"), *UEquipmentComponent::GetSlotDisplayName(Worn.Slot).ToString(), *Worn.Item.GetDisplayName().ToString()));
	}

	return Parts.Num() > 0 ? FString::Join(Parts, TEXT(", ")) : FString(TEXT("(nothing)"));
}

void AStrategyPlayerController::Server_DebugRecord_Implementation(AStrategyUnit* Unit)
{
	if (!IsValid(Unit))
	{
		return;
	}

	const UCharacterRecordComponent* Store = UCharacterRecordComponent::Get(this);
	const FCharacterRecord* Record = Unit->GetRecord();

	if (!Store)
	{
		UE_LOG(Logsmores, Warning, TEXT("[RecordDebug] No record store - is the game mode's GameStateClass set to a BP_StrategyGameState?"));
		return;
	}

	if (!Record)
	{
		UE_LOG(Logsmores, Warning, TEXT("[RecordDebug] %s has no record (see the log from its BeginPlay for why)."), *Unit->GetName());
		return;
	}

	const UCharacterDefinition* Definition = Unit->GetCharacterDefinition();

	UE_LOG(Logsmores, Warning, TEXT("[RecordDebug] %s - record %s of %d, bound to %s"),
		*Unit->GetName(), *Record->RecordId.ToString(), Store->GetRecords().Num(), *GetNameSafe(Store->GetBoundActor(Record->RecordId)));

	UE_LOG(Logsmores, Warning, TEXT("[RecordDebug]   definition %s%s, faction %s, role %s"),
		*Record->DefinitionId.ToString(),
		(Definition && Definition->bUnique) ? TEXT(" (unique)") : TEXT(""),
		*Record->FactionId.ToString(),
		Definition ? *Definition->RoleId.ToString() : TEXT("-"));

	const FCharacterAttributes& Attributes = Record->Attributes;

	UE_LOG(Logsmores, Warning, TEXT("[RecordDebug]   STR %.1f END %.1f AGI %.1f PER %.1f INT %.1f WIL %.1f CHA %.1f"),
		Attributes.Strength, Attributes.Endurance, Attributes.Agility, Attributes.Perception,
		Attributes.Intelligence, Attributes.Willpower, Attributes.Charisma);

	// identity is copied record -> actor; condition is written back actor -> record. Every line
	// below should match - a MISMATCH is a change that didn't reach the record.
	SmoresLogRecordField(TEXT("Name"), Record->Name.ToString(), Unit->GetHolderDisplayName().ToString());
	SmoresLogRecordField(TEXT("Faction"), Record->FactionId.ToString(), Unit->GetFactionId().ToString());

	const UHealthComponent* LiveHealth = Unit->GetHealth();

	SmoresLogRecordField(TEXT("Health"), FString::Printf(TEXT("%.1f"), Record->Health), FString::Printf(TEXT("%.1f"), LiveHealth->GetHealth()));
	SmoresLogRecordField(TEXT("LifeState"), UEnum::GetValueAsString(Record->LifeState), UEnum::GetValueAsString(LiveHealth->GetHealthState()));

	// location is only written on arrival and on other write-backs, so a unit mid-walk will
	// legitimately disagree here - compare it after the unit stops
	SmoresLogRecordField(TEXT("Location"), Record->LastKnownLocation.ToCompactString(), Unit->GetActorLocation().ToCompactString());

	SmoresLogRecordField(TEXT("Carried"), SmoresDescribeCarried(Record->Carried), SmoresDescribeCarried(Unit->GetInventory()->GetEntries()));
	SmoresLogRecordField(TEXT("Equipped"), SmoresDescribeEquipped(Record->Equipped), SmoresDescribeEquipped(Unit->GetEquipment()->GetEquippedItems()));
}

void AStrategyPlayerController::SmoresAddGold(int32 Amount)
{
	Server_DebugGold(Amount, /*bSpend =*/ false);
}

void AStrategyPlayerController::SmoresSpendGold(int32 Amount)
{
	Server_DebugGold(Amount, /*bSpend =*/ true);
}

void AStrategyPlayerController::Server_DebugGold_Implementation(int32 Amount, bool bSpend)
{
	UWalletComponent* Wallet = GetWallet();

	if (!Wallet)
	{
		UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] No wallet - is the game mode's PlayerStateClass set to a BP_StrategyPlayerState?"));
		return;
	}

	if (bSpend)
	{
		const bool bSpent = Wallet->TrySpendGold(Amount);

		UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] TrySpendGold(%d) -> %s, balance %d"),
			Amount, bSpent ? TEXT("paid") : TEXT("REJECTED"), Wallet->GetGold());

		return;
	}

	Wallet->AddGold(Amount);

	UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] AddGold(%d), balance %d"), Amount, Wallet->GetGold());
}

void AStrategyPlayerController::SmoresDumpInventory()
{
	SmoresAddItem(0, NAME_None);
}

void AStrategyPlayerController::SmoresDumpDefinitions()
{
	TArray<FPrimaryAssetType> DefinitionTypes;
	USmoresDefinitionLibrary::GetDefinitionTypes(DefinitionTypes);

	if (DefinitionTypes.Num() == 0)
	{
		UE_LOG(Logsmores, Warning, TEXT("[DefDebug] The Asset Manager has no Smores definition types registered - check PrimaryAssetTypesToScan in Config/DefaultGame.ini."));

		return;
	}

	for (const FPrimaryAssetType& DefinitionType : DefinitionTypes)
	{
		TArray<FName> DefinitionIds;
		USmoresDefinitionLibrary::GetDefinitionIds(DefinitionType, DefinitionIds);

		// the registry hands them back in scan order, which is neither stable nor readable
		DefinitionIds.Sort(FNameLexicalLess());

		UE_LOG(Logsmores, Warning, TEXT("[DefDebug] %s: %d definition(s)"), *DefinitionType.ToString(), DefinitionIds.Num());

		for (const FName& DefinitionId : DefinitionIds)
		{
			const USmoresDefinition* Definition = USmoresDefinitionLibrary::FindDefinition(DefinitionType, DefinitionId);

			// an id the registry knows but that won't load is exactly what a broken config or a
			// stripped mod looks like, so say so rather than skipping the line
			if (!Definition)
			{
				UE_LOG(Logsmores, Warning, TEXT("[DefDebug]   %s -> FAILED TO RESOLVE"), *DefinitionId.ToString());

				continue;
			}

			UE_LOG(Logsmores, Warning, TEXT("[DefDebug]   %s \"%s\" (%s)"),
				*DefinitionId.ToString(), *Definition->DisplayName.ToString(), *Definition->GetName());
		}
	}
}

void AStrategyPlayerController::SmoresRollTable(FName TableId, int32 Seed, int32 Count)
{
	const ULootTableDefinition* Table = Cast<ULootTableDefinition>(
		USmoresDefinitionLibrary::FindDefinition(ULootTableDefinition::DefinitionType, TableId));

	if (!Table)
	{
		UE_LOG(Logsmores, Warning, TEXT("[LootDebug] No loot table with id '%s' - run SmoresDumpDefinitions for the list."), *TableId.ToString());
		return;
	}

	// gathered once for every roll rather than once per roll - the same candidates either way
	const TArray<UItemDefinition*> TagCandidates = ULootTableDefinition::GatherRegisteredItems();

	// the tally is keyed by composed name, so a bronze and an iron knife count separately - which
	// is also how they stack
	TMap<FString, int32> Tally;
	int32 TotalItems = 0;

	Count = FMath::Clamp(Count, 1, 1000);

	for (int32 RollIndex = 0; RollIndex < Count; ++RollIndex)
	{
		// a plain stream from the seed as typed, not MakeRollStream - this samples the table, not
		// any one chest in the world
		FRandomStream Stream(Seed + RollIndex);

		TArray<FInventoryItem> Rolled;
		TArray<FName> SourceTableIds;

		Table->RollLoot(Stream, TagCandidates, Rolled, &SourceTableIds);

		// one roll's detail is useful; a hundred rolls' detail buries the tally it's there to produce
		const bool bPrintDetail = Count <= 10;

		if (bPrintDetail)
		{
			UE_LOG(Logsmores, Warning, TEXT("[LootDebug] %s, seed %d: %d item(s)"), *TableId.ToString(), Seed + RollIndex, Rolled.Num());
		}

		for (int32 ItemIndex = 0; ItemIndex < Rolled.Num(); ++ItemIndex)
		{
			const FInventoryItem& Item = Rolled[ItemIndex];
			const FString Name = Item.GetDisplayName().ToString();

			Tally.FindOrAdd(Name) += Item.Quantity;
			TotalItems += Item.Quantity;

			if (bPrintDetail)
			{
				UE_LOG(Logsmores, Warning, TEXT("[LootDebug]   %s x%d  (from %s)  %.2f weight, %d gold each"),
					*Name, Item.Quantity, *SourceTableIds[ItemIndex].ToString(), Item.GetUnitWeight(), Item.GetUnitBaseValue());
			}
		}
	}

	if (Count == 1)
	{
		return;
	}

	Tally.ValueSort([](int32 A, int32 B) { return A > B; });

	UE_LOG(Logsmores, Warning, TEXT("[LootDebug] %s across %d rolls (seeds %d-%d): %d unit(s), %.1f per roll"),
		*TableId.ToString(), Count, Seed, Seed + Count - 1, TotalItems, static_cast<float>(TotalItems) / Count);

	for (const TPair<FString, int32>& Line : Tally)
	{
		UE_LOG(Logsmores, Warning, TEXT("[LootDebug]   %5d  %s"), Line.Value, *Line.Key);
	}
}

void AStrategyPlayerController::SmoresAddItem(int32 Count, FName ModifierId)
{
	// the exec runs wherever the console was typed, but the grid it wants to inspect and mutate
	// only authoritatively exists on the server - so resolve the pawn locally and hop across
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			Server_DebugInventory(PlayerUnit->GetInventory(), Count, ModifierId);
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No player pawn selected."));
}

void AStrategyPlayerController::Server_DebugInventory_Implementation(UInventoryComponent* Inventory, int32 AddCount, FName ModifierId)
{
	if (!Inventory)
	{
		UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No inventory to inspect."));
		return;
	}

	if (AddCount > 0)
	{
		// re-add whatever's already placed first, so this needs no item-id lookup path;
		// AddItem then exercises stack-merge, auto-placement and the rotation fallback
		const TArray<FInventoryEntry>& Existing = Inventory->GetEntries();

		if (Existing.IsEmpty())
		{
			UE_LOG(Logsmores, Warning, TEXT("[InvDebug] Inventory is empty - nothing to duplicate. Give the unit a character definition with a DefaultLoadout first."));
		}
		else
		{
			FInventoryItem ItemToAdd(Existing[0].Item.Definition, AddCount);

			// an id that resolves to nothing is worth saying out loud rather than silently adding
			// a bare item - a typo'd modifier would otherwise look exactly like one that did nothing
			if (ModifierId != NAME_None)
			{
				UItemModifierDefinition* Modifier = Cast<UItemModifierDefinition>(
					USmoresDefinitionLibrary::FindDefinition(UItemModifierDefinition::DefinitionType, ModifierId));

				if (!Modifier)
				{
					UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No item modifier with id '%s' - run SmoresDumpDefinitions for the list."),
						*ModifierId.ToString());

					return;
				}

				ItemToAdd.AddModifier(Modifier);
			}

			const bool bAddedAll = Inventory->AddItem(ItemToAdd);

			UE_LOG(Logsmores, Warning, TEXT("[InvDebug] AddItem(%d x '%s', %.1f kg, %d gold each) -> %s"),
				AddCount, *ItemToAdd.GetDisplayName().ToString(), ItemToAdd.GetUnitWeight(), ItemToAdd.GetUnitBaseValue(),
				bAddedAll ? TEXT("all placed") : TEXT("PARTIAL/FAILED"));
		}
	}

	LogInventoryGrid(Inventory);
}

void AStrategyPlayerController::SmoresSortInventory(int32 Criterion)
{
	// same shape as the other inventory execs: resolve the pawn from the local selection, then
	// hop to the server, where the authoritative grid lives
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			const int32 ClampedCriterion = FMath::Clamp(Criterion, 0, static_cast<int32>(EInventorySortCriterion::Quantity));

			Server_DebugSortInventory(PlayerUnit->GetInventory(), static_cast<EInventorySortCriterion>(ClampedCriterion));
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No player pawn selected."));
}

void AStrategyPlayerController::Server_DebugSortInventory_Implementation(UInventoryComponent* Inventory, EInventorySortCriterion Criterion)
{
	if (!Inventory)
	{
		UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No inventory to sort."));
		return;
	}

	const bool bSorted = Inventory->SortEntries(Criterion);

	UE_LOG(Logsmores, Warning, TEXT("[InvDebug] SortEntries(%s) -> %s"),
		*UEnum::GetDisplayValueAsText(Criterion).ToString(),
		bSorted ? TEXT("repacked") : TEXT("no change (already sorted, empty, or nothing would fit)"));

	LogInventoryGrid(Inventory);
}

void AStrategyPlayerController::LogInventoryGrid(UInventoryComponent* Inventory)
{
	if (!Inventory)
	{
		return;
	}

	const FIntPoint GridSize = Inventory->GetGridSize();
	const TArray<FInventoryEntry>& Entries = Inventory->GetEntries();

	UE_LOG(Logsmores, Warning, TEXT("[InvDebug] %s: %dx%d grid, %d entries, %d/%d cells free, weight %.2f/%.2f%s"),
		*GetNameSafe(Inventory->GetOwner()), GridSize.X, GridSize.Y, Entries.Num(),
		Inventory->GetFreeCellCount(), GridSize.X * GridSize.Y,
		Inventory->GetTotalWeight(), Inventory->GetWeightCapacity(),
		Inventory->IsOverWeightCapacity() ? TEXT(" OVER") : TEXT(""));

	// occupancy map: one character per cell, indexing into the entry list below
	for (int32 Row = 0; Row < GridSize.Y; ++Row)
	{
		FString RowText;

		for (int32 Column = 0; Column < GridSize.X; ++Column)
		{
			const int32 EntryIndex = Entries.IndexOfByPredicate([Cell = FIntPoint(Column, Row)](const FInventoryEntry& Entry)
			{
				return Entry.CoversCell(Cell);
			});

			RowText.AppendChar(EntryIndex == INDEX_NONE ? TEXT('.') : TCHAR(TEXT('a') + (EntryIndex % 26)));
		}

		UE_LOG(Logsmores, Warning, TEXT("[InvDebug]   |%s|"), *RowText);
	}

	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		const FInventoryEntry& Entry = Entries[EntryIndex];
		const FIntPoint Footprint = Entry.GetFootprint();

		UE_LOG(Logsmores, Warning, TEXT("[InvDebug]   %c: id=%d %s x%d @ (%d,%d) %dx%d%s (cap %d)"),
			TCHAR(TEXT('a') + (EntryIndex % 26)), Entry.EntryId, *GetNameSafe(Entry.Item.Definition), Entry.Item.Quantity,
			Entry.AnchorCell.X, Entry.AnchorCell.Y, Footprint.X, Footprint.Y,
			Entry.bRotated ? TEXT(" rotated") : TEXT(""), Inventory->GetEffectiveMaxStackForItem(Entry.Item));
	}
}

void AStrategyPlayerController::SmoresEquipItem(int32 EntryIndex)
{
	DebugEquipmentForSelection(EntryIndex, INDEX_NONE);
}

void AStrategyPlayerController::SmoresUnequipItem(int32 SlotIndex)
{
	DebugEquipmentForSelection(INDEX_NONE, SlotIndex);
}

void AStrategyPlayerController::SmoresKillNPC()
{
	// SelectedNPC is client-side input state, so resolve it here and hop to the server with the
	// actor - the same shape as the equipment execs
	if (!IsValid(SelectedNPC))
	{
		UE_LOG(Logsmores, Warning, TEXT("[KillDebug] No NPC targeted - click one first."));
		return;
	}

	Server_DebugKill(SelectedNPC);
}

void AStrategyPlayerController::Server_DebugKill_Implementation(AStrategyUnit* Target)
{
	if (!IsValid(Target) || !Target->GetHealth())
	{
		return;
	}

	UE_LOG(Logsmores, Warning, TEXT("[KillDebug] Killing %s"), *Target->GetHolderDisplayName().ToString());

	Target->GetHealth()->Kill();
}

void AStrategyPlayerController::SmoresDumpEquipment()
{
	DebugEquipmentForSelection(INDEX_NONE, INDEX_NONE);
}

void AStrategyPlayerController::SmoresDumpTrader()
{
	DebugTradeForSelection(INDEX_NONE, INDEX_NONE);
}

void AStrategyPlayerController::SmoresBuyItem(int32 EntryIndex)
{
	DebugTradeForSelection(EntryIndex, INDEX_NONE);
}

void AStrategyPlayerController::SmoresSellItem(int32 EntryIndex)
{
	DebugTradeForSelection(INDEX_NONE, EntryIndex);
}

void AStrategyPlayerController::DebugTradeForSelection(int32 BuyEntryIndex, int32 SellEntryIndex)
{
	// SelectedNPC is client-side input state, exactly like the kill exec's, and the authoritative
	// stock/grid/balance all live on the server - so resolve both ends here and hop across
	if (!IsValid(SelectedNPC))
	{
		UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] No NPC targeted - click one first."));
		return;
	}

	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			Server_DebugTrade(SelectedNPC, PlayerUnit->GetInventory(), BuyEntryIndex, SellEntryIndex);
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] No player pawn selected."));
}

bool AStrategyPlayerController::DebugTradeEntry(UInventoryComponent* SourceInventory, UInventoryComponent* DestInventory, int32 EntryIndex)
{
	const TArray<FInventoryEntry>& Entries = SourceInventory->GetEntries();

	if (!Entries.IsValidIndex(EntryIndex))
	{
		UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] No entry at index %d (%d placed)."), EntryIndex, Entries.Num());
		return false;
	}

	const FInventoryEntry Entry = Entries[EntryIndex];

	// the real path always carries the cell the player dropped on; an exec has no pointer, so it
	// picks any cell that fits and then runs the ordinary transaction unchanged
	FIntPoint DestCell = FIntPoint::ZeroValue;
	bool bDestRotated = false;

	if (!DestInventory->FindFreePlacement(Entry.Item, DestCell, bDestRotated))
	{
		UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] No room for %s in the destination grid."), *Entry.Item.GetDisplayName().ToString());
		return false;
	}

	return TryTradeItem(SourceInventory, Entry.EntryId, DestInventory, DestCell, bDestRotated, 0);
}

void AStrategyPlayerController::Server_DebugTrade_Implementation(AStrategyUnit* TraderUnit, UInventoryComponent* PawnInventory, int32 BuyEntryIndex, int32 SellEntryIndex)
{
	UTraderComponent* Stock = GetTraderStock(TraderUnit);

	if (!Stock || !PawnInventory)
	{
		UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] %s is not a trader the player can deal with right now."), *GetNameSafe(TraderUnit));
		return;
	}

	if (BuyEntryIndex >= 0)
	{
		DebugTradeEntry(Stock, PawnInventory, BuyEntryIndex);
	}

	if (SellEntryIndex >= 0)
	{
		DebugTradeEntry(PawnInventory, Stock, SellEntryIndex);
	}

	const UWalletComponent* Wallet = GetWallet();

	UE_LOG(Logsmores, Warning, TEXT("[TradeDebug] %s: %d stocked entries, buy x%.2f / sell x%.2f, player balance %d"),
		*TraderUnit->GetHolderDisplayName().ToString(), Stock->GetEntries().Num(),
		Stock->BuyMarkup, Stock->SellMarkdown, Wallet ? Wallet->GetGold() : 0);

	int32 Index = 0;

	for (const FInventoryEntry& Entry : Stock->GetEntries())
	{
		UE_LOG(Logsmores, Warning, TEXT("[TradeDebug]   [%d] %s - buy %d each (%d total), sell %d each"),
			Index++,
			*UInventoryWidget::GetItemLabel(Entry.Item).ToString(),
			Stock->GetUnitBuyPrice(Entry.Item),
			Stock->GetBuyPrice(Entry.Item, Entry.Item.Quantity),
			Stock->GetUnitSellPrice(Entry.Item));
	}
}

void AStrategyPlayerController::SmoresDropItem(int32 EntryIndex)
{
	// same shape as the other item execs: resolve the pawn from the local selection, then hop to
	// the server, which owns both the grid being emptied and the actor being spawned
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			Server_DebugDropItem(PlayerUnit, PlayerUnit->GetInventory(), EntryIndex);
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[DropDebug] No player pawn selected."));
}

void AStrategyPlayerController::Server_DebugDropItem_Implementation(APawn* DroppingPawn, UInventoryComponent* Inventory, int32 EntryIndex)
{
	if (!DroppingPawn || !Inventory)
	{
		UE_LOG(Logsmores, Warning, TEXT("[DropDebug] No pawn/inventory to drop from."));
		return;
	}

	if (!WorldItemClass)
	{
		UE_LOG(Logsmores, Warning, TEXT("[DropDebug] No WorldItemClass assigned on %s - set it on the player controller Blueprint."), *GetName());
		return;
	}

	const TArray<FInventoryEntry>& Entries = Inventory->GetEntries();

	if (!Entries.IsValidIndex(EntryIndex))
	{
		UE_LOG(Logsmores, Warning, TEXT("[DropDebug] No grid entry at index %d (%d placed)."), EntryIndex, Entries.Num());
		return;
	}

	// copy before removing - the entry reference dies with it
	const FInventoryItem Dropped = Entries[EntryIndex].Item;
	const int32 EntryId = Entries[EntryIndex].EntryId;

	// place it on the ground just in front of the pawn rather than inside it
	constexpr float DropDistance = 120.0f;

	const UCapsuleComponent* Capsule = DroppingPawn->FindComponentByClass<UCapsuleComponent>();
	const float FootOffset = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.0f;

	const FVector DropLocation = DroppingPawn->GetActorLocation()
		+ DroppingPawn->GetActorForwardVector() * DropDistance
		- FVector(0.0f, 0.0f, FootOffset);

	// spawn first: if the world refuses the actor, the item stays safely in the grid
	AWorldItem* Spawned = AWorldItem::SpawnWorldItem(this, WorldItemClass, Dropped, DropLocation, FRotator::ZeroRotator);

	if (!Spawned)
	{
		UE_LOG(Logsmores, Warning, TEXT("[DropDebug] Failed to spawn %s for '%s'."), *GetNameSafe(WorldItemClass), *GetNameSafe(Dropped.Definition));
		return;
	}

	Inventory->RemoveEntry(EntryId);

	UE_LOG(Logsmores, Warning, TEXT("[DropDebug] Dropped %d x '%s' from %s at %s."),
		Dropped.Quantity, *GetNameSafe(Dropped.Definition), *GetNameSafe(Inventory->GetOwner()), *DropLocation.ToCompactString());
}

void AStrategyPlayerController::DebugEquipmentForSelection(int32 EquipEntryIndex, int32 UnequipSlotIndex)
{
	// same shape as the inventory execs: resolve the pawn from the local selection, then hop to
	// the server, where the authoritative equipment and grid actually live
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			Server_DebugEquipment(PlayerUnit->GetEquipment(), PlayerUnit->GetInventory(), EquipEntryIndex, UnequipSlotIndex);
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] No player pawn selected."));
}

void AStrategyPlayerController::Server_DebugEquipment_Implementation(UEquipmentComponent* Equipment, UInventoryComponent* Inventory, int32 EquipEntryIndex, int32 UnequipSlotIndex)
{
	if (!Equipment || !Inventory)
	{
		UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] No equipment/inventory to inspect."));
		return;
	}

	const TArray<EEquipSlot> AllSlots = UEquipmentComponent::GetAllEquipSlots();

	if (EquipEntryIndex >= 0)
	{
		const TArray<FInventoryEntry>& Entries = Inventory->GetEntries();

		if (!Entries.IsValidIndex(EquipEntryIndex))
		{
			UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] No grid entry at index %d (%d placed)."), EquipEntryIndex, Entries.Num());
		}
		else
		{
			const FInventoryEntry Entry = Entries[EquipEntryIndex];

			// EEquipSlot::None exercises the same "wherever it belongs" path right-click uses
			const bool bEquipped = Equipment->Equip(Inventory, Entry.EntryId, EEquipSlot::None);

			UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] Equip(%s) -> %s"),
				*GetNameSafe(Entry.Item.Definition), bEquipped ? TEXT("worn") : TEXT("REJECTED"));
		}
	}

	if (UnequipSlotIndex >= 0)
	{
		if (!AllSlots.IsValidIndex(UnequipSlotIndex))
		{
			UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] No slot at index %d (%d slots)."), UnequipSlotIndex, AllSlots.Num());
		}
		else
		{
			const EEquipSlot Slot = AllSlots[UnequipSlotIndex];
			const bool bRemoved = Equipment->Unequip(Slot, Inventory);

			UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] Unequip(%s) -> %s"),
				*UEquipmentComponent::GetSlotDisplayName(Slot).ToString(), bRemoved ? TEXT("stowed") : TEXT("REJECTED"));
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[EquipDebug] %s: %d worn, equipped weight %.2f"),
		*GetNameSafe(Equipment->GetOwner()), Equipment->GetEquippedItems().Num(), Equipment->GetTotalWeight());

	for (int32 SlotIndex = 0; SlotIndex < AllSlots.Num(); ++SlotIndex)
	{
		const FInventoryItem Worn = Equipment->GetEquippedItem(AllSlots[SlotIndex]);

		UE_LOG(Logsmores, Warning, TEXT("[EquipDebug]   %d %s: %s"),
			SlotIndex, *UEquipmentComponent::GetSlotDisplayName(AllSlots[SlotIndex]).ToString(),
			Worn.IsEmpty() ? TEXT("(empty)") : *GetNameSafe(Worn.Definition));
	}
}

void AStrategyPlayerController::Server_AttackCommand_Implementation(const TArray<AStrategyUnit*>& Units, AStrategyUnit* Target)
{
	if (!IsValid(Target) || Target->IsIncapacitated())
	{
		return;
	}

	// harmless if already Aggressive - this is what flips a Passive NPC on the A-key path
	Target->SetAggressive(true);

	// a squad-wide engage - every selected unit attacks the same target, unlike
	// DoMoveUnitsCommand's spread-to-nearby-points formation logic
	for (AStrategyUnit* CurrentUnit : Units)
	{
		if (IsValid(CurrentUnit))
		{
			UE_LOG(Logsmores, Warning, TEXT("[Combat] Server_AttackCommand: commanding %s to attack %s"),
				*CurrentUnit->GetName(), *Target->GetName());
			CurrentUnit->AttackTarget(Target);
		}
	}
}

void AStrategyPlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	// add the delta
	// NOTE: unclamped (no cap) for feel-testing - MinZoomLevel/MaxZoomLevel are still used by
	// the touch percentage API (DoCameraSetZoomPercentageCommand/GetDefaultZoomPercentage), just
	// not by this desktop mouse-wheel path; re-introduce a clamp here if a range is wanted later
	CameraZoom += ZoomDelta;

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

void AStrategyPlayerController::DoCameraResetZoomCommand()
{
	// reset to default zoom
	CameraZoom = DefaultZoom;

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

void AStrategyPlayerController::DoCameraSetZoomPercentageCommand(float Percentage)
{
	// lerp between min and max zoom
	CameraZoom = FMath::Lerp(MinZoomLevel, MaxZoomLevel, FMath::Clamp(Percentage, 0.0f, 1.0f));

	// set the zoom on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetZoomModifier(CameraZoom);
	}
}

void AStrategyPlayerController::DoCameraRotateCommand(const FVector2D& MouseDelta)
{
	if (!ControlledCameraPawn)
	{
		return;
	}

	// compose via quaternions (world-space yaw, camera-local-space pitch) rather than editing
	// Yaw/Pitch as independent Euler fields - a naive Euler update can't pass smoothly through
	// +/-90 degrees pitch (a real loop needs roll to emerge there) and gimbal-locks near vertical
	// NOTE: unconstrained (no clamp) for feel-testing - MinCameraPitch/MaxCameraPitch are unused
	// right now, re-introduce a clamp once a range is settled on
	//
	// composed from the camera's own current relative rotation, not GetControlRotation() - the
	// engine can reset ControlRotation independently of the camera's actual orientation (confirmed
	// via logging: it read back as an exact zero rotator moments after OnPossess had set it to
	// match the camera), so rebasing off ControlRotation here is what caused the camera to snap to
	// a wildly different (sky-facing) orientation the instant a rotate drag started
	const FQuat CurrentQuat = ControlledCameraPawn->GetCamera()->GetRelativeRotation().Quaternion();

	const FQuat YawDelta(FVector::UpVector, FMath::DegreesToRadians(MouseDelta.X * CameraYawSpeed));
	const FQuat PitchDelta(CurrentQuat.GetRightVector(), FMath::DegreesToRadians(-MouseDelta.Y * CameraPitchSpeed));

	const FRotator NewRotation = (YawDelta * PitchDelta * CurrentQuat).GetNormalized().Rotator();

	SetControlRotation(NewRotation);
	ControlledCameraPawn->SetCameraRotation(NewRotation);
}

void AStrategyPlayerController::DoCameraModifyHeightCommand(float HeightDelta)
{
	// add the delta
	CameraHeight += HeightDelta;

	// clamp between min and max
	CameraHeight = FMath::Clamp(CameraHeight, MinCameraHeight, MaxCameraHeight);

	// set the height on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetHeight(CameraHeight);
	}
}

void AStrategyPlayerController::DoCameraResetHeightCommand()
{
	// reset to default height
	CameraHeight = DefaultCameraHeight;

	// set the height on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetHeight(CameraHeight);
	}
}

void AStrategyPlayerController::DoCameraResetRotationCommand()
{
	// reset to the default pitch and the pawn's original resting yaw
	FRotator NewRotation(DefaultCameraPitch, DefaultCameraYaw, 0.0f);

	SetControlRotation(NewRotation);

	// set the rotation on the camera pawn
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->SetCameraRotation(NewRotation);
	}
}

AStrategyUnit* AStrategyPlayerController::GetClosestSelectedUnitToLocation(FVector TargetLocation)
{
	// closest unit and distance
	AStrategyUnit* OutUnit = nullptr;
	float Closest = 0.0f;

	// process each unit on the list
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (IsValid(CurrentUnit))
		{
			// have we selected a unit already?
			if (OutUnit != nullptr)
			{
				// calculate the squared distance to the target location
				float Dist = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());

				// is this unit closer?
				if (Dist < Closest)
				{
					// update the closest unit and distance
					OutUnit = CurrentUnit;
					Closest = Dist;
				}

			}
			else
			{

				// no previously selected unit, so use this one
				OutUnit = CurrentUnit;

				// initialize the closest distance
				Closest = FVector::DistSquared2D(TargetLocation, CurrentUnit->GetActorLocation());
			}
		}
		
	}

	// return the selected unit
	return OutUnit;
}

AActor* AStrategyPlayerController::FindHolderActorAtLocation(TSubclassOf<AActor> HolderClass, const FVector& Location, float Radius, TFunctionRef<bool(const AActor*)> Filter) const
{
	if (!HolderClass)
	{
		return nullptr;
	}

	// gathers every actor of the class, which picks up subclasses too (every AStrategyContainer
	// subclass, every AStrategyUnit subclass)
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), HolderClass, FoundActors);

	AActor* Nearest = nullptr;
	float NearestDistSq = FMath::Square(Radius);

	for (AActor* CurrentActor : FoundActors)
	{
		if (!IsValid(CurrentActor) || !Filter(CurrentActor))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(CurrentActor->GetActorLocation(), Location);

		if (DistSq <= NearestDistSq)
		{
			Nearest = CurrentActor;
			NearestDistSq = DistSq;
		}
	}

	return Nearest;
}

bool AStrategyPlayerController::IsHolderInRangeOfSelection(const AActor* HolderActor) const
{
	return IsHolderInRangeOfUnits(HolderActor, ControlledUnits);
}

bool AStrategyPlayerController::IsHolderInRangeOfUnits(const AActor* HolderActor, const TArray<AStrategyUnit*>& Units)
{
	const IInventoryHolder* Holder = Cast<IInventoryHolder>(HolderActor);

	if (!Holder)
	{
		// an actor that isn't a holder has no reach to be inside of, so it is never in range
		return false;
	}

	for (AStrategyUnit* CurrentUnit : Units)
	{
		if (IsValid(CurrentUnit) && Holder->IsInRangeOf(CurrentUnit))
		{
			return true;
		}
	}

	return false;
}

bool AStrategyPlayerController::IsLootableNPC(const AStrategyUnit* Unit)
{
	// never one of the player's own pawns, and never an NPC still on its feet. Downed and Dead
	// both qualify and are treated identically - the roadmap's settled decision: looting a body
	// is the same actor and the same code path as looting a knocked-down one, not a separate
	// corpse container.
	return IsValid(Unit) && !Cast<AStrategyPlayerUnit>(Unit) && Unit->IsIncapacitated();
}

bool AStrategyPlayerController::IsInteractableNPC(const AStrategyUnit* Unit)
{
	// the mirror of IsLootableNPC: never one of the player's own pawns, on its feet rather than
	// Downed or Dead, and not currently hostile. That last clause is the whole "never trade with
	// someone trying to kill you" rule, written down once here so dialog inherits it.
	return IsValid(Unit) && !Cast<AStrategyPlayerUnit>(Unit) && !Unit->IsIncapacitated() && !Unit->IsAggressive();
}

UTraderComponent* AStrategyPlayerController::GetTraderStock(const AStrategyUnit* Unit)
{
	if (!IsInteractableNPC(Unit))
	{
		return nullptr;
	}

	// the component's presence *is* the "is this a trader?" flag - there is no separate bool that
	// could disagree with it, which is the same single-source-of-truth reasoning IInventoryHolder
	// was built on
	return Unit->FindComponentByClass<UTraderComponent>();
}

AStrategyContainer* AStrategyPlayerController::FindContainerInRange() const
{
	TArray<AActor*> FoundContainers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyContainer::StaticClass(), FoundContainers);

	AStrategyContainer* FirstInRange = nullptr;

	for (AActor* CurrentActor : FoundContainers)
	{
		AStrategyContainer* CurrentContainer = Cast<AStrategyContainer>(CurrentActor);

		if (!IsValid(CurrentContainer) || !IsHolderInRangeOfSelection(CurrentContainer))
		{
			continue;
		}

		// the player explicitly picked this one - always prefer it over any other in-range container
		if (CurrentContainer == SelectedContainer)
		{
			return CurrentContainer;
		}

		if (!FirstInRange)
		{
			FirstInRange = CurrentContainer;
		}
	}

	return FirstInRange;
}

AStrategyUnit* AStrategyPlayerController::FindLootableNPCInRange() const
{
	// deliberately unlike FindContainerInRange: only SelectedNPC is ever a candidate, since it's
	// the only NPC the player has actually targeted. Sweeping the level for bodies the way that
	// one sweeps for containers would open whichever corpse happened to be nearest.
	if (!IsLootableNPC(SelectedNPC))
	{
		return nullptr;
	}

	return IsHolderInRangeOfSelection(SelectedNPC) ? SelectedNPC : nullptr;
}

AStrategyUnit* AStrategyPlayerController::FindInteractableNPCInRange() const
{
	// same shape as FindLootableNPCInRange, and deliberately: a key press acts on whoever the
	// player targeted, never on whoever happens to be standing closest
	if (!IsInteractableNPC(SelectedNPC))
	{
		return nullptr;
	}

	return IsHolderInRangeOfSelection(SelectedNPC) ? SelectedNPC : nullptr;
}

AStrategyContainer* AStrategyPlayerController::FindContainerAtLocation(const FVector& Location) const
{
	// every container qualifies - a chest is a chest whether or not the player can reach it, and
	// the double-click handler highlights an out-of-range one rather than ignoring it
	return Cast<AStrategyContainer>(FindHolderActorAtLocation(AStrategyContainer::StaticClass(), Location, ContainerSelectionRadius,
		[](const AActor*) { return true; }));
}

AStrategyUnit* AStrategyPlayerController::FindNPCAtLocation(const FVector& Location) const
{
	// shares the container's click radius: a person, standing or fallen, is about as big a thing
	// to aim at as a chest. The filter is only "not one of ours" - what the NPC's state *means*
	// is the caller's branch, not a second sweep, so a body and a living NPC can never be found
	// by two lookups that disagree about which was nearer.
	return Cast<AStrategyUnit>(FindHolderActorAtLocation(AStrategyUnit::StaticClass(), Location, ContainerSelectionRadius,
		[](const AActor* Actor) { return IsValid(Actor) && !Cast<AStrategyPlayerUnit>(Actor); }));
}

AStrategyPlayerUnit* AStrategyPlayerController::FindClosestPlayerPawn(const FVector& Location)
{
	// checks every player-controlled pawn, not just ControlledUnits - a container may be opened
	// (e.g. via double-click) without the nearest pawn being the one currently selected
	RefreshPlayerPawns();

	AStrategyPlayerUnit* Closest = nullptr;
	float ClosestDistSq = 0.0f;

	for (const TObjectPtr<AStrategyPlayerUnit>& PlayerPawn : PlayerPawns)
	{
		if (!IsValid(PlayerPawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Location);

		if (!Closest || DistSq < ClosestDistSq)
		{
			Closest = PlayerPawn;
			ClosestDistSq = DistSq;
		}
	}

	return Closest;
}

AWorldItem* AStrategyPlayerController::FindWorldItemAtLocation(const FVector& Location) const
{
	// an item holding no definition is skipped as unpickable, and the radius is the tighter
	// WorldItemSelectionRadius (see that property for why)
	return Cast<AWorldItem>(FindHolderActorAtLocation(AWorldItem::StaticClass(), Location, WorldItemSelectionRadius,
		[](const AActor* Actor)
		{
			const AWorldItem* Item = Cast<AWorldItem>(Actor);
			return Item && !Item->GetItem().IsEmpty();
		}));
}

AStrategyPlayerUnit* AStrategyPlayerController::FindPlayerPawnInRangeOfHolder(const AActor* HolderActor)
{
	const IInventoryHolder* Holder = Cast<IInventoryHolder>(HolderActor);

	if (!Holder)
	{
		return nullptr;
	}

	// checks every player-controlled pawn, not just ControlledUnits - see FindClosestPlayerPawn
	RefreshPlayerPawns();

	AStrategyPlayerUnit* Closest = nullptr;
	float ClosestDistSq = 0.0f;

	for (const TObjectPtr<AStrategyPlayerUnit>& PlayerPawn : PlayerPawns)
	{
		if (!IsValid(PlayerPawn) || !Holder->IsInRangeOf(PlayerPawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), HolderActor->GetActorLocation());

		if (!Closest || DistSq < ClosestDistSq)
		{
			Closest = PlayerPawn;
			ClosestDistSq = DistSq;
		}
	}

	return Closest;
}

void AStrategyPlayerController::SetSelectedContainer(AStrategyContainer* NewContainer)
{
	if (NewContainer == SelectedContainer)
	{
		// re-clicking the same container reclaims "most recent" for the selection label
		if (SelectedContainer)
		{
			LastSelectionTarget = SelectedContainer;
		}

		return;
	}

	if (SelectedContainer)
	{
		SelectedContainer->SetSelected(false);

		if (LastSelectionTarget.Get() == SelectedContainer)
		{
			LastSelectionTarget = nullptr;
		}
	}

	SelectedContainer = NewContainer;

	if (SelectedContainer)
	{
		SelectedContainer->SetSelected(true);

		LastSelectionTarget = SelectedContainer;
	}
}

void AStrategyPlayerController::SetSelectedNPC(AStrategyUnit* NewNPC)
{
	if (NewNPC == SelectedNPC)
	{
		// re-clicking the same NPC reclaims "most recent" for the selection label
		if (SelectedNPC)
		{
			LastSelectionTarget = SelectedNPC;
		}

		return;
	}

	if (SelectedNPC)
	{
		SelectedNPC->UnitDeselected();

		if (LastSelectionTarget.Get() == SelectedNPC)
		{
			LastSelectionTarget = nullptr;
		}
	}

	SelectedNPC = NewNPC;

	if (SelectedNPC)
	{
		SelectedNPC->UnitSelected();

		LastSelectionTarget = SelectedNPC;
	}
}

TArray<AStrategyUnit*> AStrategyPlayerController::GetControlledPlayerUnits()
{
	// Answering honestly means a sweep of the level, and the HUD asks every frame - so the sweep
	// runs on a real-time interval instead. Real time, not world time: at the paused tier world
	// time is 1/10,000 speed, and a roster that only refreshed on world seconds would be frozen
	// for as long as the game was.
	const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;

	if (LastRosterRefreshTime < 0.0 || (Now - LastRosterRefreshTime) >= RosterRefreshIntervalSeconds)
	{
		LastRosterRefreshTime = Now;

		RefreshPlayerPawns();

		// a unit that joined or left the level since the last sweep needs watching or forgetting,
		// and this is the only cadence that notices either
		RefreshActivityWatchers();
	}

	TArray<AStrategyUnit*> Roster;
	Roster.Reserve(PlayerPawns.Num());

	for (const TObjectPtr<AStrategyPlayerUnit>& PlayerPawn : PlayerPawns)
	{
		if (IsValid(PlayerPawn))
		{
			Roster.Add(PlayerPawn);
		}
	}

	return Roster;
}

FStrategyTargetInfo AStrategyPlayerController::GetSelectionTargetInfo() const
{
	return BuildTargetInfo(LastSelectionTarget.Get(), ControlledUnits);
}

FStrategyTargetInfo AStrategyPlayerController::BuildTargetInfo(const AActor* Target, const TArray<AStrategyUnit*>& SelectionUnits)
{
	FStrategyTargetInfo Info;

	if (!IsValid(Target))
	{
		// bHasTarget stays false, which is how the panel knows to hide itself entirely
		return Info;
	}

	// distance is measured from the nearest selected unit, because that is the unit that would
	// actually carry out whatever the player asks for. With nothing selected there is nothing to
	// measure from, and the panel says so by leaving the figure negative rather than printing 0m.
	float NearestDistanceSquared = -1.0f;

	for (const AStrategyUnit* CurrentUnit : SelectionUnits)
	{
		if (!IsValid(CurrentUnit))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(CurrentUnit->GetActorLocation(), Target->GetActorLocation());

		if (NearestDistanceSquared < 0.0f || DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
		}
	}

	if (NearestDistanceSquared >= 0.0f)
	{
		// Unreal units are centimetres; the wireframe reads in metres
		Info.DistanceMeters = FMath::Sqrt(NearestDistanceSquared) / 100.0f;
	}

	// real reach, not click precision - the holder's own interaction sphere, which is the same
	// gate every transfer in the game already uses
	const bool bInRange = IsHolderInRangeOfUnits(Target, SelectionUnits);
	const ESmoresRefusalReason RangeRefusal = bInRange ? ESmoresRefusalReason::None : ESmoresRefusalReason::TooFar;

	if (const AStrategyContainer* Container = Cast<AStrategyContainer>(Target))
	{
		Info.bHasTarget = true;
		Info.DisplayName = Container->GetHolderDisplayName();
		Info.Classification = LOCTEXT("TargetClassContainer", "CONTAINER");

		FTargetAction OpenAction;
		OpenAction.Id = StrategyTargetAction::Open();
		OpenAction.Label = LOCTEXT("TargetActionOpen", "Open");
		OpenAction.KeyHint = LOCTEXT("TargetKeyOpen", "O");
		OpenAction.bEnabled = bInRange;
		OpenAction.DisabledReason = RangeRefusal;

		Info.Actions.Add(OpenAction);

		return Info;
	}

	const AStrategyUnit* Unit = Cast<AStrategyUnit>(Target);

	if (!Unit)
	{
		// something targetable that is neither a container nor a unit doesn't exist today; if one
		// ever does, it gets a name and no actions rather than a wrong action row
		return Info;
	}

	Info.bHasTarget = true;
	Info.DisplayName = Unit->GetHolderDisplayName();

	if (const UHealthComponent* Health = Unit->GetHealth())
	{
		Info.bHasHealth = true;
		Info.HealthFraction = Health->MaxHealth > 0.0f
			? FMath::Clamp(Health->GetHealth() / Health->MaxHealth, 0.0f, 1.0f)
			: 0.0f;
	}

	// AStrategyPlayerUnit derives from AStrategyUnit, so it has to be checked first
	if (Cast<AStrategyPlayerUnit>(Unit))
	{
		Info.Classification = Unit->IsIncapacitated()
			? LOCTEXT("TargetClassSquadDown", "SQUAD - DOWN")
			: LOCTEXT("TargetClassSquad", "SQUAD");

		// Deliberately no actions. Everything the player does with their own pawn - the pack, the
		// paperdoll, a move order - already has a route that doesn't involve this panel, and a row
		// of disabled buttons on a squad member would teach a rule that doesn't exist.
		return Info;
	}

	if (AStrategyPlayerController::IsLootableNPC(Unit))
	{
		// Downed and Dead are the same thing here, exactly as they are to the `O` key
		Info.Classification = LOCTEXT("TargetClassBody", "BODY");

		FTargetAction LootAction;
		LootAction.Id = StrategyTargetAction::Loot();
		LootAction.Label = LOCTEXT("TargetActionLoot", "Loot");
		LootAction.KeyHint = LOCTEXT("TargetKeyLoot", "O");
		LootAction.bEnabled = bInRange;
		LootAction.DisabledReason = RangeRefusal;

		Info.Actions.Add(LootAction);

		return Info;
	}

	const bool bHostile = Unit->IsAggressive();

	Info.Classification = bHostile
		? LOCTEXT("TargetClassPersonHostile", "PERSON - HOSTILE")
		: LOCTEXT("TargetClassPersonNeutral", "PERSON - NEUTRAL");

	// Talk is offered on anyone on their feet, enabled or not. Greyed out because they are
	// currently trying to kill you is the rule made visible; leaving the button off the row
	// would teach nothing.
	const bool bInteractable = AStrategyPlayerController::IsInteractableNPC(Unit);

	FTargetAction TalkAction;
	TalkAction.Id = StrategyTargetAction::Talk();
	TalkAction.Label = LOCTEXT("TargetActionTalk", "Talk");
	TalkAction.KeyHint = LOCTEXT("TargetKeyTalk", "T");
	TalkAction.bEnabled = bInteractable && bInRange;
	TalkAction.DisabledReason = !bInteractable ? ESmoresRefusalReason::NotInteractable : RangeRefusal;

	Info.Actions.Add(TalkAction);

	// Attack has no range gate, and correctly so - DoAttackCommand sends the squad to close the
	// distance. What it can't do is start a fight that is already running, which is why someone
	// already hostile gets the button disabled with no reason given: "you are already doing this"
	// is not a refusal.
	FTargetAction AttackAction;
	AttackAction.Id = StrategyTargetAction::Attack();
	AttackAction.Label = LOCTEXT("TargetActionAttack", "Attack");
	AttackAction.KeyHint = LOCTEXT("TargetKeyAttack", "H");
	AttackAction.bEnabled = !bHostile;
	AttackAction.DisabledReason = ESmoresRefusalReason::None;

	Info.Actions.Add(AttackAction);

	return Info;
}

FVector2D AStrategyPlayerController::GetMouseLocationForPlayer()
{
	// attempt to get the mouse position from this PC
	float MouseX, MouseY;

	if (GetMousePosition(MouseX, MouseY))
	{
		return FVector2D(MouseX, MouseY);
	}

	// return an invalid vector
	return FVector2D::ZeroVector;
}

bool AStrategyPlayerController::GetLocationUnderCursor(FVector& Location)
{
	// trace the visibility channel at the cursor location
	FHitResult OutHit;

	GetHitResultUnderCursorByChannel(SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

bool AStrategyPlayerController::GetLocationUnderFinger(FVector& Location)
{
	// trace the visibility channel at Touch 1 location
	FHitResult OutHit;

	GetHitResultUnderFingerByChannel(ETouchIndex::Touch1, SelectionTraceChannel, false, OutHit);

	// if there was a blocking hit, return the hit location
	if (OutHit.bBlockingHit)
	{
		Location = OutHit.Location;
		return true;
	}

	return OutHit.bBlockingHit;
}

FVector AStrategyPlayerController::ProjectTouchPointToWorldSpace()
{
	// get the touch coordinates for the first finger
	float TouchX, TouchY = 0.0f;
	bool bPressed = false;

	GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bPressed);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;

	// deproject the coords into world space
	if (DeprojectScreenPositionToWorld(TouchX, TouchY, WorldLocation, WorldDirection))
	{
		// run a line trace down the camera
		FHitResult OutHit;

		GetWorld()->LineTraceSingleByChannel(OutHit, WorldLocation, WorldLocation + WorldDirection * 10000.0f, ECC_Visibility);

		// if we hit something, return the impact point
		if (OutHit.bBlockingHit)
		{
			return OutHit.ImpactPoint;
		}
		

		// intersect with a horizontal plane and return the resulting point
		const FPlane IntersectPlane(FVector::ZeroVector, FVector::UpVector);
		return FMath::LinePlaneIntersection(WorldLocation, WorldLocation + (WorldDirection * 100000.0f), IntersectPlane);
	}

	// failed to deproject, return a zero vector
	return FVector::ZeroVector;
}

#undef LOCTEXT_NAMESPACE