// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BarkBubbleWidget.generated.h"

class UTextBlock;

/**
 *  One bark floating over the person who said it - a box and a line of text, spawned and placed by
 *  UBarkBubbleLayerWidget.
 *
 *  Deliberately **not** a UHUDRegionWidget, and never hit-testable: a bubble floats over the world,
 *  and a click on it must reach the unit underneath. See the layer for why that is an exception to
 *  hud-and-panels.md's click-shield rule rather than a breach of it.
 *
 *  The look is the legible floor from hud-and-panels.md - a dark translucent box, light text - and
 *  lives in the WBP, so the styling pass replaces it without touching this class.
 */
UCLASS(abstract)
class SMORESUI_API UBarkBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The line. Name it "LineText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LineText;

	/** Lines longer than this wrap onto another line, in slate units */
	UPROPERTY(EditAnywhere, Category = "Bark Bubble", meta = (ClampMin = 40))
	float MaxTextWidth = 260.0f;

	/** The line currently drawn */
	FText Line;

	/** FBarkBubbleEntry::Serial of the line currently drawn; 0 before the first */
	uint32 ShownSerial = 0;

public:

	/** Shows Line. Serial is the schedule's, so the layer can ask whether this widget already shows it. */
	void SetLine(const FText& InLine, uint32 Serial);

	/** The schedule serial of the line currently drawn */
	uint32 GetShownSerial() const { return ShownSerial; }

	/** Blueprint handler for anything beyond the bound text - a little pop as the line appears, say */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta = (DisplayName = "Line Changed"))
	void BP_LineChanged();

protected:

	/** Pushes Line into the bound text and applies the wrap width */
	void RefreshLineDisplay();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface
};
