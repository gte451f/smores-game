// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "StrategyPlayerState.generated.h"

/** Broadcast on every machine when this player's gold changes. Passed the new balance. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChangedDelegate, int32, NewGold);

/**
 *  Per-player state for the strategy variant. Today it owns exactly one thing: the player's
 *  gold balance.
 *
 *  Currency lives here rather than on a pawn or a UInventoryComponent because it's owned per
 *  *player*, not per pawn or per division - every sub-squad under one player draws from the
 *  same pool regardless of where in the world it is (squad divisions are an organization
 *  layer, not a separate economy). It's also deliberately not an item: gold has no weight,
 *  no grid footprint, and never occupies a cell.
 *
 *  The balance is server-owned; AddGold/TrySpendGold are authority-only and replicate the
 *  result down. Gold replicates to every machine, not just its owner, so a later squad-roster
 *  or trade UI can show a co-op partner's funds without a second path.
 */
UCLASS(abstract)
class AStrategyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	/** Balance this player starts a session with. Authored per Blueprint; applied on the server at BeginPlay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = 0))
	int32 StartingGold = 0;

	/** Fired whenever the gold balance changes, on the server and on every client that sees the change */
	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FOnGoldChangedDelegate OnGoldChanged;

	/** This player's current gold balance */
	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetGold() const { return Gold; }

	/** True if this player can currently afford the given price (a price of zero or less is always affordable) */
	UFUNCTION(BlueprintPure, Category = "Economy")
	bool CanAfford(int32 Amount) const { return Amount <= Gold; }

	/**
	 *  Credits gold to this player. Authority-only - a silent no-op on a non-authority machine,
	 *  like every other shared-state mutator in the project.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddGold(int32 Amount);

	/**
	 *  Debits gold if the player can afford it, and reports whether it went through. Authority-only;
	 *  returns false without mutating anything when called off-authority, when the balance is short,
	 *  or when Amount is negative.
	 *
	 *  This is the half of a purchase that has to succeed before the item half runs - see the
	 *  storefront/trade slice, where both happen inside one server-side transaction.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool TrySpendGold(int32 Amount);

protected:

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface

	//~ Begin UObject interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject interface

	/** Reacts on non-authority machines to a replicated balance change - authority already broadcast directly from the mutator */
	UFUNCTION()
	void OnRep_Gold();

private:

	/** Current balance. Server-owned; clients hold a replicated copy and never mutate it. */
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int32 Gold = 0;
};
