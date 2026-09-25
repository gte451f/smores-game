// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SmoresRefusalReason.h"
#include "ConversationChoiceWidget.generated.h"

class UButton;
class UTextBlock;

/** Broadcast when the player clicks an enabled choice. Carries its position in the list. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnConversationChoiceClicked, int32);

/**
 *  One choice in the conversation window - a Yarn option, a topic, or Goodbye.
 *
 *  The UTargetActionWidget shape, and the same three bound names, so a WBP for either reads the
 *  same: a real UButton that consumes its own press, the label, and a reason shown only while the
 *  choice is greyed out. A choice tagged #reason: that can't be taken still shows, with the refusal
 *  line's own words for why ("Not enough gold") - the "a disabled action still shows, with its
 *  reason" rule the target panel follows.
 */
UCLASS(abstract)
class SMORESUI_API UConversationChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The clickable part. Name it "ActionButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActionButton;

	/** The choice's words. Name it "LabelText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LabelText;

	/** Why it's greyed out. Name it "ReasonText" in the WBP to auto-bind. Collapsed while enabled. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReasonText;

public:

	/** Fired when the player clicks this choice while it is enabled */
	FOnConversationChoiceClicked OnChoiceClicked;

	/** Fills it in. Index is its position in the list, which is what a click reports. */
	void SetChoice(int32 InIndex, const FText& Label, bool bInEnabled, ESmoresRefusalReason Reason);

protected:

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	UFUNCTION()
	void HandleClicked();

	int32 Index = INDEX_NONE;

	bool bEnabled = true;
};
