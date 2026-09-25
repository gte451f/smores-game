// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"
#include "DialogCondition.h"

/**
 *  One bark as loaded: a row of a package's barks/<file>.csv, checked and ready to select.
 *
 *  A definition in the game-data sense (game-data.md) - authored, identical on every machine,
 *  never written to at runtime. What a bark has *done* this session (when it was last said, by
 *  whom) is transient and lives in the bark director, not here.
 */
struct SMORESDIALOG_API FBarkLine
{
	/** The loader-qualified id, "core.trader_greet" - what crosses the network and keys the text */
	FName Id;

	/** The id as the writer typed it, "trader_greet" */
	FName LocalId;

	/** The package it came from, "core" */
	FName PackageId;

	EBarkEvent Event = EBarkEvent::Hurt;

	FDialogCondition Condition;

	/** The source-language line. Players never see this string directly - they see the string-table entry built from it, translated. */
	FString SourceText;

	/** Breaks a tie between equally specific, equally rested lines. Always at least 1. */
	int32 Weight = 1;

	/** How long the same speaker must wait before saying this line again, in world seconds. 0 means no wait. */
	float CooldownSeconds = 0.0f;

	/** Where it was written, for every message that mentions it */
	FString File;
	int32 Line = 0;
};

/** One translated line, from a package's localization/<culture>/<file>.csv */
struct SMORESDIALOG_API FDialogTranslation
{
	/** The qualified id of the line translated */
	FName LineId;

	/** The culture folder it came from - "fr", "pt-BR" */
	FString Culture;

	FString Text;
};

/** What one package declared, and what became of it */
struct SMORESDIALOG_API FDialogPackageInfo
{
	/** From mod.json, or the folder name when the manifest couldn't be read */
	FName Id;

	/** The folder it was read from */
	FString FolderName;

	FString DisplayName;

	FString Version;

	/** The game version the package says it was made for. Informational - nothing enforces it yet. */
	FString GameVersion;

	TArray<FName> Requires;

	/** True for the base game's own package, Content/Dialog/core */
	bool bIsCore = false;

	/** False when the whole package was skipped - a bad manifest, a missing requirement, a cycle */
	bool bLoaded = false;

	int32 NumBarks = 0;

	int32 NumTranslations = 0;
};

/**
 *  Everything the loader produced: the packages, in load order, the barks and translations they
 *  hold, indexed for the questions play asks, and every problem found on the way.
 *
 *  A plain value, deliberately, so a test can load packages from strings and inspect the result
 *  without a game instance, a string table or a world.
 */
struct SMORESDIALOG_API FDialogLibrary
{
	/** Every package discovered, core first, loaded ones in load order - skipped ones included, with bLoaded false */
	TArray<FDialogPackageInfo> Packages;

	TArray<FBarkLine> Barks;

	TArray<FDialogTranslation> Translations;

	TArray<FDialogProblem> Problems;

	/** The bark with this qualified id, or null */
	const FBarkLine* FindBark(FName LineId) const;

	/** Every bark for one event, in load order */
	TArray<const FBarkLine*> GetBarksForEvent(EBarkEvent Event) const;

	/** The package with this id, or null */
	const FDialogPackageInfo* FindPackage(FName PackageId) const;

	/** How many problems of one severity, in one package or (None) in all of them */
	int32 CountProblems(EDialogProblemSeverity Severity, FName PackageId = NAME_None) const;

	/** One line per package, "mod lanterns: loaded, 12 barks, 3 errors" - what the log gets at every load */
	TArray<FString> BuildSummaryLines() const;

	/** Rebuilds the look-ups below from Barks. The loader calls it; nothing else needs to. */
	void RebuildIndex();

private:

	TMap<FName, int32> BarkIndexById;

	TMap<EBarkEvent, TArray<int32>> BarkIndicesByEvent;
};
