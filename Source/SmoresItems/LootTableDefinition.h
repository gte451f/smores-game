// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryComponent.h"
#include "WeightedTableDefinition.h"
#include "LootTableDefinition.generated.h"

class ULootTableDefinition;
class UItemDefinition;
class UItemModifierDefinition;

/** What one loot-table entry yields when a roll lands on it */
UENUM(BlueprintType)
enum class ELootEntryKind : uint8
{
	/** One specific item */
	Item		UMETA(DisplayName = "Item"),
	/** Roll another loot table in full, with its own roll count - how a desert chest and a town chest share one CommonJunk */
	Table		UMETA(DisplayName = "Sub-table"),
	/** Any one item carrying a gameplay tag, chosen evenly among them - "any mineral" */
	Tag			UMETA(DisplayName = "Any Item With Tag"),
	/** An empty pick: weight given to "nothing this time" */
	Nothing		UMETA(DisplayName = "Nothing")
};

/** One option in a modifier pool - a modifier and how likely it is. An empty Modifier is "leave this one bare". */
USTRUCT(BlueprintType)
struct FLootModifierChoice
{
	GENERATED_BODY()

	/** The material or quality this choice applies. None means the item comes out without one - weighted like any other choice. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UItemModifierDefinition> Modifier = nullptr;

	/** How likely this choice is against the others in its pool. Zero is never chosen, and the content sweep rejects it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 0))
	int32 Weight = 1;
};

/**
 *  One weighted choice of modifier for a rolled item - "bronze or iron", or "well-made one time in
 *  five". A pool is rolled once per rolled entry and lands on exactly one choice.
 *
 *  An entry can carry several pools, rolled in order, and each landing is applied with
 *  FInventoryItem::AddModifier - which already refuses a second modifier in a filled slot. That is
 *  what makes a fallback pool work: [Steel 1, none 9] followed by [Bronze 1, Iron 1] reads "steel
 *  one time in ten, otherwise bronze or iron", because the second pool's material only lands when
 *  the first left the slot empty. The table only chooses; the item enforces one per slot.
 */
USTRUCT(BlueprintType)
struct FLootModifierPool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLootModifierChoice> Choices;
};

/**
 *  One line of a loot table: what it yields, how likely it is, and - for an item - how many and
 *  what it may be made of.
 *
 *  Items and sub-tables are named by asset pointer, not by id. A loot table is authored content,
 *  and content links content by pointer (game-data.md): it cooks, the editor shows the reference,
 *  and nothing about a table is ever saved. The id rule is for records and saves.
 */
USTRUCT(BlueprintType)
struct FLootTableEntry
{
	GENERATED_BODY()

	/** What this entry yields - which of the fields below it reads */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootEntryKind Kind = ELootEntryKind::Item;

	/** How likely this entry is against the others in its table. "60 / 30 / 10" means exactly that. Zero is never picked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 0))
	int32 Weight = 1;

	/** The item this entry yields */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "Kind == ELootEntryKind::Item", EditConditionHides))
	TObjectPtr<UItemDefinition> Item = nullptr;

	/** The table this entry rolls in full, with that table's own roll count */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "Kind == ELootEntryKind::Table", EditConditionHides))
	TObjectPtr<ULootTableDefinition> Table = nullptr;

	/** Any one item carrying this tag, chosen evenly. A child tag counts - an item tagged Item.Mineral.Ore is an Item.Mineral. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "Kind == ELootEntryKind::Tag", EditConditionHides))
	FGameplayTag Tag;

	/** Fewest of the item this entry yields. A stack larger than the item's cap splits across entries when it is placed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 1, EditCondition = "Kind == ELootEntryKind::Item || Kind == ELootEntryKind::Tag", EditConditionHides))
	int32 MinQuantity = 1;

	/** Most of the item this entry yields, inclusive. Below MinQuantity reads as MinQuantity; the content sweep flags it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = 1, EditCondition = "Kind == ELootEntryKind::Item || Kind == ELootEntryKind::Tag", EditConditionHides))
	int32 MaxQuantity = 1;

	/** Materials and qualities the rolled item may come out with, one pool at a time - see FLootModifierPool */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (EditCondition = "Kind == ELootEntryKind::Item || Kind == ELootEntryKind::Tag", EditConditionHides))
	TArray<FLootModifierPool> ModifierPools;
};

