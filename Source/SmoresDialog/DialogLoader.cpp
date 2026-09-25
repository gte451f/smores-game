// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogLoader.h"
#include "DialogFacts.h"
#include "DialogEffects.h"
#include "ConversationPlayer.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "DialogLoaderInternal.h"

using namespace SmoresDialogLoading;

namespace
{
	const TCHAR* const DialogManifestFileName = TEXT("mod.json");
	const TCHAR* const DialogBarksFolder = TEXT("barks/");
	const TCHAR* const DialogLocalizationFolder = TEXT("localization/");
	const TCHAR* const DialogConversationsFolder = TEXT("conversations/");

	bool IsValidDialogPackageId(const FString& Id)
	{
		if (Id.IsEmpty() || !FChar::IsLower(Id[0]))
		{
			return false;
		}

		for (const TCHAR Character : Id)
		{
			if (!FChar::IsLower(Character) && !FChar::IsDigit(Character) && Character != TEXT('_'))
			{
				return false;
			}
		}

		return true;
	}

	bool IsValidDialogCultureName(const FString& Culture)
	{
		if (Culture.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Character : Culture)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('-') && Character != TEXT('_'))
			{
				return false;
			}
		}

		return true;
	}

	/** Every .csv under Folder ("barks/"), sorted so the load order never depends on the file system's */
	TArray<const FDialogSourceFile*> GetDialogCsvFiles(const FDialogPackageSource& Source, const TCHAR* Folder)
	{
		return GetDialogFilesIn(Source, Folder, TEXT(".csv"));
	}

	/** Reads mod.json into Info. False, with a reason, if it can't be used at all. */
	bool ParseDialogManifest(const FString& Contents, FDialogPackageInfo& Info, FString& OutError)
	{
		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Contents);

		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			OutError = FString::Printf(TEXT("mod.json isn't valid JSON (%s)"), *Reader->GetErrorMessage());
			return false;
		}

		FString Id;

		if (!Root->TryGetStringField(TEXT("Id"), Id) || Id.IsEmpty())
		{
			OutError = TEXT("mod.json has no \"Id\" - it is the prefix every line in the package gets");
			return false;
		}

		if (!IsValidDialogPackageId(Id))
		{
			OutError = FString::Printf(TEXT("\"Id\" '%s' must be lower-case letters, digits and _, starting with a letter"), *Id);
			return false;
		}

		Info.Id = FName(*Id);

		if (!Root->TryGetStringField(TEXT("DisplayName"), Info.DisplayName))
		{
			Info.DisplayName = Id;
		}

		Root->TryGetStringField(TEXT("Version"), Info.Version);
		Root->TryGetStringField(TEXT("GameVersion"), Info.GameVersion);

		const TArray<TSharedPtr<FJsonValue>>* Requires = nullptr;

		if (Root->TryGetArrayField(TEXT("Requires"), Requires) && Requires)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Requires)
			{
				FString RequiredId;

				if (!Value.IsValid() || !Value->TryGetString(RequiredId) || !IsValidDialogPackageId(RequiredId))
				{
					OutError = TEXT("every \"Requires\" entry must be a package id string");
					return false;
				}

				Info.Requires.AddUnique(FName(*RequiredId));
			}
		}

		return true;
	}

	/** The columns a bark file may have, found by header name so writers can reorder them freely */
	struct FBarkColumns
	{
		int32 Id = INDEX_NONE;
		int32 Event = INDEX_NONE;
		int32 Conditions = INDEX_NONE;
		int32 Text = INDEX_NONE;
		int32 Weight = INDEX_NONE;
		int32 Cooldown = INDEX_NONE;
		int32 NumColumns = 0;
	};

	bool ParseDialogNumber(const FString& Text, double& OutValue)
	{
		const FString Trimmed = Text.TrimStartAndEnd();

		if (Trimmed.IsEmpty() || !Trimmed.IsNumeric())
		{
			return false;
		}

		OutValue = FCString::Atod(*Trimmed);
		return true;
	}

	/** Parses one package's bark files into Library.Barks, returning how many landed */
	int32 LoadDialogBarks(
		const FDialogPackageSource& Source,
		const FDialogPackageInfo& Info,
		const FDialogFactRegistry& Facts,
		const FDialogKnownIds* KnownIds,
		const FDialogProblemSink& Sink,
		FDialogLibrary& Library)
	{
		int32 Loaded = 0;
		TMap<FName, FString> SeenIds;

		for (const FDialogSourceFile* File : GetDialogCsvFiles(Source, DialogBarksFolder))
		{
			TArray<FDialogCsvRow> Rows;
			FString CsvError;
			int32 CsvErrorLine = 0;

			if (!SmoresDialog::ParseCsv(File->Contents, Rows, CsvError, CsvErrorLine))
			{
				Sink.Error(File->Path, CsvErrorLine, CsvError + TEXT(" - the whole file was skipped"));
				continue;
			}

			// the header is the first row with anything in it
			int32 HeaderIndex = 0;

			while (HeaderIndex < Rows.Num() && IsBlankDialogRow(Rows[HeaderIndex]))
			{
				++HeaderIndex;
			}

			if (HeaderIndex >= Rows.Num())
			{
				Sink.Warning(File->Path, 0, TEXT("the file is empty"));
				continue;
			}

			const TMap<FString, int32> Header = ReadDialogHeader(Rows[HeaderIndex]);

			FBarkColumns Columns;
			Columns.NumColumns = Rows[HeaderIndex].Fields.Num();
			Columns.Id = FindDialogColumn(Header, TEXT("id"));
			Columns.Event = FindDialogColumn(Header, TEXT("event"));
			Columns.Text = FindDialogColumn(Header, TEXT("text"));
			Columns.Conditions = Header.Contains(TEXT("conditions")) ? FindDialogColumn(Header, TEXT("conditions")) : FindDialogColumn(Header, TEXT("condition"));
			Columns.Weight = FindDialogColumn(Header, TEXT("weight"));
			Columns.Cooldown = FindDialogColumn(Header, TEXT("cooldown"));

			if (Columns.Id == INDEX_NONE || Columns.Event == INDEX_NONE || Columns.Text == INDEX_NONE)
			{
				Sink.Error(File->Path, Rows[HeaderIndex].Line, TEXT("the first row must name the columns, and Id, Event and Text are required - the whole file was skipped"));
				continue;
			}

			for (int32 RowIndex = HeaderIndex + 1; RowIndex < Rows.Num(); ++RowIndex)
			{
				const FDialogCsvRow& Row = Rows[RowIndex];

				if (IsBlankDialogRow(Row))
				{
					continue;
				}

				const FString LocalId = GetDialogField(Row, Columns.Id).TrimStartAndEnd();

				// a row whose id starts with # is a writer's comment
				if (LocalId.StartsWith(TEXT("#")))
				{
					continue;
				}

				auto RowError = [&Sink, File, &Row](const FString& Message)
				{
					Sink.Error(File->Path, Row.Line, Message + TEXT(" - row skipped"));
				};

				// more fields than the header is almost always an unquoted comma in the line, which
				// would otherwise quietly cut the line short
				if (Row.Fields.Num() > Columns.NumColumns)
				{
					RowError(TEXT("more fields than the header has columns - text containing a comma needs \"double quotes\""));
					continue;
				}

				if (!IsValidDialogLocalId(LocalId))
				{
					RowError(FString::Printf(TEXT("id '%s' must be letters, digits and _ (no dots - the package prefix is added for you)"), *LocalId));
					continue;
				}

				const FName LineId(*FString::Printf(TEXT("%s.%s"), *Info.Id.ToString(), *LocalId));

				if (const FString* FirstSeen = SeenIds.Find(LineId))
				{
					RowError(FString::Printf(TEXT("id '%s' is already used in this package (%s)"), *LocalId, **FirstSeen));
					continue;
				}

				FBarkLine Line;
				Line.Id = LineId;
				Line.LocalId = FName(*LocalId);
				Line.PackageId = Info.Id;
				Line.File = File->Path;
				Line.Line = Row.Line;

				const FString EventText = GetDialogField(Row, Columns.Event);

				if (!SmoresDialog::ParseBarkEvent(EventText, Line.Event))
				{
					TArray<FString> EventNames;

					for (const EBarkEvent Event : SmoresDialog::GetAllBarkEvents())
					{
						EventNames.Add(SmoresDialog::GetBarkEventName(Event));
					}

					RowError(FString::Printf(TEXT("event '%s' isn't one of %s"), *EventText.TrimStartAndEnd(), *FString::Join(EventNames, TEXT(", "))));
					continue;
				}

				Line.SourceText = GetDialogField(Row, Columns.Text).TrimStartAndEnd();

				if (Line.SourceText.IsEmpty())
				{
					RowError(TEXT("the line has no text"));
					continue;
				}

				const FString WeightText = GetDialogField(Row, Columns.Weight);

				if (!WeightText.TrimStartAndEnd().IsEmpty() && (!ParseDialogInteger(WeightText, Line.Weight) || Line.Weight < 1))
				{
					RowError(FString::Printf(TEXT("weight '%s' must be a whole number, 1 or more"), *WeightText.TrimStartAndEnd()));
					continue;
				}

				const FString CooldownText = GetDialogField(Row, Columns.Cooldown);
				double Cooldown = 0.0;

				if (!CooldownText.TrimStartAndEnd().IsEmpty() && (!ParseDialogNumber(CooldownText, Cooldown) || Cooldown < 0.0))
				{
					RowError(FString::Printf(TEXT("cooldown '%s' must be a number of seconds, 0 or more"), *CooldownText.TrimStartAndEnd()));
					continue;
				}

				Line.CooldownSeconds = static_cast<float>(Cooldown);

				TArray<FString> ConditionErrors;
				TArray<FString> ConditionWarnings;

				const bool bConditionOk = SmoresDialog::CompileCondition(GetDialogField(Row, Columns.Conditions), Facts,
					SmoresDialog::GetEventSubjects(Line.Event), KnownIds, Line.Condition, ConditionErrors, ConditionWarnings);

				for (const FString& Warning : ConditionWarnings)
				{
					Sink.Warning(File->Path, Row.Line, Warning);
				}

				if (!bConditionOk)
				{
					for (const FString& Error : ConditionErrors)
					{
						RowError(Error);
					}

					continue;
				}

				SeenIds.Add(LineId, FString::Printf(TEXT("%s:%d"), *File->Path, Row.Line));
				Library.Barks.Add(MoveTemp(Line));
				++Loaded;
			}
		}

		return Loaded;
	}

	/** Parses one package's translation files into Library.Translations, returning how many landed */
	int32 LoadDialogTranslations(const FDialogPackageSource& Source, const FDialogPackageInfo& Info, const FDialogProblemSink& Sink, FDialogLibrary& Library)
	{
		int32 Loaded = 0;
		TSet<FString> Seen;

		for (const FDialogSourceFile* File : GetDialogCsvFiles(Source, DialogLocalizationFolder))
		{
			// localization/<culture>/<file>.csv
			TArray<FString> Segments;
			File->Path.ParseIntoArray(Segments, TEXT("/"));

			if (Segments.Num() != 3)
			{
				Sink.Error(File->Path, 0, TEXT("translations go in localization/<culture>/<file>.csv - the file was skipped"));
				continue;
			}

			const FString Culture = Segments[1];

			if (!IsValidDialogCultureName(Culture))
			{
				Sink.Error(File->Path, 0, FString::Printf(TEXT("'%s' isn't a culture name (fr, de, pt-BR) - the file was skipped"), *Culture));
				continue;
			}

			TArray<FDialogCsvRow> Rows;
			FString CsvError;
			int32 CsvErrorLine = 0;

			if (!SmoresDialog::ParseCsv(File->Contents, Rows, CsvError, CsvErrorLine))
			{
				Sink.Error(File->Path, CsvErrorLine, CsvError + TEXT(" - the whole file was skipped"));
				continue;
			}

			int32 HeaderIndex = 0;

			while (HeaderIndex < Rows.Num() && IsBlankDialogRow(Rows[HeaderIndex]))
			{
				++HeaderIndex;
			}

			if (HeaderIndex >= Rows.Num())
			{
				Sink.Warning(File->Path, 0, TEXT("the file is empty"));
				continue;
			}

			const TMap<FString, int32> Header = ReadDialogHeader(Rows[HeaderIndex]);
			const int32 NumColumns = Rows[HeaderIndex].Fields.Num();
			const int32 IdColumn = FindDialogColumn(Header, TEXT("id"));
			const int32 TextColumn = FindDialogColumn(Header, TEXT("text"));

			if (IdColumn == INDEX_NONE || TextColumn == INDEX_NONE)
			{
				Sink.Error(File->Path, Rows[HeaderIndex].Line, TEXT("the first row must name the columns, and Id and Text are required - the whole file was skipped"));
				continue;
			}

			for (int32 RowIndex = HeaderIndex + 1; RowIndex < Rows.Num(); ++RowIndex)
			{
				const FDialogCsvRow& Row = Rows[RowIndex];

				if (IsBlankDialogRow(Row))
				{
					continue;
				}

				const FString LocalId = GetDialogField(Row, IdColumn).TrimStartAndEnd();

				if (LocalId.StartsWith(TEXT("#")))
				{
					continue;
				}

				auto RowError = [&Sink, File, &Row](const FString& Message)
				{
					Sink.Error(File->Path, Row.Line, Message + TEXT(" - row skipped"));
				};

				if (Row.Fields.Num() > NumColumns)
				{
					RowError(TEXT("more fields than the header has columns - text containing a comma needs \"double quotes\""));
					continue;
				}

				if (!IsValidDialogLocalId(LocalId))
				{
					RowError(FString::Printf(TEXT("id '%s' must be one of this package's own line ids, without the package prefix"), *LocalId));
					continue;
				}

				const FName LineId(*FString::Printf(TEXT("%s.%s"), *Info.Id.ToString(), *LocalId));
				const FString Key = Culture.ToLower() + TEXT("|") + LineId.ToString();

				if (Seen.Contains(Key))
				{
					RowError(FString::Printf(TEXT("'%s' is already translated for %s"), *LocalId, *Culture));
					continue;
				}

				FDialogTranslation Translation;
				Translation.LineId = LineId;
				Translation.Culture = Culture;
				Translation.Text = GetDialogField(Row, TextColumn).TrimStartAndEnd();

				if (Translation.Text.IsEmpty())
				{
					RowError(TEXT("the translation has no text"));
					continue;
				}

				// kept, but flagged: a translation for a line that no longer exists is harmless and
				// almost always means the line was renamed and the translation wasn't
				if (!Library.FindBark(LineId) && !Library.FindText(LineId))
				{
					Sink.Warning(File->Path, Row.Line, FString::Printf(TEXT("translates '%s', which this package has no loaded line for"), *LocalId));
				}

				// a conversation line's speaker is decided by the source; a translator may keep the
				// "Bandit: " in front or leave it off, and either way it isn't shown as words
				if (Library.FindText(LineId))
				{
					FString Cue;
					FString Words;
					SmoresDialog::SplitSpeaker(Translation.Text, Cue, Words);
					Translation.Text = Words;
				}

				Seen.Add(Key);
				Library.Translations.Add(MoveTemp(Translation));
				++Loaded;
			}
		}

		return Loaded;
	}

	/** True if Start can reach itself through Requires, staying inside Among - i.e. it is in a cycle, not merely behind one */
	bool IsInDialogRequiresCycle(int32 Start, const TArray<FDialogPackageInfo>& Packages, const TMap<FName, int32>& IndexById, const TSet<int32>& Among)
	{
		TArray<int32> Stack;
		TSet<int32> Visited;

		for (const FName Required : Packages[Start].Requires)
		{
			if (const int32* Index = IndexById.Find(Required))
			{
				Stack.Add(*Index);
			}
		}

		while (Stack.Num() > 0)
		{
			const int32 Current = Stack.Pop(EAllowShrinking::No);

			if (Current == Start)
			{
				return true;
			}

			if (!Among.Contains(Current) || Visited.Contains(Current))
			{
				continue;
			}

			Visited.Add(Current);

			for (const FName Required : Packages[Current].Requires)
			{
				if (const int32* Index = IndexById.Find(Required))
				{
					Stack.Add(*Index);
				}
			}
		}

		return false;
	}
}

