// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ConversationPlayer.h"
#include "ConversationScript.h"
#include "DialogFacts.h"
#include "SmoresDialog.h"
#include "Math/RandomStream.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "YarnSpinnerCore.h"
#include "YarnVariableStorage.h"
#include "YarnVirtualMachine.h"

namespace
{
	using FYarnBuiltInCall = TFunction<FYarnValue(const TArray<FYarnValue>&)>;

	struct FYarnBuiltIn
	{
		int32 ParameterCount = 0;

		/** Null for the built-ins that need the running conversation (visited, random) - the player answers those itself */
		FYarnBuiltInCall Call;
	};

	/**
	 *  The operators compiled Yarn calls as functions - `gold() >= 20` becomes a call to
	 *  "Number.GreaterThanOrEqualTo", and `not` a call to "Bool.Not" - plus the built-ins we answer.
	 *  The plugin registers its own inside its dialogue runner, not its player, so driving the
	 *  player alone means supplying them. Ours, of what scripts need.
	 *
	 *  **One deliberate difference from Yarn**: text compares case-insensitively, the way names do
	 *  in our condition language, so speaker_faction() == "raiders" matches Raiders.
	 */
	const TMap<FString, FYarnBuiltIn>& GetYarnBuiltIns()
	{
		static const TMap<FString, FYarnBuiltIn> BuiltIns = []()
		{
			TMap<FString, FYarnBuiltIn> Functions;

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
			auto AddUnary = [&Functions](const TCHAR* Name, TFunction<float(float)> Op)
			{
				Functions.Add(Name, { 1, [Op](const TArray<FYarnValue>& P) { return FYarnValue(Op(P[0].ConvertToNumber())); } });
			};

			AddNumber(TEXT("Number.Add"),      [](float A, float B) { return A + B; });
			AddNumber(TEXT("Number.Minus"),    [](float A, float B) { return A - B; });
			AddNumber(TEXT("Number.Multiply"), [](float A, float B) { return A * B; });
			AddNumber(TEXT("Number.Divide"),   [](float A, float B) { return B != 0.f ? A / B : 0.f; });
			AddNumber(TEXT("Number.Modulo"),   [](float A, float B) { return B != 0.f ? FMath::Fmod(A, B) : 0.f; });
			AddUnary(TEXT("Number.UnaryMinus"), [](float A) { return -A; });

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
			Functions.Add(TEXT("String.EqualTo"),    { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(P[0].ConvertToString().Equals(P[1].ConvertToString(), ESearchCase::IgnoreCase)); } });
			Functions.Add(TEXT("String.NotEqualTo"), { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(!P[0].ConvertToString().Equals(P[1].ConvertToString(), ESearchCase::IgnoreCase)); } });

			// a Yarn enum's cases are strings to the player, as they are to the plugin's own runner
			Functions.Add(TEXT("Enum.EqualTo"),    { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(P[0].ConvertToString().Equals(P[1].ConvertToString(), ESearchCase::CaseSensitive)); } });
			Functions.Add(TEXT("Enum.NotEqualTo"), { 2, [](const TArray<FYarnValue>& P) { return FYarnValue(!P[0].ConvertToString().Equals(P[1].ConvertToString(), ESearchCase::CaseSensitive)); } });

			AddUnary(TEXT("round"), [](float A) { return static_cast<float>(FMath::RoundToInt(A)); });
			AddUnary(TEXT("floor"), [](float A) { return FMath::FloorToFloat(A); });
			AddUnary(TEXT("ceil"),  [](float A) { return FMath::CeilToFloat(A); });
			AddUnary(TEXT("int"),   [](float A) { return static_cast<float>(FMath::TruncToInt(A)); });
			AddNumber(TEXT("min"),  [](float A, float B) { return FMath::Min(A, B); });
			AddNumber(TEXT("max"),  [](float A, float B) { return FMath::Max(A, B); });

			// answered by the running conversation, which holds the variable store and the stream
			Functions.Add(TEXT("visited"),       { 1, nullptr });
			Functions.Add(TEXT("visited_count"), { 1, nullptr });
			Functions.Add(TEXT("random"),        { 0, nullptr });
			Functions.Add(TEXT("random_range"),  { 2, nullptr });
			Functions.Add(TEXT("dice"),          { 1, nullptr });

			return Functions;
		}();

		return BuiltIns;
	}

	FYarnValue ToYarnValue(const FDialogValue& Value)
	{
		// Yarn has no "unset", so an unset fact answers its type's empty value: a missing listener's
		// faction is "", never "None" - so it can't accidentally equal a writer's "None"
		switch (Value.Type)
		{
		case EDialogValueType::Number:
			return FYarnValue(Value.bIsSet ? static_cast<float>(Value.Number) : 0.0f);

		case EDialogValueType::Bool:
			return FYarnValue(Value.bIsSet && Value.bBool);

		case EDialogValueType::Name:
		default:
			return FYarnValue(Value.bIsSet ? Value.Name.ToString() : FString());
		}
	}
}

