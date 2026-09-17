// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ResearchPanelWidget.h"

#define LOCTEXT_NAMESPACE "ResearchPanelWidget"

UResearchPanelWidget::UResearchPanelWidget()
{
	PanelId = EHUDPanel::Research;
	WindowTitle = LOCTEXT("ResearchPanelTitle", "Research");
}

FText UResearchPanelWidget::GetDefaultBodyText() const
{
	return LOCTEXT("ResearchPanelStub",
		"Research and crafting recipes will live here: what the squad knows how to make, and what "
		"it would take to learn the rest.\n\nNothing is researchable yet. Design intent: "
		"game-design/tech-and-crafting.md.");
}

#undef LOCTEXT_NAMESPACE
