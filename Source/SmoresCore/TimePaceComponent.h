// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePace.h"
#include "TimePaceComponent.generated.h"

/**
 *  The simulation's current speed, owned by the GameState.
 *
 *  **Pace is shared world state, not per-player UI state.** One pace applies to everyone in the
 *  session, which is why this hangs off the GameState rather than a player controller: a
 *  GameState component replicates to every client for free, and there is exactly one of it.
 *
 *  A client cannot RPC the GameState - it doesn't own it - so a player asking for a different
 *  pace routes through their own player controller
 *  (AStrategyPlayerController::Server_RequestPace), which they do own. The server then calls
 *  SetPace here. Applying it is UGameplayStatics::SetGlobalTimeDilation, and
 *  AWorldSettings::TimeDilation replicates on its own, so clients follow without this component
 *  doing anything on their end.
 *
 *  **Any player may change the pace.** That is what the code does with no extra work, and it is
 *  a deliberately provisional call - see Docs/roadmaps/hud-roadmap.md's Open Questions. The
 *  alternatives (host only, slowest request wins) are one `if` in SetPace away and want a real
 *  co-op session to judge.
 *
 *  The ladder helpers are all static and world-free on purpose: the tier arithmetic is the part
 *  that can be wrong in a way nobody notices, so it is the part that gets tested.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESCORE_API UTimePaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Constructor */
	UTimePaceComponent();

	/**
	 *  Every tier, slowest first. This is EGamePace's declaration order made iterable - stepping,
	 *  clamping and "which button is lit" all walk this rather than casting to uint8 in three
	 *  different places.
	 */
	static const TArray<EGamePace>& GetPaceLadder();

	/**
	 *  The global time dilation a tier means.
	 *
	 *  Paused is 0.0001 rather than 0: AWorldSettings clamps whatever it is handed to
	 *  [MinGlobalTimeDilation, MaxGlobalTimeDilation], and MinGlobalTimeDilation is 0.0001 by
	 *  default - so passing 0 would be silently raised to this same number anyway. Naming it here
	 *  means the value the world actually runs at is the value this function claims. At 1/10,000
	 *  speed a minute of real time is six milliseconds of game time, which is frozen by any
	 *  standard a player can perceive.
	 */
	static float GetDilationForPace(EGamePace Pace);

	/** The short readout for a tier ("Paused", "1x", "4x") - what the pace strip's text shows */
	static FText GetPaceLabel(EGamePace Pace);

	/**
	 *  The tier Steps rungs away from Pace, clamped at both ends of the ladder.
	 *
	 *  Clamping rather than wrapping is the whole point: `=` held down at 8x should stay at 8x,
	 *  not drop the player back to paused.
	 */
	static EGamePace StepPace(EGamePace Pace, int32 Steps);

	/** The current tier. Replicated, so this is valid on clients too. */
	UFUNCTION(BlueprintPure, Category = "Time Pace")
	EGamePace GetPace() const { return Pace; }

	/** True while the simulation is frozen */
	UFUNCTION(BlueprintPure, Category = "Time Pace")
	bool IsPaused() const { return Pace == EGamePace::Paused; }

	/**
	 *  The tier unpausing returns to - whatever was running when the player paused.
	 *
	 *  It lives here rather than on the controller that asked, because pause is shared: in co-op
	 *  one player pausing and another unpausing has to come back to the same speed, and it would
	 *  not if each controller remembered its own.
	 */
	UFUNCTION(BlueprintPure, Category = "Time Pace")
	EGamePace GetResumePace() const { return ResumePace; }

	/**
	 *  Sets the pace and applies it to the world. **Authority only** - a client calling this does
	 *  nothing at all, by design; the route in is Server_RequestPace on the player controller.
	 */
	void SetPace(EGamePace NewPace);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent interface

	/** Pushes the current tier's dilation into the world. Server-side; TimeDilation replicates. */
	void ApplyDilation() const;

	/** The current tier. Replicated so a client's HUD can read it without asking the server. */
	UPROPERTY(Replicated)
	EGamePace Pace = EGamePace::Normal;

	/** See GetResumePace. Replicated for the same reason Pace is - the unpause key reads it locally. */
	UPROPERTY(Replicated)
	EGamePace ResumePace = EGamePace::Normal;
};