namespace SmoresDialog
{
	FName CorePackageId()
	{
		return FName(TEXT("core"));
	}

	FString GetCoreDirectory()
	{
		return FPaths::ProjectContentDir() / TEXT("Dialog") / TEXT("core");
	}

	FString GetModsDirectory()
	{
		return FPaths::ProjectModsDir();
	}

	bool ParseCsv(const FString& Text, TArray<FDialogCsvRow>& OutRows, FString& OutError, int32& OutErrorLine)
	{
		OutRows.Reset();

		const int32 Length = Text.Len();
		int32 Index = 0;
		int32 Line = 1;

		// a spreadsheet saving "UTF-8 CSV" puts a byte-order mark first
		if (Length > 0 && Text[0] == TCHAR(0xFEFF))
		{
			Index = 1;
		}

		FDialogCsvRow Row;
		Row.Line = Line;

		FString Field;
		bool bInQuotes = false;
		bool bFieldWasQuoted = false;
		int32 QuoteStartLine = 0;

		auto EndField = [&]()
		{
			Row.Fields.Add(bFieldWasQuoted ? Field : Field.TrimStartAndEnd());
			Field.Reset();
			bFieldWasQuoted = false;
		};

		while (Index < Length)
		{
			const TCHAR Character = Text[Index];

			if (bInQuotes)
			{
				if (Character == TEXT('"'))
				{
					if (Index + 1 < Length && Text[Index + 1] == TEXT('"'))
					{
						Field.AppendChar(TEXT('"'));
						Index += 2;
						continue;
					}

					bInQuotes = false;
					++Index;
					continue;
				}

				if (Character == TEXT('\n'))
				{
					++Line;
				}

				// a line break inside a quoted field is kept as a plain \n, whichever the file used
				if (Character != TEXT('\r'))
				{
					Field.AppendChar(Character);
				}

				++Index;
				continue;
			}

			if (Character == TEXT(','))
			{
				EndField();
				++Index;
				continue;
			}

			if (Character == TEXT('\r') || Character == TEXT('\n'))
			{
				if (Character == TEXT('\r') && Index + 1 < Length && Text[Index + 1] == TEXT('\n'))
				{
					++Index;
				}

				EndField();
				OutRows.Add(MoveTemp(Row));

				++Index;
				++Line;

				Row = FDialogCsvRow();
				Row.Line = Line;
				continue;
			}

			if (bFieldWasQuoted)
			{
				// "like this"trailing - nothing sensible to make of it
				if (!FChar::IsWhitespace(Character))
				{
					OutError = TEXT("text after a closing quote");
					OutErrorLine = Line;
					return false;
				}

				++Index;
				continue;
			}

			if (Character == TEXT('"') && Field.TrimStartAndEnd().IsEmpty())
			{
				bInQuotes = true;
				bFieldWasQuoted = true;
				QuoteStartLine = Line;
				Field.Reset();
				++Index;
				continue;
			}

			Field.AppendChar(Character);
			++Index;
		}

		if (bInQuotes)
		{
			OutError = TEXT("a quoted field is never closed");
			OutErrorLine = QuoteStartLine;
			return false;
		}

		// the last record, when the file doesn't end with a line break
		if (!Field.IsEmpty() || bFieldWasQuoted || Row.Fields.Num() > 0)
		{
			EndField();
			OutRows.Add(MoveTemp(Row));
		}

		return true;
	}

