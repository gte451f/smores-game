// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ConversationComponent.h"
#include "ConversationSelection.h"
#include "DialogEffects.h"
#include "DialogHost.h"
#include "DialogMemoryComponent.h"
#include "SmoresDialog.h"
#include "SmoresDialogSubsystem.h"
#include "SmoresActivityLog.h"
#include "StrategyUnit.h"
#include "HealthComponent.h"
#include "CombatComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "SmoresConversation"

namespace
{
	float GetUnitHealth(const AStrategyUnit* Unit)
	{
		const UHealthComponent* Health = Unit ? Unit->GetHealth() : nullptr;
		return Health ? Health->GetHealth() : 0.0f;
	}

	bool HasAttackTarget(const AStrategyUnit* Unit)
	{
		const UCombatComponent* Combat = Unit ? Unit->GetCombat() : nullptr;
		return Combat && Combat->GetCurrentAttackTarget() != nullptr;
	}
}

UConversationComponent::UConversationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// its RPCs carry the conversation between the server and the owning client
	SetIsReplicatedByDefault(true);
}

UConversationComponent::~UConversationComponent() = default;

UConversationComponent* UConversationComponent::Get(const AActor* PlayerController)
{
	return PlayerController ? PlayerController->FindComponentByClass<UConversationComponent>() : nullptr;
}

void UConversationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
		{
			WillReloadHandle = Dialog->OnLibraryWillReload.AddUObject(this, &UConversationComponent::HandleLibraryWillReload);
		}
	}
}

void UConversationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this))
	{
		Dialog->OnLibraryWillReload.Remove(WillReloadHandle);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WatchTimerHandle);
	}

	Player.Reset();

	Super::EndPlay(EndPlayReason);
}

// ---- server ----------------------------------------------------------------------------------

bool UConversationComponent::TryStartGreeting(AStrategyUnit* InNpc, AStrategyUnit* InSquadMember, TArray<FString>* OutExplanation)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const UDialogMemoryComponent* Memory = Controller ? UDialogMemoryComponent::Get(Controller->PlayerState) : nullptr;

	if (!GetOwner() || !GetOwner()->HasAuthority() || !Dialog || !Memory || !IsValid(InNpc) || !IsValid(InSquadMember))
	{
		return false;
	}

	FDialogContext Context;
	Context.Speaker = InNpc;
	Context.Listener = InSquadMember;
	Context.Player = Controller->PlayerState;

	const FConversationDefinition* Greeting = SmoresDialog::SelectConversation(
		Dialog->GetLibrary().GetConversationsOfKind(EConversationKind::Greeting), Context, Dialog->GetFacts(), Memory->GetRecord(), OutExplanation);

	return Greeting && StartConversation(*Greeting, InNpc, InSquadMember, /*bIgnoreRange*/ false);
}

bool UConversationComponent::StartConversation(const FConversationDefinition& Conversation, AStrategyUnit* InNpc, AStrategyUnit* InSquadMember, bool bInIgnoreRange)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Conversation.Kind == EConversationKind::Ambient || !IsValid(InNpc) || !IsValid(InSquadMember))
	{
		return false;
	}

	// one conversation at a time, per player: talking to somebody else ends the last one
	if (IsInConversation())
	{
		EndConversation(EConversationEndReason::Replaced);
	}

	Npc = InNpc;
	SquadMember = InSquadMember;
	NpcStartHealth = GetUnitHealth(InNpc);
	SquadMemberStartHealth = GetUnitHealth(InSquadMember);
	bIgnoreRange = bInIgnoreRange;
	PendingEnd.Reset();

	Client_Begin(InNpc, InSquadMember, InNpc->GetHolderDisplayName(), InSquadMember->GetHolderDisplayName());

	UE_LOG(LogSmoresDialog, Log, TEXT("Dialog: %s starts %s with %s"),
		*InSquadMember->GetHolderDisplayName().ToString(), *Conversation.Id.ToString(), *InNpc->GetHolderDisplayName().ToString());

	RunScript(Conversation);

	// world time, like the approach check: nothing moves while the game is paused, so nothing needs checking
	if (IsInConversation())
	{
		GetWorld()->GetTimerManager().SetTimer(WatchTimerHandle, this, &UConversationComponent::WatchConversation, WatchSeconds, /*bLoop*/ true);
	}

	return true;
}

