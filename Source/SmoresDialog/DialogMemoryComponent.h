// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogMemoryComponent.generated.h"

/** How often, and how recently, a squad has seen one conversation */
USTRUCT()
struct SMORESDIALOG_API FDialogSeenEntry
{
	GENERATED_BODY()

	/** The qualified conversation id, "example.Shakedown" */
	UPROPERTY()
	FName ConversationId;

	UPROPERTY()
	int32 TimesSeen = 0;

	/** FDialogMemoryRecord::SeenCounter when it was last seen - bigger is more recent */
	UPROPERTY()
	int32 LastSeenOrder = 0;
};

/**
 *  What one squad remembers of dialog: the flags its conversations set, and the conversations it
 *  has seen.
 *
 *  **A plain reflected record**, the FCharacterRecord shape, so the save system can serialize it
 *  unchanged when it exists. Recency is a counter rather than a time, because world time starts
 *  again on every load and a counter survives one.
 *
 *  Flag names are shared by every package - a mod can read a flag the base game sets. Prefix a
 *  mod's own flags with its id to keep them apart.
 */
USTRUCT()
struct SMORESDIALOG_API FDialogMemoryRecord
{
	GENERATED_BODY()

	/** Every flag that is set. A flag not in the list is not set. */
	UPROPERTY()
	TArray<FName> Flags;

	UPROPERTY()
	TArray<FDialogSeenEntry> Seen;

	/** Goes up by one each time any conversation is seen */
	UPROPERTY()
	int32 SeenCounter = 0;

	bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }

	/** Sets or clears a flag. True if that changed anything. */
	bool SetFlag(FName Flag, bool bValue);

	/** The entry for exactly this qualified id, or null if never seen */
	const FDialogSeenEntry* FindSeen(FName ConversationId) const;

	/**
	 *  True if this conversation has been seen. Id may be qualified ("example.Shakedown") or just the
	 *  node's title ("Shakedown"), which matches that title in any package - so a writer can write
	 *  Seen(Shakedown) without the package prefix.
	 */
	bool HasSeen(FName Id) const;

	/** Counts one more viewing and makes it the most recent */
	void MarkSeen(FName ConversationId);
};

/**
 *  One player's squad's dialog memory - on AStrategyPlayerState, next to its standing and wallet.
 *
 *  **Dialog state is squad-scoped** (the dialog roadmap's settled rule): each player controls one
 *  squad, so "per squad" is "per player state", the UPlayerStandingComponent precedent. One
 *  player's conversation with a bandit is never another player's.
 *
 *  **Server-owned and not replicated.** Everything that reads it - conversation selection, the
 *  Flag() and Seen() facts, the effects - runs on the server. Mutators are authority-only and
 *  silently do nothing elsewhere, like every other shared-state mutator in the project.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESDIALOG_API UDialogMemoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDialogMemoryComponent();

	/** The memory on this player state, or null */
	static UDialogMemoryComponent* Get(const AActor* PlayerState);

	bool HasFlag(FName Flag) const { return Record.HasFlag(Flag); }

	/** Sets or clears a flag. Authority-only; false (and nothing changed) off-authority. */
	bool SetFlag(FName Flag, bool bValue);

	bool HasSeen(FName ConversationId) const { return Record.HasSeen(ConversationId); }

	/** Records that this squad just saw a conversation. Authority-only. */
	bool MarkSeen(FName ConversationId);

	/** Everything remembered, for selection and the debug dump */
	const FDialogMemoryRecord& GetRecord() const { return Record; }

private:

	bool HasOwnerAuthority() const;

	UPROPERTY()
	FDialogMemoryRecord Record;
};
