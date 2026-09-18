// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ActivityEntry.h"
#include "SmoresActivityLog.generated.h"

class APlayerController;

/** Broadcast once per posted entry, carrying the entry that was just added. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivityEntryAdded, const FActivityEntry&);

/**
 *  The record of what just happened, as the activity feed draws it.
 *
 *  **A ULocalPlayerSubsystem, which is the whole point.** The feed is per-local-player client-side
 *  UI state: which lines *this* player has seen is nobody else's business, and in split-screen or
 *  co-op two players' feeds legitimately differ. A local player subsystem is keyed to a local
 *  player by construction, so there is no way to write the singleton-player bug here - see
 *  multiplayer-discipline.md. Nothing on this class is replicated and nothing is authority-gated,
 *  because nothing here is shared state.
 *
 *  Server-side events reach it the way every other server-side message reaches a player: through
 *  an existing client RPC (AStrategyPlayerController::Client_NotifyActivity) or a delegate that
 *  already fires locally (UHealthComponent's). Nothing here reads server state directly.
 *
 *  **It lives in SmoresCore because everything has to be able to post to it.** Combat, items,
 *  economy and the controller all have something to say, and SmoresCore is the one module all of
 *  them can see.
 *
 *  The store is a fixed-capacity ring: past Capacity, the oldest line is evicted. That is what
 *  makes "notifications persist until dismissed" affordable - nothing is thrown away on a timer,
 *  only pushed out by newer news. The 8-second fade the feed applies is a *display* rule; the
 *  entry is still there, and expanding the feed shows it.
 */
UCLASS()
class SMORESCORE_API USmoresActivityLog : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	/**
	 *  This player's activity log, or null before their local player exists.
	 *
	 *  Takes the controller rather than a world, so there is no "the" player to get wrong: a
	 *  remote controller has no local player and correctly gets nothing back.
	 */
	static USmoresActivityLog* Get(const APlayerController* OwningPlayer);

	/** Fired once per posted entry. Non-dynamic, so a widget binds with AddUObject and a test with AddLambda. */
	FOnActivityEntryAdded OnEntryAdded;

	/**
	 *  Records one line and broadcasts it. Text is already worded; Source names who it came from,
	 *  or is empty for something nobody in particular did.
	 *
	 *  Returns the entry as stored, id and timestamp filled in.
	 */
	FActivityEntry Post(EActivityCategory Category, EActivitySeverity Severity, const FText& Text, const FText& Source = FText::GetEmpty());

	/** Everything still held, oldest first */
	const TArray<FActivityEntry>& GetEntries() const { return Entries; }

	/** Everything still held in one category, oldest first. The feed's SQUAD / COMMS / QUESTS tabs. */
	TArray<FActivityEntry> GetEntries(EActivityCategory Category) const;

	/** How many entries are held before the oldest starts being evicted */
	int32 GetCapacity() const { return Capacity; }

	/**
	 *  Changes how many entries are held. Shrinking evicts the oldest immediately, so the
	 *  invariant "never more than Capacity entries" holds the moment this returns rather than
	 *  after the next post.
	 */
	void SetCapacity(int32 NewCapacity);

	/** Throws the record away. Nothing in the game calls this; it exists for tests and for a future "clear" control. */
	void Clear();

protected:

	/**
	 *  How many lines are kept. The mock shows a handful and an expandable history; this is the
	 *  history. Whether the number is right is a thing to feel rather than reason about - listed
	 *  as an open tuning question in game-systems/hud-and-panels.md.
	 */
	int32 Capacity = 64;

	/** Oldest first, so the feed reads top-to-bottom in the order things happened */
	TArray<FActivityEntry> Entries;

	/** Id given to the next entry. Never reused, never reset by eviction. */
	int32 NextEntryId = 1;

	/** Drops the oldest entries until the store is within Capacity */
	void EvictToCapacity();
};