void UConversationComponent::EndConversation(EConversationEndReason Reason)
{
	if (!IsInConversation())
	{
		return;
	}

	UE_LOG(LogSmoresDialog, Log, TEXT("Dialog: %s ends (%s)"), *Current.Id.ToString(), *StaticEnum<EConversationEndReason>()->GetNameStringByValue(static_cast<int64>(Reason)));

	if (Player.IsValid())
	{
		Player->Stop();
		Player.Reset();
	}

	Stage = EConversationStage::None;
	Npc = nullptr;
	SquadMember = nullptr;
	ShownChoices.Reset();
	MenuTopics.Reset();
	PendingEnd.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WatchTimerHandle);
	}

	Client_End(Reason);
}

bool UConversationComponent::IsInConversation() const
{
	return Stage != EConversationStage::None;
}

FDialogContext UConversationComponent::MakeContext() const
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());

	FDialogContext Context;
	Context.Speaker = Npc.Get();
	Context.Listener = SquadMember.Get();
	Context.Player = Controller ? Controller->PlayerState : nullptr;

	return Context;
}

void UConversationComponent::RunScript(const FConversationDefinition& Conversation)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());

	if (!Dialog || !Controller)
	{
		EndConversation(EConversationEndReason::Failed);
		return;
	}

	Current = Conversation;
	Stage = EConversationStage::Script;

	// it counts as seen the moment it starts - walking off halfway still uses up a once: conversation
	if (UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Controller->PlayerState))
	{
		Memory->MarkSeen(Conversation.Id);
	}

	FConversationPlayerSetup Setup;
	Setup.Script = Conversation.Script;
	Setup.StartNode = Conversation.LocalId.ToString();
	Setup.Facts = &Dialog->GetFacts();
	Setup.Context = MakeContext();
	Setup.RandomSeed = static_cast<int32>(GetTypeHash(Conversation.Id) ^ static_cast<uint32>(GetWorld()->GetTimeSeconds() * 1000.0));
	Setup.RunCommand = [this](FName Command, const TArray<FString>& Arguments)
	{
		return RunEffect(Command, Arguments);
	};

	Player = MakeUnique<FDialogConversationPlayer>(MoveTemp(Setup));

	if (!Player->Start())
	{
		EndConversation(EConversationEndReason::Failed);
		return;
	}

	PresentScript();
}

void UConversationComponent::PresentScript()
{
	if (!Player.IsValid())
	{
		return;
	}

	if (PendingEnd.IsSet())
	{
		EndConversation(PendingEnd.GetValue());
		return;
	}

	switch (Player->GetState())
	{
	case EConversationPlayerState::Line:
	{
		const FConversationPlayerLine& Line = Player->GetLine();

		FConversationLineView LineView;
		LineView.LineId = Line.LineId;
		LineView.SpeakerSlot = Line.SpeakerSlot;
		LineView.Substitutions = Line.Substitutions;

		Client_ShowLine(LineView);
		break;
	}

	case EConversationPlayerState::Choices:
	{
		TArray<FConversationChoiceView> Views;
		ShownChoices.Reset();

		const TArray<FConversationPlayerChoice>& Choices = Player->GetChoices();

		for (int32 Index = 0; Index < Choices.Num(); ++Index)
		{
			if (!Choices[Index].bShown)
			{
				continue;
			}

			FConversationChoiceView& ChoiceView = Views.AddDefaulted_GetRef();
			ChoiceView.TextId = Choices[Index].LineId;
			ChoiceView.Substitutions = Choices[Index].Substitutions;
			ChoiceView.bEnabled = Choices[Index].bAvailable;
			ChoiceView.Reason = Choices[Index].Reason;

			ShownChoices.Add(Index);
		}

		Client_ShowChoices(Views);
		break;
	}

	case EConversationPlayerState::Ended:
		// a greeting or a topic both come back to the topic list - the player leaves with Goodbye
		Player.Reset();
		ShowTopicMenu();
		break;

	case EConversationPlayerState::Failed:
	default:
		EndConversation(EConversationEndReason::Failed);
		break;
	}
}

