// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogLoaderInternal.h"
#include "ConversationPlayer.h"
#include "ConversationScript.h"
#include "DialogCondition.h"
#include "DialogEffects.h"
#include "DialogFacts.h"
#include "DialogLibrary.h"
#include "YarnProtobufParser.h"
#include "YarnSpinnerCore.h"
#include "Misc/Paths.h"

using namespace SmoresDialogLoading;

/**
 *  Loading a package's conversations/ folder - the Yarn half of the loader.
 *
 *  Each conversations/<name>.yarn is shipped with the three files ysc writes from it: <name>.yarnc
 *  (the compiled program, which holds ids only), <name>-Lines.csv (every line's text by id) and
 *  <name>-Metadata.csv (each line's other tags). All four are read; the .yarn itself is what lets an
 *  error name the line a writer should look at, and what proves every line's id was written rather
 *  than made up.
 *
 *  **ysc checks the language, not the game.** It accepts any function or command name, and makes
 *  up an id for a line without one. So everything the game cares about is checked here, at load
 *  time, with the file and line: ids, facts and effects, headers, #reason: keys, and what an
 *  Ambient conversation may do.
 */

namespace
{
	const TCHAR* const DialogConversationsFolder = TEXT("conversations/");

	/** Our node headers - a node with any of them is a conversation */
	const TCHAR* const KindHeader = TEXT("kind");
	const TCHAR* const AttachHeader = TEXT("attach");
	const TCHAR* const RequiresHeader = TEXT("requires");
	const TCHAR* const PriorityHeader = TEXT("priority");
	const TCHAR* const OnceHeader = TEXT("once");
	const TCHAR* const ParticipantsHeader = TEXT("participants");
	const TCHAR* const LabelHeader = TEXT("label");

	const TCHAR* const YarnLineIdPrefix = TEXT("line:");

	/** Where each node sits in its .yarn, so a problem can name the line a writer should look at */
	struct FYarnSourceIndex
	{
		struct FNodeSpan
		{
			int32 TitleLine = 0;

			/** 1-based, inclusive */
			int32 BodyStart = 0;
			int32 BodyEnd = 0;

			/** Lower-cased header key -> its line */
			TMap<FString, int32> HeaderLines;
		};

		TArray<FString> Lines;

		TMap<FString, FNodeSpan> Nodes;

		void Build(const FString& Text)
		{
			Text.ParseIntoArray(Lines, TEXT("\n"), /*InCullEmpty*/ false);

			for (FString& Line : Lines)
			{
				Line.RemoveFromEnd(TEXT("\r"));
			}

			FNodeSpan Current;
			FString CurrentTitle;
			bool bInBody = false;

			for (int32 Index = 0; Index < Lines.Num(); ++Index)
			{
				const int32 LineNumber = Index + 1;
				const FString Trimmed = Lines[Index].TrimStartAndEnd();

				if (!bInBody)
				{
					if (Trimmed == TEXT("---"))
					{
						bInBody = true;
						Current.BodyStart = LineNumber + 1;
						continue;
					}

					FString Key;
					FString Value;

					if (!Trimmed.StartsWith(TEXT("//")) && Trimmed.Split(TEXT(":"), &Key, &Value))
					{
						Key = Key.TrimStartAndEnd().ToLower();
						Current.HeaderLines.Add(Key, LineNumber);

						if (Key == TEXT("title"))
						{
							CurrentTitle = Value.TrimStartAndEnd();
							Current.TitleLine = LineNumber;
						}
					}

					continue;
				}

				if (Trimmed == TEXT("==="))
				{
					Current.BodyEnd = LineNumber - 1;

					if (!CurrentTitle.IsEmpty())
					{
						Nodes.Add(CurrentTitle, Current);
					}

					Current = FNodeSpan();
					CurrentTitle.Reset();
					bInBody = false;
				}
			}
		}

		int32 GetNodeLine(const FString& Node) const
		{
			const FNodeSpan* Span = Nodes.Find(Node);
			return Span ? Span->TitleLine : 0;
		}

		int32 GetHeaderLine(const FString& Node, const TCHAR* Key) const
		{
			const FNodeSpan* Span = Nodes.Find(Node);
			const int32* Line = Span ? Span->HeaderLines.Find(Key) : nullptr;

			return Line ? *Line : GetNodeLine(Node);
		}

		/** The first line of the node's body containing Needle, else the node's title line */
		int32 FindInNode(const FString& Node, const FString& Needle) const
		{
			const FNodeSpan* Span = Nodes.Find(Node);

			if (!Span)
			{
				return 0;
			}

			for (int32 LineNumber = Span->BodyStart; LineNumber <= Span->BodyEnd && Lines.IsValidIndex(LineNumber - 1); ++LineNumber)
			{
				if (Lines[LineNumber - 1].Contains(Needle, ESearchCase::CaseSensitive))
				{
					return LineNumber;
				}
			}

			return Span->TitleLine;
		}

