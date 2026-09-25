// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "BanterDirectorComponent.h"
#include "BarkDirectorComponent.h"
#include "ConversationComponent.h"
#include "ConversationSelection.h"
#include "DialogEffects.h"
#include "DialogHost.h"
#include "DialogMemoryComponent.h"
#include "SmoresDialog.h"
#include "SmoresDialogSubsystem.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "HealthComponent.h"
#include "CombatComponent.h"
#include "WorldSeedComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"

namespace
{
	APlayerState* GetBanterPlayerState(const AStrategyPlayerUnit* Unit)
	{
		const APlayerController* Controller = Unit ? Unit->GetOwningController() : nullptr;
		return Controller ? Controller->PlayerState : nullptr;
	}

	float GetBanterHealth(const AStrategyUnit* Unit)
	{
		const UHealthComponent* Health = Unit ? Unit->GetHealth() : nullptr;
		return Health ? Health->GetHealth() : 0.0f;
	}

	bool IsBanterFighting(const AStrategyUnit* Unit)
	{
		const UCombatComponent* Combat = Unit ? Unit->GetCombat() : nullptr;
		return Combat && Combat->GetCurrentAttackTarget() != nullptr;
	}
}

UBanterDirectorComponent::UBanterDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// server-only - what reaches a client is each line, through the bark director's delivery
	SetIsReplicatedByDefault(false);
}

UBanterDirectorComponent::~UBanterDirectorComponent() = default;

UBanterDirectorComponent* UBanterDirectorComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	return GameState ? GameState->FindComponentByClass<UBanterDirectorComponent>() : nullptr;
}

float UBanterDirectorComponent::GetReadingSeconds(int32 Characters)
{
	return FMath::Min(2.0f + 0.06f * static_cast<float>(FMath::Max(0, Characters)), 6.0f);
}

void UBanterDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	// a client's copy of the GameState carries this component too, and must do nothing with it
	if (!Owner || !Owner->HasAuthority() || !World)
	{
		return;
	}

	Stream.Initialize(UWorldSeedComponent::GetWorldSeedFor(this) ^ 0x62616E74);

	if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
	{
		WillReloadHandle = Dialog->OnLibraryWillReload.AddUObject(this, &UBanterDirectorComponent::HandleLibraryWillReload);
	}

	if (CheckSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(CheckTimerHandle, this, &UBanterDirectorComponent::CheckSquads, CheckSeconds, /*bLoop*/ true);
	}
}

void UBanterDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
	{
		Dialog->OnLibraryWillReload.Remove(WillReloadHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);

		for (const TUniquePtr<FRunningBanter>& Banter : Running)
		{
			World->GetTimerManager().ClearTimer(Banter->LineTimer);
		}
	}

	Running.Reset();

	Super::EndPlay(EndPlayReason);
}

void UBanterDirectorComponent::HandleLibraryWillReload()
{
	// a banter holds its old script alive, but its lines may be gone from the new library
	TArray<TWeakObjectPtr<APlayerState>> Players;

	for (const TUniquePtr<FRunningBanter>& Banter : Running)
	{
		Players.Add(Banter->PlayerState);
	}

	for (const TWeakObjectPtr<APlayerState>& Player : Players)
	{
		StopBanter(Player.Get());
	}
}

void UBanterDirectorComponent::CheckSquads()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();

	// every squad, grouped by whose it is
	TMap<APlayerState*, TArray<AStrategyPlayerUnit*>> MembersByPlayer;

	for (TActorIterator<AStrategyPlayerUnit> It(World); It; ++It)
	{
		if (APlayerState* Player = GetBanterPlayerState(*It))
		{
			MembersByPlayer.FindOrAdd(Player).Add(*It);
		}
	}

	for (TPair<APlayerState*, TArray<AStrategyPlayerUnit*>>& Pair : MembersByPlayer)
	{
		APlayerState* Player = Pair.Key;
		FSquadBanter* Squad = Squads.Find(Player);

		if (!Squad)
		{
			// first sight of a squad: nothing to banter about the moment it arrives
			Squad = &Squads.Add(Player);
			Squad->NextIdleTime = Now + IdleSeconds;
		}

		// fighting: somebody has a target, or somebody lost health since the last look
		bool bFighting = false;

		for (AStrategyPlayerUnit* Member : Pair.Value)
		{
			const float Health = GetBanterHealth(Member);
			const float* LastHealth = Squad->LastHealth.Find(Member);

			bFighting |= IsBanterFighting(Member) || (LastHealth && Health < *LastHealth);
			Squad->LastHealth.Add(Member, Health);
		}

		if (bFighting)
		{
			Squad->bWasFighting = true;
			Squad->CalmSince = Now;

			// a banter under way stops when a fight starts
			StopBanter(Player);
			continue;
		}

		if (FindRunning(Player))
		{
			continue;
		}

		const bool bAfterFight = Squad->bWasFighting && Now - Squad->CalmSince >= AfterFightSeconds;
		const bool bIdle = Now >= Squad->NextIdleTime;

		if (!bAfterFight && !bIdle)
		{
			continue;
		}

		// a fight that ended while the squad was on cooldown gets no banter of its own
		Squad->bWasFighting = false;

		if (Now - Squad->LastBanterTime < CooldownSeconds)
		{
			Squad->NextIdleTime = FMath::Max(Squad->NextIdleTime, Squad->LastBanterTime + CooldownSeconds);
			continue;
		}

		Squad->NextIdleTime = Now + IdleSeconds;
		TryBanter(Player);
	}

	// forget squads that have gone, and units that have
	for (auto It = Squads.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		for (auto HealthIt = It.Value().LastHealth.CreateIterator(); HealthIt; ++HealthIt)
		{
			if (!HealthIt.Key().IsValid())
			{
				HealthIt.RemoveCurrent();
			}
		}
	}
}

