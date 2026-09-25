// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "DialogConversationTypes.h"
#include "ConversationPlayer.h"
#include "BanterDirectorComponent.generated.h"

class AStrategyUnit;
class APlayerState;

/**
 *  Squad banter: Ambient conversations, played among one player's own squad members with no
 *  window - each line goes to the feed and floats over whoever says it, exactly as a bark does.
 *
 *  **Server-only, on the GameState**, like the bark director it delivers through. It tries a banter
 *  for a squad after a fight ends, and now and then while the squad is idle, never more often than
 *  CooldownSeconds - rare enough to feel earned. The participants are cast from that player's squad
 *  members on their feet and standing near each other; the conversation's requires: is asked with
 *  the first participant as the Speaker and the second as the Listener.
 *
 *  What it remembers (who bantered when, who was fighting) is transient and never saved. Which
 *  conversations a squad has seen is its dialog memory's (UDialogMemoryComponent), like any other.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESDIALOG_API UBanterDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UBanterDirectorComponent();
	virtual ~UBanterDirectorComponent() override;

	/** The director on this world's GameState, or null */
	static UBanterDirectorComponent* Get(const UObject* WorldContextObject);

	/**
	 *  Tries a banter for Player's squad right now, as the timer would, but ignoring the cooldown.
	 *  **Authority only.** The id of the conversation that started, or None. OutExplanation gets
	 *  every candidate and why it did or didn't play.
	 */
	FName TryBanter(APlayerState* Player, TArray<FString>* OutExplanation = nullptr);

	/**
	 *  Plays one Ambient conversation for Player's squad now, whatever its requires: say and however
	 *  far apart the squad stands - the debug exec's way in. **Authority only.** False when the squad
	 *  has too few members on their feet to cast it, or it isn't Ambient.
	 */
	bool PlayAmbient(const FConversationDefinition& Conversation, APlayerState* Player, TArray<FString>* OutExplanation = nullptr);

	/** How often squads are looked at, in world seconds. Read at BeginPlay; 0 turns banter off. */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "s"))
	float CheckSeconds = 2.0f;

	/** The least time between one squad's banters, in world seconds */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "s"))
	float CooldownSeconds = 180.0f;

	/** While nothing is happening, a squad banters about this often, in world seconds */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "s"))
	float IdleSeconds = 300.0f;

	/** How long after the last blow of a fight the squad banters about it, in world seconds */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "s"))
	float AfterFightSeconds = 6.0f;

	/** How close the other participants must stand to the first, in cm */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "cm"))
	float CastRange = 1000.0f;

	/** The pause after a line has had its reading time, in real seconds */
	UPROPERTY(EditAnywhere, Category = "Banter", meta = (ClampMin = 0, Units = "s"))
	float LineGapSeconds = 0.6f;

	/**
	 *  How long a line is left to be read before the next, in real seconds: the bark bubble's own
	 *  formula (2 s, plus 0.06 s a character, at most 6 s), so the next line comes as the last fades.
	 */
	static float GetReadingSeconds(int32 Characters);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

private:

	/** The timer: is any squad due a banter? */
	void CheckSquads();

	/** One banter being played */
	struct FRunningBanter
	{
		TUniquePtr<FDialogConversationPlayer> Player;

		TWeakObjectPtr<APlayerState> PlayerState;

		/** In participants: order */
		TArray<TWeakObjectPtr<AStrategyUnit>> Cast;

		FName ConversationId;

		FTimerHandle LineTimer;
	};

	/** What the director knows about one squad between checks */
	struct FSquadBanter
	{
		double LastBanterTime = -1.0e9;

		double NextIdleTime = 0.0;

		bool bWasFighting = false;

		double CalmSince = 0.0;

		/** Each member's health at the last check - a drop means a fight */
		TMap<TWeakObjectPtr<AStrategyUnit>, float> LastHealth;
	};

	/** Player's squad members on their feet who aren't in a window conversation, in a stable order */
	TArray<AStrategyUnit*> GetAvailableMembers(const APlayerState* Player) const;

	/** Participants for Conversation from Members, or empty. bStrict asks its requires: and CastRange. */
	TArray<AStrategyUnit*> FindCast(const FConversationDefinition& Conversation, APlayerState* Player, const TArray<AStrategyUnit*>& Members, bool bStrict) const;

	/** Starts Conversation with Cast. False if its script wouldn't start. */
	bool StartBanter(const FConversationDefinition& Conversation, APlayerState* Player, const TArray<AStrategyUnit*>& Cast);

	/** Delivers the running banter's current line and schedules the next - or finishes it */
	void StepBanter(FRunningBanter& Banter);

	/** The next line's timer */
	void AdvanceBanter(TWeakObjectPtr<APlayerState> PlayerState);

	/** Carries out a command a banter's script ran */
	bool RunBanterEffect(TWeakObjectPtr<APlayerState> PlayerState, FName Command, const TArray<FString>& Arguments);

	/** Ends Player's running banter, if any */
	void StopBanter(const APlayerState* Player);

	FRunningBanter* FindRunning(const APlayerState* Player);

	void HandleLibraryWillReload();

	TArray<TUniquePtr<FRunningBanter>> Running;

	TMap<TWeakObjectPtr<APlayerState>, FSquadBanter> Squads;

	FTimerHandle CheckTimerHandle;

	FDelegateHandle WillReloadHandle;

	/** Which squad members are cast. Seeded from the world seed. */
	mutable FRandomStream Stream;
};
