// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WalletComponent.generated.h"

/** Broadcast on every machine when this wallet's balance changes. Passed the new balance. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChangedDelegate, int32, NewGold);

/**
 *  One holder's currency balance. Hosted on AStrategyPlayerState, which is what makes gold
 *  per-*player* rather than per-pawn or per-division: every sub-squad under one player draws
 *  from the same pool regardless of where in the world it is, because divisions are an
 *  organization layer and not a separate economy.
 *
 *  It's a component rather than an int32 on the player state for the reason
 *  unreal-module-organization.md's "Framework Classes vs. Feature Modules" gives: per-player
 *  state accumulating inline on a framework class is how that class becomes a junk drawer, and
 *  gold is only the first of several values headed there (squad roster, faction standing,
 *  research progress). A component also puts the balance in a module SmoresUI already depends
 *  on, which is what let IStrategyResourceHost be deleted - the HUD reaches it through
 *  PlayerState->FindComponentByClass<UWalletComponent>() with no interface at all.
 *
 *  Gold is deliberately not an item: it has no weight, no grid footprint, and never occupies a
 *  cell. The balance is server-owned; AddGold/TrySpendGold are authority-only and replicate the
 *  result down. It replicates to every machine rather than only its owner, so a later
 *  squad-roster or trade UI can show a co-op partner's funds without a second path.
 */
UCLASS(ClassGroup = (Economy), meta = (BlueprintSpawnableComponent))
class SMORESECONOMY_API UWalletComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UWalletComponent();

	/** Balance this wallet starts a session with. Authored per Blueprint; applied on the server at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = 0))
	int32 StartingGold = 0;

	/** Fired whenever the balance changes, on the server and on every client that sees the change */
	UPROPERTY(BlueprintAssignable, Category = "Economy")
	FOnGoldChangedDelegate OnGoldChanged;

	/** This wallet's current balance */
	UFUNCTION(BlueprintPure, Category = "Economy")
	int32 GetGold() const { return Gold; }

	/** True if this wallet can currently cover the given price (a price of zero or less is always affordable) */
	UFUNCTION(BlueprintPure, Category = "Economy")
	bool CanAfford(int32 Amount) const { return Amount <= Gold; }

	/**
	 *  Credits gold. Authority-only - a silent no-op on a non-authority machine, like every
	 *  other shared-state mutator in the project.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddGold(int32 Amount);

	/**
	 *  Debits gold if the balance covers it, and reports whether it went through. Authority-only;
	 *  returns false without mutating anything when called off-authority, when the balance is
	 *  short, or when Amount is negative.
	 *
	 *  This is the half of a purchase that has to succeed before the item half runs - see
	 *  AStrategyPlayerController::TryTradeItem, where both happen inside one server-side call so
	 *  the transaction can't half-apply.
	 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	bool TrySpendGold(int32 Amount);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent interface

	/** True if this component's owner is the authoritative copy - the one gate on every mutator here */
	bool HasOwnerAuthority() const;

	/** Reacts on non-authority machines to a replicated balance change - authority already broadcast directly from the mutator */
	UFUNCTION()
	void OnRep_Gold();

private:

	/** Current balance. Server-owned; clients hold a replicated copy and never mutate it. */
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int32 Gold = 0;
};
