// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogText.h"
#include "Internationalization/TextLocalizationResource.h"

/**
 *  How dialog translations reach Unreal's localization manager - see FDialogLocalizedTextSource.
 *
 *  The source is built and asked directly, never registered: switching the culture of the editor
 *  process running the suite would change every other test's text out from under it. What a real
 *  culture switch looks like is Jim's PIE check (SmoresSetCulture fr); this pins the part that can
 *  be wrong silently - which words a culture gets, and that every line is supplied every time.
 */

/** The display string the resource holds for Key, or "(missing)" */
static FString SmoresDialogTextTest_Entry(const FTextLocalizationResource& Resource, const TCHAR* Key)
{
	const FTextLocalizationResource::FEntry* Entry = Resource.Entries.Find(FTextId(FTextKey(TEXT("SmoresDialog.test")), FTextKey(Key)));

	return Entry && Entry->LocalizedString.IsValid() ? *Entry->LocalizedString : FString(TEXT("(missing)"));
}

static FDialogLocalizedTextSource::FLine SmoresDialogTextTest_Line(const TCHAR* Key, const TCHAR* Source, const TMap<FString, FString>& Translations)
{
	FDialogLocalizedTextSource::FLine Line;
	Line.Namespace = TEXT("SmoresDialog.test");
	Line.Key = Key;
	Line.SourceString = Source;
	Line.TextByCulture = Translations;

	return Line;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogTextFallbackTest,
	"Smores.Dialog.Text.TranslationsFallBackInCultureOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogTextFallbackTest::RunTest(const FString& Parameters)
{
	FDialogLocalizedTextSource Source;

	// culture keys are lower-cased on the way in, as PublishDialogText does
	Source.SetLines({
		SmoresDialogTextTest_Line(TEXT("test.greet"), TEXT("Hello."), { { TEXT("fr"), TEXT("Bonjour.") }, { TEXT("pt-br"), TEXT("Ol00E1.") } }),
		SmoresDialogTextTest_Line(TEXT("test.leave"), TEXT("Goodbye."), {})
	});

	// the manager hands a source its cultures most specific first: fr-CA, then fr, then the native en
	{
		FTextLocalizationResource Resource;
		const TArray<FString> Cultures = { TEXT("fr-CA"), TEXT("fr"), TEXT("en") };
		Source.BuildResource(Cultures, Resource);

		TestEqual(TEXT("fr-CA falls back to fr"), SmoresDialogTextTest_Entry(Resource, TEXT("test.greet")), FString(TEXT("Bonjour.")));
		TestEqual(TEXT("An untranslated line is supplied in its source text"), SmoresDialogTextTest_Entry(Resource, TEXT("test.leave")), FString(TEXT("Goodbye.")));
		TestEqual(TEXT("Every line is supplied, translated or not"), Resource.Entries.Num(), 2);

		const FTextLocalizationResource::FEntry* Entry = Resource.Entries.Find(FTextId(FTextKey(TEXT("SmoresDialog.test")), FTextKey(TEXT("test.greet"))));

		// the manager only shows a translation whose source hash matches the text's current source -
		// which is also what stops a stale translation outliving an edited line
		TestTrue(TEXT("The entry carries its source text's hash"), Entry && Entry->SourceStringHash == FTextLocalizationResource::HashString(FString(TEXT("Hello."))));
	}

	// a culture nobody translated gets the source text for everything - this is what makes switching
	// *back* work, since the manager never clears what an earlier culture put in its live table
	{
		FTextLocalizationResource Resource;
		const TArray<FString> Cultures = { TEXT("de"), TEXT("en") };
		Source.BuildResource(Cultures, Resource);

		TestEqual(TEXT("German, untranslated, shows the source"), SmoresDialogTextTest_Entry(Resource, TEXT("test.greet")), FString(TEXT("Hello.")));
		TestEqual(TEXT("...for every line"), Resource.Entries.Num(), 2);
	}

	{
		FTextLocalizationResource Resource;
		const TArray<FString> Cultures = { TEXT("pt-BR"), TEXT("pt"), TEXT("en") };
		Source.BuildResource(Cultures, Resource);

		TestEqual(TEXT("A culture's case doesn't matter - the files say pt-br, ICU says pt-BR"), SmoresDialogTextTest_Entry(Resource, TEXT("test.greet")), FString(TEXT("Ol00E1.")));
	}

	// dialog is game text: an engine- or editor-only load adds nothing, and a forced game load adds it all
	{
		FTextLocalizationResource Native;
		FTextLocalizationResource Localized;
		const TArray<FString> Cultures = { TEXT("fr"), TEXT("en") };

		Source.LoadLocalizedResources(ELocalizationLoadFlags::Engine | ELocalizationLoadFlags::Editor | ELocalizationLoadFlags::Native, Cultures, Native, Localized);
		TestEqual(TEXT("An engine/editor load adds nothing"), Localized.Entries.Num(), 0);

		Source.LoadLocalizedResources(ELocalizationLoadFlags::Game | ELocalizationLoadFlags::ForceLocalizedGame, Cultures, Native, Localized);
		TestEqual(TEXT("A game load (forced, as the editor's preview does) adds every line"), Localized.Entries.Num(), 2);
		TestEqual(TEXT("...translated"), SmoresDialogTextTest_Entry(Localized, TEXT("test.greet")), FString(TEXT("Bonjour.")));
	}

	FString NativeCulture;
	TestTrue(TEXT("The source names a native game culture, which the editor's preview requires"), Source.GetNativeCultureName(ELocalizedTextSourceCategory::Game, NativeCulture));
	TestEqual(TEXT("...the dialog files' own language"), NativeCulture, FString(TEXT("en")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