TArray<AStrategyUnit*> UBanterDirectorComponent::GetAvailableMembers(const APlayerState* Player) const
{
	TArray<AStrategyUnit*> Members;
	const UWorld* World = GetWorld();

	if (!World || !Player)
	{
		return Members;
	}

	// a squad member already talking at a conversation window is busy; the rest of the squad isn't
	const UConversationComponent* Conversation = UConversationComponent::Get(Player->GetPlayerController());
	const AStrategyUnit* Busy = Conversation && Conversation->IsInConversation() ? Conversation->GetConversationSquadMember() : nullptr;

	for (TActorIterator<AStrategyPlayerUnit> It(World); It; ++It)
	{
		if (GetBanterPlayerState(*It) == Player && !It->IsIncapacitated() && *It != Busy)
		{
			Members.Add(*It);
		}
	}

	// a stable order first, so the shuffle below is the only thing that varies
	Members.Sort([](const AStrategyUnit& A, const AStrategyUnit& B)
	{
		return A.GetName() < B.GetName();
	});

	return Members;
}

TArray<AStrategyUnit*> UBanterDirectorComponent::FindCast(const FConversationDefinition& Conversation, APlayerState* Player, const TArray<AStrategyUnit*>& Members, bool bStrict) const
{
	const int32 Needed = Conversation.Participants.Num();

	if (Needed < 2 || Members.Num() < Needed)
	{
		return {};
	}

	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);

	if (!Dialog)
	{
		return {};
	}

	// who goes first varies from banter to banter, so it isn't always the same two talking
	TArray<AStrategyUnit*> Order = Members;

	for (int32 Index = Order.Num() - 1; Index > 0; --Index)
	{
		Order.Swap(Index, Stream.RandRange(0, Index));
	}

	const float RangeSquared = FMath::Square(CastRange);

	for (AStrategyUnit* First : Order)
	{
		for (AStrategyUnit* Second : Order)
		{
			if (Second == First || (bStrict && FVector::DistSquared(First->GetActorLocation(), Second->GetActorLocation()) > RangeSquared))
			{
				continue;
			}

			if (bStrict)
			{
				FDialogContext Context;
				Context.Speaker = First;
				Context.Listener = Second;
				Context.Player = Player;

				if (!Conversation.Requires.Evaluate(Context, Dialog->GetFacts()))
				{
					continue;
				}
			}

			TArray<AStrategyUnit*> Cast = { First, Second };

			// anyone past the second only has to be near the first
			for (AStrategyUnit* Other : Order)
			{
				if (Cast.Num() >= Needed)
				{
					break;
				}

				if (!Cast.Contains(Other) && (!bStrict || FVector::DistSquared(First->GetActorLocation(), Other->GetActorLocation()) <= RangeSquared))
				{
					Cast.Add(Other);
				}
			}

			if (Cast.Num() == Needed)
			{
				return Cast;
			}
		}
	}

	return {};
}

