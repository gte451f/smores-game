// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogFacts.h"
#include "DialogEffects.h"
#include "DialogLibrary.h"
#include "SmoresDialogSubsystem.generated.h"

/** Broadcast after every load, first or hot reload, once the new library is in place */
DECLARE_MULTICAST_DELEGATE(FOnDialogLibraryLoaded);

/** Broadcast at the start of a hot reload, while the old library is still in place */
DECLARE_MULTICAST_DELEGATE(FOnDialogLibraryWillReload);

/**
 *  The loaded dialog library, and the loader that fills it.
 *
 *  **A game instance subsystem**, so it outlives map changes and exists on every machine - the
 *  server needs the lines to decide what is said, and a client needs them to display it in its own
 *  language. Dialog crosses the network as a line id, never as text (see UBarkDirectorComponent).
 *
 *  Loads at startup and again on demand (SmoresReloadDialog). Everything it holds is a
 *  *definition* in the game-data sense: authored, identical everywhere, never written to in play.
 *  The loading itself is SmoresDialog::GatherPackagesFromDisk and SmoresDialog::LoadPackages; this
 *  class adds the disk locations, the game's fact list and known content ids, the report, and the
 *  text registration.
 */
UCLASS()
class SMORESDIALOG_API USmoresDialogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** This world's dialog subsystem, or null (a test world, the editor outside play) */
	static USmoresDialogSubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	/**
	 *  Re-reads every package from disk, replaces the library, republishes the text and logs the
	 *  summary. The whole load, every time - a writer edits a file, types SmoresReloadDialog and
	 *  sees the change without restarting.
	 */
	void Reload();

	/** Everything loaded */
	const FDialogLibrary& GetLibrary() const { return Library; }

	/** The facts conditions may use - and conversation scripts may call */
	const FDialogFactRegistry& GetFacts() const { return Facts; }

	/** The effects conversation scripts may run */
	const FDialogEffectRegistry& GetEffects() const { return Effects; }

	/** The id and version of every package that loaded, in load order - what a join-time mismatch check would compare (the session roadmap's job) */
	TArray<FString> GetLoadedPackageVersions() const;

	/** A line's text in the current culture - a bark's, a conversation line's or a topic's label - or empty for an id this machine doesn't have */
	FText GetLineText(FName LineId) const;

	/** Logs the per-package summary and every problem. bIncludeFacts adds the writers' reference: every fact, and who each event carries. */
	void LogReport(bool bIncludeFacts) const;

	/** Fired after every load. The bark director forgets its recency state here, since the lines may have changed. */
	FOnDialogLibraryLoaded OnLibraryLoaded;

	/** Fired before a hot reload replaces the library. Running conversations end here rather than being carried across. */
	FOnDialogLibraryWillReload OnLibraryWillReload;

private:

	FDialogFactRegistry Facts;

	FDialogEffectRegistry Effects;

	/** False until the first load, so the first isn't announced as a reload */
	bool bHasLoaded = false;

	FDialogLibrary Library;
};