		/** True if the source line carries exactly this tag - "#line:shakedown_toll", not "#line:shakedown_toll2" */
		bool LineHasTag(int32 LineNumber, const FString& Tag) const
		{
			if (!Lines.IsValidIndex(LineNumber - 1))
			{
				return false;
			}

			const FString& Line = Lines[LineNumber - 1];
			int32 From = 0;

			while (true)
			{
				const int32 At = Line.Find(Tag, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);

				if (At == INDEX_NONE)
				{
					return false;
				}

				const int32 After = At + Tag.Len();

				if (After >= Line.Len() || (!FChar::IsAlnum(Line[After]) && Line[After] != TEXT('_')))
				{
					return true;
				}

				From = After;
			}
		}
	};

	/** What the loader learned about one node while reading its instructions */
	struct FYarnNodeReport
	{
		/** Something in it is wrong, so no conversation that plays it can load */
		bool bBroken = false;

		/** The nodes it jumps or detours to, by title */
		TArray<FString> Targets;

		/** The Yarn line ids it says or offers */
		TArray<FString> LineIds;

		/** Lines (by Yarn id) that insert values */
		TSet<FString> SubstitutedLines;

		/** Its options, by Yarn id */
		TArray<FString> OptionIds;

		/** Effects it runs that need the window (OpenTrade), by name */
		TArray<FString> WindowEffects;
	};

	FString StripYarnLineId(const FString& YarnId)
	{
		return YarnId.StartsWith(YarnLineIdPrefix) ? YarnId.Mid(FCString::Strlen(YarnLineIdPrefix)) : YarnId;
	}

	/** The header's value by lower-cased key */
	TMap<FString, FString> ReadNodeHeaders(const FYarnNode& Node)
	{
		TMap<FString, FString> Headers;

		for (const FYarnHeader& Header : Node.Headers)
		{
			Headers.Add(Header.Key.TrimStartAndEnd().ToLower(), Header.Value.TrimStartAndEnd());
		}

		return Headers;
	}

	bool HasConversationHeaders(const TMap<FString, FString>& Headers)
	{
		for (const TCHAR* Key : { KindHeader, AttachHeader, RequiresHeader, PriorityHeader, OnceHeader, ParticipantsHeader, LabelHeader })
		{
			if (Headers.Contains(Key))
			{
				return true;
			}
		}

		return false;
	}

	bool ParseConversationKind(const FString& Text, EConversationKind& OutKind)
	{
		const UEnum* Kinds = StaticEnum<EConversationKind>();

		for (int32 Index = 0; Index < Kinds->NumEnums() - 1; ++Index)
		{
			if (Text.Equals(Kinds->GetNameStringByIndex(Index), ESearchCase::IgnoreCase))
			{
				OutKind = static_cast<EConversationKind>(Kinds->GetValueByIndex(Index));
				return true;
			}
		}

		return false;
	}

	FString DescribeEffectNames(const FDialogEffectRegistry& Effects)
	{
		return FString::JoinBy(Effects.GetEffects(), TEXT(", "), [](const FDialogEffect& Effect) { return Effect.Name.ToString(); });
	}

	/** Every node Start plays, itself included, following jumps and detours */
	TArray<FString> GetReachableNodes(const FString& Start, const TMap<FString, FYarnNodeReport>& Reports)
	{
		TArray<FString> Reached;
		TArray<FString> Stack = { Start };

		while (Stack.Num() > 0)
		{
			const FString Node = Stack.Pop(EAllowShrinking::No);

			if (Reached.Contains(Node))
			{
				continue;
			}

			Reached.Add(Node);

			if (const FYarnNodeReport* Report = Reports.Find(Node))
			{
				Stack.Append(Report->Targets);
			}
		}

		return Reached;
	}

	/** Reads a lines or metadata CSV into rows under a lower-cased header. False, with a reason, when unreadable. */
	bool ReadYarnCsv(const FDialogSourceFile& File, TArray<FDialogCsvRow>& OutRows, TMap<FString, int32>& OutHeader, FString& OutError)
	{
		FString CsvError;
		int32 CsvErrorLine = 0;

		if (!SmoresDialog::ParseCsv(File.Contents, OutRows, CsvError, CsvErrorLine))
		{
			OutError = FString::Printf(TEXT("line %d: %s"), CsvErrorLine, *CsvError);
			return false;
		}

		if (OutRows.Num() == 0)
		{
			OutError = TEXT("the file is empty");
			return false;
		}

		OutHeader = ReadDialogHeader(OutRows[0]);
		return true;
	}

