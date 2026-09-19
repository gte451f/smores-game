// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "TraderComponent.h"
#include "SmoresEconomy.h"
#include "ItemDefinition.h"

UTraderComponent::UTraderComponent()
{
	// a shop shelf isn't carried anywhere, so a carried-density limit on it would be meaningless
	// - 0 means no limit, the same value every static holder wants
	WeightCapacity = 0.0f;

	// a shelf stacks deeper than a backpack; MoveItem already leaves behind whatever a shallower
	// destination can't take in one entry, so this needs no per-transfer special case
	StackMultiplier = 2.0f;
}

void UTraderComponent::BeginPlay()
{
	Super::BeginPlay();

	// authored stock is content, so only the authoritative copy places it - clients receive the
	// result through Entries replicating, exactly like every other change to a grid
	if (HasOwnerAuthority())
	{
		for (const FInventoryItem& StockItem : StartingStock)
		{
			AddItem(StockItem);
		}
	}
}

int32 UTraderComponent::GetUnitBuyPrice(const FInventoryItem& Item) const
{
	// the copy's own value, not the definition's - a masterwork bronze spear is worth a
	// multiple of a bare spear, and the accessor is where that multiplication lives
	const int32 BaseValue = Item.GetUnitBaseValue();

	// a definition with no authored value is genuinely worthless rather than cheap, so it stays
	// free; anything the designer did price is never rounded down to nothing by a small markup
	if (BaseValue <= 0)
	{
		return 0;
	}

	return FMath::Max(1, FMath::RoundToInt(BaseValue * BuyMarkup));
}

int32 UTraderComponent::GetUnitSellPrice(const FInventoryItem& Item) const
{
	const int32 BaseValue = Item.GetUnitBaseValue();

	if (BaseValue <= 0)
	{
		return 0;
	}

	// unlike the buy price this may legitimately round to zero: a markdown steep enough to make
	// a near-worthless item worth nothing is the trader declining to pay for junk, not a bug
	return FMath::Max(0, FMath::RoundToInt(BaseValue * SellMarkdown));
}
