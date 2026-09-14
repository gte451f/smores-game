// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "StrategyPlayerState.generated.h"

class UWalletComponent;

/**
 *  Per-player state for the strategy variant. It owns no gameplay state of its own - it hosts
 *  the components that do.
 *
 *  That shape is deliberate and is the rule recorded in unreal-module-organization.md's
 *  "Framework Classes vs. Feature Modules": a player state is Unreal's composition root for
 *  per-player data, and the way it turns into an 800-line junk drawer is fields being added
 *  inline because no feature module obviously owns them yet. Gold was inline here for exactly
 *  that reason until SmoresEconomy existed; the squad roster, faction standing and research
 *  progress still to come each want a component of their own in the module that owns them,
 *  never a field here.
 */
UCLASS(abstract)
class AStrategyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	/** Constructor */
	AStrategyPlayerState();

	/** This player's currency balance. Never null - it's a default subobject. */
	UWalletComponent* GetWallet() const { return Wallet; }

private:

	/** This player's gold, in SmoresEconomy. See the class comment for why it isn't an int32 here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWalletComponent> Wallet;
};
