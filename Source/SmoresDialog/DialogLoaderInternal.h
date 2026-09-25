// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"
#include "DialogLoader.h"

class FDialogFactRegistry;
class FDialogEffectRegistry;
struct FDialogKnownIds;
struct FDialogLibrary;
struct FDialogPackageInfo;

/**
 *  The loader's shared helpers - used by the bark and translation loading in DialogLoader.cpp and
 *  the conversation loading in ConversationLoader.cpp. SmoresDialog's own business: nothing outside
 *  the module includes this.
 */
namespace SmoresDialogLoading
{
	/** Collects problems for one package, so each call site only says what went wrong and where */
	struct FDialogProblemSink
	{
		TArray<FDialogProblem>& Problems;
		FString Package;

		void Add(EDialogProblemSeverity Severity, const FString& File, int32 Line, const FString& Message) const
		{
			FDialogProblem Problem;
			Problem.Severity = Severity;
			Problem.Package = Package;
			Problem.File = File;
			Problem.Line = Line;
			Problem.Message = Message;

			Problems.Add(MoveTemp(Problem));
		}

		void Error(const FString& File, int32 Line, const FString& Message) const
		{
			Add(EDialogProblemSeverity::Error, File, Line, Message);
		}

		void Warning(const FString& File, int32 Line, const FString& Message) const
		{
			Add(EDialogProblemSeverity::Warning, File, Line, Message);
		}
	};

	/** Letters, digits and _. No dot - the dot is what the loader adds when it qualifies the id. */
	inline bool IsValidDialogLocalId(const FString& Id)
	{
		if (Id.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Character : Id)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				return false;
			}
		}

		return true;
	}

	inline FString GetDialogField(const FDialogCsvRow& Row, int32 Column)
	{
		return Row.Fields.IsValidIndex(Column) ? Row.Fields[Column] : FString();
	}

	inline bool IsBlankDialogRow(const FDialogCsvRow& Row)
	{
		for (const FString& Field : Row.Fields)
		{
			if (!Field.TrimStartAndEnd().IsEmpty())
			{
				return false;
			}
		}

		return true;
	}

	/** Maps each header cell to its lower-cased name. Unknown columns - a writer's Notes column, say - are simply never read. */
	inline TMap<FString, int32> ReadDialogHeader(const FDialogCsvRow& Header)
	{
		TMap<FString, int32> Columns;

		for (int32 Index = 0; Index < Header.Fields.Num(); ++Index)
		{
			const FString Name = Header.Fields[Index].TrimStartAndEnd().ToLower();

			if (!Name.IsEmpty() && !Columns.Contains(Name))
			{
				Columns.Add(Name, Index);
			}
		}

		return Columns;
	}

	/** The column a header named, or INDEX_NONE */
	inline int32 FindDialogColumn(const TMap<FString, int32>& Header, const TCHAR* Name)
	{
		const int32* Column = Header.Find(Name);
		return Column ? *Column : INDEX_NONE;
	}

	/** A strictly-formatted whole number - "2" yes, "2.5" and "two" no. A leading minus is allowed. */
	inline bool ParseDialogInteger(const FString& Text, int32& OutValue)
	{
		const FString Trimmed = Text.TrimStartAndEnd();

		if (Trimmed.IsEmpty() || !Trimmed.IsNumeric() || Trimmed.Contains(TEXT(".")))
		{
			return false;
		}

		OutValue = FCString::Atoi(*Trimmed);
		return true;
	}

	/** Every file under Folder ("barks/") ending in Extension (".csv"), sorted so the load order never depends on the file system's */
	inline TArray<const FDialogSourceFile*> GetDialogFilesIn(const FDialogPackageSource& Source, const TCHAR* Folder, const TCHAR* Extension)
	{
		TArray<const FDialogSourceFile*> Found;

		for (const FDialogSourceFile& File : Source.Files)
		{
			if (File.Path.StartsWith(Folder, ESearchCase::IgnoreCase) && File.Path.EndsWith(Extension, ESearchCase::IgnoreCase))
			{
				Found.Add(&File);
			}
		}

		Found.Sort([](const FDialogSourceFile& A, const FDialogSourceFile& B)
		{
			return A.Path < B.Path;
		});

		return Found;
	}

	/** The file at exactly this package-relative path, or null */
	inline const FDialogSourceFile* FindDialogSourceFile(const FDialogPackageSource& Source, const FString& Path)
	{
		return Source.Files.FindByPredicate([&Path](const FDialogSourceFile& File)
		{
			return File.Path.Equals(Path, ESearchCase::IgnoreCase);
		});
	}

	/**
	 *  Loads one package's conversations/ folder into Library - Conversations and their Texts - and
	 *  returns how many conversations landed. ConversationLoader.cpp; the package's barks must already
	 *  be in the library, indexed, so a line id can't quietly reuse a bark's.
	 */
	int32 LoadDialogConversations(
		const FDialogPackageSource& Source,
		const FDialogPackageInfo& Info,
		const FDialogFactRegistry& Facts,
		const FDialogEffectRegistry& Effects,
		const FDialogKnownIds* KnownIds,
		const FDialogProblemSink& Sink,
		FDialogLibrary& Library);
}
