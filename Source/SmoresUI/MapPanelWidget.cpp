// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "MapPanelWidget.h"

#define LOCTEXT_NAMESPACE "MapPanelWidget"

UMapPanelWidget::UMapPanelWidget()
{
	PanelId = EHUDPanel::Map;
	WindowTitle = LOCTEXT("MapPanelTitle", "Map");
}

FText UMapPanelWidget::GetDefaultBodyText() const
{
	return LOCTEXT("MapPanelStub",
		"The world map will live here: where the squad is, where it can travel, and what the "
		"world is doing while you are elsewhere.\n\nThere is no world map yet. Design intent: "
		"game-design/world-map-and-travel.md.");
}

#undef LOCTEXT_NAMESPACE
