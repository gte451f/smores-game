// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpikeConversation.h"
#include "SpikeConversationSubsystem.generated.h"

/**
 *  Holds the one conversation the PIE commands are playing - THROWAWAY, part of the spike.
 *
 *  SmoresSpikeTalk [gold]   starts the bandit shakedown (gold defaults to 50)
 *  SmoresSpikeChoose <n>    picks choice n (counting from 1)
 *
 *  Lines, choices and the script's commands go to the COMMS feed. Deliberately single-player and
 *  local, unlike Slice 3's real conversations (server-run, one per squad): this only exists so
 *  Jim could play the scene in PIE while choosing a language.
 */
UCLASS()
class USpikeConversationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	void StartTalk(int32 StartingGold);
	void Choose(int32 OneBasedChoice);

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	/** Posts every line up to the next choice or the end, then the choices themselves */
	void ShowUntilChoice();

	void Post(const FString& Text, const FString& Source = FString()) const;

	/** The script stays loaded between talks, so a second talk plays the same loaded copy */
	TSharedPtr<FSpikeYarnScript> YarnScript;

	TUniquePtr<ISpikeConversation> Conversation;

	/** A pretend wallet: gold() reads it and TakeMoney spends it, so paying twice shows the difference */
	int32 Gold = 50;
};
