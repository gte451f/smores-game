// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogText.h"
#include "DialogLibrary.h"
#include "SmoresDialog.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Misc/ScopeLock.h"

namespace
{
	/** The dialog files' source language. Every package is assumed to be written in it - see dialog.md's Known Gaps. */
	const TCHAR* const DialogSourceCulture = TEXT("en");

	/** The one text source, registered with the localization manager on first publish and kept for the life of the process */
	TSharedPtr<FDialogLocalizedTextSource> GDialogTextSource;

	/** Every string table the last publish registered, so the next can unregister the ones that went away */
	TSet<FName> GPublishedDialogTables;

#if WITH_EDITOR
	/** The culture SetDialogCulture is previewing in the editor, or empty */
	FString GDialogPreviewCulture;
#endif

	/** Tells the localization manager to ask every source again */
	void RefreshDialogLocalization()
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			// a plain refresh in the editor loads no game text at all, so while a preview is running
			// it has to be the preview that re-runs, or a reloaded line would lose its translation.
			// Ours if SmoresSetCulture started it; otherwise it is the preview language configured in
			// Editor Preferences, which the no-argument overload re-applies rather than switching off.
			if (FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled())
			{
				if (GDialogPreviewCulture.IsEmpty())
				{
					FTextLocalizationManager::Get().EnableGameLocalizationPreview();
				}
				else
				{
					FTextLocalizationManager::Get().EnableGameLocalizationPreview(GDialogPreviewCulture);
				}
			}

			// with no preview the editor shows game text in its source language, which a
			// string-table entry does with no help - a full refresh would reload every editor
			// translation for nothing
			return;
		}
#endif

		FTextLocalizationManager::Get().RefreshResources();
	}
}

void FDialogLocalizedTextSource::SetLines(TArray<FLine> InLines)
{
	FScopeLock ScopeLock(&Lock);

	Lines = MoveTemp(InLines);
	Cultures.Reset();

	for (const FLine& Line : Lines)
	{
		for (const TPair<FString, FString>& Translation : Line.TextByCulture)
		{
			Cultures.Add(Translation.Key);
		}
	}
}

void FDialogLocalizedTextSource::BuildResource(TArrayView<const FString> PrioritizedCultures, FTextLocalizationResource& OutResource) const
{
	FScopeLock ScopeLock(&Lock);

	TArray<FString> CultureKeys;

	for (const FString& Culture : PrioritizedCultures)
	{
		CultureKeys.Add(Culture.ToLower());
	}

	for (const FLine& Line : Lines)
	{
		const FString* Chosen = nullptr;
		int32 Priority = CultureKeys.Num();

		for (int32 Index = 0; Index < CultureKeys.Num() && !Chosen; ++Index)
		{
			Chosen = Line.TextByCulture.Find(CultureKeys[Index]);
			Priority = Index;
		}

		// the source text stands in for a culture nobody translated - see the class comment for why
		// it has to be supplied rather than left out
		OutResource.AddEntry(Line.Namespace, Line.Key, Line.SourceString, Chosen ? *Chosen : Line.SourceString, Priority);
	}
}

int32 FDialogLocalizedTextSource::GetPriority() const
{
	// low, so that a real game localization target, once the project has one, answers first where
	// the two overlap (the native culture question below); the namespaces themselves never collide
	return ELocalizedTextSourcePriority::Low;
}

bool FDialogLocalizedTextSource::GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName)
{
	// Game localization preview refuses to start without a native game culture, and the project has
	// no localization target to supply one - so the dialog files' own language is the answer
	if (InCategory == ELocalizedTextSourceCategory::Game)
	{
		OutNativeCultureName = DialogSourceCulture;
		return true;
	}

	return false;
}

void FDialogLocalizedTextSource::GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames)
{
	if (!ShouldLoadGame(InLoadFlags))
	{
		return;
	}

	FScopeLock ScopeLock(&Lock);

	OutLocalizedCultureNames.Append(Cultures);
}

void FDialogLocalizedTextSource::LoadLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource)
{
	// dialog is game text: nothing to do for an editor- or engine-only load
	if (!ShouldLoadGame(InLoadFlags))
	{
		return;
	}

	// the editor shows game text in its source language unless a preview asks otherwise, and a
	// string-table entry with no live entry already displays its source
	if (ShouldLoadNativeGameData(InLoadFlags))
	{
		return;
	}

	BuildResource(InPrioritizedCultures, InOutLocalizedResource);
}

namespace SmoresDialog
{
	FName GetStringTableId(FName PackageId)
	{
		return FName(*FString::Printf(TEXT("SmoresDialog.%s"), *PackageId.ToString()));
	}

