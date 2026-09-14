// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "InventoryHolder.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"

bool IInventoryHolder::IsActorWithinSphere(const AActor* HolderActor, const USphereComponent* RangeSphere, const AActor* Other)
{
	if (!HolderActor || !RangeSphere || !Other)
	{
		return false;
	}

	return FVector::Dist(HolderActor->GetActorLocation(), Other->GetActorLocation()) <= RangeSphere->GetScaledSphereRadius();
}