void UConversationComponent::ShowTopicMenu()
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	const UDialogMemoryComponent* Memory = Controller ? UDialogMemoryComponent::Get(Controller->PlayerState) : nullptr;

	if (!Dialog || !Memory)
	{
		EndConversation(EConversationEndReason::Failed);
		return;
	}

	Stage = EConversationStage::TopicMenu;
	MenuTopics.Reset();

	TArray<FConversationChoiceView> Views;

	// rebuilt every time: a topic just played may have set the flag another one needed, or used itself up
	for (const FConversationDefinition* Topic : SmoresDialog::GetEligibleConversations(
		Dialog->GetLibrary().GetConversationsOfKind(EConversationKind::Topic), MakeContext(), Dialog->GetFacts(), Memory->GetRecord()))
	{
		FConversationChoiceView& ChoiceView = Views.AddDefaulted_GetRef();
		ChoiceView.TextId = Topic->LabelId;

		MenuTopics.Add(Topic->Id);
	}

	FConversationChoiceView& Goodbye = Views.AddDefaulted_GetRef();
	Goodbye.bGoodbye = true;

	MenuTopics.Add(NAME_None);

	Client_ShowChoices(Views);
}

bool UConversationComponent::RunEffect(FName Command, const TArray<FString>& Arguments)
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	APlayerController* Controller = Cast<APlayerController>(GetOwner());

	if (!Dialog || !Controller)
	{
		return true;
	}

	FDialogEffectContext Context;
	Context.World = GetWorld();
	Context.Speaker = Npc.Get();
	Context.Listener = SquadMember.Get();
	Context.Player = Controller->PlayerState;
	Context.Host = Cast<IDialogHost>(Controller);

	const FDialogEffectOutcome Outcome = Dialog->GetEffects().Run(Command, Context, Arguments);

	UE_LOG(LogSmoresDialog, Log, TEXT("Dialog: %s runs <<%s %s>>: %s"), *Current.Id.ToString(), *Command.ToString(), *FString::Join(Arguments, TEXT(" ")),
		Outcome.bDone ? TEXT("done") : TEXT("refused"));

	if (!Outcome.FeedLine.IsEmpty())
	{
		Client_PostOutcome(Outcome.FeedLine);
	}

	// a script that forgot to guard a choice lands here; the player is told why, and it goes on
	if (Outcome.Refusal != ESmoresRefusalReason::None && Context.Host)
	{
		Context.Host->NotifyDialogRefusal(Outcome.Refusal);
	}

	if (Outcome.bEndsConversation)
	{
		PendingEnd = EConversationEndReason::OpenedTrade;
		return false;
	}

	return true;
}

void UConversationComponent::WatchConversation()
{
	if (!IsInConversation())
	{
		return;
	}

	const AStrategyUnit* CurrentNpc = Npc.Get();
	const AStrategyUnit* CurrentSquadMember = SquadMember.Get();

	FConversationWatch Watch;
	Watch.bSomeoneGone = !IsValid(CurrentNpc) || !IsValid(CurrentSquadMember);

	if (!Watch.bSomeoneGone)
	{
		Watch.bNpcDown = CurrentNpc->IsIncapacitated();
		Watch.bSquadMemberDown = CurrentSquadMember->IsIncapacitated();
		Watch.bNpcHostile = CurrentNpc->IsAggressive();
		Watch.bEitherAttacking = HasAttackTarget(CurrentNpc) || HasAttackTarget(CurrentSquadMember);
		Watch.bEitherHurt = GetUnitHealth(CurrentNpc) < NpcStartHealth || GetUnitHealth(CurrentSquadMember) < SquadMemberStartHealth;
		Watch.Distance = FVector::Dist(CurrentNpc->GetActorLocation(), CurrentSquadMember->GetActorLocation());
		Watch.BreakOffRange = BreakOffRange;
		Watch.bCheckRange = !bIgnoreRange;
	}

	EConversationEndReason Reason = EConversationEndReason::Finished;

	if (SmoresDialog::GetConversationInterruption(Watch, Reason))
	{
		EndConversation(Reason);
	}
}

