// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AttackHit.generated.h"

/**
 *  Placed at the swing-connect frame of an attack montage. Applies the owning unit's
 *  attack damage to its current attack target, decoupling "when the swing looks like it
 *  connects" from "when damage actually applies".
 */
UCLASS()
class SMORESCOMBAT_API UAnimNotify_AttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:

	//~ Begin UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	//~ End UAnimNotify interface
};