	void PublishDialogText(const FDialogLibrary& Library)
	{
		check(IsInGameThread());

		FStringTableRegistry& Registry = FStringTableRegistry::Get();

		// one table per loaded package, reused across reloads so text already on screen keeps
		// pointing at a live table
		TSet<FName> Tables;

		for (const FDialogPackageInfo& Package : Library.Packages)
		{
			if (!Package.bLoaded)
			{
				continue;
			}

			const FName TableId = GetStringTableId(Package.Id);
			FStringTablePtr Table = Registry.FindMutableStringTable(TableId);

			if (Table.IsValid())
			{
				Table->ClearSourceStrings();
			}
			else
			{
				FStringTableRef NewTable = FStringTable::NewStringTable();
				NewTable->SetNamespace(TableId.ToString());
				Registry.RegisterStringTable(TableId, NewTable);
			}

			Tables.Add(TableId);
		}

		TArray<FDialogLocalizedTextSource::FLine> SourceLines;
		TMap<FName, int32> SourceLineById;

		for (const FBarkLine& Bark : Library.Barks)
		{
			const FName TableId = GetStringTableId(Bark.PackageId);

			if (FStringTablePtr Table = Registry.FindMutableStringTable(TableId))
			{
				// the editor's string tables carry translator notes and a game's don't, so the setter's
				// signature differs between the two builds
#if WITH_EDITORONLY_DATA
				Table->SetSourceString(Bark.Id.ToString(), Bark.SourceText, FString());
#else
				Table->SetSourceString(Bark.Id.ToString(), Bark.SourceText);
#endif
			}

			FDialogLocalizedTextSource::FLine Line;
			Line.Namespace = TableId.ToString();
			Line.Key = Bark.Id.ToString();
			Line.SourceString = Bark.SourceText;

			SourceLineById.Add(Bark.Id, SourceLines.Add(MoveTemp(Line)));
		}

		for (const FDialogTranslation& Translation : Library.Translations)
		{
			if (const int32* Index = SourceLineById.Find(Translation.LineId))
			{
				SourceLines[*Index].TextByCulture.Add(Translation.Culture.ToLower(), Translation.Text);
			}
		}

		for (const FName Previous : GPublishedDialogTables)
		{
			if (!Tables.Contains(Previous))
			{
				Registry.UnregisterStringTable(Previous);
			}
		}

		GPublishedDialogTables = MoveTemp(Tables);

		const bool bFirstPublish = !GDialogTextSource.IsValid();

		if (bFirstPublish)
		{
			GDialogTextSource = MakeShared<FDialogLocalizedTextSource>();
		}

		GDialogTextSource->SetLines(MoveTemp(SourceLines));

		if (bFirstPublish)
		{
			FTextLocalizationManager::Get().RegisterTextSource(GDialogTextSource.ToSharedRef(), /*bRefreshResources*/ false);
		}

		RefreshDialogLocalization();
	}

	FText GetPublishedLineText(FName LineId, FName PackageId)
	{
		const FName TableId = GetStringTableId(PackageId);
		const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(TableId);

		if (!Table.IsValid() || !Table->FindEntry(LineId.ToString()).IsValid())
		{
			return FText::GetEmpty();
		}

		return FText::FromStringTable(TableId, LineId.ToString(), EStringTableLoadingPolicy::Find);
	}

	bool SetDialogCulture(const FString& Culture, FString& OutMessage)
	{
		const FCulturePtr Target = FInternationalization::Get().GetCulture(Culture);

		if (!Target.IsValid())
		{
			OutMessage = FString::Printf(TEXT("this build doesn't know the culture '%s' - a packaged build only has the cultures its InternationalizationPreset includes"), *Culture);
			return false;
		}

		const FString CultureName = Target->GetName();

#if WITH_EDITOR
		if (GIsEditor)
		{
			// preview rather than switch, so the editor's own menus stay in the editor's language;
			// previewing the native culture is how the engine turns a preview off
			GDialogPreviewCulture = CultureName.Equals(DialogSourceCulture, ESearchCase::IgnoreCase) ? FString() : CultureName;
			FTextLocalizationManager::Get().EnableGameLocalizationPreview(CultureName);

			OutMessage = GDialogPreviewCulture.IsEmpty()
				? FString::Printf(TEXT("game text back in %s"), *CultureName)
				: FString::Printf(TEXT("previewing game text in %s (the editor's own menus are unchanged) - 'SmoresSetCulture %s' to return"), *CultureName, DialogSourceCulture);
			return true;
		}
#endif

		FInternationalization::Get().SetCurrentLanguage(CultureName);
		OutMessage = FString::Printf(TEXT("language set to %s"), *CultureName);
		return true;
	}

	void EndDialogCulturePreview()
	{
#if WITH_EDITOR
		if (GIsEditor && !GDialogPreviewCulture.IsEmpty())
		{
			GDialogPreviewCulture.Reset();
			FTextLocalizationManager::Get().DisableGameLocalizationPreview();
		}
#endif
	}
}