void UConversationComponent::HandleLibraryWillReload()
{
	EndConversation(EConversationEndReason::Reloaded);
}

void UConversationComponent::Server_Continue_Implementation()
{
	if (Stage == EConversationStage::Script && Player.IsValid() && Player->GetState() == EConversationPlayerState::Line)
	{
		Player->Advance();
		PresentScript();
	}
}

void UConversationComponent::Server_Choose_Implementation(int32 Index)
{
	if (Stage == EConversationStage::Script && Player.IsValid() && Player->GetState() == EConversationPlayerState::Choices)
	{
		if (!ShownChoices.IsValidIndex(Index))
		{
			return;
		}

		const FConversationPlayerChoice Choice = Player->GetChoices()[ShownChoices[Index]];

		// a greyed choice can't be picked - the client knows that too, but the server is the one that decides
		if (!Choice.bAvailable)
		{
			return;
		}

		// said before it runs, so whatever the choice does lands in the feed after it
		FConversationLineView Picked;
		Picked.LineId = Choice.LineId;
		Picked.SpeakerSlot = SmoresDialog::SquadMemberSlot;
		Picked.Substitutions = Choice.Substitutions;
		Client_ShowPicked(Picked);

		Player->Choose(ShownChoices[Index]);
		PresentScript();
		return;
	}

	if (Stage == EConversationStage::TopicMenu && MenuTopics.IsValidIndex(Index))
	{
		const FName TopicId = MenuTopics[Index];

		if (TopicId.IsNone())
		{
			EndConversation(EConversationEndReason::Goodbye);
			return;
		}

		const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
		const FConversationDefinition* Topic = Dialog ? Dialog->GetLibrary().FindConversation(TopicId) : nullptr;

		if (!Topic)
		{
			ShowTopicMenu();
			return;
		}

		FConversationLineView Picked;
		Picked.LineId = Topic->LabelId;
		Picked.SpeakerSlot = SmoresDialog::SquadMemberSlot;
		Client_ShowPicked(Picked);

		RunScript(*Topic);
	}
}

void UConversationComponent::Server_Leave_Implementation()
{
	EndConversation(EConversationEndReason::Goodbye);
}

// ---- client ----------------------------------------------------------------------------------

FText UConversationComponent::ResolveText(FName TextId, const TArray<FString>& Substitutions) const
{
	const USmoresDialogSubsystem* Dialog = USmoresDialogSubsystem::Get(this);
	const FText Text = Dialog ? Dialog->GetLineText(TextId) : FText::GetEmpty();

	if (Text.IsEmpty())
	{
		// the id came from the server's files and this machine doesn't have it - host and client
		// are running different dialog, exactly as a bark would report it
		UE_LOG(LogSmoresDialog, Warning, TEXT("Dialog: the server showed '%s', which this machine's dialog doesn't have."), *TextId.ToString());
		return FText::FromName(TextId);
	}

	if (Substitutions.Num() == 0)
	{
		return Text;
	}

	// a line's {0}, {1} are Unreal's own ordered format arguments
	FFormatOrderedArguments Arguments;

	for (const FString& Value : Substitutions)
	{
		Arguments.Add(FText::AsCultureInvariant(Value));
	}

	return FText::Format(FTextFormat(Text), Arguments);
}

FText UConversationComponent::GetSpeakerName(uint8 SpeakerSlot) const
{
	switch (SpeakerSlot)
	{
	case SmoresDialog::NpcSlot:
		return View.NpcName;

	case SmoresDialog::SquadMemberSlot:
		return View.SquadMemberName;

	default:
		return FText::GetEmpty();
	}
}

void UConversationComponent::AddTranscriptLine(const FConversationLineView& Line, bool bPicked)
{
	FConversationTranscriptLine& Entry = View.Transcript.AddDefaulted_GetRef();
	Entry.Speaker = GetSpeakerName(Line.SpeakerSlot);
	Entry.Text = ResolveText(Line.LineId, Line.Substitutions);
	Entry.bSquadMember = bPicked || Line.SpeakerSlot == SmoresDialog::SquadMemberSlot;

	// every line said goes to the feed as well - the transcript the window doesn't keep once it closes
	if (USmoresActivityLog* Feed = USmoresActivityLog::Get(Cast<APlayerController>(GetOwner())))
	{
		Feed->Post(EActivityCategory::Comms, EActivitySeverity::Normal,
			FText::Format(LOCTEXT("ConversationFeedLine", "\u201C{0}\u201D"), Entry.Text), Entry.Speaker);
	}
}

