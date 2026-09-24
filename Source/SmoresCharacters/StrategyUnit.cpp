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
#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "Net/UnrealNetwork.h"
#include "SmoresCharacters.h"

AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	// Looting reach, measured centre-to-centre (see IInventoryHolder::IsActorWithinSphere).
	// This was 100, which is barely reachable at all: the capsule radius is 42, so two
	// characters standing in contact are already 84 apart, and MovementAcceptanceRadius is
	// itself 100 - a pawn ordered to walk to a body routinely parks just outside the gate and
	// the double-click then highlights the body without opening it. A sideways step of a few
	// centimetres flips it, which reads as "the double-click is unreliable" rather than as a
	// range problem. 250 leaves real headroom while staying tighter than a chest's 312.5.
	InteractionRange->SetSphereRadius(250.0f);
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
	Health->OnDied.AddDynamic(this, &AStrategyUnit::OnHealthDied);
	Health->OnDamaged.AddDynamic(this, &AStrategyUnit::OnHealthDamaged);

	// react to our own attack requests that turned out to be out of range
	Combat->OnTargetOutOfRange.AddDynamic(this, &AStrategyUnit::OnCombatTargetOutOfRange);

	// every change to the working copy goes back to the record. Bound before registering, so a
	// record restored as Dead or Downed runs this unit's own OnHealthDied/OnHealthDowned and goes
	// inert exactly as it would have in front of you. OnDamaged carries an argument, so its
	// write-back is in OnHealthDamaged instead.
	Health->OnDowned.AddDynamic(this, &AStrategyUnit::OnWorkingCopyChanged);
	Health->OnRecovered.AddDynamic(this, &AStrategyUnit::OnWorkingCopyChanged);
	Health->OnDied.AddDynamic(this, &AStrategyUnit::OnWorkingCopyChanged);
	Inventory->OnInventoryChanged.AddDynamic(this, &AStrategyUnit::OnWorkingCopyChanged);
	Equipment->OnEquipmentChanged.AddDynamic(this, &AStrategyUnit::OnWorkingCopyChanged);

	RegisterWithRecordStore();

	// NOTE: Combat's auto-attack continuation is deliberately NOT bound here to a persistent
	// AnimInstance::OnMontageEnded subscription. Switching the mesh's Animation Mode (e.g. the
	// old EventInteractionBehavior's PlayAnimation/SetAnimationMode pair) destroys and recreates
	// the AnimInstance, which would silently orphan a BeginPlay-time binding - every attack after
	// that would still play once (Montage_Play always re-fetches the current AnimInstance) but
	// nothing would ever fire the continuation. UCombatComponent::PerformAttack binds fresh per
	// swing instead, via Montage_SetEndDelegate, which works no matter how many times the
	// AnimInstance is replaced.
}

void AStrategyUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// the record outlives its actor: take the last word, then let go. Nothing removes the record -
	// under the soft split nothing destroys a unit mid-session, and a record that stays behind is
	// exactly what a dead named character needs.
	if (HasAuthority() && RecordId.IsValid())
	{
		WriteBackToRecord();

		if (UCharacterRecordComponent* Store = RecordStore.Get())
		{
			Store->UnbindActor(RecordId, this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AStrategyUnit::PostActorCreated()
{
	Super::PostActorCreated();

#if WITH_EDITOR
	// a unit newly placed in the editor gets its record key now, so nobody has to remember to
	// author one. Game worlds are skipped: a unit spawned at runtime isn't placed, and gets a
	// fresh record rather than a key that pretends it was.
	const UWorld* World = GetWorld();

	if (World && !World->IsGameWorld() && !PlacedRecordId.IsValid() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		PlacedRecordId = FGuid::NewGuid();
	}
#endif
}

#if WITH_EDITOR
void AStrategyUnit::PostEditImport()
{
	Super::PostEditImport();

	// a pasted or alt-dragged copy is a different person - two placed actors sharing a key would
	// fight over one record, and the second would be refused its binding at BeginPlay
	PlacedRecordId = FGuid::NewGuid();
}
#endif

void AStrategyUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStrategyUnit, UnitDisplayName);
	DOREPLIFETIME(AStrategyUnit, FactionId);
}

UTexture2D* AStrategyUnit::GetPortraitTexture() const
{
	if (PortraitTexture)
	{
		return PortraitTexture;
	}

	return CharacterDefinition ? CharacterDefinition->Portrait.Get() : nullptr;
}

const FCharacterRecord* AStrategyUnit::GetRecord() const
{
	const UCharacterRecordComponent* Store = RecordStore.Get();

	return Store ? Store->FindRecord(RecordId) : nullptr;
}

void AStrategyUnit::RegisterWithRecordStore()
{
	// records are server-owned; a client sees this unit through its replicated components
	if (!HasAuthority())
	{
		return;
	}

	UCharacterRecordComponent* Store = UCharacterRecordComponent::Get(this);

	if (!Store)
	{
		// the main menu, or a test world with no store - run as a plain actor, as before records existed
		UE_LOG(LogSmoresCharacters, Log, TEXT("%s found no character record store - running without a record."), *GetName());
		return;
	}

	FGuid Key = PlacedRecordId;

	if (!Key.IsValid() && IsNetStartupActor())
	{
		// placed in the level but never given a key - it works, but a save could never find it again
		UE_LOG(LogSmoresCharacters, Warning, TEXT("%s is placed in the level with no PlacedRecordId - it gets a fresh record every session. Author one."), *GetName());
	}

	// adopt: a record already exists for this key, so this actor is its puppet and the record wins
	if (Store->FindRecord(Key))
	{
		if (Store->BindActor(Key, this))
		{
			RecordId = Key;
			RecordStore = Store;

			ApplyRecordToActor();

			return;
		}

		// another live actor already stands in for that record - two placed units sharing a key
		UE_LOG(LogSmoresCharacters, Error, TEXT("%s's PlacedRecordId %s is already bound to %s - giving this one a fresh record instead."),
			*GetName(), *Key.ToString(), *GetNameSafe(Store->GetBoundActor(Key)));

		Key.Invalidate();
	}

	if (!CharacterDefinition)
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("%s has no CharacterDefinition - its record has no definition, attributes or faction."), *GetName());
	}

	// create: identity from the definition, named by the level designer if they named this one
	const FGuid NewId = Store->CreateRecord(CharacterDefinition, Key, UnitDisplayName);

	if (!NewId.IsValid() || !Store->BindActor(NewId, this))
	{
		// CreateRecord has already said why (a second copy of a unique character, most likely)
		UE_LOG(LogSmoresCharacters, Error, TEXT("%s could not be given a record - running without one."), *GetName());
		return;
	}

	RecordId = NewId;
	RecordStore = Store;

	const FCharacterRecord* Record = Store->FindRecord(NewId);

	UnitDisplayName = Record->Name;
	FactionId = Record->FactionId;

	// the loadout goes through the ordinary grid rules; each AddItem writes itself back
	if (CharacterDefinition)
	{
		for (const FInventoryItem& Item : CharacterDefinition->DefaultLoadout)
		{
			Inventory->AddItem(Item);
		}
	}

	// and the condition half: health, location, and an empty-handed character's empty grid
	WriteBackToRecord();
}

