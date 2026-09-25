// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/RandomStream.h"
#include "DialogTypes.h"
#include "BarkSelection.h"
#include "ApproachTracker.h"
#include "TimerManager.h"
#include "BarkDirectorComponent.generated.h"

class AStrategyUnit;
class APlayerState;
class ULevel;
class UWorld;
class UBarkDirectorComponent;
struct FBarkLine;

/**
 *  Turns one unit's health events into bark events for the director.
 *
 *  The USquadActivityWatcher shape, for the same reason: UHealthComponent's delegates are dynamic
 *  and three of them carry no parameters, so a handler can't tell *who* went down. One watcher per
 *  unit supplies the missing parameter by knowing its unit.
 */
UCLASS()
class SMORESDIALOG_API UBarkUnitWatcher : public UObject
{
	GENERATED_BODY()

public:

	void Watch(AStrategyUnit* InUnit, UBarkDirectorComponent* InDirector);

	void Unwatch();

	AStrategyUnit* GetUnit() const { return Unit.Get(); }

protected:

	TWeakObjectPtr<AStrategyUnit> Unit;

	TWeakObjectPtr<UBarkDirectorComponent> Director;

	UFUNCTION()
	void HandleDamaged(AActor* DamageInstigator);

	UFUNCTION()
	void HandleDowned();

	UFUNCTION()
	void HandleDied();
};

