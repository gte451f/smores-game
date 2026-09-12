// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryCellWidget.h"
#include "Components/Border.h"

void UInventoryCellWidget::SetCell(FIntPoint InCellCoord)
{
	CellCoord = InCellCoord;

	// the grid has to be one continuous drop surface: a hit-test-invisible cell would let the
	// pointer fall through to whatever happens to sit behind the panel, and the drop would
	// never bubble up to this window's handler
	SetVisibility(ESlateVisibility::Visible);

	RefreshVisuals();
}

void UInventoryCellWidget::SetHighlight(EInventoryCellHighlight InHighlight)
{
	if (Highlight == InHighlight)
	{
		return;
	}

	Highlight = InHighlight;

	RefreshVisuals();
}

void UInventoryCellWidget::RefreshVisuals()
{
	if (CellBorder)
	{
		switch (Highlight)
		{
		case EInventoryCellHighlight::Valid:
			CellBorder->SetBrushColor(ValidHighlightColor);
			break;

		case EInventoryCellHighlight::Invalid:
			CellBorder->SetBrushColor(InvalidHighlightColor);
			break;

		default:
			CellBorder->SetBrushColor(EmptyCellColor);
			break;
		}
	}

	BP_HighlightChanged(Highlight);
}

void UInventoryCellWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// CellBorder only binds once the widget tree is built, so the initial tint has to wait
	// until here rather than being applied by whichever SetCell call preceded it
	RefreshVisuals();
}
