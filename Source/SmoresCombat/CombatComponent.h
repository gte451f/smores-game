// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackDamageDealer.h"
#include "CombatComponent.generated.h"

class UAnimMontage;

/** Broadcast when AttackTarget is called on a target that's out of AttackRange - the owner is
 *  expected to move into range and call AttackTarget again on arrival; this component never
 *  moves its owner itself. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatTargetOutOfRangeDelegate, AActor*, Target);

/**
 *  Why a unit's danger alert fired. Every value is flash-worthy: the whole point of the
 *  engagement state below is that the hits in between are *not*, so they never get a value here.
 */
UENUM(BlueprintType)
enum class EDangerSignal : uint8
{
	/** Entered a hostile engagement from clear */
	Entered,

	/** Got worse inside one: health fell to the wound floor (UCombatComponent::WoundedHealthFraction) */
	Wounded,

	/** Went down inside one - knocked Downed, or killed */
	Down
};

/**
 *  How early being *attacked* counts as a unit entering danger. game-design's
 *  notifications-and-alerts.md left this open for play, so it is a setting on the unit rather than a
 *  decision in code. It never affects a unit's own attacks: a unit that attacks has joined a fight
 *  whatever this says. "Detected by something hostile" is the third candidate in the design and
 *  needs a perception system that doesn't exist yet.
 */
UENUM(BlueprintType)
enum class EDangerTrigger : uint8
{
	/** Something hostile chose this unit as its attack target - which may be while it is still walking over */
	Targeted,

	/** Something hostile landed a hit. Being targeted still keeps an engagement going; it just can't start one. */
	Hit
};

/** The kinds of hostile attention one unit's combat pays another. See UCombatComponent::NoteHostileAttention. */
enum class EHostileAttention : uint8
{
	/** The attacker picked this unit as its target */
	Targeted,

	/** The attacker's swing connected */
	Hit
};

/**
 *  Broadcast on every machine when a unit's danger alert should fire. Carries the unit, so one
 *  handler can listen to many units - the parameterless health delegates' missing "who" is why the
 *  activity feed needs a watcher object per unit, and this doesn't repeat that.
 *
 *  Native rather than dynamic: nothing designer-facing binds it (the portrait's BP_ hook is the
 *  designer's end), and a native delegate takes the portrait's AddUObject and a test's lambda.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDangerSignal, AActor* /*Unit*/, EDangerSignal /*Signal*/);

