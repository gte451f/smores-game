// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ConversationChoiceWidget.h"
#include "RefusalWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UConversationChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ActionButton)
	{
		ActionButton->OnClicked.AddUniqueDynamic(this, &UConversationChoiceWidget::HandleClicked);
	}
}

void UConversationChoiceWidget::SetChoice(int32 InIndex, const FText& Label, bool bInEnabled, ESmoresRefusalReason Reason)
{
	Index = InIndex;
	bEnabled = bInEnabled;

	if (LabelText)
	{
		LabelText->SetText(Label);
	}

	if (ReasonText)
	{
		// the refusal line's own words, so "Not enough gold" reads the same here as anywhere else
		const FText ReasonWords = bInEnabled || Reason == ESmoresRefusalReason::None ? FText::GetEmpty() : URefusalWidget::GetRefusalText(Reason);

		ReasonText->SetText(ReasonWords);
		ReasonText->SetVisibility(ReasonWords.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (ActionButton)
	{
		ActionButton->SetIsEnabled(bInEnabled);
	}
}

void UConversationChoiceWidget::HandleClicked()
{
	// a disabled UButton never fires OnClicked; belt and braces for a click on the frame it changed
	if (bEnabled)
	{
		OnChoiceClicked.Broadcast(Index);
	}
}
