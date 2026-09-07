// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyUnit.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "Engine/OverlapResult.h"
#include "Combat/HealthComponent.h"
#include "StrategyPlayerUnit.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "smores.h"

AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(100.0f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// create the inventory component
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	// create the health component
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	// configure movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 20.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->SetFixedBrakingDistance(200.0f);
	GetCharacterMovement()->SetFixedBrakingDistance(true);
}

void AStrategyUnit::NotifyControllerChanged()
{
	// validate and save a copy of the AI controller reference
	AIController = Cast<AAIController>(Controller);
	
	if (AIController)
	{
		// subscribe to the move finished handler on the path following component
		UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
		if (PFComp)
		{
			PFComp->OnRequestFinished.AddUObject(this, &AStrategyUnit::OnMoveFinished);
		}
	}
}

void AStrategyUnit::BeginPlay()
{
	Super::BeginPlay();

	// react to our own health going Downed/recovering
	Health->OnDowned.AddDynamic(this, &AStrategyUnit::OnHealthDowned);
	Health->OnRecovered.AddDynamic(this, &AStrategyUnit::OnHealthRecovered);
	Health->OnDamaged.AddDynamic(this, &AStrategyUnit::OnHealthDamaged);

	// NOTE: the auto-attack continuation is deliberately NOT bound here to a persistent
	// AnimInstance::OnMontageEnded subscription. Switching the mesh's Animation Mode (e.g. the
	// old EventInteractionBehavior's PlayAnimation/SetAnimationMode pair) destroys and recreates
	// the AnimInstance, which would silently orphan a BeginPlay-time binding - every attack after
	// that would still play once (Montage_Play always re-fetches the current AnimInstance) but
	// nothing would ever fire the continuation. PerformAttack binds fresh per swing instead, via
	// Montage_SetEndDelegate, which works no matter how many times the AnimInstance is replaced.
}

void AStrategyUnit::StopMoving()
{
	// use the character movement component to stop movement
	GetCharacterMovement()->StopMovementImmediately();

	// stop the unit's interaction animation
	BP_StopAnimation();
}

void AStrategyUnit::UnitSelected()
{
	// pass control to BP
	BP_UnitSelected();
}

void AStrategyUnit::UnitDeselected()
{
	// pass control to BP
	BP_UnitDeselected();
}

void AStrategyUnit::Interact(AStrategyUnit* Interactor)
{
	// ensure the interactor is valid
	if (IsValid(Interactor))
	{
		// a Downed unit stays in its Downed pose - don't rotate to face whoever's interacting with it
		if (!IsDowned())
		{
			SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Interactor->GetActorLocation()));
		}

		// signal the interactor to play its interaction behavior
		Interactor->BP_InteractionBehavior(this);

		// play our own interaction behavior
		BP_InteractionBehavior(Interactor);
	}
	
}

void AStrategyUnit::MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList)
{
	// a Downed unit can't move - without this guard, a move command issued while Downed still
	// kicks off the EQS/AIController move, which visibly slides the ragdoll-posed unit around
	if (IsDowned())
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s MoveToLocation bailed early: unit is Downed"), *GetName());
		return;
	}

	// cache the movement and interaction parameters
	CurrentMovementGoal = Location;
	bInteractOnArrival = bInteract;
	InteractIgnoreList = IgnoreList;

	// a new movement command always interrupts any live or pending attack engagement
	bAttackOnArrival = false;
	PendingAttackTarget = nullptr;
	CurrentAttackTarget = nullptr;

	// stop movement and animation
	StopMoving();

	// choose the EnvQuery to use
	UEnvQuery* MoveQuery = bInteractOnArrival ? InteractionQuery : NoInteractionQuery;

	// choose the run mode to use. The main interacting unit gets the closest result, all others choose randomly from top 25%
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = bInteractOnArrival ? EEnvQueryRunMode::SingleResult : EEnvQueryRunMode::RandomBest25Pct;

	// run an EQS to resolve the movement destination using the NavMesh
	EnvQueryInstance = UEnvQueryManager::RunEQSQuery(this, MoveQuery, this,  RunMode, UEnvQueryInstanceBlueprintWrapper::StaticClass());

	if (IsValid(EnvQueryInstance))
	{
		EnvQueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &AStrategyUnit::OnEQSFinished);
	}
}

FVector AStrategyUnit::GetMovementGoal() const
{
	return CurrentMovementGoal;
}

void AStrategyUnit::OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	// was the EnvQuery successful?
	if (QueryInstance)
	{
		// get the query result locations
		TArray<FVector> ResultLocations;

		if(QueryInstance->GetQueryResultsAsLocations(ResultLocations))
		{
			// grab the top result
			CurrentMovementGoal = ResultLocations[0];

			// ensure we have a valid AI Controller
			if (AIController)
			{
				// set up the AI Move Request
				FAIMoveRequest MoveReq;

				MoveReq.SetGoalLocation(CurrentMovementGoal);
				MoveReq.SetAcceptanceRadius(MovementAcceptanceRadius);
				MoveReq.SetAllowPartialPath(true);
				MoveReq.SetUsePathfinding(true);
				MoveReq.SetProjectGoalLocation(true);
				MoveReq.SetRequireNavigableEndLocation(true);
				MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
				MoveReq.SetCanStrafe(false);

				// request a move to the AI Controller
				FNavPathSharedPtr FollowedPath;
				const FPathFollowingRequestResult ResultData = AIController->MoveTo(MoveReq, &FollowedPath);
		
				// check if we're already at the goal
				if(ResultData.Code == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					// finish movement immediately
					HandleMoveFinished();
				}
			}
		}
	}
}