/**
 *  What a container (and later a body, a caravan's stock, a quest reward) finds inside it: a
 *  weighted table whose entries are items, other loot tables, tags, or nothing.
 *
 *  The weights, the roll count, the nesting and the seeding are all UWeightedTableDefinition's; this
 *  class only says what a landed pick turns into. That split is deliberate - the world-activity
 *  roadmap's spawn table is the same machinery with characters where the items are.
 *
 *  **Rolling is pure.** RollLoot turns a stream into a list of item instances and touches nothing
 *  else - no inventory, no world - which is what lets SmoresRollTable print a sample and lets a
 *  test roll a thousand times. Putting the result somewhere is the caller's job
 *  (AStrategyContainer uses UInventoryComponent::AddItemsAllOrNothing).
 */
UCLASS(BlueprintType)
class SMORESITEMS_API ULootTableDefinition : public UWeightedTableDefinition
{
	GENERATED_BODY()

public:

	/** The Asset Manager type loot tables are registered under - see Config/DefaultGame.ini's PrimaryAssetTypesToScan */
	static const FPrimaryAssetType DefinitionType;

	/** Everything this table can yield, one weighted line each */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLootTableEntry> Entries;

	/**
	 *  Rolls this table once into OutItems (appended, not cleared), drawing only from Stream.
	 *
	 *  TagCandidates is every item a Tag entry may choose among. The order it arrives in doesn't
	 *  matter - matches are sorted by DefinitionId before the pick, because the Asset Manager hands
	 *  back items in scan order and a roll that depended on scan order wouldn't be deterministic.
	 *
	 *  OutSourceTableIds, when given, receives one id per item appended: the table whose entry
	 *  produced it, which for a nested roll is the innermost one. It's what lets the debug exec say
	 *  "Apple x2, from CommonJunk".
	 *
	 *  An entry that names nothing it can use (an Item entry with no item, a tag no candidate
	 *  carries) yields nothing and warns - the content sweep is where that's meant to be caught.
	 *
	 *  Returns false only when the roll was abandoned at MaxNestingDepth (a table that names
	 *  itself); OutItems still holds everything rolled before that.
	 */
	bool RollLoot(FRandomStream& Stream, const TArray<UItemDefinition*>& TagCandidates, TArray<FInventoryItem>& OutItems, TArray<FName>* OutSourceTableIds = nullptr) const;

	/** RollLoot with every item definition the Asset Manager knows as the tag candidates - what a real game rolls against */
	bool RollLoot(FRandomStream& Stream, TArray<FInventoryItem>& OutItems, TArray<FName>* OutSourceTableIds = nullptr) const;

	/** Every registered item definition, loaded - the tag candidates for a real roll, and for the content sweep's tag check */
	static TArray<UItemDefinition*> GatherRegisteredItems();

	/**
	 *  The item a Tag entry lands on: one of the candidates carrying Tag, chosen evenly after
	 *  sorting them by DefinitionId. Null, having drawn nothing from the stream, when no candidate
	 *  carries it.
	 */
	static UItemDefinition* PickTaggedItem(const FGameplayTag& Tag, const TArray<UItemDefinition*>& TagCandidates, FRandomStream& Stream);

	/** One pool's pick: the chosen choice's modifier, which may be null ("leave it bare"). Null too, drawing nothing, for a pool with no weight. */
	static UItemModifierDefinition* PickModifier(const FLootModifierPool& Pool, FRandomStream& Stream);

	//~ Begin UWeightedTableDefinition interface
	virtual int32 GetNumEntries() const override { return Entries.Num(); }
	virtual int32 GetEntryWeight(int32 EntryIndex) const override;
	virtual const UWeightedTableDefinition* GetEntrySubTable(int32 EntryIndex) const override;
	//~ End UWeightedTableDefinition interface

	//~ Begin USmoresDefinition interface
	virtual FPrimaryAssetType GetDefinitionType() const override { return DefinitionType; }
	//~ End USmoresDefinition interface

private:

	/** What one landed entry yields. False (and OutItem untouched) for a Nothing entry or one naming nothing usable. */
	bool ResolveEntry(const FLootTableEntry& Entry, FRandomStream& Stream, const TArray<UItemDefinition*>& TagCandidates, FInventoryItem& OutItem) const;
};
