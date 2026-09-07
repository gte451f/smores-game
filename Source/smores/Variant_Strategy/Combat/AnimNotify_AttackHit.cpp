// Copyright Epic Games, Inc. All Rights Reserved.


#include "AnimNotify_AttackHit.h"
#include "Components/SkeletalMeshComponent.h"
#include "StrategyUnit.h"

void UAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		if (AStrategyUnit* Unit = Cast<AStrategyUnit>(MeshComp->GetOwner()))
		{
			Unit->ApplyAttackDamage();
		}
	}
}
