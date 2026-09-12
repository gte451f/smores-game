// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPawn.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AStrategyPawn::AStrategyPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);

	// create the movement component
	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Floating Pawn Movement"));

	// configure the camera
	Camera->ProjectionMode = ECameraProjectionMode::Perspective;
	Camera->FieldOfView = 60.0f;

	// configure the movement comp
	FloatingPawnMovement->bConstrainToPlane = true;
	FloatingPawnMovement->SetPlaneConstraintNormal(FVector::UpVector);
	FloatingPawnMovement->SetPlaneConstraintOrigin(FVector::UpVector * 1500.0f);
}

void AStrategyPawn::UpdateCameraDollyOffset()
{
	// keep the camera DollyDistance behind the root, along its current look direction
	const FRotator CamRot = Camera->GetRelativeRotation();
	Camera->SetRelativeLocation(-CamRot.Vector() * DollyDistance);
}

void AStrategyPawn::SetZoomModifier(float Value)
{
	// set the dolly distance and re-apply it along the current look direction
	DollyDistance = Value;
	UpdateCameraDollyOffset();
}

void AStrategyPawn::SetCameraRotation(const FRotator& NewRotation)
{
	// roll is intentionally preserved (not forced to zero) - a genuine full vertical loop needs
	// it, since a pure pitch/yaw pair can't represent "upside down" on its own
	Camera->SetRelativeRotation(NewRotation);
	UpdateCameraDollyOffset();
}

void AStrategyPawn::SetHeight(float NewHeight)
{
	// move the pawn to the new height
	FVector Location = GetActorLocation();
	Location.Z = NewHeight;
	SetActorLocation(Location);

	// keep the movement plane constraint at the new height so panning doesn't pull it back down
	FloatingPawnMovement->SetPlaneConstraintOrigin(FVector::UpVector * NewHeight);
}
