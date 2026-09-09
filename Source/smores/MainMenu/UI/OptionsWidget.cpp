// Copyright Epic Games, Inc. All Rights Reserved.

#include "OptionsWidget.h"
#include "Components/Button.h"

void UOptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (KeybindingsButton)
	{
		KeybindingsButton->OnClicked.AddDynamic(this, &UOptionsWidget::HandleKeybindingsClicked);
	}

	if (AudioButton)
	{
		AudioButton->OnClicked.AddDynamic(this, &UOptionsWidget::HandleAudioClicked);
	}

	if (GraphicsButton)
	{
		GraphicsButton->OnClicked.AddDynamic(this, &UOptionsWidget::HandleGraphicsClicked);
	}

	if (GameButton)
	{
		GameButton->OnClicked.AddDynamic(this, &UOptionsWidget::HandleGameClicked);
	}

	// default to whichever category panel is bound first
	if (KeybindingsPanel)
	{
		ShowCategory(KeybindingsPanel);
	}
	else if (AudioPanel)
	{
		ShowCategory(AudioPanel);
	}
	else if (GraphicsPanel)
	{
		ShowCategory(GraphicsPanel);
	}
	else if (GamePanel)
	{
		ShowCategory(GamePanel);
	}
}

void UOptionsWidget::RequestClose_Implementation()
{
	Super::RequestClose_Implementation();

	if (IsInViewport())
	{
		RemoveFromParent();
	}

	OnOptionsClosed.Broadcast();
}

void UOptionsWidget::HandleKeybindingsClicked()
{
	ShowCategory(KeybindingsPanel);
}

void UOptionsWidget::HandleAudioClicked()
{
	ShowCategory(AudioPanel);
}

void UOptionsWidget::HandleGraphicsClicked()
{
	ShowCategory(GraphicsPanel);
}

void UOptionsWidget::HandleGameClicked()
{
	ShowCategory(GamePanel);
}

void UOptionsWidget::ShowCategory(UWidget* PanelToShow)
{
	for (UWidget* Panel : { KeybindingsPanel.Get(), AudioPanel.Get(), GraphicsPanel.Get(), GamePanel.Get() })
	{
		if (Panel)
		{
			Panel->SetVisibility(Panel == PanelToShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}
