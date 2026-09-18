// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "HelpPanelWidget.h"

#define LOCTEXT_NAMESPACE "HelpPanelWidget"

UHelpPanelWidget::UHelpPanelWidget()
{
	PanelId = EHUDPanel::Help;
	WindowTitle = LOCTEXT("HelpPanelTitle", "Controls");
}

FText UHelpPanelWidget::GetDefaultBodyText() const
{
	// Keep in step with game-systems/input-and-keybinds.md, which is the authoritative table.
	// Only list a key once it does something - see the class comment.
	return LOCTEXT("HelpPanelBindings",
		"CAMERA\n"
		"W A S D            Pan the camera\n"
		"Q / E              Lower / raise the camera\n"
		"Mouse wheel        Zoom\n"
		"Middle mouse       Hold to rotate the camera\n"
		"\n"
		"SQUAD\n"
		"Left mouse         Select a unit; hold and drag to box-select\n"
		"Shift + left       Add to or remove from the selection\n"
		"Right mouse        Move order\n"
		"Tab                Cycle to the next squad member\n"
		"Double-click       Pick up an item, open a container or a body, talk to\n"
		"                   someone, or - on empty ground - select everyone on screen\n"
		"\n"
		"ACTIONS\n"
		"O                  Open the nearest container, or loot a downed body\n"
		"T                  Talk to / trade with the targeted person\n"
		"H                  Attack the targeted person\n"
		"\n"
		"TIME\n"
		"Space              Pause / resume\n"
		"-  /  =            Slow down / speed up\n"
		"\n"
		"HUD\n"
		"L                  Expand / collapse the activity feed\n"
		"\n"
		"PANELS\n"
		"P                  Squad\n"
		"I                  Inventory\n"
		"M                  Map\n"
		"U                  Research\n"
		"F1                 This list\n"
		"\n"
		"WHILE AN INVENTORY WINDOW IS OPEN\n"
		"R                  Turn the item you are dragging 90 degrees\n"
		"Right-click        On an item in your own pack: wear it\n"
		"                   On a worn item: take it off\n");
}

#undef LOCTEXT_NAMESPACE