void AStrategyUnit::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	HandleMoveFinished();
}

void AStrategyUnit::HandleMoveFinished()
{
	// broadcast the move completed delegate
	OnMoveCompleted.Broadcast(this);

	if (bAttackOnArrival)
	{
		// now in range (or as close as the move could get) - attack again, this time via the in-range branch
		bAttackOnArrival = false;
		AttackTarget(PendingAttackTarget.Get());
		return;
	}

	if (bInteractOnArrival)
	{
		// do an overlap test to find nearby interactive objects
		TArray<FOverlapResult> OutOverlaps;

		FCollisionShape CollisionSphere;
		CollisionSphere.SetSphere(InteractionRadius);

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams QueryParams;

		// add the selected units to the ignored list
		QueryParams.AddIgnoredActor(this);

		for (const AActor* Current : InteractIgnoreList)
		{
			QueryParams.AddIgnoredActor(Current);
		}

		if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, GetActorLocation(), FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
		{
			// find the first unit we've overlapped, and interact with it
			for (const FOverlapResult& CurrentOverlap : OutOverlaps)
			{
				if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
				{
					CurrentUnit->Interact(this);
					return;
				}
			}
		}
	}
}

bool AStrategyUnit::IsDowned() const
{
	return Health->IsDowned();
}

void AStrategyUnit::SetAggressive(bool bAggressive)
{
	Disposition = bAggressive ? EStrategyDisposition::Aggressive : EStrategyDisposition::Passive;

	if (bAggressive)
	{
		// periodically look for the nearest player pawn to hunt, starting immediately
		GetWorldTimerManager().SetTimer(AggroRetargetTimerHandle, this, &AStrategyUnit::TryEngageNearestPlayerPawn, 0.5f, true);

		TryEngageNearestPlayerPawn();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(AggroRetargetTimerHandle);

		CurrentAttackTarget = nullptr;
		PendingAttackTarget = nullptr;
	}
}

bool AStrategyUnit::IsUnitInRange(const AStrategyUnit* Unit) const
{
	return Unit && FVector::Dist(GetActorLocation(), Unit->GetActorLocation()) <= InteractionRange->GetScaledSphereRadius();
}

void AStrategyUnit::TryEngageNearestPlayerPawn()
{
	// gather all player-controlled pawns in the level, same pattern as
	// AStrategyPlayerController::RefreshPlayerPawns
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategyPlayerUnit::StaticClass(), FoundActors);

	AStrategyUnit* Nearest = nullptr;
	float NearestDistSq = 0.0f;

	for (AActor* CurrentActor : FoundActors)
	{
		AStrategyPlayerUnit* CurrentUnit = Cast<AStrategyPlayerUnit>(CurrentActor);

		if (!IsValid(CurrentUnit) || CurrentUnit->IsDowned())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), CurrentUnit->GetActorLocation());

		if (!Nearest || DistSq < NearestDistSq)
		{
			Nearest = CurrentUnit;
			NearestDistSq = DistSq;
		}
	}

	if (!Nearest)
	{
		// nothing left to fight - reset trigger #2
		SetAggressive(false);
		return;
	}

	// already engaged with this target (fighting in range, or moving in to engage it) - the
	// ongoing auto-attack loop (OnAttackMontageEnded) keeps that going on its own; re-issuing
	// the attack command here on every retarget tick would restart the swing mid-play and the
	// hit-frame notify would never be reached
	if (Nearest == CurrentAttackTarget.Get() || Nearest == PendingAttackTarget.Get())
	{
		return;
	}

	AttackTarget(Nearest);
}

void AStrategyUnit::AttackTarget(AStrategyUnit* Target)
{
	// a Downed unit can't initiate or continue an attack - without this, a Downed unit's own
	// AggroRetargetTimerHandle (still running - OnHealthDowned doesn't touch it) keeps calling
	// back in here forever, replaying an attack montage on top of the Downed pose on the same
	// anim slot, which looks like a broken/glitched animation regardless of which montage plays
	if (IsDowned() || !IsValid(Target) || Target->IsDowned())
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s AttackTarget(%s) bailed early: %s"),
			*GetName(), Target ? *Target->GetName() : TEXT("null"),
			IsDowned() ? TEXT("attacker is Downed") : (!IsValid(Target) ? TEXT("Target invalid") : TEXT("Target already Downed")));
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	if (Dist <= AttackRange)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s AttackTarget(%s): in range (%.0f <= %.0f), swinging now"),
			*GetName(), *Target->GetName(), Dist, AttackRange);
		PerformAttack(Target);
		return;
	}

	UE_LOG(Logsmores, Warning, TEXT("[Combat] %s AttackTarget(%s): out of range (%.0f > %.0f), moving in"),
		*GetName(), *Target->GetName(), Dist, AttackRange);

	// out of range - move into range, then attack again on arrival. MoveToLocation resets
	// bAttackOnArrival/PendingAttackTarget itself, so set them after calling it, not before.
	MoveToLocation(Target->GetActorLocation(), false, {});

	bAttackOnArrival = true;
	PendingAttackTarget = Target;
}