	/** Loads one .yarn and the files beside it. Returns how many conversations landed. */
	int32 LoadYarnFile(
		const FDialogSourceFile& YarnFile,
		const FDialogPackageSource& Source,
		const FDialogPackageInfo& Info,
		const FDialogFactRegistry& Facts,
		const FDialogEffectRegistry& Effects,
		const FDialogKnownIds* KnownIds,
		const FDialogProblemSink& Sink,
		TMap<FName, FString>& SeenIds,
		FDialogLibrary& Library)
	{
		const FString& FilePath = YarnFile.Path;
		const FString BasePath = FilePath.LeftChop(5);
		const FString BaseName = FPaths::GetCleanFilename(BasePath);

		const FDialogSourceFile* Compiled = FindDialogSourceFile(Source, BasePath + TEXT(".yarnc"));
		const FDialogSourceFile* LinesFile = FindDialogSourceFile(Source, BasePath + TEXT("-Lines.csv"));
		const FDialogSourceFile* MetadataFile = FindDialogSourceFile(Source, BasePath + TEXT("-Metadata.csv"));

		const FString CompileHint = FString::Printf(TEXT("compile it with: ysc compile %s.yarn -o . -n %s"), *BaseName, *BaseName);

		if (!Compiled || !LinesFile)
		{
			Sink.Error(FilePath, 0, FString::Printf(TEXT("has no %s beside it - %s - the file was skipped"),
				!Compiled ? *(BaseName + TEXT(".yarnc")) : *(BaseName + TEXT("-Lines.csv")), *CompileHint));
			return 0;
		}

#if WITH_EDITOR
		// in the editor only: a packaged build's files all carry the time they were staged, and a
		// writer only ever edits in the editor anyway
		if (YarnFile.Timestamp.GetTicks() > 0 && Compiled->Timestamp.GetTicks() > 0 && Compiled->Timestamp < YarnFile.Timestamp)
		{
			Sink.Error(FilePath, 0, FString::Printf(TEXT("was edited after %s.yarnc was compiled, so it would play old lines - %s - the file was skipped"), *BaseName, *CompileHint));
			return 0;
		}
#endif

		TSharedRef<FDialogConversationScript> Script = MakeShared<FDialogConversationScript>();
		Script->PackageId = Info.Id;
		Script->File = FilePath;

		{
			FYarnProtobufParser Parser(Compiled->Bytes);
			FString ParseError;

			if (!Parser.ParseProgram(Script->Program, ParseError) || Script->Program.Nodes.Num() == 0)
			{
				Sink.Error(Compiled->Path, 0, FString::Printf(TEXT("isn't a compiled Yarn program%s - %s - the file was skipped"),
					ParseError.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *ParseError), *CompileHint));
				return 0;
			}
		}

		FYarnSourceIndex SourceIndex;
		SourceIndex.Build(YarnFile.Contents);

		TMap<FString, FYarnNodeReport> Reports;
		bool bFileOk = true;

		auto BreakNode = [&Reports](const FString& Node)
		{
			Reports.FindOrAdd(Node).bBroken = true;
		};

		// 1. The lines table: every line's id, text and where it was written
		TArray<FDialogCsvRow> LineRows;
		TMap<FString, int32> LineHeader;
		FString CsvError;

		if (!ReadYarnCsv(*LinesFile, LineRows, LineHeader, CsvError))
		{
			Sink.Error(LinesFile->Path, 0, CsvError + TEXT(" - the file was skipped"));
			return 0;
		}

		const int32 IdColumn = FindDialogColumn(LineHeader, TEXT("id"));
		const int32 TextColumn = FindDialogColumn(LineHeader, TEXT("text"));
		const int32 NodeColumn = FindDialogColumn(LineHeader, TEXT("node"));
		const int32 LineNumberColumn = FindDialogColumn(LineHeader, TEXT("linenumber"));

		if (IdColumn == INDEX_NONE || TextColumn == INDEX_NONE || NodeColumn == INDEX_NONE || LineNumberColumn == INDEX_NONE)
		{
			Sink.Error(LinesFile->Path, 1, FString::Printf(TEXT("needs id, text, node and lineNumber columns, as ysc writes them - %s - the file was skipped"), *CompileHint));
			return 0;
		}

		TArray<FDialogText> FileTexts;

		for (int32 RowIndex = 1; RowIndex < LineRows.Num(); ++RowIndex)
		{
			const FDialogCsvRow& Row = LineRows[RowIndex];

			if (IsBlankDialogRow(Row))
			{
				continue;
			}

			const FString YarnId = GetDialogField(Row, IdColumn).TrimStartAndEnd();
			const FString Node = GetDialogField(Row, NodeColumn).TrimStartAndEnd();
			int32 SourceLine = 0;
			ParseDialogInteger(GetDialogField(Row, LineNumberColumn), SourceLine);

			const FString LocalId = StripYarnLineId(YarnId);

			auto LineError = [&](const FString& Message)
			{
				Sink.Error(FilePath, SourceLine, Message);
				BreakNode(Node);
			};

			// ysc makes up an id for a line that has none, and it changes whenever the line moves -
			// orphaning its translations and any recording. The source shows whether it was written.
			if (!YarnId.StartsWith(YarnLineIdPrefix) || !SourceIndex.LineHasTag(SourceLine, TEXT("#") + YarnId))
			{
				LineError(FString::Printf(TEXT("the line has no #line: id of its own ('%s' is one ysc made up, which changes whenever the line moves) - run 'ysc tag %s.yarn' to stamp real ids, then recompile"),
					*YarnId, *BaseName));
				continue;
			}

			if (!IsValidDialogLocalId(LocalId))
			{
				LineError(FString::Printf(TEXT("#line:%s - a line id is letters, digits and _"), *LocalId));
				continue;
			}

			const FName QualifiedId(*FString::Printf(TEXT("%s.%s"), *Info.Id.ToString(), *LocalId));

			if (const FString* FirstSeen = SeenIds.Find(QualifiedId))
			{
				LineError(FString::Printf(TEXT("#line:%s is already used in this package (%s)"), *LocalId, **FirstSeen));
				continue;
			}

			FString Cue;
			FString Text;
			SmoresDialog::SplitSpeaker(GetDialogField(Row, TextColumn), Cue, Text);

			if (Text.IsEmpty())
			{
				LineError(FString::Printf(TEXT("#line:%s has no text"), *LocalId));
				continue;
			}

			FConversationLineInfo& LineInfo = Script->Lines.Add(YarnId);
			LineInfo.QualifiedId = QualifiedId;
			LineInfo.SpeakerCue = Cue;
			LineInfo.SourceText = Text;
			LineInfo.Line = SourceLine;

			SeenIds.Add(QualifiedId, FString::Printf(TEXT("%s:%d"), *FilePath, SourceLine));

			FDialogText& Entry = FileTexts.AddDefaulted_GetRef();
			Entry.Id = QualifiedId;
			Entry.PackageId = Info.Id;
			Entry.SourceText = Text;
			Entry.File = FilePath;
			Entry.Line = SourceLine;
		}

