// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "SpikeConversation.h"
#include "SmoresDialogSpike.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// The converter's header pulls in its whole JSON library when INK_EXPOSE_JSON is set, which only
// inkcpp's own converter files need. This file only calls the stream overload.
#undef INK_EXPOSE_JSON

THIRD_PARTY_INCLUDES_START
#include <story.h>
#include <runner.h>
#include <globals.h>
#include <choice.h>
#include <compiler.h>
#include <exception>
#include <sstream>
#include <string>
THIRD_PARTY_INCLUDES_END

/**
 *  The Ink half of the spike: inkcpp (github.com/JBenda/inkcpp, MIT), built into this module.
 *
 *  Loading is two steps, both at runtime and both in memory: the JSON a writer exports from Inky
 *  is converted to inkcpp's own compact form by inkcpp's converter, then handed to its player. A
 *  modder therefore needs only Inky.
 */
struct FSpikeInkScript
{
	ink::runtime::story* Story = nullptr;

	~FSpikeInkScript()
	{
		delete Story;
	}
};

namespace
{
	FString InkToString(const char* Text)
	{
		return Text ? FString(UTF8_TO_TCHAR(Text)) : FString();
	}

	FString InkToString(const std::string& Text)
	{
		return FString(UTF8_TO_TCHAR(Text.c_str()));
	}

	/** Ink has no line ids, so the scene carries one per line as an "#id:..." tag */
	template <typename TTagSource>
	FString FindInkIdTag(const TTagSource& Source)
	{
		for (size_t Index = 0; Index < Source.num_tags(); ++Index)
		{
			const FString Tag = InkToString(Source.get_tag(Index)).TrimStartAndEnd();
			if (Tag.StartsWith(TEXT("id:")))
			{
				return Tag.Mid(3).TrimStartAndEnd();
			}
		}
		return FString();
	}

	class FSpikeInkConversation final : public ISpikeConversation
	{
	public:

		FSpikeInkConversation(const TSharedRef<FSpikeInkScript>& InScript, FSpikeGameHooks InHooks)
			: Script(InScript)
			, Hooks(MoveTemp(InHooks))
		{
		}

		virtual ~FSpikeInkConversation() override
		{
			// The runner and its globals point into the story, so they go first.
			Runner = nullptr;
			Globals = nullptr;
		}

		virtual FString GetLanguage() const override { return TEXT("Ink"); }

		virtual bool Start() override
		{
			try
			{
				// Each conversation gets its own globals, so one squad's asked_about_road never
				// leaks into another's. (inkcpp can share them too; Slice 3 would choose per variable.)
				Globals = Script->Story->new_globals();
				Runner = Script->Story->new_runner(Globals);

				Runner->bind("gold", [this]() -> int
				{
					return Hooks.GetGold ? Hooks.GetGold() : 0;
				});

				Runner->bind("TakeMoney", [this](int Amount)
				{
					Command(TEXT("TakeMoney"), { FString::FromInt(Amount) });
				});

				Runner->bind("ChangeStanding", [this](const char* Faction, int Amount)
				{
					Command(TEXT("ChangeStanding"), { InkToString(Faction), FString::FromInt(Amount) });
				});
			}
			catch (const std::exception& Exception)
			{
				Fail(FString::Printf(TEXT("couldn't start: %s"), *InkToString(Exception.what())));
				return false;
			}

			Step();
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
				Step();
			}
		}

		virtual bool Choose(int32 Index) override
		{
			if (State != ESpikeState::Choices || !CurrentChoices.IsValidIndex(Index))
			{
				return false;
			}

			try
			{
				Runner->choose(static_cast<size_t>(Index));
			}
			catch (const std::exception& Exception)
			{
				Fail(FString::Printf(TEXT("choose(%d) failed: %s"), Index, *InkToString(Exception.what())));
				return false;
			}

			Step();
			return true;
		}

	private:

