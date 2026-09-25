// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "SpikeConversation.h"
#include "SmoresDialogSpike.h"
#include "DialogLoader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "YarnProgram.h"
#include "YarnProtobufParser.h"
#include "YarnSpinnerCore.h"
#include "YarnVariableStorage.h"
#include "YarnVirtualMachine.h"

/**
 *  The Yarn half of the spike: the Yarn Spinner for Unreal plugin's player (its "virtual machine"),
 *  driven directly - none of the plugin's dialogue runner, presenters or on-screen widgets.
 *
 *  Loading reads the two files ysc writes: the compiled program, and a table of every line's text
 *  keyed by its id. The program itself holds only ids, which is exactly our "ids cross the network,
 *  each client looks up its own text" rule. The plugin only reads the program inside the editor, so
 *  the spike moved that one reader into the plugin's runtime half (see YarnProtobufParser.h).
 */
struct FSpikeYarnScript
{
	FYarnProgram Program;

	/** Line id -> text, from <Name>-Lines.csv */
	TMap<FString, FString> LineText;
};

namespace
{
	using FYarnFunction = TFunction<FYarnValue(const TArray<FYarnValue>&)>;

	struct FYarnFunctionEntry
	{
		int32 ParameterCount = 0;
		FYarnFunction Call;
	};