		// the other way round: a #line: tag in the source that the table doesn't have means the
		// .yarn changed after it was compiled - the check a timestamp can't be trusted with
		for (int32 Index = 0; Index < SourceIndex.Lines.Num(); ++Index)
		{
			const FString& SourceText = SourceIndex.Lines[Index];

			if (SourceText.TrimStart().StartsWith(TEXT("//")))
			{
				continue;
			}

			int32 From = 0;

			while (true)
			{
				const int32 At = SourceText.Find(TEXT("#line:"), ESearchCase::CaseSensitive, ESearchDir::FromStart, From);

				if (At == INDEX_NONE)
				{
					break;
				}

				int32 End = At + 6;

				while (End < SourceText.Len() && (FChar::IsAlnum(SourceText[End]) || SourceText[End] == TEXT('_')))
				{
					++End;
				}

				const FString YarnId = SourceText.Mid(At + 1, End - At - 1);

				if (End > At + 6 && !Script->Lines.Contains(YarnId) && !LineRows.ContainsByPredicate([&YarnId, IdColumn](const FDialogCsvRow& Row) { return GetDialogField(Row, IdColumn).TrimStartAndEnd() == YarnId; }))
				{
					Sink.Error(FilePath, Index + 1, FString::Printf(TEXT("#%s isn't in %s-Lines.csv, so the .yarn changed after it was compiled - %s - the file was skipped"), *YarnId, *BaseName, *CompileHint));
					bFileOk = false;
				}

				From = End;
			}
		}

		if (!bFileOk)
		{
			return 0;
		}

