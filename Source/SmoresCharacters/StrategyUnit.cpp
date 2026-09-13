// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyUnit.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "InventoryComponent.h"
#include "EquipmentComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "Engine/OverlapResult.h"
#include "HealthComponent.h"
#include "CombatComponent.h"
#include "StrategyPlayerUnit.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "SmoresCharacters.h"

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

	// create the equipment component - the worn slots, separate from the carried grid
	Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

	// create the health component
	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));

	// create the combat component
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

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

	// react to our own attack requests that turned out to be out of range
	Combat->OnTargetOutOfRange.AddDynamic(this, &AStrategyUnit::OnCombatTargetOutOfRange);

	// NOTE: Combat's auto-attack continuation is deliberately NOT bound here to a persistent
	// AnimInstance::OnMontageEnded subscription. Switching the mesh's Animation Mode (e.g. the
	// old EventInteractionBehavior's PlayAnimation/SetAnimationMode pair) destroys and recreates
	// the AnimInstance, which would silently orphan a BeginPlay-time binding - every attack after
	// that would still play once (Montage_Play always re-fetches the current AnimInstance) but
	// nothing would ever fire the continuation. UCombatComponent::PerformAttack binds fresh per
	// swing instead, via Montage_SetEndDelegate, which works no matter how many times the
	// AnimInstance is replaced.
}

void AStrategyUnit::StopMoving()
{
	// cancel any active AIController move request - without this, the PathFollowingComponent
	// keeps driving toward its last goal every tick (including rotating to face it via
	// bOrientRotationToMovement), which StopMovementImmediately() alone doesn't stop
	if (AIController)
	{
		AIController->StopMovement();
	}

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
	// drives shared AI/movement state (CurrentMovementGoal, the AIController) - only the server may mutate it
	if (!HasAuthority())
	{
		return;
	}

	// a Downed unit can't move - without this guard, a move command issued while Downed still
	// kicks off the EQS/AIController move, which visibly slides the ragdoll-posed unit around
	if (IsDowned())
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s MoveToLocation bailed early: unit is Downed"), *GetName());
		return;
	}

	// cache the movement and interaction parameters
	CurrentMovementGoal = Location;
	bInteractOnArrival = bInteract;
	InteractIgnoreList = IgnoreList;

	// a new movement command always interrupts any live or pending attack engagement
	bAttackOnArrival = false;
	PendingAttackTarget = nullptr;
	Combat->ClearCurrentAttackTarget();

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
	// drives shared AI state (Disposition, the self-aggro timer) - only the server may mutate it
	if (!HasAuthority())
	{
		return;
	}

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

		Combat->ClearCurrentAttackTarget();
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
	// ongoing auto-attack loop (owned by Combat now) keeps that going on its own; re-issuing
	// the attack command here on every retarget tick would restart the swing mid-play and the
	// hit-frame notify would never be reached
	if (Nearest == Combat->GetCurrentAttackTarget() || Nearest == PendingAttackTarget.Get())
	{
		return;
	}

	AttackTarget(Nearest);
}

void AStrategyUnit::AttackTarget(AStrategyUnit* Target)
{
	if (Combat)
	{
		Combat->AttackTarget(Target);
	}
}

void AStrategyUnit::OnCombatTargetOutOfRange(AActor* Target)
{
	AStrategyUnit* TargetUnit = Cast<AStrategyUnit>(Target);

	if (!IsValid(TargetUnit))
	{
		return;
	}

	// move into range, then attack again on arrival. MoveToLocation resets
	// bAttackOnArrival/PendingAttackTarget itself, so set them after calling it, not before.
	MoveToLocation(TargetUnit->GetActorLocation(), false, {});

	bAttackOnArrival = true;
	PendingAttackTarget = TargetUnit;
}

void AStrategyUnit::OnHealthDowned()
{
	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthDowned"), *GetName());

	StopMoving();

	// stop being anyone's live attack target and stop this unit's own attack loop
	PendingAttackTarget = nullptr;
	bAttackOnArrival = false;
	Combat->NotifyOwnerDowned();

	// stop self-hunting while Downed - AttackTarget() also refuses to act while Downed, but
	// clearing this too avoids a pointless TryEngageNearestPlayerPawn call every 0.5s until recovery
	GetWorldTimerManager().ClearTimer(AggroRetargetTimerHandle);
}

void AStrategyUnit::OnHealthRecovered()
{
	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthRecovered"), *GetName());

	Combat->NotifyOwnerRecovered();

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
	if (Combat->GetCurrentAttackTarget() || bAttackOnArrival)
	{
		return;
	}

	AStrategyUnit* Attacker = Cast<AStrategyUnit>(DamageInstigator);

	if (!IsValid(Attacker) || Attacker->IsDowned())
	{
		return;
	}

	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthDamaged: auto-retaliating against %s"), *GetName(), *Attacker->GetName());

	AttackTarget(Attacker);
}