/** The player's workings, kept out of the header so no Yarn type appears in one */
class FDialogConversationPlayerImpl
{
public:

	explicit FDialogConversationPlayerImpl(FConversationPlayerSetup InSetup)
		: Setup(MoveTemp(InSetup))
		, Stream(Setup.RandomSeed)
	{
	}

	~FDialogConversationPlayerImpl()
	{
		VM.LineHandler.Unbind();
		VM.OptionsHandler.Unbind();
		VM.CommandHandler.Unbind();
		VM.DialogueCompleteHandler.Unbind();
		VM.CallFunctionHandler.Unbind();
		VM.FunctionExistsHandler.Unbind();
		VM.FunctionParamCountHandler.Unbind();
		VM.FunctionErroredHandler.Unbind();
	}

	bool Start()
	{
		if (!Setup.Script.IsValid() || !Setup.Facts)
		{
			Fail(TEXT("the conversation has no script or no facts"));
			return false;
		}

		// Each conversation gets its own memory, so one squad's $asked_about_road never leaks into
		// another's. The plugin's in-memory store is a component, but works as a plain object.
		Variables.Reset(NewObject<UYarnInMemoryVariableStorage>(GetTransientPackage()));

		VM.SetProgram(Setup.Script->Program);
		VM.VariableStorage = TScriptInterface<IYarnVariableStorage>(Variables.Get());

		VM.LineHandler.BindRaw(this, &FDialogConversationPlayerImpl::HandleLine);
		VM.OptionsHandler.BindRaw(this, &FDialogConversationPlayerImpl::HandleOptions);
		VM.CommandHandler.BindRaw(this, &FDialogConversationPlayerImpl::HandleCommand);
		VM.DialogueCompleteHandler.BindRaw(this, &FDialogConversationPlayerImpl::HandleComplete);
		VM.CallFunctionHandler.BindRaw(this, &FDialogConversationPlayerImpl::CallFunction);
		VM.FunctionExistsHandler.BindRaw(this, &FDialogConversationPlayerImpl::FunctionExists);
		VM.FunctionParamCountHandler.BindRaw(this, &FDialogConversationPlayerImpl::FunctionParamCount);
		VM.FunctionErroredHandler.BindLambda([](FString&) { return false; });

		if (!VM.SetNode(Setup.StartNode))
		{
			Fail(FString::Printf(TEXT("there is no node '%s'"), *Setup.StartNode));
			return false;
		}

		Run();
		return State != EConversationPlayerState::Failed;
	}

	void Advance()
	{
		if (State == EConversationPlayerState::Line)
		{
			Run();
		}
	}

	bool Choose(int32 Index)
	{
		if (State != EConversationPlayerState::Choices || !CurrentChoices.IsValidIndex(Index) || !CurrentChoices[Index].bAvailable)
		{
			return false;
		}

		// the option set's positions match CurrentChoices', unavailable ones included
		VM.SetSelectedOption(Index);
		Run();
		return true;
	}

	void Stop()
	{
		VM.Stop();
		State = EConversationPlayerState::Ended;
		CurrentChoices.Reset();
	}

