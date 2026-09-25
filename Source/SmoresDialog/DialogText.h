// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Internationalization/ILocalizedTextSource.h"

struct FDialogLibrary;
class FTextLocalizationResource;

/**
 *  Hands loaded dialog translations to Unreal's localization manager.
 *
 *  **Why this exists at all.** Unreal's normal translation pipeline gathers text from assets and
 *  code at edit time and compiles it into .locres files; it has no idea about lines read from a
 *  file at startup. Each package's lines are registered as a runtime string table (namespace
 *  "SmoresDialog.<package>", key = the line's qualified id), and a string-table entry asks the
 *  localization manager for its display string every time it is shown. So this source's only job
 *  is to tell the manager, per culture, what each of those keys should display - and because the
 *  manager re-asks every source whenever the culture changes, a bark switches language live, with
 *  no code of ours watching for it.
 *
 *  **Every line is supplied every time, translated or not.** The manager's live table is only ever
 *  added to or overwritten, never cleared, so a culture this source said nothing about would keep
 *  showing the previous culture's words. Switching back from French to English therefore works only
 *  because English "translations" (the source text) are supplied for every line.
 *
 *  Thread-safe: the manager loads sources on a worker thread.
 */
class SMORESDIALOG_API FDialogLocalizedTextSource : public ILocalizedTextSource
{
public:

	/** One line as the manager sees it */
	struct FLine
	{
		FString Namespace;

		FString Key;

		FString SourceString;

		/** Keyed by lower-cased culture name - "fr", "pt-br" */
		TMap<FString, FString> TextByCulture;
	};

	/** Replaces everything this source supplies */
	void SetLines(TArray<FLine> InLines);

	/**
	 *  What a load adds, without the load flags: every line in the first of PrioritizedCultures that
	 *  has a translation for it, else in its source text. Public so a test can prove the fallback
	 *  order without switching the editor's culture.
	 */
	void BuildResource(TArrayView<const FString> PrioritizedCultures, FTextLocalizationResource& OutResource) const;

	//~ Begin ILocalizedTextSource interface
	virtual int32 GetPriority() const override;
	virtual bool GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName) override;
	virtual void GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames) override;
	virtual void LoadLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource) override;
	//~ End ILocalizedTextSource interface

private:

	mutable FCriticalSection Lock;

	TArray<FLine> Lines;

	/** Every culture any line has a translation in, as the files spelled it */
	TSet<FString> Cultures;
};

namespace SmoresDialog
{
	/** "SmoresDialog.core" - the string table (and text namespace) a package's lines live in */
	SMORESDIALOG_API FName GetStringTableId(FName PackageId);

	/**
	 *  Registers every loaded package's lines as a runtime string table, hands their translations to
	 *  the localization manager, and asks it to refresh. Game thread only.
	 *
	 *  Process-wide, because string tables and the localization manager are: two PIE instances
	 *  loading the same files publish the same thing. A package that stopped loading has its table
	 *  unregistered.
	 */
	SMORESDIALOG_API void PublishDialogText(const FDialogLibrary& Library);

	/** A published line's text - translated to whatever culture is current, and following it when it changes. Empty if nothing published that line. */
	SMORESDIALOG_API FText GetPublishedLineText(FName LineId, FName PackageId);

	/**
	 *  Switches the language game text is shown in. In the editor this is game localization
	 *  preview, so the editor's own menus don't change language with it; in a game it is the
	 *  current language. False, with a reason, for a culture this build doesn't have.
	 */
	SMORESDIALOG_API bool SetDialogCulture(const FString& Culture, FString& OutMessage);

	/** Undoes an editor preview SetDialogCulture started, if one is running. Called when play ends, so the editor isn't left previewing French. */
	SMORESDIALOG_API void EndDialogCulturePreview();
}
