// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WeightedTableDefinition.h"
#include "Misc/Crc.h"
#include "SmoresCore.h"

FRandomStream UWeightedTableDefinition::MakeRollStream(int32 WorldSeed, const FGuid& StableId)
{
	// five plain 32-bit words, so the CRC reads exactly the same bytes on every machine and in
	// every engine version - see the header for why this isn't HashCombine
	const uint32 Words[5] = { static_cast<uint32>(WorldSeed), StableId.A, StableId.B, StableId.C, StableId.D };

	return FRandomStream(static_cast<int32>(FCrc::MemCrc32(Words, sizeof(Words))));
}

bool UWeightedTableDefinition::RollEntries(FRandomStream& Stream, FOnEntryPicked OnEntryPicked) const
{
	if (RollEntriesAtDepth(Stream, OnEntryPicked, 0))
	{
		return true;
	}

	// logged once here rather than at the cut, so a table naming itself twice per level reports
	// one problem instead of one per branch
	UE_LOG(LogSmoresCore, Error, TEXT("%s nests more than %d tables deep, so the roll was abandoned part-way. Does it name itself, directly or through another table?"),
		*GetNameSafe(this), MaxNestingDepth);

	return false;
}

bool UWeightedTableDefinition::RollEntriesAtDepth(FRandomStream& Stream, FOnEntryPicked OnEntryPicked, int32 Depth) const
{
	if (Depth >= MaxNestingDepth)
	{
		return false;
	}

	const int32 LowestRolls = FMath::Max(MinRolls, 0);
	const int32 Picks = Stream.RandRange(LowestRolls, FMath::Max(LowestRolls, MaxRolls));

	for (int32 Pick = 0; Pick < Picks; ++Pick)
	{
		const int32 EntryIndex = PickEntry(Stream);

		// no entry has any weight, so every further pick would land on nothing too
		if (EntryIndex == INDEX_NONE)
		{
			break;
		}

		if (const UWeightedTableDefinition* SubTable = GetEntrySubTable(EntryIndex))
		{
			// a sub-table rolls in full - its own pick count, its own weights. Abandoning stops the
			// whole roll rather than just this branch, which is what keeps a table that names
			// itself from costing (picks ^ depth) before it gives up.
			if (!SubTable->RollEntriesAtDepth(Stream, OnEntryPicked, Depth + 1))
			{
				return false;
			}

			continue;
		}

		OnEntryPicked(*this, EntryIndex);
	}

	return true;
}

int32 UWeightedTableDefinition::PickEntry(FRandomStream& Stream) const
{
	const int32 TotalWeight = GetTotalWeight();

	if (TotalWeight <= 0)
	{
		return INDEX_NONE;
	}

	// one draw across the whole weight, then walk the entries until it lands - integer weights,
	// so "60 / 30 / 10" means exactly that rather than approximately
	int32 Roll = Stream.RandRange(0, TotalWeight - 1);

	for (int32 EntryIndex = 0; EntryIndex < GetNumEntries(); ++EntryIndex)
	{
		const int32 Weight = FMath::Max(GetEntryWeight(EntryIndex), 0);

		if (Roll < Weight)
		{
			return EntryIndex;
		}

		Roll -= Weight;
	}

	return INDEX_NONE;
}

int32 UWeightedTableDefinition::GetTotalWeight() const
{
	int32 TotalWeight = 0;

	for (int32 EntryIndex = 0; EntryIndex < GetNumEntries(); ++EntryIndex)
	{
		TotalWeight += FMath::Max(GetEntryWeight(EntryIndex), 0);
	}

	return TotalWeight;
}

bool UWeightedTableDefinition::ContainsNestingLoop() const
{
	TArray<const UWeightedTableDefinition*> Path;

	return ContainsNestingLoopFrom(Path);
}

bool UWeightedTableDefinition::ContainsNestingLoopFrom(TArray<const UWeightedTableDefinition*>& Path) const
{
	// only the tables on the way down count - a table two branches both name is a diamond, and a
	// diamond ends
	if (Path.Contains(this))
	{
		return true;
	}

	Path.Push(this);

	for (int32 EntryIndex = 0; EntryIndex < GetNumEntries(); ++EntryIndex)
	{
		const UWeightedTableDefinition* SubTable = GetEntrySubTable(EntryIndex);

		if (SubTable && SubTable->ContainsNestingLoopFrom(Path))
		{
			return true;
		}
	}

	Path.Pop();

	return false;
}