	/**
	 *  The operators compiled Yarn calls as functions - `gold() >= 20` becomes a call to
	 *  "Number.GreaterThanOrEqualTo". The plugin registers these inside its dialogue runner, not its
	 *  player, so driving the player alone means supplying them. Our own implementation, of the
	 *  subset expressions need; the plugin's library also has visited(), dice() and the like.
	 */
	TMap<FString, FYarnFunctionEntry> MakeYarnOperators()
	{
		TMap<FString, FYarnFunctionEntry> Functions;

		auto AddNumber = [&Functions](const TCHAR* Name, TFunction<float(float, float)> Op)
		{
			Functions.Add(Name, { 2, [Op](const TArray<FYarnValue>& P) { return FYarnValue(Op(P[0].ConvertToNumber(), P[1].ConvertToNumber())); } });
		};
		auto AddCompare = [&Functions](const TCHAR* Name, TFunction<bool(float, float)> Op)
		{
			Functions.Add(Name, { 2, [Op](const TArray<FYarnValue>& P) { return FYarnValue(Op(P[0].ConvertToNumber(), P[1].ConvertToNumber())); } });
		};
		auto AddBool = [&Functions](const TCHAR* Name, TFunction<bool(bool, bool)> Op)
		{
			Functions.Add(Name, { 2, [Op](const TArray<FYarnValue>& P) { return FYarnValue(Op(P[0].ConvertToBool(), P[1].ConvertToBool())); } });
		};

		AddNumber(TEXT("Number.Add"),      [](float A, float B) { return A + B; });
		AddNumber(TEXT("Number.Minus"),    [](float A, float B) { return A - B; });
		AddNumber(TEXT("Number.Multiply"), [](float A, float B) { return A * B; });
		AddNumber(TEXT("Number.Divide"),   [](float A, float B) { return B != 0.f ? A / B : 0.f; });
		AddNumber(TEXT("Number.Modulo"),   [](float A, float B) { return B != 0.f ? FMath::Fmod(A, B) : 0.f; });
		Functions.Add(TEXT("Number.UnaryMinus"), { 1, [](const TArray<FYarnValue>& P) { return FYarnValue(-P[0].ConvertToNumber()); } });

		AddCompare(TEXT("Number.EqualTo"),              [](float A, float B) { return FMath::IsNearlyEqual(A, B); });
		AddCompare(TEXT("Number.NotEqualTo"),           [](float A, float B) { return !FMath::IsNearlyEqual(A, B); });
		AddCompare(TEXT("Number.GreaterThan"),          [](float A, float B) { return A > B; });
		AddCompare(TEXT("Number.GreaterThanOrEqualTo"), [](float A, float B) { return A >= B; });
		AddCompare(TEXT("Number.LessThan"),             [](float A, float B) { return A < B; });
		AddCompare(TEXT("Number.LessThanOrEqualTo"),    [](float A, float B) { return A <= B; });

		AddBool(TEXT("Bool.EqualTo"),    [](bool A, bool B) { return A == B; });
		AddBool(TEXT("Bool.NotEqualTo"), [](bool A, bool B) { return A != B; });
		AddBool(TEXT("Bool.And"),        [](bool A, bool B) { return A && B; });
		AddBool(TEXT("Bool.Or"),         [](bool A, bool B) { return A || B; });
		AddBool(TEXT("Bool.Xor"),        [](bool A, bool B) { return A != B; });
		Functions.Add(TEXT("Bool.Not"), { 1, [](const TArray<FYarnValue>& P) { return FYarnValue(!P[0].ConvertToBool()); } });

		Functions.Add(TEXT("String.Add"),        { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(P[0].ConvertToString() + P[1].ConvertToString()); } });
		Functions.Add(TEXT("String.EqualTo"),    { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(P[0].ConvertToString() == P[1].ConvertToString()); } });
		Functions.Add(TEXT("String.NotEqualTo"), { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(P[0].ConvertToString() != P[1].ConvertToString()); } });

		return Functions;
	}

	class FSpikeYarnConversation final : public ISpikeConversation
	{
	public:

		FSpikeYarnConversation(const TSharedRef<FSpikeYarnScript>& InScript, const FString& InStartNode, FSpikeGameHooks InHooks)
			: Script(InScript)
			, StartNode(InStartNode)
			, Hooks(MoveTemp(InHooks))
			, Functions(MakeYarnOperators())
		{
			// The game's questions, next to the operators. Slice 3 would register these from the fact
			// registry instead of by hand.
			Functions.Add(TEXT("gold"), { 0, [this](const TArray<FYarnValue>&) { return FYarnValue(static_cast<float>(Hooks.GetGold ? Hooks.GetGold() : 0)); } });
		}

		virtual FString GetLanguage() const override { return TEXT("Yarn"); }

		virtual bool Start() override
		{
			// Each conversation gets its own memory, so one squad's $asked_about_road never leaks into
			// another's. The plugin's in-memory store is a component, but works as a plain object.
			Variables.Reset(NewObject<UYarnInMemoryVariableStorage>(GetTransientPackage()));

			VM.SetProgram(Script->Program);
			VM.VariableStorage = TScriptInterface<IYarnVariableStorage>(Variables.Get());

			VM.LineHandler.BindRaw(this, &FSpikeYarnConversation::HandleLine);
			VM.OptionsHandler.BindRaw(this, &FSpikeYarnConversation::HandleOptions);
			VM.CommandHandler.BindRaw(this, &FSpikeYarnConversation::HandleCommand);
			VM.DialogueCompleteHandler.BindRaw(this, &FSpikeYarnConversation::HandleComplete);
			VM.CallFunctionHandler.BindRaw(this, &FSpikeYarnConversation::CallFunction);
			VM.FunctionExistsHandler.BindRaw(this, &FSpikeYarnConversation::FunctionExists);
			VM.FunctionParamCountHandler.BindRaw(this, &FSpikeYarnConversation::FunctionParamCount);
			VM.FunctionErroredHandler.BindLambda([](FString&) { return false; });

			if (!VM.SetNode(StartNode))
			{
				Fail(FString::Printf(TEXT("there is no node '%s'"), *StartNode));
				return false;
			}

			Run();
			return State != ESpikeState::Failed;
		}

		virtual ESpikeState GetState() const override { return State; }
		virtual const FSpikeLine& GetLine() const override { return CurrentLine; }
		virtual const TArray<FSpikeChoice>& GetChoices() const override { return CurrentChoices; }
		virtual FString GetError() const override { return Error; }

		virtual void Advance() override
		{
			if (State == ESpikeState::Line)
			{
				Run();
			}
		}

		virtual bool Choose(int32 Index) override
		{
			if (State != ESpikeState::Choices || !CurrentChoices.IsValidIndex(Index) || !CurrentChoices[Index].bAvailable)
			{
				return false;
			}

			// The option set's positions match CurrentChoices', unavailable ones included.
			VM.SetSelectedOption(Index);
			Run();
			return true;
		}

	private:

		/**
		 *  Continue until the script shows a line, offers choices or ends.
		 *
		 *  The player pauses after a command as well as after a line, waiting to be told the command
		 *  has finished (so a <<wait 2>> can take two seconds). Every spike command finishes at once,
		 *  so a pause after one is continued straight through.
		 */
		void Run()
		{
			State = ESpikeState::Ended;
			bPausedOnCommand = false;
			bHaveContent = false;

			VM.Continue();
			while (bPausedOnCommand && !bHaveContent && State != ESpikeState::Failed)
			{
				bPausedOnCommand = false;
				VM.Continue();
			}

			if (VM.GetExecutionState() == EYarnExecutionState::Error && State != ESpikeState::Failed)
			{
				Fail(TEXT("the player stopped with an error (see LogYarnSpinner)"));
			}
		}

		/** Yarn's ids are the whole tag, "line:shakedown_toll"; the spike compares the part after it */
		static FString ShortId(const FString& LineId)
		{
			return LineId.StartsWith(TEXT("line:")) ? LineId.Mid(5) : LineId;
		}

		void HandleLine(const FYarnLine& Line)
		{
			CurrentLine.Id = ShortId(Line.LineID);
			SmoresDialogSpike::SplitSpeaker(LookUp(Line.LineID, Line.Substitutions), CurrentLine.Speaker, CurrentLine.Text);
			CurrentChoices.Reset();
			State = ESpikeState::Line;
			bHaveContent = true;
		}

		void HandleOptions(const FYarnOptionSet& Options)
		{
			CurrentChoices.Reset();
			for (const FYarnOption& Option : Options.Options)
			{
				FSpikeChoice& Choice = CurrentChoices.AddDefaulted_GetRef();
				Choice.Id = ShortId(Option.Line.RawLine.LineID);
				Choice.Text = LookUp(Option.Line.RawLine.LineID, Option.Line.RawLine.Substitutions);
				Choice.bAvailable = Option.bIsAvailable;
			}
			State = ESpikeState::Choices;
			bHaveContent = true;
		}

		void HandleCommand(const FYarnCommand& Command)
		{
			bPausedOnCommand = true;
			if (Hooks.RunCommand)
			{
				Hooks.RunCommand(Command.CommandName, Command.Parameters);
			}
		}

		void HandleComplete()
		{
			State = ESpikeState::Ended;
			bHaveContent = true;
		}

		FYarnValue CallFunction(const FString& Name, const TArray<FYarnValue>& Parameters)
		{
			const FYarnFunctionEntry* Entry = Functions.Find(Name);
			if (!Entry || Parameters.Num() < Entry->ParameterCount)
			{
				Fail(FString::Printf(TEXT("the script called %s(), which the game doesn't answer"), *Name));
				return FYarnValue();
			}
			return Entry->Call(Parameters);
		}

		bool FunctionExists(const FString& Name) { return Functions.Contains(Name); }

		int32 FunctionParamCount(const FString& Name)
		{
			const FYarnFunctionEntry* Entry = Functions.Find(Name);
			return Entry ? Entry->ParameterCount : -1;
		}

		FString LookUp(const FString& Id, const TArray<FString>& Substitutions) const
		{
			const FString* Text = Script->LineText.Find(Id);
			if (!Text)
			{
				return FString::Printf(TEXT("<missing text for %s>"), *Id);
			}
			return Substitutions.Num() > 0 ? FYarnVirtualMachine::ExpandSubstitutions(*Text, Substitutions) : *Text;
		}

		void Fail(const FString& Message)
		{
			Error = Message;
			State = ESpikeState::Failed;
			UE_LOG(LogSmoresDialogSpike, Warning, TEXT("[Yarn] %s"), *Message);
		}

		TSharedRef<FSpikeYarnScript> Script;
		FString StartNode;
		FSpikeGameHooks Hooks;
		TMap<FString, FYarnFunctionEntry> Functions;

		FYarnVirtualMachine VM;
		TStrongObjectPtr<UYarnInMemoryVariableStorage> Variables;

		ESpikeState State = ESpikeState::Ended;
		FSpikeLine CurrentLine;
		TArray<FSpikeChoice> CurrentChoices;
		FString Error;

		bool bPausedOnCommand = false;
		bool bHaveContent = false;
	};
}

