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
#include "InventoryDragDropOperation.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "StrategyContainer.h"
#include "Blueprint/UserWidget.h"
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

			// Drag rotate. Bound here but only *mapped* while an inventory window is open (see
			// UpdateInventoryInputContext). Started rather than Completed so the item turns on the
			// key press instead of the release - and never Triggered, which for a held key would
			// spin the item once per frame.
			if (RotateDraggedItemAction)
			{
				EnhancedInputComponent->BindAction(RotateDraggedItemAction, ETriggerEvent::Started, this, &AStrategyPlayerController::RotateDraggedItem);
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
		InventoryWidget->SetWindowTitle(FText::Format(LOCTEXT("PawnInventoryTitle", "{0} Inventory"), PlayerUnit->GetUnitDisplayName()));
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
		EquipmentWidget->SetWindowTitle(FText::Format(LOCTEXT("PawnEquipmentTitle", "{0} Equipment"), PlayerUnit->GetUnitDisplayName()));
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
	}
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
		ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("ContainerInventoryTitle", "{0} Contents"), Container->GetContainerDisplayName()));
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
		ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("LootInventoryTitle", "{0} (Downed)"), LootTarget->GetUnitDisplayName()));
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

void AStrategyPlayerController::AttackKeyPressed(const FInputActionValue& Value)
{
	// no effect on a selected container or player pawn - neither ever populates SelectedNPC
	if (SelectedNPC && !SelectedNPC->IsAggressive())
	{
		DoAttackCommand(SelectedNPC);
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
		if (AStrategyContainer* Clicked = FindContainerAtLocation(CursorLocation))
		{
			// highlight it dark green, same as a single click, regardless of range
			SetSelectedContainer(Clicked);

			// open it if any player-controlled pawn is close enough. Checked against every player
			// pawn (not just ControlledUnits) since the plain SelectClickAction fires alongside
			// this gesture and, being non-additive, may have just cleared the current selection.
			RefreshPlayerPawns();

			for (const TObjectPtr<AStrategyPlayerUnit>& PlayerPawn : PlayerPawns)
			{
				if (IsValid(PlayerPawn) && Clicked->IsUnitInRange(PlayerPawn))
				{
					OpenContainer(Clicked);
					break;
				}
			}

			return;
		}

		// no container at this location - try a Downed NPC instead, same proximity rule
		if (AStrategyUnit* Clicked = FindLootableNPCAtLocation(CursorLocation))
		{
			// highlight it, same as a single click, regardless of range
			SetSelectedNPC(Clicked);

			// open it if any player-controlled pawn is close enough. Checked against every player
			// pawn (not just ControlledUnits), for the same reason as the container branch above.
			RefreshPlayerPawns();

			for (const TObjectPtr<AStrategyPlayerUnit>& PlayerPawn : PlayerPawns)
			{
				if (IsValid(PlayerPawn) && Clicked->IsUnitInRange(PlayerPawn))
				{
					OpenLoot(Clicked);
					break;
				}
			}

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
			// NPCs are targetable (highlighted, shown in the selection label) but never commandable
			SetSelectedNPC(NearestUnit);

			// an already-aggressive NPC is attacked directly by the same click that targets it
			if (NearestUnit->IsAggressive())
			{
				DoAttackCommand(NearestUnit);
			}
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
	if (!IsValid(Target) || Target->IsDowned())
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] DoAttackCommand bailed early: Target %s"),
			!IsValid(Target) ? TEXT("invalid") : TEXT("already Downed"));
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

void AStrategyPlayerController::Server_MoveInventoryItem_Implementation(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity)
{
	UInventoryComponent::MoveItem(SourceInventory, EntryId, DestInventory, DestCell, bRotated, Quantity);
}

void AStrategyPlayerController::Server_EquipItem_Implementation(UInventoryComponent* SourceInventory, int32 EntryId, UEquipmentComponent* Equipment, EEquipSlot Slot)
{
	if (!Equipment)
	{
		return;
	}

	// no validation of its own, same as the move RPC - UEquipmentComponent::Equip checks
	// authority, slot matching and room for the displaced item, and mutates nothing if any fails
	Equipment->Equip(SourceInventory, EntryId, Slot);
}

void AStrategyPlayerController::Server_UnequipItem_Implementation(UEquipmentComponent* Equipment, EEquipSlot Slot, UInventoryComponent* DestInventory)
{
	if (!Equipment)
	{
		return;
	}

	Equipment->Unequip(Slot, DestInventory);
}

AStrategyPlayerState* AStrategyPlayerController::GetStrategyPlayerState() const
{
	// keyed off this controller's own player state - there is no single "the" player in a co-op session
	return GetPlayerState<AStrategyPlayerState>();
}

int32 AStrategyPlayerController::GetPlayerGold() const
{
	const AStrategyPlayerState* StrategyPlayerState = GetStrategyPlayerState();

	return StrategyPlayerState ? StrategyPlayerState->GetGold() : 0;
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
	AStrategyPlayerState* StrategyPlayerState = GetStrategyPlayerState();

	if (!StrategyPlayerState)
	{
		UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] No AStrategyPlayerState - is the game mode's PlayerStateClass set to a BP_StrategyPlayerState?"));
		return;
	}

	if (bSpend)
	{
		const bool bSpent = StrategyPlayerState->TrySpendGold(Amount);

		UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] TrySpendGold(%d) -> %s, balance %d"),
			Amount, bSpent ? TEXT("paid") : TEXT("REJECTED"), StrategyPlayerState->GetGold());

		return;
	}

	StrategyPlayerState->AddGold(Amount);

	UE_LOG(Logsmores, Warning, TEXT("[GoldDebug] AddGold(%d), balance %d"), Amount, StrategyPlayerState->GetGold());
}

