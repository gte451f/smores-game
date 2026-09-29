// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresInteractable.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool ISmoresInteractable::IsActorWithinSphere(const AActor* InteractableActor, const USphereComponent* RangeSphere, const AActor* Other)
{
	if (!InteractableActor || !RangeSphere || !Other)
	{
		return false;
	}

	if (FVector::Dist(InteractableActor->GetActorLocation(), Other->GetActorLocation()) > RangeSphere->GetScaledSphereRadius())
	{
		return false;
	}

	// close enough - but not through a wall
	return HasClearReach(InteractableActor, Other);
}

bool ISmoresInteractable::HasClearReach(const AActor* InteractableActor, const AActor* Other)
{
	if (!InteractableActor || !Other)
	{
		return false;
	}

	// treating your own wounds: there is nothing between you and yourself
	if (InteractableActor == Other)
	{
		return true;
	}

	const UWorld* World = InteractableActor->GetWorld();

	if (!World)
	{
		return true;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SmoresClearReach), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(InteractableActor);
	Params.AddIgnoredActor(Other);

	// World-static only: walls, rocks, the level, and a shut door's leaf (BlockAll is world-static).
	// Not pawns - another squad member standing in the way is not a wall - and not the reach
	// spheres, which are world-dynamic overlaps.
	const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);

	const FVector Start = Other->GetActorLocation();
	const FVector End = InteractableActor->GetActorLocation() + FVector(0.0f, 0.0f, ReachLineHeight);

	return !World->LineTraceTestByObjectType(Start, End, ObjectParams, Params);
}
