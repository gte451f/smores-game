// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ActivityEntryWidget.h"
#include "Components/TextBlock.h"

void UActivityEntryWidget::SetEntry(const FActivityEntry& InEntry)
{
	// Ids are never reused, so this is the whole comparison - two lines with identical words are
	// still different entries, and the same entry is never worth redrawing. See FActivityEntry::Id.
	if (Entry.Id == InEntry.Id && Entry.Id != 0)
	{
		return;
	}

	Entry = InEntry;

	RefreshEntryDisplay();
}

FLinearColor UActivityEntryWidget::GetSeverityColor() const
{
	switch (Entry.Severity)
	{
	case EActivitySeverity::Good:
		return GoodColor;

	case EActivitySeverity::Warning:
		return WarningColor;

	case EActivitySeverity::Bad:
		return BadColor;

	case EActivitySeverity::Normal:
	default:
		return NormalColor;
	}
}

void UActivityEntryWidget::RefreshEntryDisplay()
{
	const FLinearColor Color = GetSeverityColor();

	if (MessageText)
	{
		MessageText->SetText(Entry.Text);
		MessageText->SetColorAndOpacity(FSlateColor(Color));
	}

	if (SourceText)
	{
		SourceText->SetText(Entry.Source);

		// Collapsed rather than Hidden, so a line nobody in particular said doesn't leave a gap
		// the width of the longest name
		SourceText->SetVisibility(Entry.Source.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		SourceText->SetColorAndOpacity(FSlateColor(Color));
	}

	BP_UpdateEntry();
}

void UActivityEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// SetEntry may well have run before this widget was constructed
	RefreshEntryDisplay();
}
