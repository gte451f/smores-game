// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "ConversationWidget.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UConversationChoiceWidget;
class UConversationComponent;

/**
 *  The conversation window: who you're talking to, what has been said, and either Continue or the
 *  choices on offer.
 *
 *  **Its own panel, never floating text** (Jim, after dialog Slice 1's PIE pass): a bark floats,
 *  anything with back-and-forth gets this window. A UWindowWidget, so it drags, resizes and
 *  swallows presses like every other window.
 *
 *  **It shows, it never decides.** Everything here comes from the owning player's
 *  UConversationComponent view - built on this machine from the ids the server sent, in this
 *  player's language - and every click is a request to that component. A greyed choice shows its
 *  reason; the window never guesses what a pick will do. Closing it (its X, or Talk again) is
 *  Goodbye.
 *
 *  The controller opens it when a conversation begins and removes it when one ends; it rebuilds
 *  itself the frame after its view changes.
 */
UCLASS(abstract)
class SMORESUI_API UConversationWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Who you're talking to. Name it "SpeakerNameText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerNameText;

	/** Their portrait, when they have one. Name it "PortraitImage". Collapsed when they don't. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PortraitImage;

	/** Their initials - the portrait fallback every portrait in the HUD uses. Name it "InitialsText". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitialsText;

	/** What has been said, oldest first. Name it "TranscriptText". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TranscriptText;

	/** Optional scroll box around the transcript; kept scrolled to the newest line. Name it "TranscriptScroll". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> TranscriptScroll;

	/** Shown while a line waits to be read. Name it "ContinueButton". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	/** Where the choices go - a vertical box. Name it "ChoiceBox". */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ChoiceBox;

	/** One choice's widget - WBP_ConversationChoice. Without it no choice can be shown, and the window says so once. */
	UPROPERTY(EditAnywhere, Category = "Conversation")
	TSubclassOf<UConversationChoiceWidget> ChoiceWidgetClass;

	/** How many of the most recent lines the transcript keeps on screen. The feed keeps them all. */
	UPROPERTY(EditAnywhere, Category = "Conversation", meta = (ClampMin = 1))
	int32 MaxTranscriptLines = 12;

public:

	UConversationWidget(const FObjectInitializer& ObjectInitializer);

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface

	/** The first letter of up to two words - "Merchant Ada" is "MA" - the HUD's portrait fallback */
	static FText MakeInitials(const FText& Name);

protected:

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End UUserWidget interface

	/** Redraws everything from the component's view */
	void Rebuild();

	void HandleViewChanged();

	UFUNCTION()
	void HandleContinueClicked();

	void HandleChoiceClicked(int32 Index);

	/** The owning player's conversation, or null */
	UConversationComponent* GetConversation() const;

	/** Spawned so far; the extras are collapsed rather than destroyed, so a click never removes the button under it */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UConversationChoiceWidget>> ChoiceWidgets;

	FDelegateHandle ViewChangedHandle;

	TWeakObjectPtr<UConversationComponent> BoundConversation;

	bool bDirty = true;

	/** Ticks left to keep the transcript scrolled to its end - the choices under it resize the box a frame after a rebuild */
	int32 ScrollToEndTicks = 0;

	bool bWarnedNoChoiceClass = false;
};
