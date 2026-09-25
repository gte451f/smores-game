// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "BarkDirectorComponent.h"
#include "SmoresDialog.h"
#include "SmoresDialogSubsystem.h"
#include "DialogHost.h"
#include "DialogLibrary.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "HealthComponent.h"
#include "WorldSeedComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UBarkUnitWatcher::Watch(AStrategyUnit* InUnit, UBarkDirectorComponent* InDirector)
{
	Unwatch();

	Unit = InUnit;
	Director = InDirector;

	UHealthComponent* Health = InUnit ? InUnit->GetHealth() : nullptr;

	// a unit that can't be hurt has nothing to bark about, which is legitimate rather than wrong
	if (!Health)
	{
		return;
	}

	Health->OnDamaged.AddUniqueDynamic(this, &UBarkUnitWatcher::HandleDamaged);
	Health->OnDowned.AddUniqueDynamic(this, &UBarkUnitWatcher::HandleDowned);
	Health->OnDied.AddUniqueDynamic(this, &UBarkUnitWatcher::HandleDied);
}

void UBarkUnitWatcher::Unwatch()
{
	if (AStrategyUnit* CurrentUnit = Unit.Get())
	{
		if (UHealthComponent* Health = CurrentUnit->GetHealth())
		{
			Health->OnDamaged.RemoveDynamic(this, &UBarkUnitWatcher::HandleDamaged);
			Health->OnDowned.RemoveDynamic(this, &UBarkUnitWatcher::HandleDowned);
			Health->OnDied.RemoveDynamic(this, &UBarkUnitWatcher::HandleDied);
		}
	}

	Unit = nullptr;
}

void UBarkUnitWatcher::HandleDamaged(AActor* DamageInstigator)
{
	if (UBarkDirectorComponent* CurrentDirector = Director.Get())
	{
		CurrentDirector->HandleUnitHurt(Unit.Get(), DamageInstigator);
	}
}

void UBarkUnitWatcher::HandleDowned()
{
	if (UBarkDirectorComponent* CurrentDirector = Director.Get())
	{
		CurrentDirector->HandleUnitDowned(Unit.Get());
	}
}

void UBarkUnitWatcher::HandleDied()
{
	if (UBarkDirectorComponent* CurrentDirector = Director.Get())
	{
		CurrentDirector->HandleUnitDied(Unit.Get());
	}
}

UBarkDirectorComponent::UBarkDirectorComponent()
{
	// everything here is a reaction to an event; nothing needs a frame
	PrimaryComponentTick.bCanEverTick = false;

	// server-only - clients receive line ids through IDialogHost, never this component's state
	SetIsReplicatedByDefault(false);
}

UBarkDirectorComponent* UBarkDirectorComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	return GameState ? GameState->FindComponentByClass<UBarkDirectorComponent>() : nullptr;
}

void UBarkDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	// a client's copy of the GameState carries this component too, and must do nothing with it
	if (!Owner || !Owner->HasAuthority() || !World)
	{
		return;
	}

	Stream.Initialize(UWorldSeedComponent::GetWorldSeedFor(this));

	for (TActorIterator<AStrategyUnit> It(World); It; ++It)
	{
		WatchUnit(*It);
	}

	// the units loaded so far are all here now; anything spawned later announces itself here, and
	// anything streamed in later arrives with its level
	ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UBarkDirectorComponent::HandleActorSpawned));
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UBarkDirectorComponent::HandleLevelAdded);

	if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
	{
		LibraryLoadedHandle = Dialog->OnLibraryLoaded.AddUObject(this, &UBarkDirectorComponent::HandleLibraryLoaded);
	}
}

void UBarkDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);

	if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
	{
		Dialog->OnLibraryLoaded.Remove(LibraryLoadedHandle);
	}

	for (UBarkUnitWatcher* Watcher : Watchers)
	{
		if (Watcher)
		{
			Watcher->Unwatch();
		}
	}

	Watchers.Reset();

	Super::EndPlay(EndPlayReason);
}

void UBarkDirectorComponent::WatchUnit(AStrategyUnit* Unit)
{
	if (!IsValid(Unit))
	{
		return;
	}

	// drop watchers whose unit has gone, so a long session doesn't accumulate them
	Watchers.RemoveAll([](const UBarkUnitWatcher* Watcher)
	{
		return !Watcher || !IsValid(Watcher->GetUnit());
	});

	const bool bAlreadyWatched = Watchers.ContainsByPredicate([Unit](const UBarkUnitWatcher* Watcher)
	{
		return Watcher->GetUnit() == Unit;
	});

	if (bAlreadyWatched)
	{
		return;
	}

	UBarkUnitWatcher* Watcher = NewObject<UBarkUnitWatcher>(this);
	Watcher->Watch(Unit, this);

	Watchers.Add(Watcher);
}

void UBarkDirectorComponent::HandleActorSpawned(AActor* Actor)
{
	WatchUnit(Cast<AStrategyUnit>(Actor));
}