	FConversationPlayerSetup Setup;

	EConversationPlayerState State = EConversationPlayerState::Ended;
	FConversationPlayerLine CurrentLine;
	TArray<FConversationPlayerChoice> CurrentChoices;
	FString Error;

private:

	/**
	 *  Continue until the script shows a line, offers choices or ends.
	 *
	 *  The player pauses after a command as well as after a line, waiting to be told the command
	 *  has finished (so a <<wait 2>> could take two seconds). Every effect finishes at once, so a
	 *  pause after one is continued straight through.
	 */
	void Run()
	{
		State = EConversationPlayerState::Ended;
		bPausedOnCommand = false;
		bHaveContent = false;

		VM.Continue();

		while (bPausedOnCommand && !bHaveContent && !bEndRequested && State != EConversationPlayerState::Failed)
		{
			bPausedOnCommand = false;
			VM.Continue();
		}

		// a command asked to end it here - OpenTrade, whose screen replaces the window. Stopped from
		// out here rather than inside the handler, which the VM is still in the middle of calling.
		if (bEndRequested && State != EConversationPlayerState::Failed)
		{
			VM.Stop();
			State = EConversationPlayerState::Ended;
			CurrentChoices.Reset();
			return;
		}

		if (VM.GetExecutionState() == EYarnExecutionState::Error && State != EConversationPlayerState::Failed)
		{
			Fail(TEXT("the player stopped with an error (see LogYarnSpinner)"));
		}
	}

	void HandleLine(const FYarnLine& Line)
	{
		const FConversationLineInfo* Info = Setup.Script->FindLine(Line.LineID);

		if (!Info)
		{
			Fail(FString::Printf(TEXT("the script said %s, which its lines file doesn't have"), *Line.LineID));
			return;
		}

		CurrentLine.LineId = Info->QualifiedId;
		CurrentLine.SpeakerCue = Info->SpeakerCue;
		CurrentLine.SpeakerSlot = SmoresDialog::GetSpeakerSlot(Info->SpeakerCue, Setup.Participants);
		CurrentLine.Substitutions = Line.Substitutions;
		CurrentLine.SourceText = Line.Substitutions.Num() > 0 ? FYarnVirtualMachine::ExpandSubstitutions(Info->SourceText, Line.Substitutions) : Info->SourceText;

		CurrentChoices.Reset();
		State = EConversationPlayerState::Line;
		bHaveContent = true;
	}

	void HandleOptions(const FYarnOptionSet& Options)
	{
		CurrentChoices.Reset();

		bool bAnyShown = false;

		for (const FYarnOption& Option : Options.Options)
		{
			const FConversationLineInfo* Info = Setup.Script->FindLine(Option.Line.RawLine.LineID);
			FConversationPlayerChoice& Choice = CurrentChoices.AddDefaulted_GetRef();

			Choice.LineId = Info ? Info->QualifiedId : FName(*Option.Line.RawLine.LineID);
			Choice.Substitutions = Option.Line.RawLine.Substitutions;
			Choice.bAvailable = Option.bIsAvailable;
			Choice.SourceText = Info ? Info->SourceText : FString();

			// Yarn offers every choice, marked. Ours to decide: greyed with its reason, or left off.
			const bool bHasReason = Info && !Info->ReasonKey.IsNone();
			Choice.Reason = !Choice.bAvailable && bHasReason ? Info->Reason : ESmoresRefusalReason::None;
			Choice.bShown = Choice.bAvailable || bHasReason;

			bAnyShown |= Choice.bShown;
		}

		// nothing left to say that the player may see - end here rather than wait on a pick nobody can make
		if (!bAnyShown)
		{
			UE_LOG(LogSmoresDialog, Log, TEXT("Dialog: every choice in %s node %s was unavailable and untagged - the conversation ends there"),
				*Setup.Script->File, *VM.GetCurrentNodeName());
			bEndRequested = true;
			bHaveContent = true;
			return;
		}

		State = EConversationPlayerState::Choices;
		bHaveContent = true;
	}