		// 2. The program: every line it says, every function it calls, every command it runs
		for (const TPair<FString, FYarnNode>& NodePair : Script->Program.Nodes)
		{
			const FString& NodeName = NodePair.Key;
			const TArray<FYarnInstruction>& Instructions = NodePair.Value.Instructions;
			FYarnNodeReport& Report = Reports.FindOrAdd(NodeName);

			auto NodeError = [&](int32 Line, const FString& Message)
			{
				Sink.Error(FilePath, Line, FString::Printf(TEXT("node '%s': %s"), *NodeName, *Message));
				Report.bBroken = true;
			};

			for (int32 Index = 0; Index < Instructions.Num(); ++Index)
			{
				const FYarnInstruction& Instruction = Instructions[Index];
				const FYarnInstruction* Previous = Index > 0 ? &Instructions[Index - 1] : nullptr;

				switch (Instruction.Type)
				{
				case EYarnInstructionType::RunLine:
				case EYarnInstructionType::AddOption:
				{
					const bool bIsOption = Instruction.Type == EYarnInstructionType::AddOption;
					const int32 Substitutions = bIsOption ? Instruction.IntOperand2 : Instruction.IntOperand;

					Report.LineIds.Add(Instruction.StringOperand);

					if (Substitutions > 0)
					{
						Report.SubstitutedLines.Add(Instruction.StringOperand);
					}

					FConversationLineInfo* LineInfo = Script->Lines.Find(Instruction.StringOperand);

					if (!LineInfo)
					{
						// a line whose row was refused above is already reported; anything else is a stale table
						const bool bAlreadyReported = LineRows.ContainsByPredicate([&Instruction, IdColumn](const FDialogCsvRow& Row)
						{
							return GetDialogField(Row, IdColumn).TrimStartAndEnd() == Instruction.StringOperand;
						});

						if (!bAlreadyReported)
						{
							NodeError(SourceIndex.GetNodeLine(NodeName), FString::Printf(TEXT("says %s, which %s-Lines.csv doesn't have - %s"), *Instruction.StringOperand, *BaseName, *CompileHint));
						}

						Report.bBroken = true;
						break;
					}

					if (bIsOption)
					{
						LineInfo->bIsOption = true;
						LineInfo->bHasCondition = Instruction.BoolOperand;
						Report.OptionIds.Add(Instruction.StringOperand);
					}

					break;
				}

				case EYarnInstructionType::CallFunction:
				{
					const FString& Name = Instruction.StringOperand;

					// the compiler pushes how many values it passes just before the call
					const int32 Passed = Previous && Previous->Type == EYarnInstructionType::PushFloat ? FMath::RoundToInt(Previous->FloatOperand) : INDEX_NONE;
					const int32 Line = SourceIndex.FindInNode(NodeName, Name + TEXT("("));

					int32 Expected = 0;

					if (!SmoresDialog::FindYarnBuiltIn(Name, Expected))
					{
						const FDialogFact* Fact = SmoresDialog::FindFactForYarnFunction(Facts, Name);

						if (!Fact)
						{
							NodeError(Line, FString::Printf(TEXT("calls %s(), which isn't a fact the game knows - every fact is a function with '.' written as '_' (speaker_faction(), gold()); SmoresDialogReport facts lists them"), *Name));
							break;
						}

						if (EnumHasAnyFlags(Fact->Reads, EDialogSubject::Victim))
						{
							NodeError(Line, FString::Printf(TEXT("calls %s(), which asks about Event.Victim - a conversation has only the Speaker, the Listener and the Player"), *Name));
							break;
						}

						Expected = Fact->bTakesArgument ? 1 : 0;
					}

					if (Passed != INDEX_NONE && Passed != Expected)
					{
						NodeError(Line, FString::Printf(TEXT("%s() takes %d value%s, and this passes %d"), *Name, Expected, Expected == 1 ? TEXT("") : TEXT("s"), Passed));
					}

					break;
				}

				case EYarnInstructionType::RunCommand:
				{
					FString Name;
					TArray<FString> Arguments;
					FYarnCommand::ParseCommandText(Instruction.StringOperand, Name, Arguments);

					const int32 Line = SourceIndex.FindInNode(NodeName, TEXT("<<") + Name);
					const FDialogEffect* Effect = Effects.Find(FName(*Name));

					if (!Effect)
					{
						NodeError(Line, FString::Printf(TEXT("runs <<%s>>, which isn't an effect the game has - the effects are %s"), *Name, *DescribeEffectNames(Effects)));
						break;
					}

					if (Arguments.Num() < Effect->MinArguments || Arguments.Num() > Effect->MaxArguments)
					{
						const FString Expected = Effect->MinArguments == Effect->MaxArguments
							? FString::Printf(TEXT("%d"), Effect->MinArguments)
							: FString::Printf(TEXT("%d to %d"), Effect->MinArguments, Effect->MaxArguments);

						NodeError(Line, FString::Printf(TEXT("<<%s>> takes %s word%s after its name (%s), and this has %d"),
							*Name, *Expected, Effect->MaxArguments == 1 ? TEXT("") : TEXT("s"), *Effect->Usage, Arguments.Num()));
						break;
					}

					TArray<FString> Errors;
					TArray<FString> Warnings;

					if (Effect->Check)
					{
						Effect->Check(Arguments, KnownIds, Errors, Warnings);
					}

					for (const FString& Warning : Warnings)
					{
						Sink.Warning(FilePath, Line, FString::Printf(TEXT("node '%s': <<%s>>: %s"), *NodeName, *Name, *Warning));
					}

					for (const FString& Error : Errors)
					{
						NodeError(Line, FString::Printf(TEXT("<<%s>>: %s"), *Name, *Error));
					}

					if (Effect->bNeedsWindow)
					{
						Report.WindowEffects.Add(Effect->Name.ToString());
					}

					break;
				}

				case EYarnInstructionType::RunNode:
				case EYarnInstructionType::DetourToNode:
					Report.Targets.Add(Instruction.StringOperand);
					break;

				case EYarnInstructionType::PeekAndRunNode:
				case EYarnInstructionType::PeekAndDetourToNode:
					// <<jump {$somewhere}>> can't be followed; a jump to a name written out can
					if (Previous && Previous->Type == EYarnInstructionType::PushString)
					{
						Report.Targets.Add(Previous->StringOperand);
					}
					break;

				default:
					break;
				}
			}
		}

