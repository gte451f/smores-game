// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "CombatComponent.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "SmoresCombat.h"

UCombatComponent::UCombatComponent()
{
	// combat resolution is pure state/RPC driven - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	// The escalation half of the engagement reads health; it doesn't keep its own copy. Bound
	// here rather than by the owning unit so the engagement rules live in one module and can be
	// tested without a unit. Authority only - the handlers change server state, and on a client
	// these delegates fire from RepNotifies that the server's own broadcast already covered.
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (UHealthComponent* OwnerHealth = Owner->FindComponentByClass<UHealthComponent>())
	{
		OwnerHealth->OnDamaged.AddUniqueDynamic(this, &UCombatComponent::HandleOwnerDamaged);
		OwnerHealth->OnDowned.AddUniqueDynamic(this, &UCombatComponent::HandleOwnerWentDown);
		OwnerHealth->OnDied.AddUniqueDynamic(this, &UCombatComponent::HandleOwnerWentDown);
		OwnerHealth->OnRecovered.AddUniqueDynamic(this, &UCombatComponent::HandleOwnerRecovered);
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, bEngaged);
}

void UCombatComponent::AttackTarget(AActor* Target)
{
	AActor* Owner = GetOwner();

	// drives shared combat state (CurrentAttackTarget, the attack montage) - only the server may mutate it
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const UHealthComponent* OwnerHealth = Owner->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* TargetHealth = IsValid(Target) ? Target->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bOwnerOut = OwnerHealth && OwnerHealth->IsIncapacitated();
	const bool bTargetOut = TargetHealth && TargetHealth->IsIncapacitated();

	// an incapacitated owner can't initiate or continue an attack - without this, a still-running
	// aggro/retarget loop on the owner could keep calling back in here forever, replaying an
	// attack montage on top of the grounded pose on the same anim slot. Dead counts the same as
	// Downed here: neither a corpse nor a knocked-down unit fights or is worth swinging at.
	if (bOwnerOut || !IsValid(Target) || bTargetOut)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s) bailed early: %s"),
			*Owner->GetName(), Target ? *Target->GetName() : TEXT("null"),
			bOwnerOut ? TEXT("attacker is down") : (!IsValid(Target) ? TEXT("Target invalid") : TEXT("Target already down")));
		return;
	}

	// Committing to an attack is entering the fight. The attacker is engaged from the moment it is
	// sent in, not from whenever something swings back, so a whole squad ordered in on one key press
	// flashes together rather than only whoever the enemy happens to pick. Before the range branch,
	// so a unit still walking over counts; the loop comes back through here every swing, which keeps
	// the attacker's own fight alive even against a target that never fights back.
	JoinEngagement();

	// being picked as a target is hostile attention whether or not the swing can land yet - it is
	// what lets "targeted" count as entering danger while the attacker is still walking over. The
	// auto-attack loop comes back through here every swing, which is what keeps a fight alive.
	if (UCombatComponent* TargetCombat = Target->FindComponentByClass<UCombatComponent>())
	{
		TargetCombat->NoteHostileAttention(Owner, EHostileAttention::Targeted);
	}

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

	if (Dist <= AttackRange)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s): in range (%.0f <= %.0f), swinging now"),
			*Owner->GetName(), *Target->GetName(), Dist, AttackRange);
		PerformAttack(Target);
		return;
	}

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s AttackTarget(%s): out of range (%.0f > %.0f), requesting a move into range"),
		*Owner->GetName(), *Target->GetName(), Dist, AttackRange);

	// out of range - the owner is responsible for moving closer and calling AttackTarget again on arrival
	OnTargetOutOfRange.Broadcast(Target);
}

void UCombatComponent::PerformAttack(AActor* Target)
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return;
	}

	// rotate towards the target, same helper the owner's Interact() uses
	Owner->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(Owner->GetActorLocation(), Target->GetActorLocation()));

	CurrentAttackTarget = Target;

	if (AttackMontages.Num() == 0)
	{
		return;
	}

	// chosen once, authoritatively (PerformAttack only ever runs server-side - see AttackTarget's
	// HasAuthority guard), and replicated to every machine so the swing plays in lock-step
	// everywhere rather than each machine picking its own random montage
	if (UAnimMontage* ChosenMontage = AttackMontages[FMath::RandHelper(AttackMontages.Num())])
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s PerformAttack: calling Multicast_PlayAttackMontage(%s)"),
			*Owner->GetName(), *ChosenMontage->GetName());
		Multicast_PlayAttackMontage(ChosenMontage);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s PerformAttack: AttackMontages[chosen index] was null"), *Owner->GetName());
	}
}