	void GatherPackagesFromDisk(const FString& CoreDirectory, const FString& ModsDirectory, TArray<FDialogPackageSource>& OutSources, TArray<FDialogProblem>& OutProblems)
	{
		IFileManager& FileManager = IFileManager::Get();

		auto GatherPackage = [&FileManager, &OutProblems](const FString& Root, const FString& FolderName, bool bIsCore) -> FDialogPackageSource
		{
			FDialogPackageSource Source;
			Source.FolderName = FolderName;
			Source.bIsCore = bIsCore;

			FString NormalizedRoot = Root;
			FPaths::NormalizeDirectoryName(NormalizedRoot);
			NormalizedRoot += TEXT("/");

			TArray<FString> Found;
			FileManager.FindFilesRecursive(Found, *NormalizedRoot, TEXT("*"), /*Files*/ true, /*Directories*/ false);
			Found.Sort();

			for (FString& FullPath : Found)
			{
				FPaths::NormalizeFilename(FullPath);

				FString RelativePath = FullPath;

				if (!RelativePath.RemoveFromStart(NormalizedRoot, ESearchCase::IgnoreCase))
				{
					continue;
				}

				// only what the loader understands; anything else (a README, notes) is left alone
				// rather than reported
				const bool bIsManifest = RelativePath.Equals(DialogManifestFileName, ESearchCase::IgnoreCase);
				const bool bIsCsv = RelativePath.EndsWith(TEXT(".csv"), ESearchCase::IgnoreCase)
					&& (RelativePath.StartsWith(DialogBarksFolder, ESearchCase::IgnoreCase) || RelativePath.StartsWith(DialogLocalizationFolder, ESearchCase::IgnoreCase)
						|| RelativePath.StartsWith(DialogConversationsFolder, ESearchCase::IgnoreCase));
				const bool bIsConversation = RelativePath.StartsWith(DialogConversationsFolder, ESearchCase::IgnoreCase)
					&& (RelativePath.EndsWith(TEXT(".yarn"), ESearchCase::IgnoreCase) || RelativePath.EndsWith(TEXT(".yarnc"), ESearchCase::IgnoreCase));

				if (!bIsManifest && !bIsCsv && !bIsConversation)
				{
					continue;
				}

				FDialogSourceFile File;
				File.Path = RelativePath;

				// the stale-compile check compares a .yarn's time with its .yarnc's
				File.Timestamp = FileManager.GetTimeStamp(*FullPath);

				// a compiled conversation is bytes, not text
				const bool bIsBinary = RelativePath.EndsWith(TEXT(".yarnc"), ESearchCase::IgnoreCase);
				const bool bRead = bIsBinary ? FFileHelper::LoadFileToArray(File.Bytes, *FullPath) : FFileHelper::LoadFileToString(File.Contents, *FullPath);

				if (!bRead)
				{
					FDialogProblem Problem;
					Problem.Package = FolderName;
					Problem.File = RelativePath;
					Problem.Message = TEXT("couldn't read the file - it was skipped");
					OutProblems.Add(MoveTemp(Problem));
					continue;
				}

				Source.Files.Add(MoveTemp(File));
			}

			return Source;
		};

		if (FileManager.DirectoryExists(*CoreDirectory))
		{
			OutSources.Add(GatherPackage(CoreDirectory, CorePackageId().ToString(), /*bIsCore*/ true));
		}
		else
		{
			FDialogProblem Problem;
			Problem.Package = CorePackageId().ToString();
			Problem.Message = FString::Printf(TEXT("the base game's dialog folder is missing (%s) - in a packaged build, check Content/Dialog is in DirectoriesToAlwaysStageAsUFS"), *CoreDirectory);
			OutProblems.Add(MoveTemp(Problem));
		}

		if (!ModsDirectory.IsEmpty() && FileManager.DirectoryExists(*ModsDirectory))
		{
			TArray<FString> ModFolders;
			FileManager.FindFiles(ModFolders, *(ModsDirectory / TEXT("*")), /*Files*/ false, /*Directories*/ true);
			ModFolders.Sort();

			for (const FString& ModFolder : ModFolders)
			{
				OutSources.Add(GatherPackage(ModsDirectory / ModFolder, ModFolder, /*bIsCore*/ false));
			}
		}
	}

