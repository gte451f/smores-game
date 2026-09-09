// Copyright Epic Games, Inc. All Rights Reserved.

#include "MainMenuWidget.h"
#include "OptionsWidget.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "smores.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleContinueClicked);
	}

	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleNewGameClicked);
	}

	if (LoadGameButton)
	{
		LoadGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleLoadGameClicked);
	}

	if (OptionsButton)
	{
		OptionsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleOptionsClicked);
	}

	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleExitClicked);
	}
}

void UMainMenuWidget::HandleContinueClicked()
{
	EnterGame();
}

void UMainMenuWidget::HandleNewGameClicked()
{
	EnterGame();
}

void UMainMenuWidget::HandleLoadGameClicked()
{
	EnterGame();
}

void UMainMenuWidget::HandleOptionsClicked()
{
	if (!OptionsWidget)
	{
		if (!OptionsWidgetClass)
		{
			UE_LOG(Logsmores, Warning, TEXT("UMainMenuWidget has no OptionsWidgetClass set; can't open the options screen."));
			return;
		}

		OptionsWidget = CreateWidget<UOptionsWidget>(GetOwningPlayer(), OptionsWidgetClass);

		if (OptionsWidget)
		{
			OptionsWidget->OnOptionsClosed.AddDynamic(this, &UMainMenuWidget::HandleOptionsClosed);
		}
	}

	if (OptionsWidget)
	{
		OptionsWidget->AddToViewport(10);
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::HandleOptionsClosed()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UMainMenuWidget::HandleExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UMainMenuWidget::EnterGame()
{
	if (GameLevel.IsNull())
	{
		UE_LOG(Logsmores, Warning, TEXT("UMainMenuWidget has no GameLevel set; can't start/continue/load a game."));
		return;
	}

	// AMainMenuHUD put this PlayerController into UI-only input mode, which pointed Slate's focus
	// and mouse-capture path at this widget (in addition to setting bIgnoreInput on the
	// UGameViewportClient) - all of which outlives this PlayerController/widget. Directly resetting
	// just the UGameViewportClient's ignore-input/capture-mode/lock-mode flags (a prior attempt at
	// this fix) left mouse input completely dead in the next level even though those flags matched
	// a fresh PlayerController's defaults - the dangling Slate focus/capture path pointed at this
	// widget's now-destroyed SWidget was still swallowing mouse events. FInputModeGameAndUI's
	// ApplyInputMode redirects that focus back to the game viewport and releases any stale capture,
	// which is the actual fix; FInputModeGameOnly was avoided for this because it also switches to
	// permanent mouse capture, a locked cursor, and high-precision (relative-delta) mouse movement -
	// an FPS-style mode AStrategyPlayerController never opts into and that broke its
	// absolute-cursor-position click/drag selection.
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameAndUI());
	}

	RemoveFromParent();

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameLevel);
}
