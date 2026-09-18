// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActivityEntry.h"
#include "ActivityEntryWidget.generated.h"

class UTextBlock;

/**
 *  One line in the activity feed.
 *
 *  Deliberately **not** a UHUDRegionWidget, for the same reason UTargetActionWidget isn't: it
 *  lives inside one, and the feed's own shield catches anything that lands on it. It is also not
 *  clickable - a feed line is a record, not a control - so it needs no UButton either.
 *
 *  Colour comes from the entry's severity and nothing else. That is the one piece of the
 *  wireframe's styling that isn't deferred to the styling pass, because a feed whose lines all
 *  read the same is a feed nobody scans after a fight, which is the only time it is worth having.
 */
UCLASS(abstract)
class SMORESUI_API UActivityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The line itself. Name it "MessageText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	/** Who it came from. Name it "SourceText" to auto-bind. Hidden for an entry with no source. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SourceText;

	/** Colour for an ordinary record */
	UPROPERTY(EditAnywhere, Category = "Activity Entry")
	FLinearColor NormalColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

	/** Colour for something that went the player's way */
	UPROPERTY(EditAnywhere, Category = "Activity Entry")
	FLinearColor GoodColor = FLinearColor(0.45f, 0.85f, 0.45f, 1.0f);

	/** Colour for something worth noticing - a refusal */
	UPROPERTY(EditAnywhere, Category = "Activity Entry")
	FLinearColor WarningColor = FLinearColor(0.95f, 0.8f, 0.35f, 1.0f);

	/** Colour for something that went badly - squad damage, a squad member down */
	UPROPERTY(EditAnywhere, Category = "Activity Entry")
	FLinearColor BadColor = FLinearColor(0.95f, 0.4f, 0.35f, 1.0f);

	/** The entry this line currently draws */
	FActivityEntry Entry;

public:

	/** Fills this line in from an entry. Called every time the feed refreshes its list. */
	void SetEntry(const FActivityEntry& InEntry);

	/**
	 *  Sets how faded this line is, 0 (gone) to 1 (fresh). Applied as render opacity rather than
	 *  as a text colour so the source, the message and any future icon fade together.
	 */
	void SetFadeAlpha(float Alpha);

	/** The entry currently drawn - the feed compares ids rather than re-pushing identical lines */
	const FActivityEntry& GetEntry() const { return Entry; }

	/** Blueprint handler for anything beyond the bound message, source and colour */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Entry"))
	void BP_UpdateEntry();

protected:

	/** The colour this entry's severity means */
	UFUNCTION(BlueprintPure, Category = "UI")
	FLinearColor GetSeverityColor() const;

	/** Pushes the current entry into the bound widgets and the BP hook */
	void RefreshEntryDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