		// 3. Line tags: #reason:<key> on a choice
		if (MetadataFile)
		{
			TArray<FDialogCsvRow> MetadataRows;
			TMap<FString, int32> MetadataHeader;

			if (!ReadYarnCsv(*MetadataFile, MetadataRows, MetadataHeader, CsvError))
			{
				Sink.Error(MetadataFile->Path, 0, CsvError + TEXT(" - the file was skipped"));
				return 0;
			}

			const int32 MetaIdColumn = FindDialogColumn(MetadataHeader, TEXT("id"));
			const int32 TagsColumn = FindDialogColumn(MetadataHeader, TEXT("tags"));
			const int32 MetaNodeColumn = FindDialogColumn(MetadataHeader, TEXT("node"));

			for (int32 RowIndex = 1; RowIndex < MetadataRows.Num() && MetaIdColumn != INDEX_NONE && TagsColumn != INDEX_NONE; ++RowIndex)
			{
				const FDialogCsvRow& Row = MetadataRows[RowIndex];
				FConversationLineInfo* LineInfo = Script->Lines.Find(GetDialogField(Row, MetaIdColumn).TrimStartAndEnd());

				if (!LineInfo)
				{
					continue;
				}

				const FString Node = GetDialogField(Row, MetaNodeColumn).TrimStartAndEnd();

				TArray<FString> Tags;
				GetDialogField(Row, TagsColumn).ParseIntoArrayWS(Tags);

				for (const FString& Tag : Tags)
				{
					FString Key;

					if (!Tag.Split(TEXT(":"), nullptr, &Key) || !Tag.StartsWith(TEXT("reason:")))
					{
						continue;
					}

					ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

					if (!SmoresDialog::FindChoiceReason(FName(*Key), Reason))
					{
						Sink.Error(FilePath, LineInfo->Line, FString::Printf(TEXT("#reason:%s isn't a reason the game has - they are %s"),
							*Key, *FString::JoinBy(SmoresDialog::GetChoiceReasonKeys(), TEXT(", "), [](FName ReasonKey) { return ReasonKey.ToString(); })));
						BreakNode(Node);
						continue;
					}

					if (!LineInfo->bIsOption)
					{
						Sink.Error(FilePath, LineInfo->Line, FString::Printf(TEXT("#reason:%s is on a line, and only a choice (->) can carry one"), *Key));
						BreakNode(Node);
						continue;
					}

					if (!LineInfo->bHasCondition)
					{
						Sink.Warning(FilePath, LineInfo->Line, FString::Printf(TEXT("#reason:%s is on a choice with no <<if>>, which can never be unavailable - so it never shows"), *Key));
					}

					LineInfo->ReasonKey = FName(*Key);
					LineInfo->Reason = Reason;
				}
			}
		}

		// 4. The conversations: every node with our headers, in the order they were written
		TArray<FString> NodeNames;
		Script->Program.Nodes.GetKeys(NodeNames);
		NodeNames.Sort([&SourceIndex](const FString& A, const FString& B)
		{
			const int32 LineA = SourceIndex.GetNodeLine(A);
			const int32 LineB = SourceIndex.GetNodeLine(B);
			return LineA != LineB ? LineA < LineB : A < B;
		});

		const EDialogSubject ConversationSubjects = EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player;
		TArray<FConversationDefinition> FileConversations;