void AStrategyUnit::PerformAttack(AStrategyUnit* Target)
{
	// rotate towards the target, same helper Interact() uses
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target->GetActorLocation()));

	CurrentAttackTarget = Target;

	if (AttackMontages.Num() == 0)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		if (UAnimMontage* ChosenMontage = AttackMontages[FMath::RandHelper(AttackMontages.Num())])
		{
			AnimInstance->Montage_Play(ChosenMontage);

			// bind fresh every swing (see the NOTE in BeginPlay) rather than relying on a
			// persistent AnimInstance::OnMontageEnded subscription, which an AnimInstance
			// recreation (e.g. an Animation Mode switch elsewhere) would silently orphan
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AStrategyUnit::OnAttackMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, ChosenMontage);
		}
	}
}

void AStrategyUnit::ApplyAttackDamage()
{
	AStrategyUnit* Target = CurrentAttackTarget.Get();

	// re-check range at the hit frame, not the swing-start frame, so a target that fled
	// mid-swing doesn't take a phantom hit
	if (!IsValid(Target))
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s ApplyAttackDamage: no valid CurrentAttackTarget - hit whiffed"), *GetName());
		return;
	}

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());

	if (Dist > AttackRange)
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s ApplyAttackDamage: %s moved out of range at hit frame (%.0f > %.0f) - hit whiffed"),
			*GetName(), *Target->GetName(), Dist, AttackRange);
		return;
	}

	Target->GetHealth()->TakeDamage(25.0f, this);

	UE_LOG(Logsmores, Warning, TEXT("[Combat] %s ApplyAttackDamage: hit %s for 25, health now %.0f, IsDowned=%s"),
		*GetName(), *Target->GetName(), Target->GetHealth()->GetHealth(), Target->IsDowned() ? TEXT("true") : TEXT("false"));
}

void AStrategyUnit::OnHealthDowned()
{
	UE_LOG(Logsmores, Warning, TEXT("[Combat] %s OnHealthDowned"), *GetName());

	StopMoving();

	// stop being anyone's live attack target and stop this unit's own attack loop
	CurrentAttackTarget = nullptr;
	PendingAttackTarget = nullptr;
	bAttackOnArrival = false;

	// stop self-hunting while Downed - AttackTarget() also refuses to act while Downed, but
	// clearing this too avoids a pointless TryEngageNearestPlayerPawn call every 0.5s until recovery
	GetWorldTimerManager().ClearTimer(AggroRetargetTimerHandle);

	if (DownedMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(DownedMontage);
		}
	}
}

void AStrategyUnit::OnHealthRecovered()
{
	UE_LOG(Logsmores, Warning, TEXT("[Combat] %s OnHealthRecovered"), *GetName());

	// blend back to locomotion - accepted small pop, no "get up" anim exists
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.25f, DownedMontage);
	}

	if (Disposition == EStrategyDisposition::Aggressive)
	{
		// reset trigger #1 - the Aggressive unit itself went Downed and has now recovered
		SetAggressive(false);
	}
}

void AStrategyUnit::OnHealthDamaged(AActor* DamageInstigator)
{
	// already mid-engagement (fighting in range, or moving in to engage) - don't hijack an
	// existing attack order onto whoever just landed a hit
	if (CurrentAttackTarget.IsValid() || bAttackOnArrival)
	{
		return;
	}

	AStrategyUnit* Attacker = Cast<AStrategyUnit>(DamageInstigator);

	if (!IsValid(Attacker) || Attacker->IsDowned())
	{
		return;
	}

	UE_LOG(Logsmores, Warning, TEXT("[Combat] %s OnHealthDamaged: auto-retaliating against %s"), *GetName(), *Attacker->GetName());

	AttackTarget(Attacker);
}

void AStrategyUnit::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// ignore montages that aren't one of ours (e.g. DownedMontage ending)
	if (!AttackMontages.Contains(Montage))
	{
		return;
	}

	// keep re-swinging the same target automatically until it goes Downed, this attacker is
	// invalid (CurrentAttackTarget is cleared by OnHealthDowned/MoveToLocation), or this attacker
	// is given a new command - symmetric for player-issued attacks and NPC self-attacks alike
	AStrategyUnit* Target = CurrentAttackTarget.Get();

	if (IsValid(Target) && !Target->IsDowned())
	{
		AttackTarget(Target);
	}
	else
	{
		UE_LOG(Logsmores, Warning, TEXT("[Combat] %s OnAttackMontageEnded: stopping the auto-attack loop (Target %s)"),
			*GetName(), !IsValid(Target) ? TEXT("invalid") : TEXT("Downed"));
	}
}