		/** To the next line with words in it, or the choices, or the end */
		void Step()
		{
			try
			{
				while (Runner->can_continue())
				{
					const std::string Raw = Runner->getline();

					FSpikeLine Line;
					Line.Id = FindInkIdTag(*Runner);
					SmoresDialogSpike::SplitSpeaker(InkToString(Raw), Line.Speaker, Line.Text);

					// Ink emits a bare newline around some choices and gathers; those aren't lines.
					if (Line.Text.IsEmpty())
					{
						continue;
					}

					CurrentLine = MoveTemp(Line);
					CurrentChoices.Reset();
					State = ESpikeState::Line;
					return;
				}

				CurrentChoices.Reset();
				for (const ink::runtime::choice* Choice = Runner->begin(); Choice != Runner->end(); ++Choice)
				{
					FSpikeChoice& Entry = CurrentChoices.AddDefaulted_GetRef();
					Entry.Id = FindInkIdTag(*Choice);
					Entry.Text = InkToString(Choice->text()).TrimStartAndEnd();
				}

				State = CurrentChoices.Num() > 0 ? ESpikeState::Choices : ESpikeState::Ended;
			}
			catch (const std::exception& Exception)
			{
				Fail(InkToString(Exception.what()));
			}
		}

		void Command(const FString& Name, const TArray<FString>& Arguments)
		{
			if (Hooks.RunCommand)
			{
				Hooks.RunCommand(Name, Arguments);
			}
		}

		void Fail(const FString& Message)
		{
			Error = Message;
			State = ESpikeState::Failed;
			UE_LOG(LogSmoresDialogSpike, Warning, TEXT("[Ink] %s"), *Message);
		}

		/** Keeps the story alive for as long as this conversation plays it */
		TSharedRef<FSpikeInkScript> Script;

		FSpikeGameHooks Hooks;
		ink::runtime::globals Globals;
		ink::runtime::runner Runner;

		ESpikeState State = ESpikeState::Ended;
		FSpikeLine CurrentLine;
		TArray<FSpikeChoice> CurrentChoices;
		FString Error;
	};
}

TSharedPtr<FSpikeInkScript> SmoresDialogSpike::LoadInkScript(const FString& Directory, const FString& Name, FString& OutError)
{
	const FString Path = FPaths::Combine(Directory, Name + TEXT(".ink.json"));

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
	{
		OutError = FString::Printf(TEXT("can't read %s"), *Path);
		return nullptr;
	}

	std::string Converted;

	try
	{
		std::istringstream In(std::string(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.Num()));
		std::ostringstream Out(std::ios::out | std::ios::binary);

		ink::compiler::compilation_results Results;
		ink::compiler::run(In, Out, &Results);

		if (!Results.errors.empty())
		{
			OutError = FString::Printf(TEXT("%s: %s"), *Path, *InkToString(Results.errors[0]));
			return nullptr;
		}

		for (const std::string& Warning : Results.warnings)
		{
			UE_LOG(LogSmoresDialogSpike, Warning, TEXT("[Ink] %s: %s"), *Path, *InkToString(Warning));
		}

		Converted = Out.str();
	}
	catch (const std::exception& Exception)
	{
		OutError = FString::Printf(TEXT("%s isn't an Ink story: %s"), *Path, *InkToString(Exception.what()));
		return nullptr;
	}

	if (Converted.empty())
	{
		OutError = FString::Printf(TEXT("%s converted to nothing"), *Path);
		return nullptr;
	}

	// from_binary takes ownership and frees this with delete[] when the story goes.
	unsigned char* Data = new unsigned char[Converted.size()];
	FMemory::Memcpy(Data, Converted.data(), Converted.size());

	TSharedRef<FSpikeInkScript> Script = MakeShared<FSpikeInkScript>();

	try
	{
		Script->Story = ink::runtime::story::from_binary(Data, Converted.size(), /*freeOnDestroy*/ true);
	}
	catch (const std::exception& Exception)
	{
		OutError = FString::Printf(TEXT("%s: the player rejected it: %s"), *Path, *InkToString(Exception.what()));
		return nullptr;
	}

	if (!Script->Story)
	{
		OutError = FString::Printf(TEXT("%s: the player rejected it"), *Path);
		return nullptr;
	}

	return Script;
}

TUniquePtr<ISpikeConversation> SmoresDialogSpike::MakeInkConversation(const TSharedRef<FSpikeInkScript>& Script, FSpikeGameHooks Hooks)
{
	return MakeUnique<FSpikeInkConversation>(Script, MoveTemp(Hooks));
}
