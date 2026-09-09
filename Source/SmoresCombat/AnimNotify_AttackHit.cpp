// Copyright Epic Games, Inc. All Rights Reserved.


#include "AnimNotify_AttackHit.h"
#include "Components/SkeletalMeshComponent.h"
#include "AttackDamageDealer.h"

void UAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp)
	{
		if (IAttackDamageDealer* Dealer = Cast<IAttackDamageDealer>(MeshComp->GetOwner()))
		{
			Dealer->ApplyAttackDamage();
		}
	}
}
