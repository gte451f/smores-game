// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresActivityLog.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

USmoresActivityLog* USmoresActivityLog::Get(const APlayerController* OwningPlayer)
{
	if (!OwningPlayer)
	{
		return nullptr;
	}

	// GetLocalPlayer() is null on a controller that isn't this machine's, which is the correct
	// answer: a remote player's feed is on their machine, not here
	ULocalPlayer* LocalPlayer = OwningPlayer->GetLocalPlayer();

	return LocalPlayer ? LocalPlayer->GetSubsystem<USmoresActivityLog>() : nullptr;
}

FActivityEntry USmoresActivityLog::Post(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source)
{
	FActivityEntry Entry;

	Entry.Category = Category;
	Entry.Severity = Severity;
	Entry.Text = Text;
	Entry.Source = Source;

	// wall-clock, not world time - the feed fades on seconds the player experiences, so pausing
	// or running at 8x mustn't change how long a line stays legible
	Entry.Timestamp = FPlatformTime::Seconds();
	Entry.Id = NextEntryId++;

	Entries.Add(Entry);

	EvictToCapacity();

	// broadcast after the store is settled, so a handler that reads GetEntries() sees the same
	// thing it would see on the next frame rather than a list one eviction out of date
	OnEntryAdded.Broadcast(Entry);

	return Entry;
}

TArray<FActivityEntry> USmoresActivityLog::GetEntries(EActivityCategory Category) const
{
	TArray<FActivityEntry> Filtered;

	for (const FActivityEntry& Entry : Entries)
	{
		if (Entry.Category == Category)
		{
			Filtered.Add(Entry);
		}
	}

	return Filtered;
}

void USmoresActivityLog::SetCapacity(int32 NewCapacity)
{
	Capacity = FMath::Max(1, NewCapacity);

	EvictToCapacity();
}

void USmoresActivityLog::Clear()
{
	Entries.Reset();
}

void USmoresActivityLog::EvictToCapacity()
{
	if (Entries.Num() <= Capacity)
	{
		return;
	}

	// oldest-first, in one shift rather than one RemoveAt per entry. At a capacity in the tens
	// this is a memmove of a few kilobytes on the frame something is posted, which is cheaper
	// than the circular-index bookkeeping a "real" ring buffer would need everywhere it is read.
	Entries.RemoveAt(0, Entries.Num() - Capacity, EAllowShrinking::No);
}