	void HandleCommand(const FYarnCommand& Command)
	{
		bPausedOnCommand = true;

		if (Setup.RunCommand && !Setup.RunCommand(FName(*Command.CommandName), Command.Parameters))
		{
			bEndRequested = true;
		}
	}

	void HandleComplete()
	{
		State = EConversationPlayerState::Ended;
		bHaveContent = true;
	}

	bool FunctionExists(const FString& Name)
	{
		int32 Parameters = 0;
		return SmoresDialog::FindYarnBuiltIn(Name, Parameters) || SmoresDialog::FindFactForYarnFunction(*Setup.Facts, Name);
	}

	int32 FunctionParamCount(const FString& Name)
	{
		int32 Parameters = 0;

		if (SmoresDialog::FindYarnBuiltIn(Name, Parameters))
		{
			return Parameters;
		}

		const FDialogFact* Fact = SmoresDialog::FindFactForYarnFunction(*Setup.Facts, Name);
		return Fact ? (Fact->bTakesArgument ? 1 : 0) : -1;
	}

	FYarnValue CallFunction(const FString& Name, const TArray<FYarnValue>& Parameters)
	{
		if (const FYarnBuiltIn* BuiltIn = GetYarnBuiltIns().Find(Name))
		{
			if (Parameters.Num() < BuiltIn->ParameterCount)
			{
				Fail(FString::Printf(TEXT("%s() was called with too few values"), *Name));
				return FYarnValue();
			}

			return BuiltIn->Call ? BuiltIn->Call(Parameters) : CallRunningBuiltIn(Name, Parameters);
		}

		if (const FDialogFact* Fact = SmoresDialog::FindFactForYarnFunction(*Setup.Facts, Name))
		{
			const FName Argument = Fact->bTakesArgument && Parameters.Num() > 0 ? FName(*Parameters[0].ConvertToString()) : NAME_None;
			return ToYarnValue(Setup.Facts->Ask(Fact->Name, Setup.Context, Argument));
		}

		Fail(FString::Printf(TEXT("the script called %s(), which the game doesn't answer"), *Name));
		return FYarnValue();
	}

	/** The built-ins that need this conversation: its variable store, or its random stream */
	FYarnValue CallRunningBuiltIn(const FString& Name, const TArray<FYarnValue>& Parameters)
	{
		if (Name == TEXT("visited") || Name == TEXT("visited_count"))
		{
			// the VM counts a node's visits into this variable itself, for any node a script asks
			// visited() about - the compiler marks those nodes with a tracking header
			const FString VariableName = FString::Printf(TEXT("$Yarn.Internal.Visiting.%s"), *Parameters[0].ConvertToString());
			FYarnValue Count(0.0f);

			if (Variables.IsValid())
			{
				IYarnVariableStorage::Execute_TryGetValue(Variables.Get(), VariableName, Count);
			}

			return Name == TEXT("visited") ? FYarnValue(Count.ConvertToNumber() > 0.0f) : FYarnValue(Count.ConvertToNumber());
		}

		if (Name == TEXT("random"))
		{
			return FYarnValue(Stream.GetFraction());
		}

		if (Name == TEXT("random_range"))
		{
			const int32 Low = FMath::RoundToInt(Parameters[0].ConvertToNumber());
			const int32 High = FMath::RoundToInt(Parameters[1].ConvertToNumber());
			return FYarnValue(static_cast<float>(Stream.RandRange(FMath::Min(Low, High), FMath::Max(Low, High))));
		}

		if (Name == TEXT("dice"))
		{
			const int32 Sides = FMath::Max(1, FMath::RoundToInt(Parameters[0].ConvertToNumber()));
			return FYarnValue(static_cast<float>(Stream.RandRange(1, Sides)));
		}

		Fail(FString::Printf(TEXT("%s() has no answer"), *Name));
		return FYarnValue();
	}

	void Fail(const FString& Message)
	{
		Error = Message;
		State = EConversationPlayerState::Failed;
		UE_LOG(LogSmoresDialog, Warning, TEXT("Dialog: conversation %s (%s) failed: %s"),
			*Setup.StartNode, Setup.Script.IsValid() ? *Setup.Script->File : TEXT("no script"), *Message);
	}

