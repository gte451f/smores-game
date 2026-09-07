// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "StrategyPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;

/**
 *  Simple pawn that implements a top-down camera perspective for a strategy game.
 *  Units are indirectly controlled by other means.
 */
UCLASS(abstract)
class AStrategyPawn : public APawn
{
	GENERATED_BODY()

	/** Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** Movement Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UFloatingPawnMovement* FloatingPawnMovement;

public:

	/** Constructor */
	AStrategyPawn();

private:

	/** Distance the camera sits behind Root along its own look direction (the "zoom" representation for a perspective camera) */
	float DollyDistance = 1500.0f;

	/** Recomputes Camera's relative location so it stays DollyDistance behind Root along Camera's current relative rotation */
	void UpdateCameraDollyOffset();

public:

	/** Sets the camera zoom modifier value */
	void SetZoomModifier(float Value);

	/** Sets the camera's pitch/yaw (roll is forced to zero) and keeps the dolly offset consistent with the new orientation */
	void SetCameraRotation(const FRotator& NewRotation);

	/** Sets the pawn's world height and keeps the movement plane constraint consistent with it */
	void SetHeight(float NewHeight);

	/** Returns the pawn's current world height */
	float GetHeight() const { return GetActorLocation().Z; }

	/** Returns the camera component */
	UCameraComponent* GetCamera() const { return Camera; }
};