FName UBanterDirectorComponent::TryBanter(APlayerState* Player, TArray<FString>* OutExplanation)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Player);

	if (!GetOwner() || !GetOwner()->HasAuthority() || !Dialog || !Memory || FindRunning(Player))
	{
		return NAME_None;
	}

	const TArray<AStrategyUnit*> Members = GetAvailableMembers(Player);

	// the same order selection uses - priority, then least recently seen, then load order - with
	// "can it be cast?" standing in for requires:, since requires: is asked of a cast
	TArray<const FConversationDefinition*> Candidates = Dialog->GetLibrary().GetConversationsOfKind(EConversationKind::Ambient);

	Candidates.Sort([Memory](const FConversationDefinition& A, const FConversationDefinition& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}

		const FDialogSeenEntry* SeenA = Memory->GetRecord().FindSeen(A.Id);
		const FDialogSeenEntry* SeenB = Memory->GetRecord().FindSeen(B.Id);
		const int32 OrderA = SeenA ? SeenA->LastSeenOrder : 0;
		const int32 OrderB = SeenB ? SeenB->LastSeenOrder : 0;

		return OrderA != OrderB ? OrderA < OrderB : A.LoadOrder < B.LoadOrder;
	});

	for (const FConversationDefinition* Candidate : Candidates)
	{
		if (Candidate->bOnce && Memory->GetRecord().FindSeen(Candidate->Id))
		{
			if (OutExplanation)
			{
				OutExplanation->Add(FString::Printf(TEXT("  %s: once: this squad has seen it"), *Candidate->Id.ToString()));
			}

			continue;
		}

		const TArray<AStrategyUnit*> Cast = FindCast(*Candidate, Player, Members, /*bStrict*/ true);

		if (Cast.Num() == 0)
		{
			if (OutExplanation)
			{
				OutExplanation->Add(FString::Printf(TEXT("  %s: no %d squad members on their feet within %.0f m of each other for whom requires: '%s' holds"),
					*Candidate->Id.ToString(), Candidate->Participants.Num(), CastRange / 100.0f, *Candidate->Requires.ToString()));
			}

			continue;
		}

		if (OutExplanation)
		{
			OutExplanation->Add(FString::Printf(TEXT("  %s: cast %s"), *Candidate->Id.ToString(),
				*FString::JoinBy(Cast, TEXT(", "), [](const AStrategyUnit* Member) { return Member->GetHolderDisplayName().ToString(); })));
		}

		if (StartBanter(*Candidate, Player, Cast))
		{
			if (OutExplanation)
			{
				OutExplanation->Add(FString::Printf(TEXT("  => %s"), *Candidate->Id.ToString()));
			}

			return Candidate->Id;
		}
	}

	if (OutExplanation)
	{
		OutExplanation->Add(TEXT("  => none"));
	}

	return NAME_None;
}

bool UBanterDirectorComponent::PlayAmbient(const FConversationDefinition& Conversation, APlayerState* Player, TArray<FString>* OutExplanation)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Conversation.Kind != EConversationKind::Ambient)
	{
		return false;
	}

	StopBanter(Player);

	const TArray<AStrategyUnit*> Cast = FindCast(Conversation, Player, GetAvailableMembers(Player), /*bStrict*/ false);

	if (Cast.Num() == 0)
	{
		if (OutExplanation)
		{
			OutExplanation->Add(FString::Printf(TEXT("%s needs %d squad members on their feet"), *Conversation.Id.ToString(), Conversation.Participants.Num()));
		}

		return false;
	}

	return StartBanter(Conversation, Player, Cast);
}

bool UBanterDirectorComponent::StartBanter(const FConversationDefinition& Conversation, APlayerState* Player, const TArray<AStrategyUnit*>& Cast)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Player);

	if (!Dialog || !Memory || Cast.Num() < 2)
	{
		return false;
	}

	TUniquePtr<FRunningBanter> Banter = MakeUnique<FRunningBanter>();
	Banter->PlayerState = Player;
	Banter->ConversationId = Conversation.Id;

	for (AStrategyUnit* Member : Cast)
	{
		Banter->Cast.Add(Member);
	}

	const TWeakObjectPtr<APlayerState> WeakPlayer = Player;

	FConversationPlayerSetup Setup;
	Setup.Script = Conversation.Script;
	Setup.StartNode = Conversation.LocalId.ToString();
	Setup.Facts = &Dialog->GetFacts();
	Setup.Context.Speaker = Cast[0];
	Setup.Context.Listener = Cast[1];
	Setup.Context.Player = Player;
	Setup.Participants = Conversation.Participants;
	Setup.RandomSeed = Stream.RandHelper(MAX_int32);
	Setup.RunCommand = [this, WeakPlayer](FName Command, const TArray<FString>& Arguments)
	{
		return RunBanterEffect(WeakPlayer, Command, Arguments);
	};

	Banter->Player = MakeUnique<FDialogConversationPlayer>(MoveTemp(Setup));

	if (!Banter->Player->Start())
	{
		return false;
	}

	Memory->MarkSeen(Conversation.Id);

	if (FSquadBanter* Squad = Squads.Find(Player))
	{
		Squad->LastBanterTime = GetWorld()->GetTimeSeconds();
		Squad->NextIdleTime = Squad->LastBanterTime + IdleSeconds;
	}
	else
	{
		FSquadBanter& NewSquad = Squads.Add(Player);
		NewSquad.LastBanterTime = GetWorld()->GetTimeSeconds();
		NewSquad.NextIdleTime = NewSquad.LastBanterTime + IdleSeconds;
	}

	UE_LOG(LogSmoresDialog, Log, TEXT("Dialog: banter %s among %s"), *Conversation.Id.ToString(),
		*FString::JoinBy(Cast, TEXT(", "), [](const AStrategyUnit* Member) { return Member->GetHolderDisplayName().ToString(); }));

	FRunningBanter& Started = *Banter;
	Running.Add(MoveTemp(Banter));

	StepBanter(Started);
	return true;
}

