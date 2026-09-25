// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "DialogConversationTypes.h"
#include "ConversationPlayer.h"
#include "SmoresRefusalReason.h"
#include "ConversationComponent.generated.h"

class AStrategyUnit;

/** One line in the window's transcript, already in this player's language */
struct SMORESDIALOG_API FConversationTranscriptLine
{
	/** Who said it - empty for narration */
	FText Speaker;

	FText Text;

	/** True for a line the squad member said - a choice the player picked */
	bool bSquadMember = false;
};

/** One choice as the window shows it */
struct SMORESDIALOG_API FConversationShownChoice
{
	/** Empty for Goodbye, which the window words itself */
	FText Text;

	bool bEnabled = true;

	/** Why it is greyed out */
	ESmoresRefusalReason Reason = ESmoresRefusalReason::None;

	bool bGoodbye = false;
};

/**
 *  What the conversation window shows - this player's side of their conversation, built on their
 *  own machine from the ids the server sent, in their own language.
 */
struct SMORESDIALOG_API FConversationView
{
	bool bOpen = false;

	TWeakObjectPtr<AActor> Npc;

	TWeakObjectPtr<AActor> SquadMember;

	FText NpcName;

	FText SquadMemberName;

	/** Every line so far, oldest first */
	TArray<FConversationTranscriptLine> Transcript;

	/** Showing a line, with Continue to go on */
	bool bAwaitingContinue = false;

	/** Showing these instead */
	TArray<FConversationShownChoice> Choices;

	/** A request has gone to the server and nothing has come back yet - the window holds still */
	bool bWaitingForServer = false;

	/** Why it closed, once bOpen is false */
	EConversationEndReason EndReason = EConversationEndReason::Finished;
};

/**
 *  One player's conversations - on AStrategyPlayerController, because a client can only send an RPC
 *  through an actor it owns.
 *
 *  **The server runs the conversation; the client only ever sees ids.** On the server this holds the
 *  running Yarn player, picks the greeting, builds the topic list and carries out the effects. Every
 *  line and choice goes to the owning client as a line id, and the client looks the words up in its
 *  own loaded, translated library - the same rule barks follow, so in co-op each player reads their
 *  own conversation in their own language. The pick comes back as a position in the list shown.
 *
 *  **The flow**: a Greeting plays; when it ends, the window offers every eligible Topic attached to
 *  this NPC, plus Goodbye; a Topic plays and comes back to the list, rebuilt (a topic may unlock
 *  another). Goodbye, the window's close button, Talk again, or talking to somebody else ends it.
 *  So does either side walking out of range, going down, or a fight starting - checked on a timer.
 *
 *  **Several players may talk to the same NPC at once.** Each player's conversation is its own, on
 *  their own controller. Nothing locks the NPC.
 */
UCLASS(ClassGroup = (Smores), meta = (BlueprintSpawnableComponent))
class SMORESDIALOG_API UConversationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UConversationComponent();
	virtual ~UConversationComponent() override;

	/** The conversation component on this player controller, or null */
	static UConversationComponent* Get(const AActor* PlayerController);

	// ---- server ------------------------------------------------------------------------------

	/**
	 *  Talk: opens the best eligible greeting attached to Npc, with SquadMember doing the talking.
	 *  **Server-side.** False, with nothing started, when Npc has no eligible greeting - the caller
	 *  then trades or says a NothingToSay bark instead. OutExplanation gets the selection's account.
	 */
	bool TryStartGreeting(AStrategyUnit* Npc, AStrategyUnit* SquadMember, TArray<FString>* OutExplanation = nullptr);

	/**
	 *  Opens one Greeting or Topic between them, eligible or not - the debug exec's way in.
	 *  bIgnoreRange keeps it open however far apart they are. **Server-side.** False for an Ambient
	 *  conversation (the banter director plays those) or a missing NPC or squad member.
	 */
	bool StartConversation(const FConversationDefinition& Conversation, AStrategyUnit* Npc, AStrategyUnit* SquadMember, bool bIgnoreRange);

	/** Ends the conversation, if there is one, and tells the client why. **Server-side.** */
	void EndConversation(EConversationEndReason Reason);

	/** True while a conversation is running. **Server-side** - a client reads GetView(). */
	bool IsInConversation() const;

	/** Who this player is talking to. **Server-side.** */
	AStrategyUnit* GetConversationNpc() const { return Npc.Get(); }

	/** Which of this player's squad is doing the talking. **Server-side.** */
	AStrategyUnit* GetConversationSquadMember() const { return SquadMember.Get(); }

	/** How far apart the squad member and the NPC may get before the conversation breaks off, in cm */
	UPROPERTY(EditAnywhere, Category = "Conversation", meta = (ClampMin = 0, Units = "cm"))
	float BreakOffRange = 500.0f;

	/** How often a running conversation checks range, health and fighting, in world seconds */
	UPROPERTY(EditAnywhere, Category = "Conversation", meta = (ClampMin = 0.05, Units = "s"))
	float WatchSeconds = 0.25f;

	// ---- client ------------------------------------------------------------------------------

	/** What the window shows. Owning client only. */
	const FConversationView& GetView() const { return View; }

	/** Fired on the owning client whenever the view changes - it opened, a line arrived, choices arrived, it closed */
	FSimpleMulticastDelegate OnViewChanged;

	/** Past the line showing. Owning client. */
	void RequestContinue();

	/** Picks the Index'th choice showing. Owning client. */
	void RequestChoose(int32 Index);

	/** Goodbye - the window's close button and Talk pressed again. Owning client. */
	void RequestLeave();

	/** Server -> owning client: a line for the feed that an effect worded ("Paid 20 gold") */
	UFUNCTION(Client, Reliable)
	void Client_PostOutcome(const FText& Text);

