// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ActivityFeedWidget.h"
#include "ActivityEntryWidget.h"
#include "SmoresActivityLog.h"
#include "SmoresUI.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"

#define LOCTEXT_NAMESPACE "ActivityFeedWidget"

USmoresActivityLog* UActivityFeedWidget::GetActivityLog() const
{
	// keyed off this widget's own player, which is what makes the feed per-local-player rather
	// than "the" player's - see USmoresActivityLog
	return USmoresActivityLog::Get(GetOwningPlayer());
}

void UActivityFeedWidget::RefreshFeed()
{
	if (bEntriesDirty)
	{
		RebuildEntries();
	}

	// The fade is a function of wall-clock time, and nothing broadcasts when a second passes - so
	// unlike the lines themselves it genuinely has to be recomputed every frame. It's a clamp and
	// a divide per visible line, against a handful of lines.
	if (bExpanded)
	{
		// expanded is the history: nothing fades, or the record the player opened the feed to read
		// would dim while they were reading it
		for (UActivityEntryWidget* EntryWidget : EntryWidgets)
		{
			if (EntryWidget)
			{
				EntryWidget->SetFadeAlpha(1.0f);
			}
		}

		return;
	}

	const double Now = FPlatformTime::Seconds();

	for (UActivityEntryWidget* EntryWidget : EntryWidgets)
	{
		if (!EntryWidget)
		{
			continue;
		}

		const double Age = Now - EntryWidget->GetEntry().Timestamp;

		if (Age <= FadeAfterSeconds)
		{
			EntryWidget->SetFadeAlpha(1.0f);
			continue;
		}

		const float Alpha = 1.0f - static_cast<float>((Age - FadeAfterSeconds) / FadeDurationSeconds);

		EntryWidget->SetFadeAlpha(Alpha);
	}
}

void UActivityFeedWidget::ToggleExpanded()
{
	bExpanded = !bExpanded;

	// the number of lines shown changes with the state, so the list has to be rebuilt rather than
	// merely re-faded
	bEntriesDirty = true;

	ApplyExpandedHeight();

	RefreshFeed();

	BP_UpdateFeed();
}

FText UActivityFeedWidget::GetEmptyText() const
{
	if (ActiveTab == EActivityFeedTab::Quests)
	{
		// Not "nothing yet" - nothing ever, until the system exists. Saying which system owns it
		// is the same honesty the stub panels use, and for the same reason: an empty tab the
		// player can't tell apart from a broken one teaches them to stop looking.
		return LOCTEXT("FeedQuestsStub", "Objectives will appear here once quests exist.");
	}

	return LOCTEXT("FeedEmpty", "Nothing yet.");
}

void UActivityFeedWidget::SetActiveTab(EActivityFeedTab Tab)
{
	if (ActiveTab == Tab)
	{
		return;
	}

	ActiveTab = Tab;
	bEntriesDirty = true;

	RefreshTabStates();
	RefreshFeed();
}

void UActivityFeedWidget::HandleLogTabClicked()
{
	SetActiveTab(EActivityFeedTab::Log);
}

void UActivityFeedWidget::HandleSquadTabClicked()
{
	SetActiveTab(EActivityFeedTab::Squad);
}

void UActivityFeedWidget::HandleQuestsTabClicked()
{
	SetActiveTab(EActivityFeedTab::Quests);
}

void UActivityFeedWidget::HandleCommsTabClicked()
{
	SetActiveTab(EActivityFeedTab::Comms);
}

void UActivityFeedWidget::HandleEntryAdded(const FActivityEntry& Entry)
{
	// deliberately not a rebuild: several things can post inside one frame (a fight resolving, a
	// trade), and the frame's push is where the one rebuild belongs
	bEntriesDirty = true;
}

void UActivityFeedWidget::ResizeEntryRows(int32 LineCount)
{
	if (!EntryBox)
	{
		return;
	}

	// shrink first, so the lines that survive keep their place in the list
	while (EntryWidgets.Num() > LineCount)
	{
		if (UActivityEntryWidget* Removed = EntryWidgets.Pop())
		{
			Removed->RemoveFromParent();
		}
	}

	if (LineCount > EntryWidgets.Num() && !EntryWidgetClass)
	{
		// an unset class is a missing property on the WBP, not a bug here - the tabs still work,
		// so say what's missing once rather than drawing an empty feed in silence
		UE_LOG(LogSmoresUI, Warning, TEXT("UActivityFeedWidget has no EntryWidgetClass set; the feed will show no lines."));
		return;
	}

	while (EntryWidgets.Num() < LineCount)
	{
		UActivityEntryWidget* Widget = CreateWidget<UActivityEntryWidget>(this, EntryWidgetClass);

		if (!Widget)
		{
			break;
		}

		EntryBox->AddChild(Widget);
		EntryWidgets.Add(Widget);
	}
}