void AStrategyPlayerController::SmoresDumpInventory()
{
	SmoresAddItem(0);
}

void AStrategyPlayerController::SmoresAddItem(int32 Count)
{
	// the exec runs wherever the console was typed, but the grid it wants to inspect and mutate
	// only authoritatively exists on the server - so resolve the pawn locally and hop across
	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(CurrentUnit))
		{
			Server_DebugInventory(PlayerUnit->GetInventory(), Count);
			return;
		}
	}

	UE_LOG(Logsmores, Warning, TEXT("[InvDebug] No player pawn selected."));
}

void AStrategyPlayerController::Server_DebugInventory_Implementation(UInventoryComponent* Inventory, int32 AddCount)
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
			UE_LOG(Logsmores, Warning, TEXT("[InvDebug] Inventory is empty - nothing to duplicate. Seed StartingItems first."));
		}
		else
		{
			const bool bAddedAll = Inventory->AddItem(FInventoryItem(Existing[0].Item.Definition, AddCount));

			UE_LOG(Logsmores, Warning, TEXT("[InvDebug] AddItem(%d x %s) -> %s"),
				AddCount, *GetNameSafe(Existing[0].Item.Definition), bAddedAll ? TEXT("all placed") : TEXT("PARTIAL/FAILED"));
		}
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
			Entry.bRotated ? TEXT(" rotated") : TEXT(""), Inventory->GetEffectiveMaxStack(Entry.Item.Definition));
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

void AStrategyPlayerController::SmoresDumpEquipment()
{
	DebugEquipmentForSelection(INDEX_NONE, INDEX_NONE);
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
	if (!IsValid(Target) || Target->IsDowned())
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

AStrategyContainer* AStrategyPlayerController::FindContainerInRange() const
{
	// gather every container in the level (picks up every AStrategyContainer subclass)
	TArray<AActor*> FoundContainers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyContainer::StaticClass(), FoundContainers);

	AStrategyContainer* FirstInRange = nullptr;

	for (AActor* CurrentActor : FoundContainers)
	{
		if (AStrategyContainer* CurrentContainer = Cast<AStrategyContainer>(CurrentActor))
		{
			for (AStrategyUnit* CurrentUnit : ControlledUnits)
			{
				if (CurrentContainer->IsUnitInRange(CurrentUnit))
				{
					// the player explicitly picked this one - always prefer it over any other in-range container
					if (CurrentContainer == SelectedContainer)
					{
						return CurrentContainer;
					}

					if (!FirstInRange)
					{
						FirstInRange = CurrentContainer;
					}

					break;
				}
			}
		}
	}

	return FirstInRange;
}

