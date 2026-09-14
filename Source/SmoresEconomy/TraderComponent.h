// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"
#include "PricingProvider.h"
#include "TraderComponent.generated.h"

/**
 *  A trader's wares: the stock they will sell, the prices they sell and buy at, and - by the
 *  simple fact of being attached at all - the answer to "is this character a trader?".
 *
 *  It is a component on a character rather than a shop *actor*, which is a settled decision
 *  (inventory-roadmap.md's Resolved Design Decisions) resting on two things:
 *
 *  - economy.md wants travelling caravans: traders that walk between towns, that patrols escort
 *    and bandits raid. A caravan is a trader that walks. As a component, a caravan NPC gets
 *    trading for free; as a building, caravans need the whole thing implemented a second time.
 *  - The component's *presence* is the only "is this a trader" flag there is, so nothing can
 *    disagree with reality - no merchant flagged as one with no stock, no stocked NPC who
 *    refuses to trade. Same single-source-of-truth reasoning as Slice 7's IInventoryHolder.
 *
 *  It derives from UInventoryComponent rather than owning one because a shop's shelf *is* a
 *  grid: deriving means the stock replicates, opens into the ordinary inventory window, and is
 *  dragged in and out of through the existing UInventoryComponent::MoveItem, with the
 *  transaction layered in front of that move rather than bolted into it. The stock is
 *  deliberately separate from whatever the NPC personally carries in its own
 *  AStrategyUnit::Inventory - a merchant's wares and a merchant's pockets are different things,
 *  and only the wares are for sale.
 *
 *  Sensible defaults for a shop shelf, overridden per Blueprint: no weight limit (nothing
 *  static carries anything anywhere) and a deeper stack cap than a pawn's pack.
 */
UCLASS(ClassGroup = (Economy), meta = (BlueprintSpawnableComponent))
class SMORESECONOMY_API UTraderComponent : public UInventoryComponent, public IPricingProvider
{
	GENERATED_BODY()

public:

	/** Constructor */
	UTraderComponent();

	/**
	 *  Multiplier on an item's BaseValue for what the player pays to buy it. Above 1 is the
	 *  trader's margin; the player always pays more than they would be paid for the same item.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy|Pricing", meta = (ClampMin = 0.0))
	float BuyMarkup = 1.5f;

	/**
	 *  Multiplier on an item's BaseValue for what the player is paid when selling it. Below 1
	 *  is the other half of the trader's margin - see BuyMarkup.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy|Pricing", meta = (ClampMin = 0.0))
	float SellMarkdown = 0.5f;

	/**
	 *  Wares this trader opens for business with. Authored per Blueprint (or per placed
	 *  instance) and applied once, on the server, at BeginPlay - never seeded from C++, since
	 *  hard-coding content paths in a constructor is what CLAUDE.md's "C++ first, Blueprints for
	 *  wiring" rule rules out. Same shape as AStrategyContainer::StartingItems.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy|Stock")
	TArray<FInventoryItem> StartingStock;

	//~ Begin IPricingProvider interface

	/** BaseValue x BuyMarkup, never rounding a saleable item down to free */
	virtual int32 GetUnitBuyPrice(const FInventoryItem& Item) const override;

	/** BaseValue x SellMarkdown */
	virtual int32 GetUnitSellPrice(const FInventoryItem& Item) const override;

	//~ End IPricingProvider interface

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface
};
