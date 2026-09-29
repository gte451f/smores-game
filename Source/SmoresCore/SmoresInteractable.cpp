// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresInteractable.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"

bool ISmoresInteractable::IsActorWithinSphere(const AActor* InteractableActor, const USphereComponent* RangeSphere, const AActor* Other)
{
	if (!InteractableActor || !RangeSphere || !Other)
	{
		return false;
	}

	return FVector::Dist(InteractableActor->GetActorLocation(), Other->GetActorLocation()) <= RangeSphere->GetScaledSphereRadius();
}