/**
 *  Decides who says what, when something bark-worthy happens, and tells the players near enough to
 *  hear it.
 *
 *  **Server-only, on the GameState.** Selection reads facts only the server holds (standing, the
 *  character records behind every unit), and there is one director per session rather than one
 *  per player - every player within hearing reads the *same* line, because one person said it. It
 *  is not replicated: what reaches a client is a line id, through IDialogHost::DeliverBark.
 *
 *  **Its memory is transient and never saved** - which line was said when, and by whom, in
 *  FBarkMemory. A reload of the dialog files clears it.
 *
 *  Events arrive three ways: health events from every unit's UHealthComponent, through one
 *  UBarkUnitWatcher per unit (found at BeginPlay, and as units spawn); interaction events
 *  (TradeOpened, NothingToSay) raised by the player controller's server RPC; and Approached, from
 *  this component's own slow timer comparing where every NPC and every squad member stands.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESDIALOG_API UBarkDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UBarkDirectorComponent();

	/** The director on this world's GameState, or null (the main menu, a bare test world) */
	static UBarkDirectorComponent* Get(const UObject* WorldContextObject);

	/**
	 *  Something bark-worthy happened. Picks the line Speaker says, if any, delivers it, and returns
	 *  its id (None when nothing was said). **Authority only.**
	 *
	 *  Listener and Player are whoever the event carries - see SmoresDialog::GetEventSubjects. For
	 *  WitnessedDeath use RaiseWitnessedDeath, which finds the speaker itself.
	 *
	 *  OutExplanation, when given, receives the full account of the selection: every line considered
	 *  and why it did or didn't win. bIgnoreQuietTime skips SpeakerQuietSeconds, for SmoresTestBark -
	 *  a writer trying lines back to back wants to hear them.
	 */
	FName RaiseEvent(EBarkEvent Event, AActor* Speaker, AActor* Listener, APlayerState* Player, TArray<FString>* OutExplanation = nullptr, bool bIgnoreQuietTime = false);

	/**
	 *  Victim just died. Its allies within WitnessRange who are still on their feet are the
	 *  candidates to react, nearest first; the first with a line to say says it. Allies are the same
	 *  player's squad, or the same (non-empty) faction - two unaffiliated strangers are not allies.
	 *  **Authority only.** Same return and explanation as RaiseEvent.
	 */
	FName RaiseWitnessedDeath(AActor* Victim, TArray<FString>* OutExplanation = nullptr, bool bIgnoreQuietTime = false);

	/** How far a bark carries, in cm. A player hears it if any of their squad is this close to the speaker. An open question - see dialog.md. */
	UPROPERTY(EditAnywhere, Category = "Barks", meta = (ClampMin = 0, Units = "cm"))
	float HearingRange = 2000.0f;

	/** How close an ally has to be to react to a death, in cm */
	UPROPERTY(EditAnywhere, Category = "Barks", meta = (ClampMin = 0, Units = "cm"))
	float WitnessRange = 1500.0f;

	/**
	 *  After saying anything, a speaker stays quiet this long, in world seconds, whatever the line's
	 *  own cooldown. Without it a unit in a fight barks on every hit it takes until each Hurt line is
	 *  on cooldown - one tuning knob for "do barks fire too often", rather than retuning every row.
	 */
	UPROPERTY(EditAnywhere, Category = "Barks", meta = (ClampMin = 0, Units = "s"))
	float SpeakerQuietSeconds = 6.0f;

	/**
	 *  How close one of a player's squad has to come to an NPC for the NPC to notice, in cm - the
	 *  Approached bark. Straight-line distance, so it notices through walls, as HearingRange does.
	 */
	UPROPERTY(EditAnywhere, Category = "Barks|Approach", meta = (ClampMin = 0, Units = "cm"))
	float ApproachRange = 800.0f;

	/**
	 *  After an NPC is approached by a player's squad, how long before that NPC can be approached by
	 *  that player again, in world seconds. The squad must also have left and come back - see
	 *  FApproachTracker. Per NPC and per player: one squad's approach never uses up another's.
	 */
	UPROPERTY(EditAnywhere, Category = "Barks|Approach", meta = (ClampMin = 0, Units = "s"))
	float ApproachCooldownSeconds = 60.0f;

	/**
	 *  How often approaches are checked, in world seconds. Slow on purpose: nobody notices a greeting
	 *  half a second late, and the check compares every NPC with every squad member. Read at
	 *  BeginPlay; 0 turns Approached off.
	 */
	UPROPERTY(EditAnywhere, Category = "Barks|Approach", meta = (ClampMin = 0, Units = "s"))
	float ApproachCheckSeconds = 0.5f;

	/**
	 *  Sends a said line to every player with a squad member within HearingRange of Speaker - to
	 *  their feed, and floating over Speaker. **Authority only.** Barks and banter both go out this
	 *  way: a banter line is heard exactly as a bark is, by whoever is near.
	 */
	void Deliver(FName LineId, AActor* Speaker) const;

	/** Called by the unit watchers */
	void HandleUnitHurt(AStrategyUnit* Unit, AActor* DamageInstigator);
	void HandleUnitDowned(AStrategyUnit* Unit);
	void HandleUnitDied(AStrategyUnit* Unit);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

	/** Starts watching a unit, if it isn't watched already */
	void WatchUnit(AStrategyUnit* Unit);

	/** OnActorSpawned - picks up units spawned after BeginPlay */
	void HandleActorSpawned(AActor* Actor);

	/**
	 *  LevelAddedToWorld - picks up units that stream in after BeginPlay. A World Partition cell
	 *  loading is a level being added, not a spawn, so OnActorSpawned never hears about its actors.
	 */
	void HandleLevelAdded(ULevel* Level, UWorld* World);

	/**
	 *  The approach timer: where does every NPC and every squad member stand, and has any squad just
	 *  come within ApproachRange of anyone? Raises Approached for each new arrival FApproachTracker
	 *  reports. Authority only.
	 */
	void CheckApproaches();

	/** The player whose squad Unit belongs to, or null for anyone else's */
	static APlayerState* GetOwningPlayerState(const AActor* Unit);

	/** True if Candidate would react to Victim's death as one of its own */
	static bool AreAllies(const AStrategyUnit* Victim, const AStrategyUnit* Candidate);

	/** The shared body of both Raise calls: the quiet-time check, selection, recording and delivery, for one prepared context */
	FName SayIfAny(const FDialogContext& Context, AActor* Speaker, bool bIgnoreQuietTime, TArray<FString>* OutExplanation);

	/** The dialog library changed - forget which lines were said, since they may not exist any more */
	void HandleLibraryLoaded();

	/** One per watched unit. Held here so the collector keeps them; pruned as units go away. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBarkUnitWatcher>> Watchers;

	FBarkMemory Memory;

	/** Who is inside ApproachRange of whom, and when each NPC was last approached by each player. Transient, like Memory. */
	FApproachTracker Approaches;

	FTimerHandle ApproachTimerHandle;

	/** Breaks weight ties. Seeded from the world seed so a session is repeatable, though nothing requires it to be. */
	FRandomStream Stream;

	FDelegateHandle ActorSpawnedHandle;

	FDelegateHandle LevelAddedHandle;

	FDelegateHandle LibraryLoadedHandle;
};