void UCombatComponent::Multicast_PlayAttackMontage_Implementation(UAnimMontage* Montage)
{
	AActor* Owner = GetOwner();
	ACharacter* OwnerCharacter = Cast<ACharacter>(Owner);

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation(%s): OwnerCharacter=%s, Mesh=%s"),
		Owner ? *Owner->GetName() : TEXT("?"), Montage ? *Montage->GetName() : TEXT("null"),
		OwnerCharacter ? TEXT("valid") : TEXT("NULL - Cast<ACharacter> failed"),
		(OwnerCharacter && OwnerCharacter->GetMesh()) ? TEXT("valid") : TEXT("NULL"));

	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation: AnimInstance=%s, calling Montage_Play"),
			*Owner->GetName(), *AnimInstance->GetClass()->GetName());

		AnimInstance->Montage_Play(Montage);

		// bind fresh every swing rather than relying on a persistent AnimInstance::OnMontageEnded
		// subscription, which an AnimInstance recreation (e.g. an Animation Mode switch elsewhere)
		// would silently orphan
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UCombatComponent::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Multicast_PlayAttackMontage_Implementation: GetAnimInstance() returned null"), *Owner->GetName());
	}
}

void UCombatComponent::ApplyAttackDamage()
{
	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: notify fired"), GetOwner() ? *GetOwner()->GetName() : TEXT("?"));

	AActor* Owner = GetOwner();

	// the anim notify fires on every machine simulating this montage (attacker + all observing
	// clients) - only the server may actually apply damage
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	AActor* Target = CurrentAttackTarget.Get();

	// re-check range at the hit frame, not the swing-start frame, so a target that fled
	// mid-swing doesn't take a phantom hit
	if (!IsValid(Target))
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: no valid CurrentAttackTarget - hit whiffed"), *Owner->GetName());
		return;
	}

	const float Dist = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());

	if (Dist > AttackRange)
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: %s moved out of range at hit frame (%.0f > %.0f) - hit whiffed"),
			*Owner->GetName(), *Target->GetName(), Dist, AttackRange);
		return;
	}

	UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();

	if (!TargetHealth)
	{
		return;
	}

	const bool bTargetWasStanding = !TargetHealth->IsIncapacitated();

	TargetHealth->TakeDamage(25.0f, Owner);

	// After the damage, not before: a hit that knocks an unengaged unit straight down then signals
	// once (Entered) rather than twice in one frame (Entered, then Down). A swing landing on a body
	// is no news at all - TakeDamage ignored it, and so does this.
	if (bTargetWasStanding)
	{
		if (UCombatComponent* TargetCombat = Target->FindComponentByClass<UCombatComponent>())
		{
			TargetCombat->NoteHostileAttention(Owner, EHostileAttention::Hit);
		}
	}

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s ApplyAttackDamage: hit %s for 25, health now %.0f, IsDowned=%s"),
		*Owner->GetName(), *Target->GetName(), TargetHealth->GetHealth(), TargetHealth->IsDowned() ? TEXT("true") : TEXT("false"));
}

void UCombatComponent::NotifyOwnerDowned()
{
	// stop being anyone's live attack target and stop this owner's own attack loop
	CurrentAttackTarget = nullptr;

	if (!DownedMontage)
	{
		return;
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(DownedMontage);
			}
		}
	}
}

void UCombatComponent::NotifyOwnerRecovered()
{
	// blend back to locomotion - accepted small pop, no "get up" anim exists
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.25f, DownedMontage);
			}
		}
	}
}

void UCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// ignore montages that aren't one of ours (e.g. DownedMontage ending)
	if (!AttackMontages.Contains(Montage))
	{
		return;
	}

	// keep re-swinging the same target automatically until it goes down, this attacker is
	// invalid (CurrentAttackTarget is cleared by NotifyOwnerDowned), or this attacker is given a
	// new command - symmetric for player-issued attacks and NPC self-attacks alike
	AActor* Target = CurrentAttackTarget.Get();
	const UHealthComponent* TargetHealth = IsValid(Target) ? Target->FindComponentByClass<UHealthComponent>() : nullptr;

	if (IsValid(Target) && TargetHealth && !TargetHealth->IsIncapacitated())
	{
		AttackTarget(Target);
	}
	else
	{
		UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s OnAttackMontageEnded: stopping the auto-attack loop (Target %s)"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("?"), !IsValid(Target) ? TEXT("invalid") : TEXT("down"));
	}
}

void UCombatComponent::NoteHostileAttention(AActor* Attacker, EHostileAttention Attention)
{
	// engagement is shared state - only the server decides it
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// DangerTrigger decides only what may *start* a fight. Once one is running, any attention at all
	// keeps it going, including a kind that couldn't have started it - an enemy still swinging is
	// still an enemy.
	const bool bCanStart = DangerTrigger == EDangerTrigger::Targeted || Attention == EHostileAttention::Hit;

	if (bEngaged || bCanStart)
	{
		JoinEngagement();
	}
}

void UCombatComponent::JoinEngagement()
{
	if (bEngaged)
	{
		// already told about this fight - just push its end back
		RestartEngagementTimer();
		return;
	}

	EnterEngagement();
}

void UCombatComponent::EnterEngagement()
{
	bEngaged = true;

	// a unit that walks into a fight already hurt has not *crossed* the floor during it, so it
	// doesn't owe the player a Wounded signal on the next hit - only on climbing back above the
	// floor and falling to it again
	bWoundSignalled = WoundedHealthFraction > 0.0f && GetOwnerHealthFraction() <= WoundedHealthFraction;

	RestartEngagementTimer();

	SignalDanger(EDangerSignal::Entered);
}

void UCombatComponent::EndEngagement()
{
	bEngaged = false;
	bWoundSignalled = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EngagementTimerHandle);
	}
}

void UCombatComponent::RestartEngagementTimer()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	// SetTimer on a live handle replaces it, so this is both "start" and "push back"
	World->GetTimerManager().SetTimer(EngagementTimerHandle, this, &UCombatComponent::EndEngagement, EngagementTimeoutSeconds, false);
}

void UCombatComponent::SignalDanger(EDangerSignal Signal)
{
	Multicast_DangerSignal(Signal);
}

void UCombatComponent::Multicast_DangerSignal_Implementation(EDangerSignal Signal)
{
	OnDangerSignal.Broadcast(GetOwner(), Signal);
}

float UCombatComponent::GetOwnerHealthFraction() const
{
	const UHealthComponent* OwnerHealth = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;

	if (!OwnerHealth || OwnerHealth->MaxHealth <= 0.0f)
	{
		return 1.0f;
	}

	return FMath::Clamp(OwnerHealth->GetHealth() / OwnerHealth->MaxHealth, 0.0f, 1.0f);
}

void UCombatComponent::HandleOwnerDamaged(AActor* DamageInstigator)
{
	// Damage from anywhere counts toward the floor, not only a swing from another unit - but it
	// only escalates a fight already running. Entering one is NoteHostileAttention's job, and it
	// runs straight after this for a hit from another unit (see ApplyAttackDamage).
	if (!bEngaged || WoundedHealthFraction <= 0.0f)
	{
		return;
	}

	const bool bBelowFloor = GetOwnerHealthFraction() <= WoundedHealthFraction;

	if (!bBelowFloor)
	{
		bWoundSignalled = false;
		return;
	}

	if (!bWoundSignalled)
	{
		bWoundSignalled = true;
		SignalDanger(EDangerSignal::Wounded);
	}
}

void UCombatComponent::HandleOwnerWentDown()
{
	// Only inside an engagement. Outside one, going down means either the hit that did it is about
	// to report itself as the engagement's start (ApplyAttackDamage), or nothing hostile happened
	// at all - UHealthComponent::RestoreState broadcasts OnDowned/OnDied for a unit loaded from its
	// record already down, and a save loading must not flash the squad bar.
	if (!bEngaged)
	{
		return;
	}

	SignalDanger(EDangerSignal::Down);
}

void UCombatComponent::HandleOwnerRecovered()
{
	// back at full health, so the next fall to the floor is a fresh one
	bWoundSignalled = false;
}
