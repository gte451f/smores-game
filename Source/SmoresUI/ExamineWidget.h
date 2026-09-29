// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "ExamineWidget.generated.h"

class UTextBlock;

/**
 *  The Examine window: a thing's name, what kind of thing it is, and what the squad can see of it,
 *  in words.
 *
 *  **Never a number about the target.** No attributes, no skills, no health figure, no lock
 *  difficulty - those surface only as imperfect words, if at all (game-design's
 *  player-interface.md, "numbers about your own squad are known; numbers about the world are
 *  not"). That rule lives in what the thing returns from ISmoresInteractable::GetExamineText; this
 *  window only shows it.
 *
 *  A UWindowWidget, so dragging, resizing, the close button and swallowing clicks that land on it
 *  all come for free. Entirely client-side: definitions and authored text are on every machine, so
 *  looking at something needs no server. Kept after it closes, like the panel windows; examining
 *  something else rebinds it.
 */
UCLASS(abstract)
class SMORESUI_API UExamineWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** What kind of thing it is ("PERSON - NEUTRAL", "CONTAINER"). Name it "ClassificationText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassificationText;

	/** What the squad can see of it. Name it "BodyText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

	/** What SetExamine last pushed, so a window constructed afterwards still shows it */
	FText Classification;
	FText Body;

public:

	UExamineWidget();

	/** Fills the window in. Title is the thing's name. */
	void SetExamine(const FText& Title, const FText& InClassification, const FText& InBody);

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface

	/** Blueprint handler for anything beyond the bound title, classification and body */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Update Examine"))
	void BP_UpdateExamine();

protected:

	/** Pushes the stored text into the bound blocks and the BP hook */
	void RefreshExamineDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