	FDialogLibrary LoadPackages(const TArray<FDialogPackageSource>& Sources, const FDialogFactRegistry& Facts, const FDialogKnownIds* KnownIds, const FDialogEffectRegistry* Effects)
	{
		FDialogLibrary Library;

		// the game's own effects unless a caller (a test) brings its own
		const FDialogEffectRegistry BuiltInEffects = Effects ? FDialogEffectRegistry() : FDialogEffectRegistry::MakeBuiltIn();
		const FDialogEffectRegistry& UsedEffects = Effects ? *Effects : BuiltInEffects;

		// 1. Manifests. Every source becomes a package entry, loaded or not, so the report can name it.
		TArray<const FDialogPackageSource*> SourceByPackage;
		TMap<FName, int32> IndexById;
		TArray<bool> Viable;

		for (const FDialogPackageSource& Source : Sources)
		{
			FDialogPackageInfo Info;
			Info.Id = FName(*Source.FolderName);
			Info.FolderName = Source.FolderName;
			Info.bIsCore = Source.bIsCore;

			const FDialogProblemSink Sink{ Library.Problems, Source.FolderName };
			bool bViable = true;

			const FDialogSourceFile* Manifest = FindDialogSourceFile(Source, DialogManifestFileName);
			FString ManifestError;

			if (!Manifest)
			{
				Sink.Error(TEXT(""), 0, TEXT("no mod.json - a package needs a manifest to load, so it was skipped"));
				bViable = false;
			}
			else if (!ParseDialogManifest(Manifest->Contents, Info, ManifestError))
			{
				Sink.Error(DialogManifestFileName, 0, ManifestError + TEXT(" - the package was skipped"));
				bViable = false;
			}
			else if (Source.bIsCore && Info.Id != CorePackageId())
			{
				Sink.Error(DialogManifestFileName, 0, FString::Printf(TEXT("the base game's package must have the id '%s', not '%s' - it was skipped"), *CorePackageId().ToString(), *Info.Id.ToString()));
				bViable = false;
			}
			else if (!Source.bIsCore && Info.Id == CorePackageId())
			{
				Sink.Error(DialogManifestFileName, 0, TEXT("'core' is the base game's id and no mod may claim it - the package was skipped"));
				bViable = false;
			}
			else if (IndexById.Contains(Info.Id))
			{
				Sink.Error(DialogManifestFileName, 0, FString::Printf(TEXT("the id '%s' is already used by the package in folder '%s' - this one was skipped"),
					*Info.Id.ToString(), *Library.Packages[IndexById[Info.Id]].FolderName));
				bViable = false;
			}
			else if (Info.Requires.Contains(Info.Id))
			{
				Sink.Error(DialogManifestFileName, 0, TEXT("a package can't require itself - it was skipped"));
				bViable = false;
			}

			if (bViable)
			{
				IndexById.Add(Info.Id, Library.Packages.Num());
			}

			Library.Packages.Add(MoveTemp(Info));
			SourceByPackage.Add(&Source);
			Viable.Add(bViable);
		}

		// problems raised against a package by its id once the manifest named one, rather than its folder
		auto SinkFor = [&Library](int32 PackageIndex)
		{
			return FDialogProblemSink{ Library.Problems, Library.Packages[PackageIndex].Id.ToString() };
		};

		// 2. Requirements. Anything needing a package that isn't here, or was skipped, is skipped too -
		// repeated until nothing changes, so a chain of dependents all goes.
		bool bChanged = true;

		while (bChanged)
		{
			bChanged = false;

			for (int32 Index = 0; Index < Library.Packages.Num(); ++Index)
			{
				if (!Viable[Index])
				{
					continue;
				}

				for (const FName Required : Library.Packages[Index].Requires)
				{
					const int32* RequiredIndex = IndexById.Find(Required);

					if (!RequiredIndex)
					{
						SinkFor(Index).Error(DialogManifestFileName, 0, FString::Printf(TEXT("requires '%s', which isn't installed - the package was skipped"), *Required.ToString()));
						Viable[Index] = false;
						bChanged = true;
						break;
					}

					if (!Viable[*RequiredIndex])
					{
						SinkFor(Index).Error(DialogManifestFileName, 0, FString::Printf(TEXT("requires '%s', which was skipped - so this package was too"), *Required.ToString()));
						Viable[Index] = false;
						bChanged = true;
						break;
					}
				}
			}
		}

		// 3. Order: core first, then mods so that everything a mod requires is already loaded. Among
		// mods that are ready together, alphabetical by id, so the order never depends on the disk.
		TArray<int32> Order;
		TSet<int32> Placed;
		TSet<int32> Waiting;

		for (int32 Index = 0; Index < Library.Packages.Num(); ++Index)
		{
			if (!Viable[Index])
			{
				continue;
			}

			if (Library.Packages[Index].bIsCore)
			{
				Order.Add(Index);
				Placed.Add(Index);
			}
			else
			{
				Waiting.Add(Index);
			}
		}

		while (Waiting.Num() > 0)
		{
			int32 Next = INDEX_NONE;

			for (const int32 Candidate : Waiting)
			{
				const bool bReady = !Library.Packages[Candidate].Requires.ContainsByPredicate([&IndexById, &Placed](FName Required)
				{
					const int32* RequiredIndex = IndexById.Find(Required);
					return !RequiredIndex || !Placed.Contains(*RequiredIndex);
				});

				if (bReady && (Next == INDEX_NONE || Library.Packages[Candidate].Id.ToString() < Library.Packages[Next].Id.ToString()))
				{
					Next = Candidate;
				}
			}

			if (Next == INDEX_NONE)
			{
				break;
			}

			Order.Add(Next);
			Placed.Add(Next);
			Waiting.Remove(Next);
		}

		// whatever is still waiting is in a Requires cycle, or needs something that is
		if (Waiting.Num() > 0)
		{
			TArray<FString> CycleIds;

			for (const int32 Index : Waiting)
			{
				if (IsInDialogRequiresCycle(Index, Library.Packages, IndexById, Waiting))
				{
					CycleIds.Add(Library.Packages[Index].Id.ToString());
				}
			}

			CycleIds.Sort();
			const FString CycleList = FString::Join(CycleIds, TEXT(", "));

			for (const int32 Index : Waiting)
			{
				const FString Id = Library.Packages[Index].Id.ToString();

				if (CycleIds.Contains(Id))
				{
					SinkFor(Index).Error(DialogManifestFileName, 0, FString::Printf(TEXT("is in a Requires cycle (%s) - none of them can load first, so all were skipped"), *CycleList));
				}
				else
				{
					SinkFor(Index).Error(DialogManifestFileName, 0, FString::Printf(TEXT("requires a package caught in a Requires cycle (%s) - skipped"), *CycleList));
				}

				Viable[Index] = false;
			}
		}

		// 4. Contents, in load order. Barks and conversations first, so translations can be checked
		// against them.
		for (const int32 Index : Order)
		{
			FDialogPackageInfo& Info = Library.Packages[Index];
			const FDialogProblemSink Sink = SinkFor(Index);

			Info.bLoaded = true;
			Info.NumBarks = LoadDialogBarks(*SourceByPackage[Index], Info, Facts, KnownIds, Sink, Library);

			Library.RebuildIndex();

			Info.NumConversations = LoadDialogConversations(*SourceByPackage[Index], Info, Facts, UsedEffects, KnownIds, Sink, Library);

			Library.RebuildIndex();

			Info.NumTranslations = LoadDialogTranslations(*SourceByPackage[Index], Info, Sink, Library);
		}

		// report packages in load order, with the skipped ones after
		TArray<FDialogPackageInfo> Ordered;
		Ordered.Reserve(Library.Packages.Num());

		for (const int32 Index : Order)
		{
			Ordered.Add(Library.Packages[Index]);
		}

		for (int32 Index = 0; Index < Library.Packages.Num(); ++Index)
		{
			if (!Order.Contains(Index))
			{
				Ordered.Add(Library.Packages[Index]);
			}
		}

		Library.Packages = MoveTemp(Ordered);

		// 5. Index for play
		Library.RebuildIndex();

		return Library;
	}
}
