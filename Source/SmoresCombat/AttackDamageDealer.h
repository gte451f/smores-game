// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttackDamageDealer.generated.h"

UINTERFACE(MinimalAPI)
class UAttackDamageDealer : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by anything whose attack montages carry a hit-frame AnimNotify_AttackHit. */
class SMORESCOMBAT_API IAttackDamageDealer
{
	GENERATED_BODY()

public:

	/** Applies this attacker's pending attack damage to its current target. Called at the swing-connect frame. */
	virtual void ApplyAttackDamage() = 0;
};
