// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "BarkBubbleWidget.h"
#include "Components/TextBlock.h"

void UBarkBubbleWidget::SetLine(const FText& InLine, uint32 Serial)
{
	Line = InLine;
	ShownSerial = Serial;

	RefreshLineDisplay();
}

void UBarkBubbleWidget::RefreshLineDisplay()
{
	if (LineText)
	{
		LineText->SetText(Line);

		// a fixed wrap width rather than AutoWrapText: the bubble sits in an auto-sized canvas slot,
		// so there is no allotted width for auto-wrap to wrap against, and a long line would run off
		// in one row across the screen
		LineText->SetAutoWrapText(false);
		LineText->SetWrapTextAt(MaxTextWidth);
	}

	BP_LineChanged();
}

void UBarkBubbleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// never takes a click - the unit under the bubble is what a click there means
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// SetLine may well have run before this widget was constructed
	RefreshLineDisplay();
}
