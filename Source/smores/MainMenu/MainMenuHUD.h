// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

class UMainMenuWidget;

/**
 *  HUD for the main menu level. Spawns the main menu widget and puts the owning player
 *  controller into UI-only input mode with the cursor shown - mirrors how AStrategyHUD
 *  owns its UI widget's lifecycle.
 */
UCLASS(abstract)
class AMainMenuHUD : public AHUD
{
	GENERATED_BODY()

protected:

	/** Type of main menu widget to spawn */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMainMenuWidget> MenuWidgetClass;

	/** Pointer to the spawned main menu widget */
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> MenuWidget;

public:

	virtual void BeginPlay() override;
};
