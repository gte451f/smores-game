// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SmoresDefinition.h"
#include "Templates/Function.h"
#include "WeightedTableDefinition.generated.h"

/**
 *  The shared base of every "roll on a table" definition: a list of weighted entries, a range for
 *  how many picks one roll makes, and entries that can point at *another table* instead of at a
 *  payload.
 *
 *  **It knows nothing about what a table yields.** ULootTableDefinition (SmoresItems) is the only
 *  subclass today and turns a pick into items; the world-activity roadmap's spawn table will turn
 *  one into characters. Everything that is the same for both - how weights choose, how many picks
 *  a roll makes, how nesting descends and where it gives up, and how a roll is seeded - lives
 *  here, so the second subclass extends this rather than rewriting it. The payload lives on the
 *  subclass, which answers three questions about its own entries (how many, how heavy, is this
 *  one a sub-table) and is handed back every pick that lands on a payload.
 *
 *  It sits in SmoresCore rather than beside the loot table in SmoresItems because nothing in it is
 *  about items, and a spawn table should not have to reach into the items module for its base.
 *
 *  **Nesting, not context filtering.** A desert chest and a town chest share a CommonJunk table by
 *  both naming it, rather than one clever table filtering itself by region at roll time. Small
 *  tables composed by hand are far easier to debug - economy.md's stance that a simple model with
 *  visible results beats a precise one running invisibly.
 *
 *  **A roll is seeded, never random.** characters-and-squads.md requires that a retry under
 *  identical conditions produces an identical result, so nothing here touches a global random
 *  number generator: every roll draws from an FRandomStream the caller made, and MakeRollStream is
 *  how a caller makes one from the world seed plus the rolling thing's own stable id.
 */
UCLASS(Abstract)
class SMORESCORE_API UWeightedTableDefinition : public USmoresDefinition
{
	GENERATED_BODY()

public:

	/** Fewest picks one roll of this table makes. Zero is allowed, and means "sometimes nothing at all". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Table|Rolls", meta = (ClampMin = 0))
	int32 MinRolls = 1;

	/** Most picks one roll of this table makes, inclusive. Below MinRolls reads as MinRolls; the content sweep flags it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Table|Rolls", meta = (ClampMin = 0))
	int32 MaxRolls = 1;

	/**
	 *  How many tables deep one roll may descend before it gives up. A table that names itself,
	 *  directly or through a chain of others, would otherwise roll forever. The content sweep
	 *  refuses such a table; this is the backstop for one built in memory, where no sweep looks.
	 *  Eight is far deeper than any hand-composed set of tables needs.
	 */
	static constexpr int32 MaxNestingDepth = 8;

	/**
	 *  The stream a roll draws from, for the thing identified by StableId in a world seeded with
	 *  WorldSeed. The same pair always makes the same stream, which is the whole point: reopening
	 *  the same chest in the same campaign - after a reload, say - finds the same contents, so
	 *  there is nothing to gain by reloading.
	 *
	 *  **The mixing is a CRC-32 of the two, spelled out, rather than HashCombine.** An engine
	 *  upgrade is free to change HashCombine, and doing so would silently re-roll every container
	 *  in every save. Smores.Items.LootTable.RollStreamIsPinned pins the result, so changing this
	 *  is a decision somebody has to make on purpose.
	 */
	static FRandomStream MakeRollStream(int32 WorldSeed, const FGuid& StableId);

	/** What a roll hands back for each pick that landed on a payload entry: the table the entry belongs to, and which entry */
	using FOnEntryPicked = TFunctionRef<void(const UWeightedTableDefinition& Table, int32 EntryIndex)>;

	/**
	 *  Rolls this table once: draws a pick count from [MinRolls, MaxRolls], and for each pick
	 *  chooses an entry by weight. A pick that lands on a sub-table rolls *that* table in full -
	 *  its own pick count and its own weights - and a pick that lands on anything else is handed
	 *  to OnEntryPicked, which is where a subclass turns it into items (or, later, characters).
	 *
	 *  Returns false if the roll was abandoned at MaxNestingDepth, having logged an error and still
	 *  handed over every pick made before that.
	 */
	bool RollEntries(FRandomStream& Stream, FOnEntryPicked OnEntryPicked) const;

	/** One weighted pick among this table's own entries - never descends. INDEX_NONE when no entry has any weight. */
	int32 PickEntry(FRandomStream& Stream) const;

	/** Every entry's weight added up, counting a negative weight as zero. An entry of weight zero is never picked. */
	int32 GetTotalWeight() const;

	/**
	 *  True if following sub-tables from here ever arrives back at a table already on the way down
	 *  - i.e. some roll starting here would never end. A table named twice by two different
	 *  branches (a diamond, not a loop) is fine and reports false.
	 */
	bool ContainsNestingLoop() const;

	//~ The three questions a subclass answers about its own entries

	/** How many entries this table has */
	virtual int32 GetNumEntries() const PURE_VIRTUAL(UWeightedTableDefinition::GetNumEntries, return 0;);

	/** One entry's weight. Zero or less means it is never picked. */
	virtual int32 GetEntryWeight(int32 EntryIndex) const PURE_VIRTUAL(UWeightedTableDefinition::GetEntryWeight, return 0;);

	/** The table one entry points at, or null when the entry is a payload of the subclass's own kind */
	virtual const UWeightedTableDefinition* GetEntrySubTable(int32 EntryIndex) const { return nullptr; }

private:

	/** RollEntries' body, one nesting level further down per call. False means "abandoned - stop picking". */
	bool RollEntriesAtDepth(FRandomStream& Stream, FOnEntryPicked OnEntryPicked, int32 Depth) const;

	/** ContainsNestingLoop's body: depth-first over sub-tables, with the tables on the way down in Path */
	bool ContainsNestingLoopFrom(TArray<const UWeightedTableDefinition*>& Path) const;
};