TSharedPtr<FSpikeYarnScript> SmoresDialogSpike::LoadYarnScript(const FString& Directory, const FString& Name, FString& OutError)
{
	const FString ProgramPath = FPaths::Combine(Directory, Name + TEXT(".yarnc"));
	const FString LinesPath = FPaths::Combine(Directory, Name + TEXT("-Lines.csv"));

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *ProgramPath, FILEREAD_Silent))
	{
		OutError = FString::Printf(TEXT("can't read %s"), *ProgramPath);
		return nullptr;
	}

	TSharedRef<FSpikeYarnScript> Script = MakeShared<FSpikeYarnScript>();

	FYarnProtobufParser Parser(Bytes);
	FString ParseError;
	if (!Parser.ParseProgram(Script->Program, ParseError))
	{
		OutError = FString::Printf(TEXT("%s isn't a compiled Yarn program: %s"), *ProgramPath, *ParseError);
		return nullptr;
	}

	if (Script->Program.Nodes.Num() == 0)
	{
		OutError = FString::Printf(TEXT("%s has no nodes"), *ProgramPath);
		return nullptr;
	}

	FString LinesText;
	if (!FFileHelper::LoadFileToString(LinesText, *LinesPath, FFileHelper::EHashOptions::None, FILEREAD_Silent))
	{
		OutError = FString::Printf(TEXT("can't read %s"), *LinesPath);
		return nullptr;
	}

	// The same CSV reader the bark loader uses - ysc's line table is an ordinary spreadsheet CSV.
	TArray<FDialogCsvRow> Rows;
	FString CsvError;
	int32 CsvErrorLine = 0;
	if (!SmoresDialog::ParseCsv(LinesText, Rows, CsvError, CsvErrorLine) || Rows.Num() == 0)
	{
		OutError = FString::Printf(TEXT("%s(%d): %s"), *LinesPath, CsvErrorLine, *CsvError);
		return nullptr;
	}

	const int32 IdColumn = Rows[0].Fields.IndexOfByKey(TEXT("id"));
	const int32 TextColumn = Rows[0].Fields.IndexOfByKey(TEXT("text"));
	if (IdColumn == INDEX_NONE || TextColumn == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("%s has no id and text columns"), *LinesPath);
		return nullptr;
	}

	for (int32 Row = 1; Row < Rows.Num(); ++Row)
	{
		const TArray<FString>& Fields = Rows[Row].Fields;
		if (Fields.IsValidIndex(IdColumn) && Fields.IsValidIndex(TextColumn) && !Fields[IdColumn].IsEmpty())
		{
			Script->LineText.Add(Fields[IdColumn], Fields[TextColumn]);
		}
	}

	return Script;
}

TUniquePtr<ISpikeConversation> SmoresDialogSpike::MakeYarnConversation(const TSharedRef<FSpikeYarnScript>& Script, const FString& StartNode, FSpikeGameHooks Hooks)
{
	return MakeUnique<FSpikeYarnConversation>(Script, StartNode, MoveTemp(Hooks));
}