void UConversationComponent::Client_Begin_Implementation(AActor* InNpc, AActor* InSquadMember, const FText& InNpcName, const FText& InSquadMemberName)
{
	View = FConversationView();
	View.bOpen = true;
	View.bWaitingForServer = true;
	View.Npc = InNpc;
	View.SquadMember = InSquadMember;
	View.NpcName = InNpcName;
	View.SquadMemberName = InSquadMemberName;

	OnViewChanged.Broadcast();
}

void UConversationComponent::Client_ShowLine_Implementation(const FConversationLineView& Line)
{
	AddTranscriptLine(Line, /*bPicked*/ false);

	View.bAwaitingContinue = true;
	View.Choices.Reset();
	View.bWaitingForServer = false;

	OnViewChanged.Broadcast();
}

void UConversationComponent::Client_ShowPicked_Implementation(const FConversationLineView& Line)
{
	AddTranscriptLine(Line, /*bPicked*/ true);

	OnViewChanged.Broadcast();
}

void UConversationComponent::Client_ShowChoices_Implementation(const TArray<FConversationChoiceView>& Choices)
{
	View.Choices.Reset();

	for (const FConversationChoiceView& Choice : Choices)
	{
		FConversationShownChoice& Shown = View.Choices.AddDefaulted_GetRef();
		Shown.bGoodbye = Choice.bGoodbye;
		Shown.bEnabled = Choice.bEnabled;
		Shown.Reason = Choice.Reason;
		Shown.Text = Choice.bGoodbye ? FText::GetEmpty() : ResolveText(Choice.TextId, Choice.Substitutions);
	}

	View.bAwaitingContinue = false;
	View.bWaitingForServer = false;

	OnViewChanged.Broadcast();
}

void UConversationComponent::Client_End_Implementation(EConversationEndReason Reason)
{
	const bool bWasOpen = View.bOpen;
	const FText NpcName = View.NpcName;

	View = FConversationView();
	View.EndReason = Reason;

	// broken off rather than finished - say so, or the window just vanishes
	const bool bInterrupted = Reason == EConversationEndReason::WalkedAway || Reason == EConversationEndReason::Downed
		|| Reason == EConversationEndReason::Combat || Reason == EConversationEndReason::Failed;

	if (bWasOpen && bInterrupted)
	{
		if (USmoresActivityLog* Feed = USmoresActivityLog::Get(Cast<APlayerController>(GetOwner())))
		{
			Feed->Post(EActivityCategory::Comms, EActivitySeverity::Warning, FText::Format(LOCTEXT("ConversationBrokenOff", "The conversation with {0} broke off"), NpcName));
		}
	}

	OnViewChanged.Broadcast();
}

void UConversationComponent::Client_PostOutcome_Implementation(const FText& Text)
{
	if (USmoresActivityLog* Feed = USmoresActivityLog::Get(Cast<APlayerController>(GetOwner())))
	{
		Feed->Post(EActivityCategory::Comms, EActivitySeverity::Normal, Text);
	}
}

void UConversationComponent::RequestContinue()
{
	if (View.bOpen && View.bAwaitingContinue && !View.bWaitingForServer)
	{
		View.bWaitingForServer = true;
		Server_Continue();
	}
}

void UConversationComponent::RequestChoose(int32 Index)
{
	if (View.bOpen && !View.bAwaitingContinue && !View.bWaitingForServer && View.Choices.IsValidIndex(Index) && View.Choices[Index].bEnabled)
	{
		View.bWaitingForServer = true;
		Server_Choose(Index);
	}
}

void UConversationComponent::RequestLeave()
{
	if (View.bOpen)
	{
		Server_Leave();
	}
}

#undef LOCTEXT_NAMESPACE
