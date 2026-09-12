// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UOptionsWidget;

/**
 *  Top-level main menu screen: Continue / Start New Game / Load Game / Options / Exit.
 *  There's no save system yet, so Continue, Load Game, and Start New Game are all
 *  equivalent for now - they all just open GameLevel (see EnterGame). Options spawns a
 *  UOptionsWidget on top and hides this widget until it closes.
 */
UCLASS(abstract)
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Name it "ContinueButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	/** Name it "NewGameButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewGameButton;

	/** Name it "LoadGameButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadGameButton;

	/** Name it "OptionsButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionsButton;

	/** Name it "ExitButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ExitButton;

	/** Level opened by Continue, Load Game, and Start New Game until a save system exists */
	UPROPERTY(EditAnywhere, Category = "Main Menu")
	TSoftObjectPtr<UWorld> GameLevel;

	/** Widget class spawned when Options is clicked */
	UPROPERTY(EditAnywhere, Category = "Main Menu")
	TSubclassOf<UOptionsWidget> OptionsWidgetClass;

	/** Options widget, spawned on first use */
	UPROPERTY()
	TObjectPtr<UOptionsWidget> OptionsWidget;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleLoadGameClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleExitClicked();

	UFUNCTION()
	void HandleOptionsClosed();

	/** Opens GameLevel - shared by Continue/Load Game/Start New Game until save games exist */
	void EnterGame();
};
