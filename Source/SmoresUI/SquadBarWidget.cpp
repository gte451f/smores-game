// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SquadBarWidget.h"
#include "SquadPortraitWidget.h"
#include "StrategyHUDCommands.h"
#include "StrategyUnit.h"
#include "SmoresUI.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "SquadBarWidget"

void USquadBarWidget::SetSquad(const TArray<AStrategyUnit*>& InRoster, const TArray<AStrategyUnit*>& SelectedUnits)
{
	// The roster itself changes only when a unit joins, dies or streams out, so this is the cheap
	// early-out for the overwhelmingly common frame. Per-unit state (health, selection) is *not*
	// compared here - each portrait does that for itself in SetUnit, which is the only widget that
	// knows what it is currently drawing.
	bool bRosterChanged = Roster.Num() != InRoster.Num();

	if (!bRosterChanged)
	{
		for (int32 Index = 0; Index < InRoster.Num(); ++Index)
		{
			if (Roster[Index].Get() != InRoster[Index])
			{
				bRosterChanged = true;
				break;
			}
		}
	}

	if (bRosterChanged)
	{
		Roster.Reset(InRoster.Num());

		for (AStrategyUnit* CurrentUnit : InRoster)
		{
			Roster.Add(CurrentUnit);
		}

		ResizePortraitRow(Roster.Num());
	}

	for (int32 Index = 0; Index < PortraitWidgets.Num() && Index < InRoster.Num(); ++Index)
	{
		if (PortraitWidgets[Index])
		{
			PortraitWidgets[Index]->SetUnit(InRoster[Index], SelectedUnits.Contains(InRoster[Index]));
		}
	}

	if (SelectedCount != SelectedUnits.Num())
	{
		SelectedCount = SelectedUnits.Num();

		if (SelectionCountText)
		{
			SelectionCountText->SetText(GetSelectionCountLine());
		}

		BP_UpdateSquadBar();
	}
}

FText USquadBarWidget::GetSelectionCountLine() const
{
	if (SelectedCount <= 0)
	{
		// blank rather than "0 selected" - a readout that reports nothing is noise, and the
		// portraits already say the squad exists
		return FText::GetEmpty();
	}

	return FText::Format(LOCTEXT("SquadSelectionCount", "{0} selected"), FText::AsNumber(SelectedCount));
}

void USquadBarWidget::HandlePortraitClicked(AStrategyUnit* Unit, bool bFocusCamera)
{
	if (IStrategyHUDCommands* Commands = GetHUDCommands())
	{
		// what selecting means - deselecting the rest, closing a stale inventory, cutting the
		// camera - is the controller's, exactly as it is for the Tab cycle
		Commands->RequestSelectUnit(Unit, bFocusCamera);
	}
}

void USquadBarWidget::ResizePortraitRow(int32 UnitCount)
{
	if (!PortraitBox)
	{
		return;
	}

	// shrink first, so the portraits that survive keep their place in the row
	while (PortraitWidgets.Num() > UnitCount)
	{
		if (USquadPortraitWidget* Removed = PortraitWidgets.Pop())
		{
			Removed->OnPortraitClicked.RemoveAll(this);
			Removed->RemoveFromParent();
		}
	}

	if (UnitCount > PortraitWidgets.Num() && !PortraitWidgetClass)
	{
		// an unset class is a missing property on the WBP, not a bug here - the count readout
		// still works, so say what's missing once rather than drawing an empty bar in silence
		UE_LOG(LogSmoresUI, Warning, TEXT("USquadBarWidget has no PortraitWidgetClass set; the squad bar will show no portraits."));
		return;
	}

	while (PortraitWidgets.Num() < UnitCount)
	{
		USquadPortraitWidget* Widget = CreateWidget<USquadPortraitWidget>(this, PortraitWidgetClass);

		if (!Widget)
		{
			break;
		}

		Widget->OnPortraitClicked.AddUObject(this, &USquadBarWidget::HandlePortraitClicked);

		PortraitBox->AddChild(Widget);
		PortraitWidgets.Add(Widget);
	}
}

void USquadBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectionCountText)
	{
		// the first push may well have happened before this widget existed
		SelectionCountText->SetText(GetSelectionCountLine());
	}
}

#undef LOCTEXT_NAMESPACE