protected:

	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent interface

	UFUNCTION(Server, Reliable)
	void Server_Continue();

	UFUNCTION(Server, Reliable)
	void Server_Choose(int32 Index);

	UFUNCTION(Server, Reliable)
	void Server_Leave();

	/** Server -> owning client: a conversation opened. Names travel as they are - a character's name isn't dialog. */
	UFUNCTION(Client, Reliable)
	void Client_Begin(AActor* InNpc, AActor* InSquadMember, const FText& InNpcName, const FText& InSquadMemberName);

	/** Server -> owning client: a line, as its id. Continue moves on. */
	UFUNCTION(Client, Reliable)
	void Client_ShowLine(const FConversationLineView& Line);

	/** Server -> owning client: the choice just picked, said by the squad member - into the transcript and feed */
	UFUNCTION(Client, Reliable)
	void Client_ShowPicked(const FConversationLineView& Line);

	/** Server -> owning client: the choices on offer, each as its text's id */
	UFUNCTION(Client, Reliable)
	void Client_ShowChoices(const TArray<FConversationChoiceView>& Choices);

	/** Server -> owning client: it's over */
	UFUNCTION(Client, Reliable)
	void Client_End(EConversationEndReason Reason);

private:

	/** Where the server-side conversation is */
	enum class EConversationStage : uint8
	{
		None,

		/** Playing a Greeting's or a Topic's script */
		Script,

		/** Offering the topics and Goodbye */
		TopicMenu
	};

	/** Starts Conversation's script. Server-side. */
	void RunScript(const FConversationDefinition& Conversation);

	/** Tells the client wherever the script has got to - a line, choices, or on to the topic list. Server-side. */
	void PresentScript();

	/** The topics this NPC offers right now, plus Goodbye. Server-side. */
	void ShowTopicMenu();

	/** Carries out one of the script's commands. Server-side. False ends the conversation there. */
	bool RunEffect(FName Command, const TArray<FString>& Arguments);

	/** The timer: range, health and fighting. Server-side. */
	void WatchConversation();

	/** Who this is about, for facts and selection */
	FDialogContext MakeContext() const;

	/** The words for a line id, in this machine's language, with its values put in */
	FText ResolveText(FName TextId, const TArray<FString>& Substitutions) const;

	/** The name a speaker slot stands for, on the client */
	FText GetSpeakerName(uint8 SpeakerSlot) const;

	/** Adds a line to the transcript and the feed. Client-side. */
	void AddTranscriptLine(const FConversationLineView& Line, bool bPicked);

	void HandleLibraryWillReload();

	// server state

	TUniquePtr<FDialogConversationPlayer> Player;

	/** The Greeting or Topic whose script is playing */
	FConversationDefinition Current;

	EConversationStage Stage = EConversationStage::None;

	TWeakObjectPtr<AStrategyUnit> Npc;

	TWeakObjectPtr<AStrategyUnit> SquadMember;

	/** Health when it began - a loss since means somebody got hurt, which ends it */
	float NpcStartHealth = 0.0f;
	float SquadMemberStartHealth = 0.0f;

	bool bIgnoreRange = false;

	/** Set by an effect (OpenTrade) that ends the conversation once the script stops */
	TOptional<EConversationEndReason> PendingEnd;

	/** Script stage: the position in the player's choices of each choice shown */
	TArray<int32> ShownChoices;

	/** Topic stage: the topic behind each choice shown. The last choice is Goodbye. */
	TArray<FName> MenuTopics;

	FTimerHandle WatchTimerHandle;

	FDelegateHandle WillReloadHandle;

	// client state

	FConversationView View;
};
