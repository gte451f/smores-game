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
#include "HealthComponent.h"
#include "CombatComponent.h"
#include "StrategyPlayerUnit.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "CharacterDefinition.h"
#include "CharacterRecordComponent.h"
#include "ActionOrderComponent.h"
#include "Net/UnrealNetwork.h"
#include "SmoresCharacters.h"

#define LOCTEXT_NAMESPACE "StrategyUnit"

AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	// Looting reach, measured centre-to-centre (see ISmoresInteractable::IsActorWithinSphere).
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

	// create the action order component - the walk-over-to-act order, server-side only
	ActionOrder = CreateDefaultSubobject<UActionOrderComponent>(TEXT("ActionOrder"));

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
	// AnimInstance::OnMontageEnded subscription. Switching the mesh's Animation Mode (as the
	// template's arrival interaction did with PlayAnimation/SetAnimationMode, since removed) destroys and recreates
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
	DOREPLIFETIME(AStrategyUnit, Attributes);
	DOREPLIFETIME(AStrategyUnit, Disposition);
}

bool AStrategyUnit::IsPerson() const
{
	// no definition is the "predates definitions" case, and every unit like that is a person
	return !CharacterDefinition || CharacterDefinition->Kind == ECharacterKind::Person;
}

FText AStrategyUnit::GetConditionText() const
{
	// words, not a figure - the squad can see someone is hurt, not read their health off them
	if (IsDead())
	{
		return LOCTEXT("ConditionDead", "Dead.");
	}

	if (IsDowned())
	{
		return LOCTEXT("ConditionDowned", "Down, but still breathing.");
	}

	const float MaxHealth = Health ? Health->MaxHealth : 0.0f;
	const float Fraction = MaxHealth > 0.0f ? Health->GetHealth() / MaxHealth : 1.0f;

	if (Fraction < 0.5f)
	{
		return LOCTEXT("ConditionBadlyHurt", "Badly hurt.");
	}

	if (Fraction < 1.0f)
	{
		return LOCTEXT("ConditionHurt", "Hurt.");
	}

	return LOCTEXT("ConditionUnhurt", "Unhurt.");
}

FText AStrategyUnit::GetExamineText() const
{
	TArray<FText> Paragraphs;

	if (CharacterDefinition)
	{
		if (!CharacterDefinition->Description.IsEmpty())
		{
			Paragraphs.Add(CharacterDefinition->Description);
		}

		// a creature has no story to tell, only what it looks like
		if (IsPerson() && !CharacterDefinition->Backstory.IsEmpty())
		{
			Paragraphs.Add(CharacterDefinition->Backstory);
		}
	}

	Paragraphs.Add(GetConditionText());

	return FText::Join(FText::FromString(TEXT("\n\n")), Paragraphs);
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
		// the main menu, or a test world with no store - run as a plain actor, as before records
		// existed. The definition's attributes still stand in for a record's, so what this unit's
		// numbers make likely reads the same with or without one.
		if (CharacterDefinition)
		{
			Attributes = CharacterDefinition->BaseAttributes;
		}

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
	Attributes = Record->Attributes;

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
	Attributes = Stored.Attributes;

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

void AStrategyUnit::FaceToward(const AActor* Other)
{
	// a Downed or Dead unit stays in its grounded pose, and nothing turns to face itself
	if (!IsValid(Other) || Other == this || IsIncapacitated())
	{
		return;
	}

	FRotator Facing = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Other->GetActorLocation());
	Facing.Pitch = 0.0f;
	Facing.Roll = 0.0f;

	SetActorRotation(Facing);
}

void AStrategyUnit::MoveToLocation(const FVector& Location, bool bInLeadUnit)
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

	// the catch-all: whoever called this means the unit is now doing something else. Cancelled
	// before StopMoving below, which would otherwise report the order's walk ending.
	ActionOrder->CancelOrder();

	// an earlier query still running would land after this one and send the unit to the old point
	DropPendingMoveQuery();

	// cache the movement parameters
	CurrentMovementGoal = Location;
	bLeadUnit = bInLeadUnit;

	// a new movement command always interrupts any live or pending attack engagement
	bAttackOnArrival = false;
	PendingAttackTarget = nullptr;
	Combat->ClearCurrentAttackTarget();

	// stop movement
	StopMoving();

	// choose the EnvQuery to use
	UEnvQuery* MoveQuery = bLeadUnit ? InteractionQuery : NoInteractionQuery;

	// choose the run mode to use. The lead unit gets the closest result, all others choose randomly from top 25%
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = bLeadUnit ? EEnvQueryRunMode::SingleResult : EEnvQueryRunMode::RandomBest25Pct;

	// run an EQS to resolve the movement destination using the NavMesh
	EnvQueryInstance = UEnvQueryManager::RunEQSQuery(this, MoveQuery, this,  RunMode, UEnvQueryInstanceBlueprintWrapper::StaticClass());

	if (IsValid(EnvQueryInstance))
	{
		EnvQueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &AStrategyUnit::OnEQSFinished);
	}
}

