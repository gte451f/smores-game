// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "TargetPanelWidget.h"
#include "TargetActionWidget.h"
#include "StrategyHUDCommands.h"
#include "SmoresUI.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/PanelWidget.h"

#define LOCTEXT_NAMESPACE "TargetPanelWidget"

void UTargetPanelWidget::SetTargetInfo(const FStrategyTargetInfo& NewTarget)
{
	if (Target.DrawsIdenticallyTo(NewTarget))
	{
		// nothing visible moved, but which actor is described still matters: two chests with the
		// same name at the same distance draw identically, and a click must reach the right one
		Target.Target = NewTarget.Target;
		return;
	}

	Target = NewTarget;

	RefreshTargetDisplay();
}

FText UTargetPanelWidget::GetClassificationLine() const
{
	// a negative distance means nothing is selected to measure from - the panel says what the
	// thing is and stays quiet about how far, rather than printing a number it doesn't have
	if (Target.DistanceMeters < 0.0f)
	{
		return Target.Classification;
	}

	const FText Distance = FText::Format(LOCTEXT("TargetDistance", "{0}m"), FText::AsNumber(FMath::RoundToInt(Target.DistanceMeters)));

	if (Target.Classification.IsEmpty())
	{
		return Distance;
	}

	return FText::Format(LOCTEXT("TargetClassificationLine", "{0} - {1}"), Target.Classification, Distance);
}

void UTargetPanelWidget::HandleActionClicked(FName ActionId)
{
	if (IStrategyHUDCommands* Commands = GetHUDCommands())
	{
		// the controller re-checks the rules rather than trusting the button that offered it - the
		// row is a frame old by the time a click lands. The panel's own target, not whatever the
		// controller has targeted since.
		Commands->RequestTargetAction(Target.Target.Get(), ActionId);
	}
}

void UTargetPanelWidget::ResizeActionRow(int32 ActionCount)
{
	if (!ActionBox)
	{
		return;
	}

	// shrink first, so the widgets that survive keep their place in the row
	while (ActionWidgets.Num() > ActionCount)
	{
		if (UTargetActionWidget* Removed = ActionWidgets.Pop())
		{
			Removed->OnActionClicked.RemoveAll(this);
			Removed->RemoveFromParent();
		}
	}

	if (ActionCount > ActionWidgets.Num() && !ActionWidgetClass)
	{
		// an unset class is a missing property on the WBP, not a bug here - the panel still draws
		// the name, the classification and the bar, so say what's missing once rather than
		// failing silently
		UE_LOG(LogSmoresUI, Warning, TEXT("UTargetPanelWidget has no ActionWidgetClass set; the target panel will show no actions."));
		return;
	}

	while (ActionWidgets.Num() < ActionCount)
	{
		UTargetActionWidget* Widget = CreateWidget<UTargetActionWidget>(this, ActionWidgetClass);

		if (!Widget)
		{
			break;
		}

		Widget->OnActionClicked.AddUObject(this, &UTargetPanelWidget::HandleActionClicked);

		ActionBox->AddChild(Widget);
		ActionWidgets.Add(Widget);
	}
}

void UTargetPanelWidget::RefreshTargetDisplay()
{
	const bool bHasTarget = Target.HasTarget();

	// With nothing targeted the panel goes away entirely rather than sitting there empty. Whether
	// that's right is a PIE call - a panel that lingered on the last thing looked at may read
	// better - and is listed in game-systems/hud-and-panels.md's Known Gaps.
	SetVisibility(bHasTarget ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (NameText)
	{
		NameText->SetText(Target.DisplayName);
	}

	if (ClassificationText)
	{
		ClassificationText->SetText(GetClassificationLine());
	}

	if (HealthBar)
	{
		HealthBar->SetVisibility(Target.bHasHealth ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		HealthBar->SetPercent(Target.HealthFraction);
	}

	ResizeActionRow(Target.Actions.Num());

	for (int32 Index = 0; Index < ActionWidgets.Num() && Index < Target.Actions.Num(); ++Index)
	{
		if (ActionWidgets[Index])
		{
			ActionWidgets[Index]->SetAction(Target.Actions[Index]);
		}
	}

	BP_UpdateTarget();
}

void UTargetPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// the first push may well have happened before this widget existed
	RefreshTargetDisplay();
}

#undef LOCTEXT_NAMESPACE
