// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ExamineWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "ExamineWidget"

UExamineWidget::UExamineWidget()
{
	WindowTitle = LOCTEXT("ExamineTitle", "Examine");

	// small: a name, a line and a few sentences, not a panel to work in
	InitialWindowPosition = FVector2D(480.f, 160.f);
	InitialWindowSize = FVector2D(440.f, 320.f);
}

void UExamineWidget::SetExamine(const FText& Title, const FText& InClassification, const FText& InBody)
{
	SetWindowTitle(Title);

	Classification = InClassification;
	Body = InBody;

	RefreshExamineDisplay();
}

void UExamineWidget::RequestClose_Implementation()
{
	if (IsInViewport())
	{
		RemoveFromParent();
	}

	// after the window is actually gone, so a listener reacting to this sees it that way
	Super::RequestClose_Implementation();
}

void UExamineWidget::RefreshExamineDisplay()
{
	if (ClassificationText)
	{
		ClassificationText->SetText(Classification);
		ClassificationText->SetVisibility(Classification.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (BodyText)
	{
		BodyText->SetText(Body);
	}

	BP_UpdateExamine();
}

void UExamineWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// SetExamine runs before the window is first added to the viewport
	RefreshExamineDisplay();
}

#undef LOCTEXT_NAMESPACE