void AStrategyUnit::WriteBackToRecord()
{
	if (!HasAuthority() || bApplyingRecord || !RecordId.IsValid())
	{
		return;
	}

	UCharacterRecordComponent* Store = RecordStore.Get();
	FCharacterRecord* Record = Store ? Store->EditRecord(RecordId) : nullptr;

	if (!Record)
	{
		return;
	}

	Record->Health = Health->GetHealth();
	Record->LifeState = Health->GetHealthState();
	Record->LastKnownLocation = GetActorLocation();
	Record->Carried = Inventory->GetEntries();
	Record->Equipped = Equipment->GetEquippedItems();
}

void AStrategyUnit::ApplyRecordToActor()
{
	if (!HasAuthority())
	{
		return;
	}

	const FCharacterRecord* Record = GetRecord();

	if (!Record)
	{
		return;
	}

	// a copy: the component broadcasts below run this unit's handlers, and nothing should be
	// reading through a pointer into the store while they do
	const FCharacterRecord Stored = *Record;

	TGuardValue<bool> ApplyingGuard(bApplyingRecord, true);

	UnitDisplayName = Stored.Name;
	FactionId = Stored.FactionId;

	SetActorLocation(Stored.LastKnownLocation, false, nullptr, ETeleportType::TeleportPhysics);

	Inventory->RestoreEntries(Stored.Carried);
	Equipment->RestoreEquippedItems(Stored.Equipped);

	// health last, so a unit restored as Dead goes inert holding the pack it died with
	Health->RestoreState(Stored.Health, Stored.LifeState);
}

void AStrategyUnit::OnWorkingCopyChanged()
{
	WriteBackToRecord();
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
		// a Downed or Dead unit stays in its grounded pose - don't rotate to face whoever's
		// interacting with it
		if (!IsIncapacitated())
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

	// an incapacitated unit can't move - without this guard, a move command issued while Downed
	// or Dead still kicks off the EQS/AIController move, which visibly slides the grounded unit around
	if (IsIncapacitated())
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s MoveToLocation bailed early: unit is %s"),
			*GetName(), IsDead() ? TEXT("Dead") : TEXT("Downed"));
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
	// arrival is where a move changes the record - LastKnownLocation isn't written every frame
	WriteBackToRecord();

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

bool AStrategyUnit::IsDead() const
{
	return Health->IsDead();
}

bool AStrategyUnit::IsIncapacitated() const
{
	return Health->IsIncapacitated();
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

bool AStrategyUnit::IsInRangeOf(const AActor* Other) const
{
	return IInventoryHolder::IsActorWithinSphere(this, InteractionRange, Other);
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

		if (!IsValid(CurrentUnit) || CurrentUnit->IsIncapacitated())
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

void AStrategyUnit::OnHealthDied()
{
	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthDied"), *GetName());

	// identical treatment to going Downed - stop moving, drop out of every attack loop, play the
	// grounded pose. What makes it death rather than a knockdown is entirely that no OnRecovered
	// is coming, so nothing here ever gets undone.
	StopMoving();

	PendingAttackTarget = nullptr;
	bAttackOnArrival = false;
	Combat->NotifyOwnerDowned();

	GetWorldTimerManager().ClearTimer(AggroRetargetTimerHandle);
}

void AStrategyUnit::OnHealthDamaged(AActor* DamageInstigator)
{
	WriteBackToRecord();

	// already mid-engagement (fighting in range, or moving in to engage) - don't hijack an
	// existing attack order onto whoever just landed a hit
	if (Combat->GetCurrentAttackTarget() || bAttackOnArrival)
	{
		return;
	}

	AStrategyUnit* Attacker = Cast<AStrategyUnit>(DamageInstigator);

	if (!IsValid(Attacker) || Attacker->IsIncapacitated())
	{
		return;
	}

	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthDamaged: auto-retaliating against %s"), *GetName(), *Attacker->GetName());

	AttackTarget(Attacker);
}