/**
 *  Melee attack-swing resolution: choosing/playing an attack montage, applying damage at the
 *  hit frame, and looping the swing while a target remains in range. Owns none of the movement
 *  or targeting-policy decisions (moving into range, who to hunt while Aggressive) - those stay
 *  on the owning character, which calls into this component once a target/range decision has
 *  been made. See the game-systems skill's unreal-module-organization topic for why this split
 *  exists (SmoresCombat must not depend back on the character/Characters-module code).
 *
 *  **It also owns the owner's engagement state** - whether this unit is in a hostile fight right
 *  now - because that is what the danger alert needs and the HUD must not work it out for itself.
 *  The rules, from game-design's notifications-and-alerts.md:
 *
 *  - A unit enters an engagement by attacking (AttackTarget - joining a fight is entering it) or
 *    by hostile attention arriving from someone else (NoteHostileAttention).
 *  - Entering an engagement from clear is the one flash-worthy transition. Every hit after it is
 *    silent - the player has already been told about this fight.
 *  - Leaving one is a timeout after the last hostile attention (EngagementTimeoutSeconds), not the
 *    end of an exchange: a unit does not leave a fight by winning one swing.
 *  - Escalation re-arms: falling to WoundedHealthFraction, or going down, signals again even
 *    mid-engagement, because the player's earlier read on the fight is now wrong.
 *
 *  The state is server truth (authority-gated, bEngaged replicated); the signal is an event, sent
 *  to every machine by multicast exactly as the attack montage is, and broadcast locally as
 *  OnDangerSignal. The flash is presentation and belongs to whoever listens - this component has
 *  no idea a HUD exists.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SMORESCOMBAT_API UCombatComponent : public UActorComponent, public IAttackDamageDealer
{
	GENERATED_BODY()

public:

	/** Constructor */
	UCombatComponent();

	/** Engages Target: swings immediately if already in AttackRange of the owner, otherwise
	 *  broadcasts OnTargetOutOfRange and does nothing further - the owner is responsible for
	 *  moving closer and calling this again on arrival. Authority-only (no-op otherwise). */
	void AttackTarget(AActor* Target);

	/** Returns the target currently being swung at, if any */
	AActor* GetCurrentAttackTarget() const { return CurrentAttackTarget.Get(); }

	/** Clears the current attack target without playing DownedMontage - used when the owner
	 *  voluntarily disengages (e.g. turning Passive, or starting an unrelated move command)
	 *  rather than going Downed. */
	void ClearCurrentAttackTarget() { CurrentAttackTarget = nullptr; }

	/** Called by the owner's OnHealthDowned reaction: stops the swing loop and plays DownedMontage */
	void NotifyOwnerDowned();

	/** Called by the owner's OnHealthRecovered reaction: stops DownedMontage */
	void NotifyOwnerRecovered();

	/**
	 *  Another unit's combat turned on this one. Authority-only (no-op otherwise).
	 *
	 *  Called by the attacker's own component - from AttackTarget (Targeted) and from
	 *  ApplyAttackDamage (Hit) - so everything hostile that happens to a unit arrives through this
	 *  one door. Starts an engagement if DangerTrigger lets this kind start one, and otherwise keeps
	 *  an existing one alive. Public so a future damage source (a trap, a ranged weapon) can report
	 *  itself the same way.
	 */
	void NoteHostileAttention(AActor* Attacker, EHostileAttention Attention);

	/** True while this unit is in a hostile engagement. Replicated, so valid on clients too. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsEngaged() const { return bEngaged; }

	//~ Begin IAttackDamageDealer interface
	/** Applies this component's owner's attack damage to CurrentAttackTarget. Called by
	 *  UAnimNotify_AttackHit at the montage's hit frame. */
	virtual void ApplyAttackDamage() override;
	//~ End IAttackDamageDealer interface

	/** Fired when AttackTarget is called on an out-of-range target; the owner should move closer
	 *  and call AttackTarget again on arrival. */
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCombatTargetOutOfRangeDelegate OnTargetOutOfRange;

	/** Fired on every machine when this unit's danger alert should fire. See EDangerSignal. */
	FOnDangerSignal OnDangerSignal;

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent interface

	/** Faces and swings at Target, playing a random attack montage */
	void PerformAttack(AActor* Target);

	/** Plays Montage locally and (re)binds OnAttackMontageEnded. Multicast so the swing (and its
	 *  hit-frame AnimNotify) plays for every machine, not just the server - PerformAttack chooses
	 *  the montage once, authoritatively, and passes it here rather than each machine picking its
	 *  own (which would desync the swing shown to different observers). */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayAttackMontage(UAnimMontage* Montage);

	/** Bound to the anim instance's OnMontageEnded; continues the auto-attack loop */
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Enters an engagement from clear, or keeps a running one going. Server-side; callers check authority. */
	void JoinEngagement();

	/** Starts an engagement from clear, and signals Entered */
	void EnterEngagement();

	/** The timeout's end of an engagement. Silent - leaving danger is not news; the feed and the health bar say how it went. */
	void EndEngagement();

	/** (Re)starts the countdown to EndEngagement */
	void RestartEngagementTimer();

	/** Sends Signal to every machine. Server-side only; see Multicast_DangerSignal. */
	void SignalDanger(EDangerSignal Signal);

	/** Broadcasts OnDangerSignal on every machine, the server included - the same route the attack montage takes */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DangerSignal(EDangerSignal Signal);

	/** The owner's health fraction, 0-1, or 1 if it has no health component */
	float GetOwnerHealthFraction() const;

	/** Bound to the owner's Health->OnDamaged: the wound-floor check */
	UFUNCTION()
	void HandleOwnerDamaged(AActor* DamageInstigator);

	/** Bound to the owner's Health->OnDowned and OnDied: the going-down escalation */
	UFUNCTION()
	void HandleOwnerWentDown();

	/** Bound to the owner's Health->OnRecovered: back at full health, so the wound floor re-arms */
	UFUNCTION()
	void HandleOwnerRecovered();

	/** Montages to play (one chosen at random) when attacking. Expects the 3 wrapped MM_Attack_0X montages. */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	/** Montage played while the owner is Downed: falls once, then holds a looping grounded pose
	 *  until NotifyOwnerRecovered stops it */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> DownedMontage;

	/** Max distance to a target for an attack to land without needing to move closer first */
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float AttackRange = 150.0f;

	/** How early being attacked counts as this unit entering danger. See EDangerTrigger. */
	UPROPERTY(EditAnywhere, Category = "Combat|Danger")
	EDangerTrigger DangerTrigger = EDangerTrigger::Targeted;

	/**
	 *  How long after the last hostile attention an engagement ends. Game time, so it runs faster at
	 *  high pace and stops while paused - it is a fact about the fight, not about the player's screen.
	 *  Long enough to outlast the gaps between swings, or one fight would flash several times.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Danger", meta = (ClampMin = 0.1, Units = "s"))
	float EngagementTimeoutSeconds = 10.0f;

	/**
	 *  Health, as a fraction of maximum, at or below which an engaged unit escalates - the "serious
	 *  wound" re-flash. 0 turns wound escalation off and leaves only going down.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Danger", meta = (ClampMin = 0, ClampMax = 1))
	float WoundedHealthFraction = 0.5f;

	/** The target currently being swung at. Set once in range; drives ApplyAttackDamage and the auto-attack loop. */
	TWeakObjectPtr<AActor> CurrentAttackTarget;

	/** True while this unit is in a hostile engagement. Server truth, replicated for anyone who wants to read it. */
	UPROPERTY(Replicated)
	bool bEngaged = false;

	/**
	 *  True once this engagement has signalled Wounded, so staying below the floor doesn't signal on
	 *  every hit. Cleared by climbing back above it (recovery) and when the engagement ends.
	 *  Server-only bookkeeping - nothing off the server reads it.
	 */
	bool bWoundSignalled = false;

	/** Pending EndEngagement while engaged */
	FTimerHandle EngagementTimerHandle;
};
