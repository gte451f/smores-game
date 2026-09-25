// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "SpikeConversationSubsystem.h"
#include "SmoresDialogSpike.h"
#include "SmoresActivityLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

bool USpikeConversationSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USpikeConversationSubsystem::StartTalk(int32 StartingGold)
{
	Gold = StartingGold;
	Conversation.Reset();

	FSpikeGameHooks Hooks;
	Hooks.GetGold = [this]() { return Gold; };
	Hooks.RunCommand = [this](const FString& Name, const TArray<FString>& Arguments)
	{
		if (Name == TEXT("TakeMoney") && Arguments.Num() > 0)
		{
			Gold -= FCString::Atoi(*Arguments[0]);
		}
		Post(FString::Printf(TEXT("[%s %s]  (gold now %d)"), *Name, *FString::Join(Arguments, TEXT(" ")), Gold), TEXT("game"));
	};

	const FString Directory = SmoresDialogSpike::GetSceneDirectory();
	FString Error;

	if (!YarnScript)
	{
		YarnScript = SmoresDialogSpike::LoadYarnScript(Directory, SmoresDialogSpike::GetSceneName(), Error);
	}
	if (YarnScript)
	{
		Conversation = SmoresDialogSpike::MakeYarnConversation(YarnScript.ToSharedRef(), TEXT("Shakedown"), MoveTemp(Hooks));
	}

	if (!Conversation)
	{
		Post(FString::Printf(TEXT("Couldn't start: %s"), *Error), TEXT("spike"));
		return;
	}

	Post(FString::Printf(TEXT("--- %s, with %d gold ---"), *Conversation->GetLanguage(), Gold), TEXT("spike"));

	if (!Conversation->Start())
	{
		Post(FString::Printf(TEXT("Failed: %s"), *Conversation->GetError()), TEXT("spike"));
		return;
	}

	ShowUntilChoice();
}

void USpikeConversationSubsystem::Choose(int32 OneBasedChoice)
{
	if (!Conversation || Conversation->GetState() != ESpikeState::Choices)
	{
		Post(TEXT("Nothing to choose - start with SmoresSpikeTalk"), TEXT("spike"));
		return;
	}

	const int32 Index = OneBasedChoice - 1;
	if (!Conversation->GetChoices().IsValidIndex(Index))
	{
		Post(FString::Printf(TEXT("There's no choice %d"), OneBasedChoice), TEXT("spike"));
		return;
	}

	const FSpikeChoice Picked = Conversation->GetChoices()[Index];
	if (!Picked.bAvailable)
	{
		Post(FString::Printf(TEXT("\"%s\" isn't available"), *Picked.Text), TEXT("spike"));
		return;
	}

	// Echo the pick before choosing, so a command the choice runs posts after it, not before.
	Post(FString::Printf(TEXT("> %s"), *Picked.Text), TEXT("you"));
	Conversation->Choose(Index);
	ShowUntilChoice();
}

void USpikeConversationSubsystem::ShowUntilChoice()
{
	while (Conversation->GetState() == ESpikeState::Line)
	{
		const FSpikeLine& Line = Conversation->GetLine();
		Post(FString::Printf(TEXT("\"%s\"   [%s]"), *Line.Text, *Line.Id), Line.Speaker);
		Conversation->Advance();
	}

	switch (Conversation->GetState())
	{
	case ESpikeState::Choices:
	{
		const TArray<FSpikeChoice>& Choices = Conversation->GetChoices();
		for (int32 Index = 0; Index < Choices.Num(); ++Index)
		{
			Post(FString::Printf(TEXT("%d. %s%s   [%s]"), Index + 1, *Choices[Index].Text,
				Choices[Index].bAvailable ? TEXT("") : TEXT("  (unavailable)"), *Choices[Index].Id), TEXT("choose"));
		}
		break;
	}
	case ESpikeState::Ended:
		Post(TEXT("--- end ---"), TEXT("spike"));
		break;
	default:
		Post(FString::Printf(TEXT("Failed: %s"), *Conversation->GetError()), TEXT("spike"));
		break;
	}
}

void USpikeConversationSubsystem::Post(const FString& Text, const FString& Source) const
{
	UE_LOG(LogSmoresDialogSpike, Display, TEXT("%s: %s"), Source.IsEmpty() ? TEXT("-") : *Source, *Text);

	if (USmoresActivityLog* Feed = USmoresActivityLog::Get(GetWorld()->GetFirstPlayerController()))
	{
		Feed->Post(EActivityCategory::Comms, EActivitySeverity::Normal, FText::FromString(Text), FText::FromString(Source));
	}
}

namespace
{
	/** The game world the command is meant for - the PIE world when typed into the editor's own console */
	USpikeConversationSubsystem* FindSpikeSubsystem(UWorld* World)
	{
		if (!World || !World->IsGameWorld())
		{
			World = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.World() && Context.World()->IsGameWorld())
				{
					World = Context.World();
					break;
				}
			}
		}

		USpikeConversationSubsystem* Subsystem = World ? World->GetSubsystem<USpikeConversationSubsystem>() : nullptr;
		if (!Subsystem)
		{
			UE_LOG(LogSmoresDialogSpike, Warning, TEXT("The spike commands need a running game - start PIE first"));
		}
		return Subsystem;
	}

	FAutoConsoleCommandWithWorldAndArgs GSmoresSpikeTalk(
		TEXT("SmoresSpikeTalk"),
		TEXT("Dialog spike: plays the bandit shakedown in Yarn. SmoresSpikeTalk [gold, default 50]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (USpikeConversationSubsystem* Subsystem = FindSpikeSubsystem(World))
			{
				Subsystem->StartTalk(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 50);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GSmoresSpikeChoose(
		TEXT("SmoresSpikeChoose"),
		TEXT("Dialog spike: picks a choice in the running shakedown. SmoresSpikeChoose <n>, counting from 1"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (USpikeConversationSubsystem* Subsystem = FindSpikeSubsystem(World))
			{
				Subsystem->Choose(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0);
			}
		}));
}
