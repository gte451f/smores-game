// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresDefinition.h"
#include "ItemModifierDefinition.generated.h"

/**
 *  Which of an item's modifier slots a modifier fills. At most one modifier per slot, so an
 *  item is "Bronze" or "Steel" but never both.
 *
 *  Material and quality are deliberately the *same* mechanism rather than two systems: a
 *  material is simply a modifier that a craftable item is expected to have, and a quality one
 *  it may or may not. The alternative - one definition asset per combination - is a trap, since
 *  three materials across eight weapon shapes is twenty-four assets and every recipe becomes
 *  combinatorial with them.
 *
 *  The declaration order is also the order names compose in, so a Bronze Spear becomes a
 *  Masterwork Bronze Spear rather than a Bronze Masterwork Spear. Don't reorder these.
 */
UENUM(BlueprintType)
enum class EItemModifierSlot : uint8
{
	/** What the item is made of - iron, bronze, steel. Inherited from a recipe's inputs when crafting arrives. */
	Material	UMETA(DisplayName = "Material"),
	/** How well it was made - well-made, masterwork. Chosen at craft time from the crafter's skill. */
	Quality		UMETA(DisplayName = "Quality")
};

/**
 *  One material or quality an item instance can carry: a multiplier on the base item's numbers
 *  plus a pattern for composing its name.
 *
 *  A modifier is never the whole item. "Bronze Spear" is the Spear definition plus the Bronze
 *  modifier; "Masterwork Bronze Spear" is the same definition plus two. The item asset's own
 *  Weight and BaseValue therefore mean "with no material applied" - a bare, unqualified one of
 *  these - and every modifier scales from there.
 *
 *  Multipliers compose by multiplication in slot order, which is why they default to 1.0 rather
 *  than 0.0: an unfilled field leaves the item alone instead of erasing it.
 */
UCLASS(BlueprintType)
class SMORESITEMS_API UItemModifierDefinition : public USmoresDefinition
{
	GENERATED_BODY()

public:

	/** The Asset Manager type modifiers are registered under - see Config/DefaultGame.ini's PrimaryAssetTypesToScan */
	static const FPrimaryAssetType DefinitionType;

	/**
	 *  Every slot, in the order modifiers apply and compose their names in.
	 *
	 *  A fixed roster rather than an iteration over the enum, for the same reason
	 *  UEquipmentComponent::GetAllEquipSlots() is one: the order is load-bearing and is meant to
	 *  be read here rather than inferred from the enum's underlying values.
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Modifier")
	static TArray<EItemModifierSlot> GetAllModifierSlots();

	/** Which slot this fills. An item carries at most one modifier per slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Identity")
	EItemModifierSlot Slot = EItemModifierSlot::Material;

	/** Scales the base item's unit weight. Steel is heavier than bronze; a better finish weighs the same. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Economy", meta = (ClampMin = 0.01))
	float WeightMultiplier = 1.0f;

	/** Scales the base item's unit value, before any trader markup */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Economy", meta = (ClampMin = 0.01))
	float ValueMultiplier = 1.0f;

	/**
	 *  Scales how much wear the item can take before it is destroyed. Authored now and read by
	 *  nothing yet - FInventoryItem::Condition is still the placeholder it always was, and the
	 *  durability/upkeep pass that consumes both is a later roadmap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Economy", meta = (ClampMin = 0.01))
	float ConditionMultiplier = 1.0f;

	/**
	 *  Colour this modifier gives the item. Tinting one icon beats authoring one icon per
	 *  material, which is the same combinatorial trap the modifier model exists to avoid.
	 *
	 *  Tints from several modifiers multiply together, so White is genuinely neutral: a quality
	 *  that leaves this at White lets the material's colour through unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Presentation")
	FLinearColor Tint = FLinearColor::White;

	/**
	 *  How this modifier's name joins the item's, as an FText::Format pattern taking {Modifier}
	 *  and {Item} - "{Modifier} {Item}" gives "Bronze Spear".
	 *
	 *  A pattern rather than string concatenation because word order differs by language and a
	 *  translator has to be able to reorder it. Leaving it empty falls back to "{Modifier} {Item}"
	 *  so an unfilled asset still shows its modifier rather than silently dropping it - but the
	 *  fallback is English word order, which is exactly what authoring this field avoids.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier|Presentation")
	FText NamePattern;

	//~ Begin USmoresDefinition interface
	virtual FPrimaryAssetType GetDefinitionType() const override { return DefinitionType; }
	//~ End USmoresDefinition interface
};
