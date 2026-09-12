// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "AnimNotify_AttackHit.h"
#include "Components/SkeletalMeshComponent.h"
#include "AttackDamageDealer.h"
#include "SmoresCombat.h"

void UAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] AnimNotify_AttackHit::Notify fired on %s"),
		(MeshComp && MeshComp->GetOwner()) ? *MeshComp->GetOwner()->GetName() : TEXT("?"));

	if (MeshComp && MeshComp->GetOwner())
	{
		// ApplyAttackDamage is implemented by a UCombatComponent, not the owning actor itself -
		// look for it as a component rather than casting the actor directly
		if (IAttackDamageDealer* Dealer = MeshComp->GetOwner()->FindComponentByInterface<IAttackDamageDealer>())
		{
			Dealer->ApplyAttackDamage();
		}
		else
		{
			UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] AnimNotify_AttackHit::Notify: no IAttackDamageDealer component found on %s"),
				*MeshComp->GetOwner()->GetName());
		}
	}
}
