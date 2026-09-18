// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDRegionWidget.h"
#include "ActivityEntry.h"
#include "ActivityFeedWidget.generated.h"

class UActivityEntryWidget;
class UButton;
class UPanelWidget;
class USmoresActivityLog;
class UTextBlock;

/** Which tab of the feed is showing. Log is "no filter", which is why it isn't an EActivityCategory. */
UENUM(BlueprintType)
enum class EActivityFeedTab : uint8
{
	/** Everything, in the order it happened */
	Log,

	/** The squad's own news - damage, downs, loot, refusals */
	Squad,

	/** Objectives. Empty by design - see UActivityFeedWidget::GetEmptyText. */
	Quests,

	/** Talking and trading */
	Comms
};

/**
 *  The bottom-right activity feed: what just happened, colour-coded, and expandable with `L` into
 *  a longer history.
 *
 *  **The feed is the record; URefusalWidget is the answer.** A refused action does both - the line
 *  at the cursor says why *now*, and the feed remembers it for the player who was looking
 *  somewhere else. Neither replaces the other, and refusals-and-feedback.md owns that split.
 *
 *  It reads USmoresActivityLog, a per-local-player subsystem, and subscribes to OnEntryAdded so it
 *  knows when to rebuild rather than diffing a list every frame.
 *
 *  **Nothing here is on a timer.** A line stays exactly as bright as the day it was posted and
 *  leaves only when newer news pushes it off the bottom - so the difference between collapsed and
 *  expanded is purely how many lines fit, which is what `L` buys. An earlier version faded a line
 *  out after eight seconds; that was dropped because the feed is a *record*, and a record that has
 *  emptied itself by the time the player looks up from the fight is no record at all. If the corner
 *  ever needs to be quieter, dim old lines rather than removing them.
 */
UCLASS(abstract)
class SMORESUI_API UActivityFeedWidget : public UHUDRegionWidget
{
	GENERATED_BODY()

protected:

	/** Holds the entry lines, oldest first. Name it "EntryBox" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EntryBox;

	/** Shown instead of the lines when the current tab has nothing in it. Name it "EmptyText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	/** The LOG tab. Name it "LogTabButton" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LogTabButton;

	/** The SQUAD tab. Name it "SquadTabButton" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SquadTabButton;

	/** The QUESTS tab. Name it "QuestsTabButton" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuestsTabButton;

	/** The COMMS tab. Name it "CommsTabButton" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CommsTabButton;

	/** Widget class used for each line */
	UPROPERTY(EditAnywhere, Category = "Activity Feed")
	TSubclassOf<UActivityEntryWidget> EntryWidgetClass;

	/** How many lines are shown while collapsed */
	UPROPERTY(EditAnywhere, Category = "Activity Feed", meta = (ClampMin = 1))
	int32 CollapsedEntryCount = 6;

	/** How many lines are shown while expanded */
	UPROPERTY(EditAnywhere, Category = "Activity Feed", meta = (ClampMin = 1))
	int32 ExpandedEntryCount = 20;

	/** Tint applied to the active tab's button */
	UPROPERTY(EditAnywhere, Category = "Activity Feed")
	FLinearColor ActiveTabColor = FLinearColor(1.0f, 0.82f, 0.4f, 1.0f);

	/** Tint applied to the tabs that aren't showing */
	UPROPERTY(EditAnywhere, Category = "Activity Feed")
	FLinearColor InactiveTabColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);

	/**
	 *  How tall the feed becomes while expanded, in slot pixels.
	 *
	 *  Expanding shows ExpandedEntryCount lines where the collapsed state shows
	 *  CollapsedEntryCount, and the authored slot is only big enough for the collapsed set - so
	 *  without this the history would draw outside its own background, which reads as broken
	 *  rather than as a layout wanting tuning. Set it to 0 to leave the slot alone, for a WBP that
	 *  sizes itself some other way.
	 */
	UPROPERTY(EditAnywhere, Category = "Activity Feed", meta = (ClampMin = 0))
	float ExpandedHeight = 460.0f;

	/** The slot height this widget was authored with, so collapsing restores exactly what the
	 *  designer laid out rather than a number guessed here. Captured on the **first toggle**, not
	 *  on construct - see ApplyExpandedHeight for why that distinction was a bug. Non-positive
	 *  until captured. */
	float CollapsedHeight = -1.0f;

	/** The tab currently showing */
	EActivityFeedTab ActiveTab = EActivityFeedTab::Log;

	/** True while the feed is showing its history rather than the last few lines */
	bool bExpanded = false;

	/** The lines currently in EntryBox, oldest first. Resized rather than rebuilt. */
	UPROPERTY()
	TArray<TObjectPtr<UActivityEntryWidget>> EntryWidgets;

	/** Set when the list of entries to draw may have changed, cleared once it has been rebuilt */
	bool bEntriesDirty = true;

	/** The log this feed is bound to, so the subscription can be undone on destruct */
	TWeakObjectPtr<USmoresActivityLog> BoundLog;

public:

	/**
	 *  Rebuilds the lines if anything was posted since last time. Pushed every frame by
	 *  AStrategyHUD::DrawHUD, the same way every other region is driven - see the .cpp for why a
	 *  per-frame call still earns its keep now that nothing fades.
	 */
	void RefreshFeed();

	/** Expands the feed to its history, or collapses it back. The `L` key. */
	void ToggleExpanded();

	/** True while the feed is expanded */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsExpanded() const { return bExpanded; }

	/** Blueprint handler for anything beyond the bound tabs and lines - a background, a scroll box */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Feed"))
	void BP_UpdateFeed();

protected:

	/** This player's activity log, or null if their local player hasn't arrived yet */
	USmoresActivityLog* GetActivityLog() const;

	/** What the current tab says when it holds nothing */
	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetEmptyText() const;

	/** Switches tabs and rebuilds */
	void SetActiveTab(EActivityFeedTab Tab);

	UFUNCTION()
	void HandleLogTabClicked();

	UFUNCTION()
	void HandleSquadTabClicked();

	UFUNCTION()
	void HandleQuestsTabClicked();

	UFUNCTION()
	void HandleCommsTabClicked();

	/** Bound to USmoresActivityLog::OnEntryAdded - marks the list dirty rather than rebuilding here */
	void HandleEntryAdded(const FActivityEntry& Entry);

	/** Grows or shrinks EntryWidgets to match the number of lines being drawn */
	void ResizeEntryRows(int32 LineCount);

	/** Rebuilds the visible lines from the log */
	void RebuildEntries();

	/** Repaints the active tab's tint */
	void RefreshTabStates();

	/**
	 *  Resizes this region's canvas slot to match the expanded or collapsed state, keeping its
	 *  bottom edge where the designer put it.
	 *
	 *  Does nothing unless the region sits in a UCanvasPanel, which is how UI_Strategy places
	 *  every region - a WBP that nests it some other way simply keeps whatever size that layout
	 *  gives it.
	 */
	void ApplyExpandedHeight();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface
};