		for (const FString& NodeName : NodeNames)
		{
			const TMap<FString, FString> Headers = ReadNodeHeaders(Script->Program.Nodes[NodeName]);

			if (!HasConversationHeaders(Headers))
			{
				continue;
			}

			const int32 NodeLine = SourceIndex.GetNodeLine(NodeName);
			bool bOk = true;

			auto HeaderError = [&](const TCHAR* Key, const FString& Message)
			{
				Sink.Error(FilePath, SourceIndex.GetHeaderLine(NodeName, Key), FString::Printf(TEXT("conversation '%s': %s - it was skipped"), *NodeName, *Message));
				bOk = false;
			};

			FConversationDefinition Conversation;
			Conversation.LocalId = FName(*NodeName);
			Conversation.Id = FName(*FString::Printf(TEXT("%s.%s"), *Info.Id.ToString(), *NodeName));
			Conversation.PackageId = Info.Id;
			Conversation.File = FilePath;
			Conversation.Line = NodeLine;

			if (!IsValidDialogLocalId(NodeName))
			{
				HeaderError(TEXT("title"), TEXT("its title is its id, so it must be letters, digits and _"));
			}

			const FString* KindText = Headers.Find(KindHeader);

			if (!KindText)
			{
				HeaderError(KindHeader, TEXT("has conversation headers but no kind: - add kind: Greeting, Topic or Ambient"));
				continue;
			}

			if (!ParseConversationKind(*KindText, Conversation.Kind))
			{
				HeaderError(KindHeader, FString::Printf(TEXT("kind: '%s' isn't Greeting, Topic or Ambient"), **KindText));
				continue;
			}

			const bool bAmbient = Conversation.Kind == EConversationKind::Ambient;

			if (const FString* PriorityText = Headers.Find(PriorityHeader))
			{
				if (!ParseDialogInteger(*PriorityText, Conversation.Priority))
				{
					HeaderError(PriorityHeader, FString::Printf(TEXT("priority: '%s' isn't a whole number"), **PriorityText));
				}
			}

			if (const FString* OnceText = Headers.Find(OnceHeader))
			{
				if (OnceText->Equals(TEXT("true"), ESearchCase::IgnoreCase))
				{
					Conversation.bOnce = true;
				}
				else if (!OnceText->Equals(TEXT("false"), ESearchCase::IgnoreCase))
				{
					HeaderError(OnceHeader, FString::Printf(TEXT("once: is true or false, not '%s'"), **OnceText));
				}
			}

			auto CompileHeaderCondition = [&](const TCHAR* Key, const FString& Text, EDialogSubject Subjects, FDialogCondition& OutCondition)
			{
				TArray<FString> Errors;
				TArray<FString> Warnings;

				const bool bCompiled = SmoresDialog::CompileCondition(Text, Facts, Subjects, KnownIds, OutCondition, Errors, Warnings);

				for (const FString& Warning : Warnings)
				{
					Sink.Warning(FilePath, SourceIndex.GetHeaderLine(NodeName, Key), FString::Printf(TEXT("conversation '%s': %s: %s"), *NodeName, Key, *Warning));
				}

				if (!bCompiled)
				{
					for (const FString& Error : Errors)
					{
						HeaderError(Key, FString::Printf(TEXT("%s: %s"), Key, *Error));
					}
				}
			};

			if (const FString* AttachText = Headers.Find(AttachHeader))
			{
				if (bAmbient)
				{
					HeaderError(AttachHeader, TEXT("an Ambient conversation is between squad members and attaches to nobody - use requires: to say who may say it"));
				}
				else
				{
					// attach says who the conversation belongs to, so it asks about the NPC and nobody else
					CompileHeaderCondition(AttachHeader, *AttachText, EDialogSubject::Speaker, Conversation.Attach);
				}
			}
			else if (!bAmbient)
			{
				HeaderError(AttachHeader, TEXT("a Greeting or Topic needs attach: to say who it belongs to (attach: Speaker.Definition == Bandit)"));
			}

			if (const FString* RequiresText = Headers.Find(RequiresHeader))
			{
				CompileHeaderCondition(RequiresHeader, *RequiresText, ConversationSubjects, Conversation.Requires);
			}

			if (const FString* ParticipantsText = Headers.Find(ParticipantsHeader))
			{
				if (!bAmbient)
				{
					HeaderError(ParticipantsHeader, TEXT("only an Ambient conversation has participants: - a window conversation is always the NPC and You"));
				}
				else
				{
					TArray<FString> Names;
					ParticipantsText->ParseIntoArray(Names, TEXT(","));

					for (FString& Name : Names)
					{
						Name.TrimStartAndEndInline();

						if (!IsValidDialogLocalId(Name) || Conversation.Participants.Contains(FName(*Name)))
						{
							HeaderError(ParticipantsHeader, FString::Printf(TEXT("participants: '%s' - each is a different name, letters, digits and _"), *Name));
							break;
						}

						Conversation.Participants.Add(FName(*Name));
					}

					if (Conversation.Participants.Num() < 2 || Conversation.Participants.Num() > 4)
					{
						HeaderError(ParticipantsHeader, TEXT("participants: names 2 to 4 squad members"));
					}
				}
			}
			else if (bAmbient)
			{
				HeaderError(ParticipantsHeader, TEXT("an Ambient conversation needs participants: - the names its lines are said under (participants: First, Second)"));
			}

			if (const FString* LabelText = Headers.Find(LabelHeader))
			{
				if (Conversation.Kind != EConversationKind::Topic)
				{
					HeaderError(LabelHeader, TEXT("only a Topic has a label: - it is the choice that offers it"));
				}
				else if (LabelText->IsEmpty())
				{
					HeaderError(LabelHeader, TEXT("label: is empty"));
				}
				else
				{
					const FString LabelLocalId = NodeName + TEXT("_label");
					Conversation.LabelId = FName(*FString::Printf(TEXT("%s.%s"), *Info.Id.ToString(), *LabelLocalId));

					if (const FString* FirstSeen = SeenIds.Find(Conversation.LabelId))
					{
						HeaderError(LabelHeader, FString::Printf(TEXT("its label's id %s is already used in this package (%s)"), *LabelLocalId, **FirstSeen));
					}
				}
			}
			else if (Conversation.Kind == EConversationKind::Topic)
			{
				HeaderError(LabelHeader, TEXT("a Topic needs label: - the choice text that offers it (label: Any chance of a better price?)"));
			}

			if (const FString* FirstSeen = SeenIds.Find(Conversation.Id))
			{
				HeaderError(TEXT("title"), FString::Printf(TEXT("the title is already a conversation or line in this package (%s)"), **FirstSeen));
			}

			// what it plays: every node it reaches must be sound, and an Ambient one may only do what
			// banter can - no window, so no choices and nothing that needs one
			const TArray<FString> Reached = GetReachableNodes(NodeName, Reports);

			for (const FString& Node : Reached)
			{
				const FYarnNodeReport* Report = Reports.Find(Node);

				if (!Script->Program.Nodes.Contains(Node))
				{
					Sink.Error(FilePath, NodeLine, FString::Printf(TEXT("conversation '%s' jumps to '%s', which this file has no node called - it was skipped"), *NodeName, *Node));
					bOk = false;
					continue;
				}

				if (Report && Report->bBroken)
				{
					Sink.Error(FilePath, NodeLine, Node == NodeName
						? FString::Printf(TEXT("conversation '%s' was skipped because of the errors above"), *NodeName)
						: FString::Printf(TEXT("conversation '%s' was skipped: it plays node '%s', which has the errors above"), *NodeName, *Node));
					bOk = false;
					continue;
				}

				if (!bAmbient || !Report)
				{
					continue;
				}

				for (const FString& OptionId : Report->OptionIds)
				{
					const FConversationLineInfo* LineInfo = Script->FindLine(OptionId);
					Sink.Error(FilePath, LineInfo ? LineInfo->Line : SourceIndex.GetNodeLine(Node),
						FString::Printf(TEXT("conversation '%s' is Ambient, and Ambient plays with no window, so it can't offer choices - it was skipped"), *NodeName));
					bOk = false;
				}

				for (const FString& EffectName : Report->WindowEffects)
				{
					Sink.Error(FilePath, SourceIndex.FindInNode(Node, TEXT("<<") + EffectName),
						FString::Printf(TEXT("conversation '%s' is Ambient, and <<%s>> needs the conversation window - it was skipped"), *NodeName, *EffectName));
					bOk = false;
				}

				for (const FString& LineId : Report->LineIds)
				{
					const FConversationLineInfo* LineInfo = Script->FindLine(LineId);

					if (!LineInfo || LineInfo->bIsOption)
					{
						continue;
					}

					if (SmoresDialog::GetSpeakerSlot(LineInfo->SpeakerCue, Conversation.Participants) == SmoresDialog::NarrationSlot)
					{
						const FString ParticipantList = FString::JoinBy(Conversation.Participants, TEXT(", "), [](FName Name) { return Name.ToString(); });

						Sink.Error(FilePath, LineInfo->Line, LineInfo->SpeakerCue.IsEmpty()
							? FString::Printf(TEXT("conversation '%s': every Ambient line needs a speaker - start it with one of %s and a colon - it was skipped"), *NodeName, *ParticipantList)
							: FString::Printf(TEXT("conversation '%s': '%s' isn't one of its participants (%s) - it was skipped"), *NodeName, *LineInfo->SpeakerCue, *ParticipantList));
						bOk = false;
					}

					if (Report->SubstitutedLines.Contains(LineId))
					{
						Sink.Error(FilePath, LineInfo->Line, FString::Printf(TEXT("conversation '%s': an Ambient line can't insert values ({...}) yet - it was skipped"), *NodeName));
						bOk = false;
					}
				}
			}

			if (!bOk)
			{
				continue;
			}

			SeenIds.Add(Conversation.Id, FString::Printf(TEXT("%s:%d"), *FilePath, NodeLine));

			if (!Conversation.LabelId.IsNone())
			{
				SeenIds.Add(Conversation.LabelId, FString::Printf(TEXT("%s:%d"), *FilePath, SourceIndex.GetHeaderLine(NodeName, LabelHeader)));

				FDialogText& Label = FileTexts.AddDefaulted_GetRef();
				Label.Id = Conversation.LabelId;
				Label.PackageId = Info.Id;
				Label.SourceText = *Headers.Find(LabelHeader);
				Label.File = FilePath;
				Label.Line = SourceIndex.GetHeaderLine(NodeName, LabelHeader);
			}

			FileConversations.Add(MoveTemp(Conversation));
		}