void UActivityFeedWidget::RebuildEntries()
{
	bEntriesDirty = false;

	TArray<FActivityEntry> Entries;

	if (const USmoresActivityLog* Log = GetActivityLog())
	{
		// LOG is "no filter" rather than a category of its own - see EActivityFeedTab
		switch (ActiveTab)
		{
		case EActivityFeedTab::Squad:
			Entries = Log->GetEntries(EActivityCategory::Squad);
			break;

		case EActivityFeedTab::Quests:
			Entries = Log->GetEntries(EActivityCategory::Quests);
			break;

		case EActivityFeedTab::Comms:
			Entries = Log->GetEntries(EActivityCategory::Comms);
			break;

		case EActivityFeedTab::Log:
		default:
			Entries = Log->GetEntries();
			break;
		}
	}

	const int32 VisibleCount = FMath::Min(Entries.Num(), bExpanded ? ExpandedEntryCount : CollapsedEntryCount);

	// the newest lines, still oldest-first within themselves, so the feed reads downwards in the
	// order things happened rather than upwards
	const int32 FirstIndex = Entries.Num() - VisibleCount;

	ResizeEntryRows(VisibleCount);

	for (int32 Index = 0; Index < EntryWidgets.Num() && Index < VisibleCount; ++Index)
	{
		if (EntryWidgets[Index])
		{
			EntryWidgets[Index]->SetEntry(Entries[FirstIndex + Index]);
		}
	}

	if (EmptyText)
	{
		EmptyText->SetText(GetEmptyText());
		EmptyText->SetVisibility(VisibleCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	BP_UpdateFeed();
}

void UActivityFeedWidget::RefreshTabStates()
{
	// a tab with no button is simply unreachable, which is a WBP that hasn't been given one rather
	// than an error - the same shape as every other BindWidgetOptional on the HUD
	auto TintTab = [this](UButton* Button, EActivityFeedTab Tab)
	{
		if (Button)
		{
			Button->SetColorAndOpacity(Tab == ActiveTab ? ActiveTabColor : InactiveTabColor);
		}
	};

	TintTab(LogTabButton, EActivityFeedTab::Log);
	TintTab(SquadTabButton, EActivityFeedTab::Squad);
	TintTab(QuestsTabButton, EActivityFeedTab::Quests);
	TintTab(CommsTabButton, EActivityFeedTab::Comms);
}

void UActivityFeedWidget::ApplyExpandedHeight()
{
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);

	if (!CanvasSlot || ExpandedHeight <= 0.0f || CollapsedHeight < 0.0f)
	{
		return;
	}

	const FVector2D CurrentSize = CanvasSlot->GetSize();
	const float NewHeight = bExpanded ? ExpandedHeight : CollapsedHeight;

	if (FMath::IsNearlyEqual(CurrentSize.Y, NewHeight))
	{
		return;
	}

	// The feed is anchored to the bottom of the screen, so it has to grow *upwards* - a taller box
	// that kept its top edge would push its newest lines off the bottom of the viewport, which is
	// exactly the half the player was reading. Where the top edge has to move depends on the slot's
	// alignment, so that is read rather than assumed: at alignment 1 the position already
	// describes the bottom edge and nothing needs moving, at 0 it describes the top and the whole
	// difference does.
	const FVector2D Position = CanvasSlot->GetPosition();
	const float AlignmentY = CanvasSlot->GetAlignment().Y;

	CanvasSlot->SetPosition(FVector2D(Position.X, Position.Y + (CurrentSize.Y - NewHeight) * (1.0f - AlignmentY)));
	CanvasSlot->SetSize(FVector2D(CurrentSize.X, NewHeight));
}

void UActivityFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// captured before anything resizes it, so collapsing restores the authored layout exactly
	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CollapsedHeight = CanvasSlot->GetSize().Y;
	}

	if (LogTabButton)
	{
		LogTabButton->OnClicked.AddUniqueDynamic(this, &UActivityFeedWidget::HandleLogTabClicked);
	}

	if (SquadTabButton)
	{
		SquadTabButton->OnClicked.AddUniqueDynamic(this, &UActivityFeedWidget::HandleSquadTabClicked);
	}

	if (QuestsTabButton)
	{
		QuestsTabButton->OnClicked.AddUniqueDynamic(this, &UActivityFeedWidget::HandleQuestsTabClicked);
	}

	if (CommsTabButton)
	{
		CommsTabButton->OnClicked.AddUniqueDynamic(this, &UActivityFeedWidget::HandleCommsTabClicked);
	}

	if (USmoresActivityLog* Log = GetActivityLog())
	{
		// held weakly so NativeDestruct can unbind from the same object it bound to, even if the
		// local player has gone away in between
		BoundLog = Log;

		Log->OnEntryAdded.AddUObject(this, &UActivityFeedWidget::HandleEntryAdded);
	}

	RefreshTabStates();

	// entries may well have been posted before this widget existed - the log outlives it
	bEntriesDirty = true;

	RefreshFeed();
}

void UActivityFeedWidget::NativeDestruct()
{
	if (USmoresActivityLog* Log = BoundLog.Get())
	{
		// the subsystem outlives every widget bound to it, so an unbound delegate here is a
		// dangling handler on the next map load rather than a leak that gets collected
		Log->OnEntryAdded.RemoveAll(this);
	}

	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
