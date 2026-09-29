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
		"Right mouse        On the ground: move order\n"
		"                   On something lit up: what the squad can do to it\n"
		"Tab                Cycle to the next squad member\n"
		"Double-click       Pick up an item, loot a container or a body, open or\n"
		"                   close a door, talk to someone - or, on empty ground,\n"
		"                   select everyone on screen\n"
		"\n"
		"ACTIONS - the nearest selected squad member walks over and does it\n"
		"O                  Loot the targeted container or body, or open or close\n"
		"                   the targeted door\n"
		"T                  Talk to the targeted person\n"
		"H                  Attack the targeted person, with everyone selected\n"
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
