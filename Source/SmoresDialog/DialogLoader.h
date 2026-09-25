// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"
#include "DialogLibrary.h"

class FDialogFactRegistry;
class FDialogEffectRegistry;
struct FDialogKnownIds;

/**
 *  One file of a package, already read. Path is relative to the package folder, with forward
 *  slashes - "barks/core.csv".
 */
struct SMORESDIALOG_API FDialogSourceFile
{
	FString Path;

	/** The text, for every file but a compiled conversation */
	FString Contents;

	/** The bytes, for a compiled conversation (conversations/<name>.yarnc) - it isn't text */
	TArray<uint8> Bytes;

	/** When the file was last written, where the disk says (zero when built in memory). Only the stale-compile check reads it. */
	FDateTime Timestamp;
};

/**
 *  One package folder's files, already read.
 *
 *  **This is the in-memory entry point.** The disk load is GatherPackagesFromDisk followed by
 *  LoadPackages; a test builds these from strings and calls LoadPackages directly, the same shape
 *  as UWorldFactionComponent::InitializeFromDefinitions. Nothing here touches the disk, a string
 *  table or a world.
 */
struct SMORESDIALOG_API FDialogPackageSource
{
	/** The folder's name - "core", or a mod's folder under Mods/ */
	FString FolderName;

	/** True for Content/Dialog/core, the base game. Exactly one source should be. */
	bool bIsCore = false;

	TArray<FDialogSourceFile> Files;
};

/** One CSV record and the line it started on */
struct SMORESDIALOG_API FDialogCsvRow
{
	/** 1-based line the record starts on. A quoted field may run on to later lines. */
	int32 Line = 0;

	TArray<FString> Fields;
};

namespace SmoresDialog
{
	/** The base game's package id. Mods may not claim it. */
	SMORESDIALOG_API FName CorePackageId();

	/** Content/Dialog/core, staged into a packaged build as-is (DirectoriesToAlwaysStageAsUFS) */
	SMORESDIALOG_API FString GetCoreDirectory();

	/** Mods/, next to the installed game - one folder per mod */
	SMORESDIALOG_API FString GetModsDirectory();

	/**
	 *  Reads a CSV the way a spreadsheet writes one: comma-separated, a field in "double quotes" may
	 *  hold commas, line breaks and "" for a quote. Unquoted fields are trimmed; quoted ones are kept
	 *  exactly. Returns false, with the line it happened on, for a quote that never closes or text
	 *  after a closing quote - the whole file is unreadable past that point.
	 */
	SMORESDIALOG_API bool ParseCsv(const FString& Text, TArray<FDialogCsvRow>& OutRows, FString& OutError, int32& OutErrorLine);

	/**
	 *  Reads every package folder: core first, then each folder under ModsDirectory in name order.
	 *  Only the files the loader understands are read: mod.json, barks/*.csv, localization/, and
	 *  conversations/ (each .yarn with the .yarnc, -Lines.csv and -Metadata.csv ysc writes beside
	 *  it). A missing Mods folder is normal; a missing core folder is an error.
	 */
	SMORESDIALOG_API void GatherPackagesFromDisk(const FString& CoreDirectory, const FString& ModsDirectory, TArray<FDialogPackageSource>& OutSources, TArray<FDialogProblem>& OutProblems);

	/**
	 *  The whole load, after discovery: read each manifest, order the packages (core first, then
	 *  mods so that everything a mod Requires loads before it), parse and check every bark and
	 *  translation, and index the result.
	 *
	 *  **Broken content is skipped, never fatal.** A bad row costs that row; a bad manifest, a
	 *  missing requirement or a Requires cycle costs that package (and anything that needs it).
	 *  Every problem is in the returned library's Problems, pinned to package, file and line.
	 *
	 *  Ids are written local to their package and qualified here: a writer types trader_greet and
	 *  the library holds core.trader_greet. Two packages may use the same local id - they never
	 *  collide. KnownIds, when given, turns a condition naming a faction or definition nobody loaded
	 *  into a warning.
	 *
	 *  Conversations are checked against Facts (every function a script calls) and Effects (every
	 *  command it runs, with its arguments) - the game's own effects when Effects is null.
	 */
	SMORESDIALOG_API FDialogLibrary LoadPackages(const TArray<FDialogPackageSource>& Sources, const FDialogFactRegistry& Facts, const FDialogKnownIds* KnownIds, const FDialogEffectRegistry* Effects = nullptr);
}