void UBanterDirectorComponent::StepBanter(FRunningBanter& Banter)
{
	const APlayerState* Player = Banter.PlayerState.Get();

	if (!Player || !Banter.Player.IsValid() || Banter.Player->GetState() != EConversationPlayerState::Line)
	{
		StopBanter(Player);
		return;
	}

	// a participant who went down, left or started fighting ends it mid-sentence
	for (const TWeakObjectPtr<AStrategyUnit>& Member : Banter.Cast)
	{
		if (!Member.IsValid() || Member->IsIncapacitated() || IsBanterFighting(Member.Get()))
		{
			StopBanter(Player);
			return;
		}
	}

	const FConversationPlayerLine& Line = Banter.Player->GetLine();
	AStrategyUnit* Speaker = Banter.Cast.IsValidIndex(Line.SpeakerSlot) ? Banter.Cast[Line.SpeakerSlot].Get() : nullptr;

	// heard exactly as a bark is: by every player with somebody near, in the feed and over the speaker
	if (UBarkDirectorComponent* Barks = UBarkDirectorComponent::Get(this))
	{
		Barks->Deliver(Line.LineId, Speaker);
	}

	// real time, like the bubble it goes with: at 8x a line still stays up long enough to read, and
	// paused it still moves on - so the wait is scaled by however fast the world is running
	UWorld* World = GetWorld();
	const float Dilation = World && World->GetWorldSettings() ? World->GetWorldSettings()->GetEffectiveTimeDilation() : 1.0f;
	const float RealSeconds = GetReadingSeconds(Line.SourceText.Len()) + LineGapSeconds;

	if (World)
	{
		World->GetTimerManager().SetTimer(Banter.LineTimer, FTimerDelegate::CreateUObject(this, &UBanterDirectorComponent::AdvanceBanter, Banter.PlayerState),
			FMath::Max(RealSeconds * Dilation, KINDA_SMALL_NUMBER), /*bLoop*/ false);
	}
}

void UBanterDirectorComponent::AdvanceBanter(TWeakObjectPtr<APlayerState> PlayerState)
{
	FRunningBanter* Banter = FindRunning(PlayerState.Get());

	if (!Banter || !Banter->Player.IsValid())
	{
		return;
	}

	Banter->Player->Advance();

	if (Banter->Player->GetState() == EConversationPlayerState::Line)
	{
		StepBanter(*Banter);
	}
	else
	{
		StopBanter(PlayerState.Get());
	}
}

bool UBanterDirectorComponent::RunBanterEffect(TWeakObjectPtr<APlayerState> PlayerState, FName Command, const TArray<FString>& Arguments)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	APlayerState* Player = PlayerState.Get();
	FRunningBanter* Banter = FindRunning(Player);

	if (!Dialog || !Player)
	{
		return true;
	}

	APlayerController* Controller = Player->GetPlayerController();

	FDialogEffectContext Context;
	Context.World = GetWorld();
	Context.Player = Player;
	Context.Host = Cast<IDialogHost>(Controller);

	// the banter is being set up when its first command runs, so it may not be in Running yet
	if (Banter)
	{
		Context.Speaker = Banter->Cast.IsValidIndex(0) ? Banter->Cast[0].Get() : nullptr;
		Context.Listener = Banter->Cast.IsValidIndex(1) ? Banter->Cast[1].Get() : nullptr;
	}

	const FDialogEffectOutcome Outcome = Dialog->GetEffects().Run(Command, Context, Arguments);

	if (!Outcome.FeedLine.IsEmpty())
	{
		if (UConversationComponent* Conversation = UConversationComponent::Get(Controller))
		{
			Conversation->Client_PostOutcome(Outcome.FeedLine);
		}
	}

	return !Outcome.bEndsConversation;
}

void UBanterDirectorComponent::StopBanter(const APlayerState* Player)
{
	for (int32 Index = Running.Num() - 1; Index >= 0; --Index)
	{
		if (Running[Index]->PlayerState.Get() == Player || !Running[Index]->PlayerState.IsValid())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(Running[Index]->LineTimer);
			}

			Running.RemoveAt(Index);
		}
	}
}

UBanterDirectorComponent::FRunningBanter* UBanterDirectorComponent::FindRunning(const APlayerState* Player)
{
	for (const TUniquePtr<FRunningBanter>& Banter : Running)
	{
		if (Banter->PlayerState.Get() == Player)
		{
			return Banter.Get();
		}
	}

	return nullptr;
}