void UBarkDirectorComponent::HandleLevelAdded(ULevel* Level, UWorld* World)
{
	// the delegate is global - every world's levels arrive here, and only ours are our business
	if (!Level || World != GetWorld())
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		WatchUnit(Cast<AStrategyUnit>(Actor));
	}
}

void UBarkDirectorComponent::HandleLibraryLoaded()
{
	Memory.Reset();
}

void UBarkDirectorComponent::HandleUnitHurt(AStrategyUnit* Unit, AActor* DamageInstigator)
{
	// Hurt fires only for a hit the unit survived, so it is still on its feet to complain about it
	if (!IsValid(Unit) || Unit->IsIncapacitated())
	{
		return;
	}

	// whoever landed the hit is who it's said to, and their player (if they have one) is whose
	// standing counts
	RaiseEvent(EBarkEvent::Hurt, Unit, DamageInstigator, GetOwningPlayerState(DamageInstigator));
}

void UBarkDirectorComponent::HandleUnitDowned(AStrategyUnit* Unit)
{
	if (IsValid(Unit))
	{
		RaiseEvent(EBarkEvent::Downed, Unit, nullptr, nullptr);
	}
}

void UBarkDirectorComponent::HandleUnitDied(AStrategyUnit* Unit)
{
	if (IsValid(Unit))
	{
		RaiseWitnessedDeath(Unit);
	}
}

FName UBarkDirectorComponent::RaiseEvent(EBarkEvent Event, AActor* Speaker, AActor* Listener, APlayerState* Player, TArray<FString>* OutExplanation, bool bIgnoreQuietTime)
{
	const AActor* Owner = GetOwner();

	if (!Owner || !Owner->HasAuthority() || !IsValid(Speaker))
	{
		return NAME_None;
	}

	FDialogContext Context;
	Context.Event = Event;
	Context.Speaker = Speaker;
	Context.Listener = Listener;
	Context.Player = Player;

	return SayIfAny(Context, Speaker, bIgnoreQuietTime, OutExplanation);
}

FName UBarkDirectorComponent::RaiseWitnessedDeath(AActor* Victim, TArray<FString>* OutExplanation, bool bIgnoreQuietTime)
{
	const AActor* Owner = GetOwner();
	const AStrategyUnit* VictimUnit = Cast<AStrategyUnit>(Victim);
	UWorld* World = GetWorld();

	if (!Owner || !Owner->HasAuthority() || !VictimUnit || !World)
	{
		return NAME_None;
	}

	const FVector VictimLocation = VictimUnit->GetActorLocation();
	TArray<AStrategyUnit*> Witnesses;

	for (TActorIterator<AStrategyUnit> It(World); It; ++It)
	{
		AStrategyUnit* Candidate = *It;

		if (Candidate != VictimUnit && !Candidate->IsIncapacitated() && AreAllies(VictimUnit, Candidate)
			&& FVector::Dist(Candidate->GetActorLocation(), VictimLocation) <= WitnessRange)
		{
			Witnesses.Add(Candidate);
		}
	}

	if (Witnesses.Num() == 0)
	{
		if (OutExplanation)
		{
			OutExplanation->Add(FString::Printf(TEXT("WitnessedDeath: no ally of %s within %.0fcm is on their feet to react"),
				*VictimUnit->GetHolderDisplayName().ToString(), WitnessRange));
		}

		return NAME_None;
	}

	Witnesses.Sort([&VictimLocation](const AStrategyUnit& A, const AStrategyUnit& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), VictimLocation) < FVector::DistSquared(B.GetActorLocation(), VictimLocation);
	});

	// the nearest ally with something to say says it; one reaction per death is plenty
	for (AStrategyUnit* Witness : Witnesses)
	{
		FDialogContext Context;
		Context.Event = EBarkEvent::WitnessedDeath;
		Context.Speaker = Witness;
		Context.Victim = Victim;

		const FName Said = SayIfAny(Context, Witness, bIgnoreQuietTime, OutExplanation);

		if (!Said.IsNone())
		{
			return Said;
		}
	}

	return NAME_None;
}