AStrategyUnit* AStrategyPlayerController::FindLootableNPCInRange() const
{
	// mirrors FindContainerInRange's shape - only SelectedNPC is ever a candidate, since it's the
	// only NPC the player has actually targeted
	if (!SelectedNPC || !SelectedNPC->IsDowned())
	{
		return nullptr;
	}

	for (AStrategyUnit* CurrentUnit : ControlledUnits)
	{
		if (SelectedNPC->IsUnitInRange(CurrentUnit))
		{
			return SelectedNPC;
		}
	}

	return nullptr;
}

AStrategyContainer* AStrategyPlayerController::FindContainerAtLocation(const FVector& Location) const
{
	// gather every container in the level (picks up every AStrategyContainer subclass)
	TArray<AActor*> FoundContainers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyContainer::StaticClass(), FoundContainers);

	// track the nearest in-range container rather than just the first found - actor order
	// isn't guaranteed, so two containers close together would otherwise pick one arbitrarily
	AStrategyContainer* Nearest = nullptr;
	float NearestDistSq = FMath::Square(ContainerSelectionRadius);

	for (AActor* CurrentActor : FoundContainers)
	{
		if (AStrategyContainer* CurrentContainer = Cast<AStrategyContainer>(CurrentActor))
		{
			const float DistSq = FVector::DistSquared(CurrentContainer->GetActorLocation(), Location);

			if (DistSq <= NearestDistSq)
			{
				Nearest = CurrentContainer;
				NearestDistSq = DistSq;
			}
		}
	}

	return Nearest;
}

AStrategyUnit* AStrategyPlayerController::FindLootableNPCAtLocation(const FVector& Location) const
{
	// mirrors FindContainerAtLocation's shape - gather every unit, keep only Downed NPCs (never
	// player pawns, and never a Passive/Aggressive NPC still on its feet), pick the nearest in range
	TArray<AActor*> FoundUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStrategyUnit::StaticClass(), FoundUnits);

	AStrategyUnit* Nearest = nullptr;
	float NearestDistSq = FMath::Square(ContainerSelectionRadius);

	for (AActor* CurrentActor : FoundUnits)
	{
		AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentActor);

		if (!CurrentUnit || Cast<AStrategyPlayerUnit>(CurrentUnit) || !CurrentUnit->IsDowned())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(CurrentUnit->GetActorLocation(), Location);

		if (DistSq <= NearestDistSq)
		{
			Nearest = CurrentUnit;
			NearestDistSq = DistSq;
		}
	}

	return Nearest;
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

FText AStrategyPlayerController::GetSelectionTargetLabel() const
{
	AActor* Target = LastSelectionTarget.Get();

	if (!IsValid(Target))
	{
		return FText::GetEmpty();
	}

	if (AStrategyContainer* Container = Cast<AStrategyContainer>(Target))
	{
		return FText::FromString(FString::Printf(TEXT("Container: %s"), *Container->GetContainerDisplayName().ToString()));
	}

	// AStrategyPlayerUnit derives from AStrategyUnit, so it must be checked first
	if (AStrategyPlayerUnit* TargetPawn = Cast<AStrategyPlayerUnit>(Target))
	{
		return FText::FromString(FString::Printf(TEXT("Pawn: %s"), *TargetPawn->GetUnitDisplayName().ToString()));
	}

	if (AStrategyUnit* NPC = Cast<AStrategyUnit>(Target))
	{
		return FText::FromString(FString::Printf(TEXT("NPC: %s"), *NPC->GetUnitDisplayName().ToString()));
	}

	return FText::GetEmpty();
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