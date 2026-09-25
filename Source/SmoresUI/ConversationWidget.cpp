// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "ConversationWidget.h"
#include "ConversationChoiceWidget.h"
#include "ConversationComponent.h"
#include "SmoresUI.h"
#include "StrategyUnit.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "ConversationWidget"

UConversationWidget::UConversationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WindowTitle = LOCTEXT("ConversationTitle", "Conversation");

	// low and central, clear of the target panel above and the portrait bar and feed below it -
	// a starting place for Jim's PIE look to move
	InitialWindowPosition = FVector2D(560.0f, 380.0f);
	InitialWindowSize = FVector2D(620.0f, 420.0f);
	MinWindowSize = FVector2D(360.0f, 240.0f);
}

FText UConversationWidget::MakeInitials(const FText& Name)
{
	FString Initials;
	bool bAtWordStart = true;

	for (const TCHAR Character : Name.ToString())
	{
		if (FChar::IsWhitespace(Character))
		{
			bAtWordStart = true;
			continue;
		}

		if (bAtWordStart)
		{
			Initials.AppendChar(FChar::ToUpper(Character));
			bAtWordStart = false;

			if (Initials.Len() >= 2)
			{
				break;
			}
		}
	}

	return FText::FromString(Initials);
}

UConversationComponent* UConversationWidget::GetConversation() const
{
	return UConversationComponent::Get(GetOwningPlayer());
}

void UConversationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UConversationWidget::HandleContinueClicked);
	}

	if (UConversationComponent* Conversation = GetConversation())
	{
		BoundConversation = Conversation;
		ViewChangedHandle = Conversation->OnViewChanged.AddUObject(this, &UConversationWidget::HandleViewChanged);
	}

	bDirty = true;
}

void UConversationWidget::NativeDestruct()
{
	// the component outlives this widget on every map, so an unbound handler would dangle
	if (UConversationComponent* Conversation = BoundConversation.Get())
	{
		Conversation->OnViewChanged.Remove(ViewChangedHandle);
	}

	BoundConversation = nullptr;

	Super::NativeDestruct();
}

void UConversationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// rebuilt the frame after a change rather than inside it: a pick arrives from one of this
	// window's own buttons, and the server's answer can come back before that click has finished
	if (bDirty)
	{
		bDirty = false;
		Rebuild();
	}

	// again once the new choices have been laid out, which shrinks the transcript's box after the
	// rebuild's own scroll - or the newest line ends up just out of sight
	if (ScrollToEndTicks > 0 && TranscriptScroll)
	{
		--ScrollToEndTicks;
		TranscriptScroll->ScrollToEnd();
	}
}

void UConversationWidget::HandleViewChanged()
{
	bDirty = true;
}

void UConversationWidget::RequestClose_Implementation()
{
	// closing is always allowed, so it goes at once; the announcement below is the Goodbye
	if (IsInViewport())
	{
		RemoveFromParent();
	}

	Super::RequestClose_Implementation();
}

void UConversationWidget::Rebuild()
{
	const UConversationComponent* Conversation = GetConversation();

	if (!Conversation)
	{
		return;
	}

	const FConversationView& View = Conversation->GetView();

	SetWindowTitle(View.NpcName.IsEmpty() ? WindowTitle : View.NpcName);

	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(View.NpcName);
	}

	// a portrait if they have one, their initials if not - the HUD's one fallback rule
	const AStrategyUnit* NpcUnit = Cast<AStrategyUnit>(View.Npc.Get());
	UTexture2D* Portrait = NpcUnit ? NpcUnit->GetPortraitTexture() : nullptr;

	if (PortraitImage)
	{
		if (Portrait)
		{
			PortraitImage->SetBrushFromTexture(Portrait);
		}

		PortraitImage->SetVisibility(Portrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (InitialsText)
	{
		InitialsText->SetText(MakeInitials(View.NpcName));
		InitialsText->SetVisibility(Portrait ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (TranscriptText)
	{
		TArray<FString> Lines;
		const int32 First = FMath::Max(0, View.Transcript.Num() - MaxTranscriptLines);

		for (int32 Index = First; Index < View.Transcript.Num(); ++Index)
		{
			const FConversationTranscriptLine& Line = View.Transcript[Index];

			Lines.Add(Line.Speaker.IsEmpty()
				? Line.Text.ToString()
				: FText::Format(LOCTEXT("TranscriptLine", "{0}: {1}"), Line.Speaker, Line.Text).ToString());
		}

		TranscriptText->SetText(FText::FromString(FString::Join(Lines, TEXT("\n\n"))));
	}

	if (TranscriptScroll)
	{
		TranscriptScroll->ScrollToEnd();
		ScrollToEndTicks = 2;
	}

	if (ContinueButton)
	{
		ContinueButton->SetVisibility(View.bAwaitingContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ContinueButton->SetIsEnabled(!View.bWaitingForServer);
	}

	if (!ChoiceBox)
	{
		return;
	}

	if (!ChoiceWidgetClass)
	{
		if (!bWarnedNoChoiceClass && View.Choices.Num() > 0)
		{
			UE_LOG(LogSmoresUI, Warning, TEXT("%s has no ChoiceWidgetClass set; the conversation's choices can't be shown."), *GetName());
			bWarnedNoChoiceClass = true;
		}

		return;
	}

	// grown as needed and never shrunk - spare rows are collapsed, so the button just clicked is
	// never destroyed underneath the click
	while (ChoiceWidgets.Num() < View.Choices.Num())
	{
		UConversationChoiceWidget* Choice = CreateWidget<UConversationChoiceWidget>(this, ChoiceWidgetClass);

		if (!Choice)
		{
			break;
		}

		Choice->OnChoiceClicked.AddUObject(this, &UConversationWidget::HandleChoiceClicked);
		ChoiceBox->AddChild(Choice);
		ChoiceWidgets.Add(Choice);
	}

	for (int32 Index = 0; Index < ChoiceWidgets.Num(); ++Index)
	{
		UConversationChoiceWidget* Choice = ChoiceWidgets[Index];

		if (!View.Choices.IsValidIndex(Index))
		{
			Choice->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FConversationShownChoice& Shown = View.Choices[Index];
		const FText Label = Shown.bGoodbye ? LOCTEXT("Goodbye", "Goodbye.") : Shown.Text;

		Choice->SetChoice(Index, Label, Shown.bEnabled && !View.bWaitingForServer, Shown.bEnabled ? ESmoresRefusalReason::None : Shown.Reason);
		Choice->SetVisibility(ESlateVisibility::Visible);
	}
}

void UConversationWidget::HandleContinueClicked()
{
	if (UConversationComponent* Conversation = GetConversation())
	{
		Conversation->RequestContinue();
	}
}

void UConversationWidget::HandleChoiceClicked(int32 Index)
{
	if (UConversationComponent* Conversation = GetConversation())
	{
		Conversation->RequestChoose(Index);
	}
}

#undef LOCTEXT_NAMESPACE