FName UBarkDirectorComponent::SayIfAny(const FDialogContext& Context, AActor* Speaker, bool bIgnoreQuietTime, TArray<FString>* OutExplanation)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const UWorld* World = GetWorld();

	auto Explain = [OutExplanation](const FString& Line)
	{
		if (OutExplanation)
		{
			OutExplanation->Add(Line);
		}
	};

	const AStrategyUnit* SpeakerUnit = Cast<AStrategyUnit>(Speaker);
	const FString SpeakerName = SpeakerUnit ? SpeakerUnit->GetHolderDisplayName().ToString() : GetNameSafe(Speaker);

	// the explanation is built only when asked for - this runs on every hit in every fight
	if (OutExplanation)
	{
		const AStrategyUnit* ListenerUnit = Cast<AStrategyUnit>(Context.Listener.Get());
		const AStrategyUnit* VictimUnit = Cast<AStrategyUnit>(Context.Victim.Get());

		OutExplanation->Add(FString::Printf(TEXT("%s: speaker %s, listener %s, victim %s, player %s"),
			*SmoresDialog::GetBarkEventName(Context.Event), *SpeakerName,
			ListenerUnit ? *ListenerUnit->GetHolderDisplayName().ToString() : TEXT("(none)"),
			VictimUnit ? *VictimUnit->GetHolderDisplayName().ToString() : TEXT("(none)"),
			Context.Player.IsValid() ? *Context.Player->GetPlayerName() : TEXT("(none)")));
	}

	if (!Dialog || !World)
	{
		Explain(TEXT("  nothing said: no dialog is loaded in this world"));
		return NAME_None;
	}

	const double Now = World->GetTimeSeconds();
	const FObjectKey SpeakerKey(Speaker);
	const double SinceSpoke = Now - Memory.GetLastSpoke(SpeakerKey);

	if (SinceSpoke < SpeakerQuietSeconds)
	{
		if (!bIgnoreQuietTime)
		{
			if (OutExplanation)
			{
				OutExplanation->Add(FString::Printf(TEXT("  nothing said: %s spoke %.1fs ago and stays quiet for %.0fs"), *SpeakerName, SinceSpoke, SpeakerQuietSeconds));
			}

			return NAME_None;
		}

		if (OutExplanation)
		{
			OutExplanation->Add(FString::Printf(TEXT("  (ignoring the %.0fs quiet time - %s spoke %.1fs ago)"), SpeakerQuietSeconds, *SpeakerName, SinceSpoke));
		}
	}

	const TArray<const FBarkLine*> Lines = Dialog->GetLibrary().GetBarksForEvent(Context.Event);

	if (Lines.Num() == 0)
	{
		Explain(TEXT("  nothing said: no line is written for this event"));
		return NAME_None;
	}

	FBarkSelection Selection;
	const FBarkLine* Winner = SmoresDialog::SelectBark(Lines, Context, Dialog->GetFacts(), Memory, SpeakerKey, Now, Stream, OutExplanation ? &Selection : nullptr);

	if (OutExplanation)
	{
		for (const FString& Line : Selection.Explain())
		{
			Explain(TEXT("  ") + Line);
		}
	}

	if (!Winner)
	{
		Explain(TEXT("  nothing said: no line both matched and was off cooldown"));
		return NAME_None;
	}

	Memory.Record(*Winner, SpeakerKey, Now);
	Deliver(Winner->Id, Speaker);

	UE_LOG(LogSmoresDialog, Verbose, TEXT("Bark %s by %s"), *Winner->Id.ToString(), *SpeakerName);

	return Winner->Id;
}

void UBarkDirectorComponent::Deliver(FName LineId, const AActor* Speaker) const
{
	const UWorld* World = GetWorld();

	if (!World || !Speaker)
	{
		return;
	}

	const AStrategyUnit* SpeakerUnit = Cast<AStrategyUnit>(Speaker);
	const FText SpeakerName = SpeakerUnit ? SpeakerUnit->GetHolderDisplayName() : FText::GetEmpty();
	const FVector Location = Speaker->GetActorLocation();

	// every player, not "the" player - each one within hearing gets their own copy of the same line
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		IDialogHost* Host = Cast<IDialogHost>(PlayerController);

		if (Host && Host->IsSquadMemberWithin(Location, HearingRange))
		{
			Host->DeliverBark(LineId, SpeakerName);
		}
	}
}

APlayerState* UBarkDirectorComponent::GetOwningPlayerState(const AActor* Unit)
{
	const AStrategyPlayerUnit* PlayerUnit = Cast<AStrategyPlayerUnit>(Unit);
	const APlayerController* Controller = PlayerUnit ? PlayerUnit->GetOwningController() : nullptr;

	return Controller ? Controller->PlayerState : nullptr;
}

bool UBarkDirectorComponent::AreAllies(const AStrategyUnit* Victim, const AStrategyUnit* Candidate)
{
	if (!Victim || !Candidate)
	{
		return false;
	}

	const AStrategyPlayerUnit* VictimSquad = Cast<AStrategyPlayerUnit>(Victim);
	const AStrategyPlayerUnit* CandidateSquad = Cast<AStrategyPlayerUnit>(Candidate);

	// a squad's allies are that same player's squad, and nobody else - squad members are
	// unaffiliated, so faction can't be what decides it
	if (VictimSquad || CandidateSquad)
	{
		return VictimSquad && CandidateSquad && VictimSquad->GetOwningController()
			&& VictimSquad->GetOwningController() == CandidateSquad->GetOwningController();
	}

	return !Victim->GetFactionId().IsNone() && Victim->GetFactionId() == Candidate->GetFactionId();
}
