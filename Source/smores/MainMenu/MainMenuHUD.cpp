// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "MainMenuHUD.h"
#include "MainMenuWidget.h"
#include "smores.h"

void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!MenuWidgetClass)
	{
		UE_LOG(Logsmores, Warning, TEXT("AMainMenuHUD has no MenuWidgetClass set; the main menu will not appear."));
		return;
	}

	MenuWidget = CreateWidget<UMainMenuWidget>(GetOwningPlayerController(), MenuWidgetClass);

	if (!MenuWidget)
	{
		return;
	}

	MenuWidget->AddToViewport(0);

	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}
