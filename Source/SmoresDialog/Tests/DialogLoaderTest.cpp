// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DialogLoader.h"
#include "DialogLibrary.h"
#include "Tests/SmoresDialogTestFactory.h"

/**
 *  The loader: discovery order, Requires, and the rule that broken content is skipped, never
 *  fatal. Every package here is built from strings (SmoresDialogTestFactory.h); nothing touches
 *  Content/Dialog or Mods/.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogCsvTest,
	"Smores.Dialog.Loader.ParsesCsvLikeASpreadsheet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogCsvTest::RunTest(const FString& Parameters)
{
	TArray<FDialogCsvRow> Rows;
	FString Error;
	int32 ErrorLine = 0;

	// a byte-order mark, CRLF line ends, a quoted comma, a doubled quote, a quoted line break, a
	// trimmed unquoted field, and no line break at the end
	const FString Text = FString(TEXT("\uFEFF")) +
		TEXT("Id,Text\r\n")
		TEXT("a,\"Stay back, stranger.\"\r\n")
		TEXT("b,\"She said \"\"no\"\".\"\r\n")
		TEXT("c,\"Two\r\nlines\"\r\n")
		TEXT("  d  ,  spaced  ");

	if (!TestTrue(TEXT("A spreadsheet's CSV parses"), SmoresDialog::ParseCsv(Text, Rows, Error, ErrorLine)))
	{
		AddInfo(Error);
		return true;
	}

	if (!TestEqual(TEXT("Five records"), Rows.Num(), 5))
	{
		return true;
	}

	TestEqual(TEXT("The byte-order mark isn't part of the first header"), Rows[0].Fields[0], FString(TEXT("Id")));
	TestEqual(TEXT("A quoted comma stays in the field"), Rows[1].Fields[1], FString(TEXT("Stay back, stranger.")));
	TestEqual(TEXT("\"\" is a quote"), Rows[2].Fields[1], FString(TEXT("She said \"no\".")));
	TestEqual(TEXT("A quoted line break is kept, as \\n"), Rows[3].Fields[1], FString(TEXT("Two\nlines")));
	TestEqual(TEXT("Unquoted fields are trimmed"), Rows[4].Fields[0], FString(TEXT("d")));
	TestEqual(TEXT("...the last one too, with no line break after it"), Rows[4].Fields[1], FString(TEXT("spaced")));

	// line numbers are where each record *starts*, counting the line break inside c
	TestEqual(TEXT("Record b starts on line 3"), Rows[2].Line, 3);
	TestEqual(TEXT("Record c starts on line 4"), Rows[3].Line, 4);
	TestEqual(TEXT("Record d starts on line 6, after c's two"), Rows[4].Line, 6);

	TestFalse(TEXT("A quote that never closes is refused"), SmoresDialog::ParseCsv(TEXT("Id,Text\na,b\nc,\"never\nclosed"), Rows, Error, ErrorLine));
	TestEqual(TEXT("...reporting the line it opened on"), ErrorLine, 3);

	TestFalse(TEXT("Text after a closing quote is refused"), SmoresDialog::ParseCsv(TEXT("Id,Text\na,\"quoted\"tail"), Rows, Error, ErrorLine));
	TestEqual(TEXT("...on its line"), ErrorLine, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogRequiresOrderTest,
	"Smores.Dialog.Loader.OrdersByRequires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogRequiresOrderTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// discovered in an order that is wrong on purpose: zeta needs alpha, mid needs zeta
	const FDialogLibrary Library = SmoresDialog::LoadPackages({
		MakeTestDialogPackage(TEXT("zeta"), TEXT("zeta"), { TEXT("alpha") }),
		MakeTestCorePackage(),
		MakeTestDialogPackage(TEXT("mid"), TEXT("mid"), { TEXT("zeta"), TEXT("core") }),
		MakeTestDialogPackage(TEXT("alpha"), TEXT("alpha")),
		MakeTestDialogPackage(TEXT("bravo"), TEXT("bravo"))
	}, Facts, nullptr);

	TestEqual(FString::Printf(TEXT("Nothing went wrong:\n%s"), *DescribeDialogProblems(Library)), Library.Problems.Num(), 0);

	// core first; then whatever is ready, alphabetically - alpha and bravo are, zeta waits for
	// alpha, mid waits for zeta
	TestEqual(TEXT("Core first, requirements before dependents, ties alphabetical"), DescribeLoadedPackages(Library), FString(TEXT("core, alpha, bravo, zeta, mid")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogMissingRequirementTest,
	"Smores.Dialog.Loader.MissingRequirementSkipsTheMod",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogMissingRequirementTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FString OneBark = MakeTestBarkCsv({ TEXT("greet,Hurt,,Hello.,1,0") });

	const FDialogLibrary Library = SmoresDialog::LoadPackages({
		MakeTestCorePackage(OneBark),
		MakeTestDialogPackage(TEXT("lanterns"), TEXT("lanterns"), { TEXT("wicks") }, OneBark),
		MakeTestDialogPackage(TEXT("oil"), TEXT("oil"), { TEXT("lanterns") }, OneBark),
		MakeTestDialogPackage(TEXT("unrelated"), TEXT("unrelated"), {}, OneBark)
	}, Facts, nullptr);

	TestEqual(TEXT("The base game and the unrelated mod still load"), DescribeLoadedPackages(Library), FString(TEXT("core, unrelated")));

	TestTrue(FString::Printf(TEXT("The missing requirement is named:\n%s"), *DescribeDialogProblems(Library)),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("requires 'wicks', which isn't installed")));
	TestTrue(TEXT("...and a mod needing the skipped mod is skipped too, saying why"),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("requires 'lanterns', which was skipped")));

	TestEqual(TEXT("Only the loaded packages' barks are in the library"), Library.Barks.Num(), 2);
	TestNull(TEXT("A skipped mod contributes nothing"), Library.FindBark(TEXT("lanterns.greet")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogRequiresCycleTest,
	"Smores.Dialog.Loader.RequiresCycleSkipsEveryoneInIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogRequiresCycleTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FDialogLibrary Library = SmoresDialog::LoadPackages({
		MakeTestCorePackage(),
		MakeTestDialogPackage(TEXT("ash"), TEXT("ash"), { TEXT("birch") }),
		MakeTestDialogPackage(TEXT("birch"), TEXT("birch"), { TEXT("ash") }),
		MakeTestDialogPackage(TEXT("cedar"), TEXT("cedar"), { TEXT("ash") }),
		MakeTestDialogPackage(TEXT("dune"), TEXT("dune"))
	}, Facts, nullptr);

	TestEqual(TEXT("Only packages outside the cycle load"), DescribeLoadedPackages(Library), FString(TEXT("core, dune")));

	TestEqual(FString::Printf(TEXT("Both members of the cycle say so:\n%s"), *DescribeDialogProblems(Library)),
		Library.Problems.FilterByPredicate([](const FDialogProblem& Problem) { return Problem.Message.Contains(TEXT("is in a Requires cycle (ash, birch)")); }).Num(), 2);
	TestTrue(TEXT("...and the mod behind it says what it was waiting for"),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("requires a package caught in a Requires cycle (ash, birch)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogDuplicateIdTest,
	"Smores.Dialog.Loader.DuplicateIdInAPackageIsAnError",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogDuplicateIdTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FDialogPackageSource Core = MakeTestCorePackage(MakeTestBarkCsv({
		TEXT("greet,Hurt,,First.,1,0"),
		TEXT("greet,Hurt,,Second.,1,0")
	}));

	// the same id again in a second file of the same package counts too - named to sort after test.csv,
	// since files load in path order
	Core.Files.Add({ TEXT("barks/zz_more.csv"), MakeTestBarkCsv({ TEXT("Greet,Downed,,Third.,1,0") }) });

	const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

	TestEqual(TEXT("One line under the id"), Library.Barks.Num(), 1);

	const FBarkLine* Greet = Library.FindBark(TEXT("core.greet"));

	TestTrue(TEXT("...and it is the first one written"), Greet && Greet->SourceText == TEXT("First."));
	TestTrue(FString::Printf(TEXT("The repeat on line 3 is an error:\n%s"), *DescribeDialogProblems(Library)),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("already used in this package"), 3));
	TestEqual(TEXT("...and so is the one in the second file, ids being case-insensitive"),
		Library.Problems.FilterByPredicate([](const FDialogProblem& Problem) { return Problem.File == TEXT("barks/zz_more.csv"); }).Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogSameLocalIdTest,
	"Smores.Dialog.Loader.SameLocalIdInTwoPackagesIsLegal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogSameLocalIdTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	const FDialogLibrary Library = SmoresDialog::LoadPackages({
		MakeTestCorePackage(MakeTestBarkCsv({ TEXT("greet,Hurt,,Core hello.,1,0") })),
		MakeTestDialogPackage(TEXT("lanterns"), TEXT("lanterns"), {}, MakeTestBarkCsv({ TEXT("greet,Hurt,,Lantern hello.,1,0") }))
	}, Facts, nullptr);

	TestEqual(FString::Printf(TEXT("No problem at all:\n%s"), *DescribeDialogProblems(Library)), Library.Problems.Num(), 0);

	const FBarkLine* CoreGreet = Library.FindBark(TEXT("core.greet"));
	const FBarkLine* LanternGreet = Library.FindBark(TEXT("lanterns.greet"));

	TestTrue(TEXT("The loader qualified each with its own package"), CoreGreet && LanternGreet);
	TestTrue(TEXT("...and they are different lines"), CoreGreet && LanternGreet && CoreGreet->SourceText != LanternGreet->SourceText);
	TestEqual(TEXT("Both are Hurt lines"), Library.GetBarksForEvent(EBarkEvent::Hurt).Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogBrokenRowTest,
	"Smores.Dialog.Loader.BrokenRowIsSkippedNeighboursLoad",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogBrokenRowTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// line 1 is the header; each broken row below names the line its error should carry
	const FDialogLibrary Library = SmoresDialog::LoadPackages({ MakeTestCorePackage(MakeTestBarkCsv({
		TEXT("good_first,Hurt,,Ow.,1,0"),                          // 2
		TEXT("bad_event,Hurtt,,Ow.,1,0"),                          // 3
		TEXT("bad_weight,Hurt,,Ow.,0,0"),                          // 4
		TEXT("bad_cooldown,Hurt,,Ow.,1,-5"),                       // 5
		TEXT("no_text,Hurt,,,1,0"),                                // 6
		TEXT("bad.id,Hurt,,Ow.,1,0"),                              // 7
		TEXT("unquoted_comma,Hurt,,Stay back, stranger.,1,0"),     // 8
		TEXT("# a comment row is skipped quietly,,,,,"),           // 9
		TEXT(",,,,,"),                                             // 10 - blank, skipped quietly
		TEXT("good_last,Hurt,,Ow again.,,")                        // 11 - weight and cooldown default
	})) }, Facts, nullptr);

	TestEqual(TEXT("Both good rows loaded"), Library.Barks.Num(), 2);
	TestNotNull(TEXT("...the one before the broken rows"), Library.FindBark(TEXT("core.good_first")));

	const FBarkLine* Last = Library.FindBark(TEXT("core.good_last"));

	if (TestNotNull(TEXT("...and the one after them"), Last))
	{
		TestEqual(TEXT("An empty weight is 1"), Last->Weight, 1);
		TestEqual(TEXT("An empty cooldown is 0"), Last->CooldownSeconds, 0.0f);
	}

	const FString Problems = DescribeDialogProblems(Library);

	TestTrue(FString::Printf(TEXT("An unknown event, on line 3:\n%s"), *Problems), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("event 'Hurtt' isn't one of"), 3));
	TestTrue(TEXT("A weight below 1, on line 4"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("weight"), 4));
	TestTrue(TEXT("A negative cooldown, on line 5"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("cooldown"), 5));
	TestTrue(TEXT("No text, on line 6"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("no text"), 6));
	TestTrue(TEXT("A dotted id, on line 7"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("no dots"), 7));
	TestTrue(TEXT("An unquoted comma, on line 8, rather than a silently truncated line"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("double quotes"), 8));
	TestEqual(TEXT("Exactly the six broken rows are reported"), Library.Problems.Num(), 6);

	const FDialogPackageInfo* Core = Library.FindPackage(TEXT("core"));
	TestTrue(TEXT("The package itself still loaded"), Core && Core->bLoaded && Core->NumBarks == 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogBadManifestTest,
	"Smores.Dialog.Loader.BadManifestsSkipThePackage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogBadManifestTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	auto WithManifest = [](const TCHAR* Folder, const FString& Json)
	{
		FDialogPackageSource Source;
		Source.FolderName = Folder;
		Source.Files.Add({ TEXT("mod.json"), Json });
		return Source;
	};

	FDialogPackageSource NoManifest;
	NoManifest.FolderName = TEXT("nomanifest");
	NoManifest.Files.Add({ TEXT("barks/test.csv"), MakeTestBarkCsv({ TEXT("a,Hurt,,A.,1,0") }) });

	const FDialogLibrary Library = SmoresDialog::LoadPackages({
		MakeTestCorePackage(),
		NoManifest,
		WithManifest(TEXT("broken"), TEXT("{ \"Id\": \"broken\", ")),
		WithManifest(TEXT("noid"), TEXT("{ \"DisplayName\": \"No id\" }")),
		WithManifest(TEXT("shouty"), TEXT("{ \"Id\": \"Shouty\" }")),
		WithManifest(TEXT("usurper"), TEXT("{ \"Id\": \"core\" }")),
		WithManifest(TEXT("first"), TEXT("{ \"Id\": \"twin\" }")),
		WithManifest(TEXT("second"), TEXT("{ \"Id\": \"twin\" }")),
		WithManifest(TEXT("selfish"), TEXT("{ \"Id\": \"selfish\", \"Requires\": [\"selfish\"] }"))
	}, Facts, nullptr);

	TestEqual(TEXT("Core and the first twin load; everything else is skipped"), DescribeLoadedPackages(Library), FString(TEXT("core, twin")));

	const FString Problems = DescribeDialogProblems(Library);

	TestTrue(FString::Printf(TEXT("No manifest:\n%s"), *Problems), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("no mod.json")));
	TestTrue(TEXT("Broken JSON"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("isn't valid JSON")));
	TestTrue(TEXT("No Id"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("has no \"Id\"")));
	TestTrue(TEXT("An Id that isn't lower case"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("lower-case")));
	TestTrue(TEXT("A mod claiming core's id"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("no mod may claim it")));
	TestTrue(TEXT("A second package with the same id"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("already used by the package in folder 'first'")));
	TestTrue(TEXT("A package requiring itself"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("can't require itself")));

	TestEqual(TEXT("Skipped packages are still listed, for the report"), Library.Packages.Num(), 9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogTranslationTest,
	"Smores.Dialog.Loader.TranslationsLoadAndAreChecked",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogTranslationTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	FDialogPackageSource Core = MakeTestCorePackage(MakeTestBarkCsv({
		TEXT("greet,TradeOpened,,Hello.,1,0"),
		TEXT("leave,TradeOpened,,Goodbye.,1,0")
	}));

	Core.Files.Add({ TEXT("localization/fr/barks.csv"),
		TEXT("Id,Text\n")
		TEXT("greet,Bonjour.\n")          // 2 - fine
		TEXT("greet,Salut.\n")            // 3 - already translated
		TEXT("renamed,Ancien.\n")         // 4 - no such line: kept, warned
		TEXT("leave,\n")                  // 5 - no text
		TEXT("core.leave,Au revoir.\n")   // 6 - qualified: translations name their own package's lines
	});

	Core.Files.Add({ TEXT("localization/not a culture/barks.csv"), TEXT("Id,Text\ngreet,Hola.\n") });
	Core.Files.Add({ TEXT("localization/de/barks.csv"), TEXT("Id,Text\ngreet,Hallo.\n") });

	const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);
	const FString Problems = DescribeDialogProblems(Library);

	TestEqual(FString::Printf(TEXT("Three translations landed - French greet, the stale one, German greet:\n%s"), *Problems), Library.Translations.Num(), 3);

	const FDialogTranslation* French = Library.Translations.FindByPredicate([](const FDialogTranslation& Translation)
	{
		return Translation.Culture == TEXT("fr") && Translation.LineId == FName(TEXT("core.greet"));
	});

	TestTrue(TEXT("French greet is the first one written"), French && French->Text == TEXT("Bonjour."));

	TestTrue(TEXT("A second translation of the same line is an error, on line 3"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("already translated"), 3));
	TestTrue(TEXT("A translation of a line that doesn't exist is a warning, on line 4"), HasDialogProblem(Library, EDialogProblemSeverity::Warning, TEXT("no loaded line"), 4));
	TestTrue(TEXT("An empty translation is an error, on line 5"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("no text"), 5));
	TestTrue(TEXT("A qualified id is an error, on line 6"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("without the package prefix"), 6));
	TestTrue(TEXT("A folder that isn't a culture name skips its file"), HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("isn't a culture name")));

	const FDialogPackageInfo* CoreInfo = Library.FindPackage(TEXT("core"));
	TestTrue(TEXT("The package counts what landed"), CoreInfo && CoreInfo->NumTranslations == 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresDialogHeaderTest,
	"Smores.Dialog.Loader.HeaderColumnsInAnyOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresDialogHeaderTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FDialogTestAnswers> Answers = MakeShared<FDialogTestAnswers>();
	const FDialogFactRegistry Facts = MakeTestDialogFacts(Answers);

	// a writer's own column order and a Notes column nobody reads; no Conditions, Weight or Cooldown at all
	FDialogPackageSource Core = MakeTestCorePackage();
	Core.Files.Add({ TEXT("barks/reordered.csv"), TEXT("TEXT,Notes,id,Event\nOw.,writer's note,ow,Hurt\n") });
	Core.Files.Add({ TEXT("barks/headless.csv"), TEXT("Id,Text\nno_event,Ow.\n") });

	const FDialogLibrary Library = SmoresDialog::LoadPackages({ Core }, Facts, nullptr);

	const FBarkLine* Ow = Library.FindBark(TEXT("core.ow"));

	if (TestNotNull(FString::Printf(TEXT("Columns are found by name, in any order and any case:\n%s"), *DescribeDialogProblems(Library)), Ow))
	{
		TestEqual(TEXT("...the text came from the TEXT column"), Ow->SourceText, FString(TEXT("Ow.")));
		TestEqual(TEXT("...and with no Conditions column the line is generic"), Ow->Condition.GetSpecificity(), 0);
	}

	TestTrue(TEXT("A file missing a required column is skipped whole, saying which are required"),
		HasDialogProblem(Library, EDialogProblemSeverity::Error, TEXT("Id, Event and Text are required"), 1));
	TestEqual(TEXT("...so only the reordered file's line loaded"), Library.Barks.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
