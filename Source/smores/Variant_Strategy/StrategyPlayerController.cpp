// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPlayerController.h"
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
#include "Inventory/InventoryComponent.h"
#include "StrategyContainer.h"
#include "Blueprint/UserWidget.h"
#include "smores.h"

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

			// Mouse Interaction
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::SelectHoldStarted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::SelectHoldTriggered);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectHoldCompleted);
			EnhancedInputComponent->BindAction(SelectHoldAction, ETriggerEvent::Canceled, this, &AStrategyPlayerController::SelectHoldCompleted);

			EnhancedInputComponent->BindAction(SelectClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClick);

			EnhancedInputComponent->BindAction(SelectClickAdditiveAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectClickAdditive);

			EnhancedInputComponent->BindAction(SelectAllDoubleClickAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::SelectAllDoubleClick);

			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Started, this, &AStrategyPlayerController::InteractHoldStarted);
			EnhancedInputComponent->BindAction(InteractHoldAction, ETriggerEvent::Triggered, this, &AStrategyPlayerController::InteractHoldTriggered);

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

	// set the zoom level from the pawn's camera
	DefaultZoom = CameraZoom = ControlledCameraPawn->GetCamera()->OrthoWidth;

	// cast the HUD pointer
	StrategyHUD = Cast<AStrategyHUD>(GetHUD());

	// if we have a touch controls widget, sync the camera zoom
	if (MobileControlsWidget)
	{
		MobileControlsWidget->BP_SetZoomPercentage(GetDefaultZoomPercentage());
	}
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
	FVector2D InputVector = Value.Get<FVector2D>();

	// get the forward input component vector
	FRotator ForwardRot = GetControlRotation();
	ForwardRot.Pitch = 0.0f;

	// get the right input component vector
	FRotator RightRot = GetControlRotation();
	RightRot.Pitch = 0.0f;
	RightRot.Roll = 0.0f;

	// add the forward input
	if (ControlledCameraPawn)
	{
		ControlledCameraPawn->AddMovementInput(ForwardRot.RotateVector(FVector::ForwardVector), InputVector.X + InputVector.Y);

		// add the right input
		ControlledCameraPawn->AddMovementInput(RightRot.RotateVector(FVector::RightVector), InputVector.X - InputVector.Y);
	}
}

void AStrategyPlayerController::ZoomCamera(const FInputActionValue& Value)
{
	DoCameraModifyZoomCommand(Value.Get<float>() * ZoomScaling);
}

void AStrategyPlayerController::ResetCamera(const FInputActionValue& Value)
{
	DoCameraResetZoomCommand();
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
			PlayerPawns.Add(CurrentUnit);
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

	// spawn the widget on first use
	if (!InventoryWidget)
	{
		if (!InventoryWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("StrategyPlayerController has no InventoryWidgetClass set; can't open the inventory screen."));
			return;
		}

		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
	}

	if (InventoryWidget)
	{
		InventoryWidget->SetInventory(SinglePlayerUnit->GetInventory());
		InventoryWidget->AddToViewport(0);
	}
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

	if (!NearbyContainer)
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
	}

	if (ContainerWidget)
	{
		ContainerWidget->SetInventory(NearbyContainer->GetInventory());
		ContainerWidget->AddToViewport(0);

		NearbyContainer->NotifyOpened();
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
	DoSelectAllUnitsOnScreenCommand();
}

void AStrategyPlayerController::InteractHoldStarted(const FInputActionValue& Value)
{

	// save the starting interaction position
	StartingDragScrollPosition = GetMouseLocationForPlayer();
}

void AStrategyPlayerController::InteractHoldTriggered(const FInputActionValue& Value)
{
	// do a drag scroll 
	DoCameraDragScrollCommand(GetMouseLocationForPlayer());
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
	// deselect any units unless this is an additive selection
	if (!bAdditiveSelection)
	{
		DoDeselectAllUnitsCommand();
	}

	// do an overlap test at the cursor location
	TArray<FOverlapResult> OutOverlaps;

	FCollisionShape CollisionSphere;
	CollisionSphere.SetSphere(SelectionRadius);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;

	if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, SelectLocation, FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
	{
		// find the first player-controlled unit we've overlapped (NPC units are not selectable)
		for (const FOverlapResult& CurrentOverlap : OutOverlaps)
		{
			if (AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentOverlap.GetActor()))
			{
				// is this unit already selected?
				if (ControlledUnits.Contains(CurrentUnit))
				{
					// deselect the unit
					ControlledUnits.Remove(CurrentUnit);

					CurrentUnit->UnitDeselected();
				}
				else
				{
					// select the unit
					ControlledUnits.Add(CurrentUnit);

					CurrentUnit->UnitSelected();
				}

				// found a unit
				return true;
			}
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
	for (AActor* CurrentActor : Units)
	{
		if (AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentActor))
		{
			// is the unit is not already selected, and is on screen?
			if (!ControlledUnits.Contains(CurrentUnit) && CurrentUnit->WasRecentlyRendered(0.2f))
			{
				// select the unit
				ControlledUnits.Add(CurrentUnit);

				CurrentUnit->UnitSelected();
			}
		}
		
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

		// tell each unit to move to the location
		for (AStrategyUnit* CurrentUnit : ControlledUnits)
		{
			if (IsValid(CurrentUnit))
			{
				CurrentUnit->MoveToLocation(GoalLocation, CurrentUnit == ClosestUnit, ControlledUnits);
			}
		}

		// show positive cursor feedback
		BP_CursorFeedback(GoalLocation, true);

	}
	else
	{
		// no units selected, so just show negative cursor feedback
		BP_CursorFeedback(GoalLocation, false);
	}
}

void AStrategyPlayerController::DoCameraModifyZoomCommand(float ZoomDelta)
{
	// add the delta
	CameraZoom += ZoomDelta;

	// clamp between min and max
	CameraZoom = FMath::Clamp(CameraZoom, MinZoomLevel, MaxZoomLevel);

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

	for (AActor* CurrentActor : FoundContainers)
	{
		if (AStrategyContainer* CurrentContainer = Cast<AStrategyContainer>(CurrentActor))
		{
			for (AStrategyUnit* CurrentUnit : ControlledUnits)
			{
				if (CurrentContainer->IsUnitInRange(CurrentUnit))
				{
					return CurrentContainer;
				}
			}
		}
	}

	return nullptr;
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