		// every conversation in the file shares the one loaded script
		for (FConversationDefinition& Conversation : FileConversations)
		{
			Conversation.Script = Script;
			Conversation.LoadOrder = Library.Conversations.Num();
			Library.Conversations.Add(MoveTemp(Conversation));
		}

		Library.Texts.Append(MoveTemp(FileTexts));

		return FileConversations.Num();
	}
}

namespace SmoresDialogLoading
{
	int32 LoadDialogConversations(
		const FDialogPackageSource& Source,
		const FDialogPackageInfo& Info,
		const FDialogFactRegistry& Facts,
		const FDialogEffectRegistry& Effects,
		const FDialogKnownIds* KnownIds,
		const FDialogProblemSink& Sink,
		FDialogLibrary& Library)
	{
		// a line id may not reuse one of the package's bark ids: they share its string table
		TMap<FName, FString> SeenIds;

		for (const FBarkLine& Bark : Library.Barks)
		{
			if (Bark.PackageId == Info.Id)
			{
				SeenIds.Add(Bark.Id, FString::Printf(TEXT("%s:%d"), *Bark.File, Bark.Line));
			}
		}

		// a compiled file without its .yarn is almost always one left behind by a rename
		for (const FDialogSourceFile* Compiled : GetDialogFilesIn(Source, DialogConversationsFolder, TEXT(".yarnc")))
		{
			if (!FindDialogSourceFile(Source, Compiled->Path.LeftChop(1)))
			{
				Sink.Warning(Compiled->Path, 0, FString::Printf(TEXT("has no %s beside it, so it was ignored - a conversation is loaded from its .yarn"),
					*FPaths::GetCleanFilename(Compiled->Path.LeftChop(1))));
			}
		}

		const int32 Before = Library.Conversations.Num();

		for (const FDialogSourceFile* YarnFile : GetDialogFilesIn(Source, DialogConversationsFolder, TEXT(".yarn")))
		{
			LoadYarnFile(*YarnFile, Source, Info, Facts, Effects, KnownIds, Sink, SeenIds, Library);
		}

		return Library.Conversations.Num() - Before;
	}
}
