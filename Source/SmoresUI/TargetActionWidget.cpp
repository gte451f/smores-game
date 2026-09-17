// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "TargetActionWidget.h"
#include "RefusalWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "TargetActionWidget"

void UTargetActionWidget::SetAction(const FTargetAction& InAction)
{
	// The panel rebuilds its row every frame, so the common case is being handed the same action
	// again. The label is fixed per id, so the id, the enabled flag and the reason are the whole
	// of what can change the display - comparing them here is what stops a Slate layout
	// invalidation per button per frame.
	if (Action.Id == InAction.Id
		&& Action.bEnabled == InAction.bEnabled
		&& Action.DisabledReason == InAction.DisabledReason)
	{
		return;
	}

	Action = InAction;

	RefreshActionDisplay();
}

FText UTargetActionWidget::GetActionLabel() const
{
	if (Action.KeyHint.IsEmpty())
	{
		return Action.Label;
	}

	return FText::Format(LOCTEXT("ActionWithKey", "{0} [{1}]"), Action.Label, Action.KeyHint);
}

void UTargetActionWidget::HandleClicked()
{
	// a disabled UButton never fires OnClicked, so this is belt and braces - but the row is
	// rebuilt every frame and a click can land on the frame an action goes away
	if (!Action.bEnabled)
	{
		return;
	}

	OnActionClicked.Broadcast(Action.Id);
}

void UTargetActionWidget::RefreshActionDisplay()
{
	if (LabelText)
	{
		LabelText->SetText(GetActionLabel());
	}

	if (ReasonText)
	{
		// the wording comes from the one place refusals are phrased, so "Too far away" reads
		// identically here and on the refusal line
		const FText Reason = Action.bEnabled ? FText::GetEmpty() : URefusalWidget::GetRefusalText(Action.DisabledReason);

		ReasonText->SetText(Reason);

		// Collapsed rather than Hidden so an action with no reason to give (attacking someone
		// already fighting you) doesn't leave a gap the width of the longest refusal
		ReasonText->SetVisibility(Reason.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (ActionButton)
	{
		ActionButton->SetIsEnabled(Action.bEnabled);
	}

	BP_UpdateAction();
}

void UTargetActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ActionButton)
	{
		ActionButton->OnClicked.AddUniqueDynamic(this, &UTargetActionWidget::HandleClicked);
	}

	// SetAction may well have run before this widget was constructed
	RefreshActionDisplay();
}

#undef LOCTEXT_NAMESPACE
