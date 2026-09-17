// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "HUDPanel.h"
#include "NavRailWidget.generated.h"

class UButton;

/**
 *  The left-hand nav rail: one button per panel, each opening or closing that panel through
 *  IStrategyHUDCommands::RequestPanel.
 *
 *  Pressing the key and clicking the button run the same path on purpose - a rail button that
 *  duplicated the controller's rules would eventually disagree with the key, and a refusal
 *  (asking for the inventory with no pawn selected) would come out differently depending on how
 *  the player asked.
 *
 *  Every button is a real UButton, which is what stops a click on one from also drag-selecting
 *  the world behind the HUD; the rail's own background catches the gaps between them (see
 *  UHUDRegionWidget). The key hint and label on each button are static text authored in the WBP -
 *  they are layout, not behaviour, and the authoritative list of keys is
 *  game-systems/input-and-keybinds.md, with UHelpPanelWidget showing it in game.
 */
UCLASS(abstract)
class SMORESUI_API UNavRailWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** Opens the squad roster. Name it "SquadButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SquadButton;

	/** Opens the selected pawn's pack. Name it "InventoryButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> InventoryButton;

	/** Opens the world map. Name it "MapButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MapButton;

	/** Opens research. Name it "ResearchButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResearchButton;

	/** Opens the keybind list. Name it "HelpButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> HelpButton;

	/** Tint applied to the button whose panel is currently open */
	UPROPERTY(EditAnywhere, Category = "Nav Rail")
	FLinearColor ActiveButtonTint = FLinearColor(1.0f, 0.85f, 0.45f, 1.0f);

	/** Tint applied to every other button */
	UPROPERTY(EditAnywhere, Category = "Nav Rail")
	FLinearColor InactiveButtonTint = FLinearColor::White;

private:

	/**
	 *  One bit per entry in RailPanels, recording what the tints were last set to. Starts at a
	 *  value no real mask can take so the first refresh always paints; after that the whole
	 *  refresh is a compare and an early return, which matters because DrawHUD calls it every
	 *  frame.
	 */
	uint8 LastOpenMask = MAX_uint8;

public:

	/** The panels this rail has a button for, in top-to-bottom order */
	static const TArray<EHUDPanel>& GetRailPanels();

	/** Repaints the "this panel is open" state. Pushed every frame by AStrategyHUD::DrawHUD. */
	void RefreshPanelStates();

protected:

	/** The button for a panel, or null if this WBP doesn't provide one */
	UButton* GetButtonForPanel(EHUDPanel Panel) const;

	/** Asks the controller to open or close a panel, if this widget can reach one */
	void RequestPanel(EHUDPanel Panel);

	UFUNCTION()
	void HandleSquadClicked();

	UFUNCTION()
	void HandleInventoryClicked();

	UFUNCTION()
	void HandleMapClicked();

	UFUNCTION()
	void HandleResearchClicked();

	UFUNCTION()
	void HandleHelpClicked();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
