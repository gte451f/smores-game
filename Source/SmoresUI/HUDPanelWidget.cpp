// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "HUDPanelWidget.h"
#include "Components/TextBlock.h"

void UHUDPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BodyText)
	{
		BodyText->SetText(GetBodyText());
	}
}

FText UHUDPanelWidget::GetBodyText() const
{
	return BodyOverride.IsEmpty() ? GetDefaultBodyText() : BodyOverride;
}

FText UHUDPanelWidget::GetDefaultBodyText() const
{
	return FText::GetEmpty();
}

void UHUDPanelWidget::RequestClose_Implementation()
{
	if (IsInViewport())
	{
		RemoveFromParent();
	}

	// after the window is actually gone, so a listener reacting to this sees it that way
	Super::RequestClose_Implementation();
}
