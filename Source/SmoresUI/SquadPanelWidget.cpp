// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SquadPanelWidget.h"

#define LOCTEXT_NAMESPACE "SquadPanelWidget"

USquadPanelWidget::USquadPanelWidget()
{
	PanelId = EHUDPanel::Squad;
	WindowTitle = LOCTEXT("SquadPanelTitle", "Squad");
}

FText USquadPanelWidget::GetDefaultBodyText() const
{
	return LOCTEXT("SquadPanelStub",
		"The squad roster will live here: every member's name, condition, skills and gear, at a "
		"size a portrait has no room for.\n\nNothing is wired up yet. Design intent: "
		"game-design/characters-and-squads.md.");
}

#undef LOCTEXT_NAMESPACE