	FRandomStream Stream;

	FYarnVirtualMachine VM;
	TStrongObjectPtr<UYarnInMemoryVariableStorage> Variables;

	bool bPausedOnCommand = false;
	bool bHaveContent = false;
	bool bEndRequested = false;
};

FDialogConversationPlayer::FDialogConversationPlayer(FConversationPlayerSetup Setup)
	: Impl(MakeUnique<FDialogConversationPlayerImpl>(MoveTemp(Setup)))
{
}

FDialogConversationPlayer::~FDialogConversationPlayer() = default;

bool FDialogConversationPlayer::Start()
{
	return Impl->Start();
}

EConversationPlayerState FDialogConversationPlayer::GetState() const
{
	return Impl->State;
}

const FConversationPlayerLine& FDialogConversationPlayer::GetLine() const
{
	return Impl->CurrentLine;
}

const TArray<FConversationPlayerChoice>& FDialogConversationPlayer::GetChoices() const
{
	return Impl->CurrentChoices;
}

void FDialogConversationPlayer::Advance()
{
	Impl->Advance();
}

bool FDialogConversationPlayer::Choose(int32 Index)
{
	return Impl->Choose(Index);
}

void FDialogConversationPlayer::Stop()
{
	Impl->Stop();
}

const FString& FDialogConversationPlayer::GetError() const
{
	return Impl->Error;
}

namespace SmoresDialog
{
	bool FindYarnBuiltIn(const FString& Name, int32& OutParameters)
	{
		// exact, as Yarn spells them: the compiler writes the operator names itself
		if (const FYarnBuiltIn* BuiltIn = GetYarnBuiltIns().Find(Name))
		{
			OutParameters = BuiltIn->ParameterCount;
			return true;
		}

		return false;
	}

	FString GetYarnFunctionName(FName FactName)
	{
		return FactName.ToString().Replace(TEXT("."), TEXT("_"));
	}

	const FDialogFact* FindFactForYarnFunction(const FDialogFactRegistry& Facts, const FString& FunctionName)
	{
		const FName Wanted(*FunctionName);

		// FName compares case-insensitively, so gold() is Gold and speaker_faction() is Speaker.Faction
		for (const FDialogFact& Fact : Facts.GetFacts())
		{
			if (FName(*GetYarnFunctionName(Fact.Name)) == Wanted)
			{
				return &Fact;
			}
		}

		return nullptr;
	}

	void SplitSpeaker(const FString& Raw, FString& OutSpeaker, FString& OutText)
	{
		const FString Line = Raw.TrimStartAndEnd();

		// "Name: words", where the name is short and has no sentence punctuation - so a line that
		// merely contains a colon ("Listen: it's a trap") isn't taken for a speaker called "Listen"
		const int32 Colon = Line.Find(TEXT(": "));

		if (Colon > 0 && Colon <= 32)
		{
			const FString Name = Line.Left(Colon);

			if (!Name.Contains(TEXT(".")) && !Name.Contains(TEXT(",")) && !Name.Contains(TEXT("!")) && !Name.Contains(TEXT("?")))
			{
				OutSpeaker = Name;
				OutText = Line.Mid(Colon + 2).TrimStartAndEnd();
				return;
			}
		}

		OutSpeaker.Reset();
		OutText = Line;
	}

	uint8 GetSpeakerSlot(const FString& Cue, const TArray<FName>& Participants)
	{
		if (Cue.IsEmpty())
		{
			return NarrationSlot;
		}

		if (Participants.Num() == 0)
		{
			return Cue.Equals(GetSquadMemberCue(), ESearchCase::IgnoreCase) ? SquadMemberSlot : NpcSlot;
		}

		const int32 Index = Participants.IndexOfByKey(FName(*Cue));
		return Index == INDEX_NONE ? NarrationSlot : static_cast<uint8>(Index);
	}

	const TCHAR* GetSquadMemberCue()
	{
		return TEXT("You");
	}
}