EActionApproachResult AStrategyUnit::MoveToActor(AActor* Goal, float AcceptanceRadius)
{
	// the same guards as MoveToLocation: shared movement state is the server's, and a grounded
	// unit doesn't slide about
	if (!HasAuthority() || !IsValid(Goal) || IsIncapacitated() || !AIController)
	{
		return EActionApproachResult::Failed;
	}

	// a MoveToLocation query still in flight would otherwise answer after this and hijack the walk
	DropPendingMoveQuery();

	// clears attack state the way MoveToLocation does - this walk is its own thing
	bAttackOnArrival = false;
	PendingAttackTarget = nullptr;
	Combat->ClearCurrentAttackTarget();

	// Forgotten *before* MoveTo, not after: MoveTo aborts whatever request is running and that
	// abort reports synchronously, from inside the call. A retry would otherwise hear its own
	// previous walk "finishing" and retry again from within itself.
	ActionMoveRequestId = FAIRequestID::InvalidRequest;

	CurrentMovementGoal = Goal->GetActorLocation();

	FAIMoveRequest MoveReq(Goal);

	MoveReq.SetAcceptanceRadius(AcceptanceRadius);

	// the gap is measured to the target's centre, not its edge: a container or a door can report
	// its reach sphere as part of its size, which would stop the walk well short of that reach
	MoveReq.SetReachTestIncludesGoalRadius(false);
	MoveReq.SetReachTestIncludesAgentRadius(true);

	// a path that can't reach all the way still walks as close as it can - the order checks reach
	// on arrival and decides between acting, trying again and CannotReach
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetRequireNavigableEndLocation(false);
	MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
	MoveReq.SetCanStrafe(false);

	const FPathFollowingRequestResult Result = AIController->MoveTo(MoveReq);

	switch (Result.Code)
	{
	case EPathFollowingRequestResult::RequestSuccessful:
		ActionMoveRequestId = Result.MoveId;
		return EActionApproachResult::Walking;

	case EPathFollowingRequestResult::AlreadyAtGoal:
		// no request is made, so no finished callback will ever come - the order handles it at once
		return EActionApproachResult::AlreadyThere;

	case EPathFollowingRequestResult::Failed:
	default:
		return EActionApproachResult::Failed;
	}
}

void AStrategyUnit::StopActionApproach()
{
	if (!ActionMoveRequestId.IsValid())
	{
		return;
	}

	// forgotten first, so the abort this reports is recognised as nobody's arrival
	ActionMoveRequestId = FAIRequestID::InvalidRequest;

	if (AIController)
	{
		AIController->StopMovement();
	}
}

void AStrategyUnit::TakeOverForActionOrder()
{
	if (!HasAuthority())
	{
		return;
	}

	DropPendingMoveQuery();

	bAttackOnArrival = false;
	PendingAttackTarget = nullptr;
	Combat->ClearCurrentAttackTarget();

	ActionMoveRequestId = FAIRequestID::InvalidRequest;

	StopMoving();
}

void AStrategyUnit::DropPendingMoveQuery()
{
	if (IsValid(EnvQueryInstance))
	{
		EnvQueryInstance->GetOnQueryFinishedEvent().RemoveDynamic(this, &AStrategyUnit::OnEQSFinished);
	}

	EnvQueryInstance = nullptr;
}

FVector AStrategyUnit::GetMovementGoal() const
{
	return CurrentMovementGoal;
}

void AStrategyUnit::OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	// a query that has since been replaced (a newer move) or dropped (an action order took over)
	// answers a question nobody is asking any more
	if (!QueryInstance || QueryInstance != EnvQueryInstance)
	{
		return;
	}

	EnvQueryInstance = nullptr;

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
	// only the action order's own walk is its business. Aborts are reported too, and a walk
	// replaced by something else has already had ActionMoveRequestId cleared by whatever replaced it.
	const bool bWasActionApproach = ActionMoveRequestId.IsValid() && RequestID == ActionMoveRequestId;

	if (bWasActionApproach)
	{
		ActionMoveRequestId = FAIRequestID::InvalidRequest;
	}

	HandleMoveFinished();

	if (bWasActionApproach && ActionOrder->HasOrder())
	{
		ActionOrder->HandleApproachFinished();
	}
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
	return ISmoresInteractable::IsActorWithinSphere(this, InteractionRange, Other);
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
	// an attack is something else to do, so any action order ends - silently, because every path
	// here is either the player's own attack order or combat already running. Retaliation, the one
	// that isn't, has ended the order with a notice before it gets here.
	ActionOrder->CancelOrder();

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
	MoveToLocation(TargetUnit->GetActorLocation(), /*bLeadUnit*/ false);

	bAttackOnArrival = true;
	PendingAttackTarget = TargetUnit;
}

void AStrategyUnit::OnHealthDowned()
{
	UE_LOG(LogSmoresCharacters, Warning, TEXT("[Combat] %s OnHealthDowned"), *GetName());

	// before StopMoving, which would otherwise report the order's walk ending as if it had arrived
	ActionOrder->AbandonOrder(EActionOrderEnd::ActorDown);

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
	ActionOrder->AbandonOrder(EActionOrderEnd::ActorDown);

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

	// the player sent this unit to do something and it stopped to fight instead - that ending
	// gets said, unlike the player's own new orders, which end an order silently
	ActionOrder->AbandonOrder(EActionOrderEnd::ActorFighting);

	AttackTarget(Attacker);
}

#undef LOCTEXT_NAMESPACE
