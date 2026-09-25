// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BarkBubbleSchedule.h"
#include "BarkBubbleLayerWidget.generated.h"

class UBarkBubbleWidget;
class UCanvasPanel;

/**
 *  Barks floating over the people who said them: a full-screen, click-through layer of the HUD,
 *  hosted by UStrategyUI beneath its regions (and so beneath every window).
 *
 *  **A HUD layer rather than a damage-number-style actor**, for three reasons (the dialog roadmap's
 *  Slice 2):
 *
 *  - it is per local player by construction - only players who heard a line see it, each in their
 *    own language, because each client's line arrives through its own controller's Client_NotifyBark;
 *  - one bubble per speaker, and bubbles that stack rather than overdraw, both need every bubble in
 *    one place;
 *  - it needs no widget class wired onto every unit Blueprint.
 *
 *  **It never takes a click** (HitTestInvisible, children included). That is a deliberate exception
 *  to hud-and-panels.md's "anything on the HUD that reads as a panel must be a UHUDRegionWidget":
 *  that rule exists so panels *eat* clicks, and a bubble floats over the world - a click on a bubble
 *  means the unit underneath it.
 *
 *  The rules - one per speaker, how long a line stays, the fade - are FBarkBubbleSchedule's, and
 *  the stacking is SmoresBarkBubbles::StackBoxes; this class only places widgets where those say.
 *  Every frame AStrategyHUD::DrawHUD pushes RefreshBubbles, which projects each speaker's head to
 *  the screen through the owning player's view and moves their bubble there.
 */
UCLASS(abstract)
class SMORESUI_API UBarkBubbleLayerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** The full-screen canvas the bubbles are placed on. Name it "BubbleCanvas" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> BubbleCanvas;

	/** Widget class used for each bubble. Must be set, or barks reach the feed and nothing floats. */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles")
	TSubclassOf<UBarkBubbleWidget> BubbleWidgetClass;

	/** How far above the top of the speaker's collision the bubble's bottom edge sits, in cm */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (Units = "cm"))
	float HeadClearance = 40.0f;

	/** The shortest time any line stays up, in real seconds */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (ClampMin = 0, Units = "s"))
	float LifetimeFloorSeconds = 2.0f;

	/** Added per character, so a long line stays long enough to read, in real seconds */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (ClampMin = 0, Units = "s"))
	float LifetimePerCharacterSeconds = 0.06f;

	/** The longest time any line stays up, in real seconds */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (ClampMin = 0, Units = "s"))
	float LifetimeCapSeconds = 6.0f;

	/** How long a bubble takes to fade once its time is up, in real seconds */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (ClampMin = 0, Units = "s"))
	float FadeSeconds = 0.6f;

	/** Space between stacked bubbles, in slate units */
	UPROPERTY(EditAnywhere, Category = "Bark Bubbles", meta = (ClampMin = 0))
	float StackGap = 4.0f;

	/**
	 *  The bubble widgets, reused frame to frame: the Nth one draws the Nth bubble currently on
	 *  screen, and any beyond that are collapsed. Grows to the most bubbles ever shown at once and
	 *  never shrinks - a few idle widgets cost less than creating one per bark.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBarkBubbleWidget>> BubbleWidgets;

	/** Who is showing what, and until when */
	FBarkBubbleSchedule Schedule;

	/** Set once a missing BubbleWidgetClass has been reported, so it warns once rather than every bark */
	bool bWarnedNoBubbleClass = false;

public:

	/**
	 *  Speaker just said Line - float it over them, replacing any bubble they already have. Called on
	 *  the owning client with the line already resolved in this machine's language.
	 */
	void ShowBark(const AActor* Speaker, const FText& Line);

	/** Moves every bubble to its speaker, fades and removes expired ones. Pushed every frame by the HUD. */
	void RefreshBubbles();

protected:

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget interface

	/** The clock bubbles run on: real seconds, so neither the pace nor a pause changes how long a line stays */
	static double GetNow();

	/** Copies the EditAnywhere timing into the schedule, so a tweak in the WBP takes effect */
	void ApplyTiming();

	/** The Nth bubble widget, creating it (and any before it) if needed. Null if there is no class to create it from. */
	UBarkBubbleWidget* GetBubbleWidget(int32 Index);

	/** Where Speaker's bubble should sit: just above the top of their collision */
	FVector GetBubbleAnchor(const AActor* Speaker) const;
};
