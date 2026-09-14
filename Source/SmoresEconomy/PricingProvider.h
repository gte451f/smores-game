// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryComponent.h"
#include "PricingProvider.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPricingProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Whatever decides what one item is worth in a given transaction.
 *
 *  It exists as an interface now, with only a flat implementation behind it
 *  (UTraderComponent: the definition's BaseValue times a fixed markup or markdown), because
 *  economy.md's real market simulation - regional supply and demand, emergent prices, trade
 *  routes - is years out and belongs to a separate SmoresMarkets module. When it arrives it
 *  implements this from above and nothing in the transaction or UI code changes. That's the
 *  settled decision recorded in inventory-roadmap.md's "Pricing before the economy exists";
 *  the alternative, stubbing a market table now, would have to be thrown away.
 *
 *  Only the *per-unit* prices are virtual. A total is per-unit times quantity and nothing more
 *  interesting than that, so it's derived here rather than being a second thing every
 *  implementer has to get right - and a bulk discount, if one is ever wanted, is a change to
 *  this interface rather than something a caller could have been doing on its own.
 *
 *  Prices take the whole FInventoryItem rather than just its UItemDefinition so a later rule
 *  can read what varies copy-to-copy - a worn item fetching less (Condition), a fence paying
 *  less for stolen goods (bStolen) - without changing this signature.
 */
class SMORESECONOMY_API IPricingProvider
{
	GENERATED_BODY()

public:

	/** What the player pays this seller for one unit of Item */
	virtual int32 GetUnitBuyPrice(const FInventoryItem& Item) const = 0;

	/** What this buyer pays the player for one unit of Item */
	virtual int32 GetUnitSellPrice(const FInventoryItem& Item) const = 0;

	/** What the player pays for Quantity units. A negative quantity prices as zero rather than as a refund. */
	int32 GetBuyPrice(const FInventoryItem& Item, int32 Quantity) const
	{
		return GetUnitBuyPrice(Item) * FMath::Max(0, Quantity);
	}

	/** What the player receives for Quantity units (see GetBuyPrice) */
	int32 GetSellPrice(const FInventoryItem& Item, int32 Quantity) const
	{
		return GetUnitSellPrice(Item) * FMath::Max(0, Quantity);
	}
};
