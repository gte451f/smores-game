// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "OptionsWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOptionsClosed);

/**
 *  Options interface opened from the main menu (or, later, an in-game pause menu):
 *  four categories - Keybindings, Audio, Graphics, Game - each with its own placeholder
 *  panel, only one visible at a time. None of the underlying settings systems exist yet,
 *  so the panels are empty; this just wires up the category switching and the close/back
 *  affordance (reusing UWindowWidget's CloseButton chrome) so it reads as a real interface.
 */
UCLASS(abstract)
class UOptionsWidget : public UWindowWidget
{
	GENERATED_BODY()

public:

	/** Broadcast when this widget closes (see RequestClose_Implementation), so an owner can react (e.g. show the main menu again) */
	UPROPERTY(BlueprintAssignable, Category = "Options")
	FOnOptionsClosed OnOptionsClosed;

protected:

	/** Name it "KeybindingsButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> KeybindingsButton;

	/** Name it "AudioButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AudioButton;

	/** Name it "GraphicsButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GraphicsButton;

	/** Name it "GameButton" in the WBP to auto-bind */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GameButton;

	/** Placeholder panel shown when KeybindingsButton is clicked. Name it "KeybindingsPanel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> KeybindingsPanel;

	/** Placeholder panel shown when AudioButton is clicked. Name it "AudioPanel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> AudioPanel;

	/** Placeholder panel shown when GraphicsButton is clicked. Name it "GraphicsPanel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GraphicsPanel;

	/** Placeholder panel shown when GameButton is clicked. Name it "GamePanel" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GamePanel;

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface

	UFUNCTION()
	void HandleKeybindingsClicked();

	UFUNCTION()
	void HandleAudioClicked();

	UFUNCTION()
	void HandleGraphicsClicked();

	UFUNCTION()
	void HandleGameClicked();

private:

	/** Shows PanelToShow (if bound) and collapses the other three category panels */
	void ShowCategory(UWidget* PanelToShow);
};
