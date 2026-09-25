// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "BarkBubbleLayerWidget.h"
#include "BarkBubbleWidget.h"
#include "SmoresUI.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

void UBarkBubbleLayerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// the whole layer, children included, is invisible to the mouse: a click on a bubble is a click
	// on the unit under it. The deliberate exception to the HUD's click-shield rule - see the header.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	ApplyTiming();
}

double UBarkBubbleLayerWidget::GetNow()
{
	// wall-clock, like the feed's timestamps: at 8x a line must still stay long enough to read, and
	// at the paused tier (world time at 1/10,000) it must still fade rather than hang there
	return FPlatformTime::Seconds();
}

void UBarkBubbleLayerWidget::ApplyTiming()
{
	Schedule.Timing.FloorSeconds = LifetimeFloorSeconds;
	Schedule.Timing.PerCharacterSeconds = LifetimePerCharacterSeconds;
	Schedule.Timing.CapSeconds = LifetimeCapSeconds;
	Schedule.Timing.FadeSeconds = FadeSeconds;
}

void UBarkBubbleLayerWidget::ShowBark(const AActor* Speaker, const FText& Line)
{
	if (!IsValid(Speaker) || Line.IsEmpty())
	{
		return;
	}

	if (!BubbleWidgetClass && !bWarnedNoBubbleClass)
	{
		UE_LOG(LogSmoresUI, Warning, TEXT("UBarkBubbleLayerWidget has no BubbleWidgetClass set; barks reach the feed but nothing floats over the speaker."));
		bWarnedNoBubbleClass = true;
	}

	ApplyTiming();

	Schedule.Show(Speaker, Line, GetNow());
}

void UBarkBubbleLayerWidget::RefreshBubbles()
{
	const double Now = GetNow();

	Schedule.Prune(Now);

	const APlayerController* PC = GetOwningPlayer();
	const FVector2D LayerSize = GetCachedGeometry().GetLocalSize();

	// the very first frame has no geometry yet; better to skip the edge test once than to hide
	// every bubble because the screen measured zero
	const bool bKnowsLayerSize = LayerSize.X > 1.0 && LayerSize.Y > 1.0;

	TArray<const FBarkBubbleEntry*> Shown;
	TArray<FBox2D> Boxes;

	if (BubbleCanvas && PC)
	{
		for (const FBarkBubbleEntry& Entry : Schedule.GetEntries())
		{
			const AActor* Speaker = Entry.Speaker.Get();
			FVector2D Anchor;

			// behind the camera projects to false. Off the edge of the screen is simply not shown -
			// the feed already covers a line said out of sight, so there are no edge arrows.
			if (!Speaker || !UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, GetBubbleAnchor(Speaker), Anchor, /*bPlayerViewportRelative*/ true))
			{
				continue;
			}

			if (bKnowsLayerSize && (Anchor.X < 0.0 || Anchor.Y < 0.0 || Anchor.X > LayerSize.X || Anchor.Y > LayerSize.Y))
			{
				continue;
			}

			UBarkBubbleWidget* Bubble = GetBubbleWidget(Shown.Num());

			if (!Bubble)
			{
				break;
			}

			// visible before measuring: a collapsed widget measures zero
			if (Bubble->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				Bubble->SetVisibility(ESlateVisibility::HitTestInvisible);
			}

			// only a new line is worth re-laying out; the same line on a moving speaker just moves
			if (Bubble->GetShownSerial() != Entry.Serial)
			{
				Bubble->SetLine(Entry.Text, Entry.Serial);
				Bubble->ForceLayoutPrepass();
			}

			// centred over the speaker, bottom edge at the anchor
			const FVector2D Size = Bubble->GetDesiredSize();

			Boxes.Emplace(FVector2D(Anchor.X - Size.X * 0.5, Anchor.Y - Size.Y), FVector2D(Anchor.X + Size.X * 0.5, Anchor.Y));
			Shown.Add(&Entry);
		}
	}

	// in the order speakers first spoke, so an earlier bubble keeps its place and a later one stacks
	SmoresBarkBubbles::StackBoxes(Boxes, StackGap);

	for (int32 Index = 0; Index < BubbleWidgets.Num(); ++Index)
	{
		UBarkBubbleWidget* Bubble = BubbleWidgets[Index];

		if (!Bubble)
		{
			continue;
		}

		if (!Shown.IsValidIndex(Index))
		{
			if (Bubble->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Bubble->SetVisibility(ESlateVisibility::Collapsed);
			}

			continue;
		}

		// compare before touching Slate, like every per-frame push on this HUD - a bubble over a
		// speaker standing still under a still camera shouldn't invalidate layout every frame
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Bubble->Slot))
		{
			const FVector2D Position = Boxes[Index].Min;

			if (!CanvasSlot->GetPosition().Equals(Position, 0.5))
			{
				CanvasSlot->SetPosition(Position);
			}
		}

		const float Opacity = Schedule.GetOpacity(*Shown[Index], Now);

		if (!FMath::IsNearlyEqual(Bubble->GetRenderOpacity(), Opacity))
		{
			Bubble->SetRenderOpacity(Opacity);
		}
	}
}

UBarkBubbleWidget* UBarkBubbleLayerWidget::GetBubbleWidget(int32 Index)
{
	while (BubbleWidgets.Num() <= Index)
	{
		if (!BubbleWidgetClass || !BubbleCanvas)
		{
			return nullptr;
		}

		UBarkBubbleWidget* Bubble = CreateWidget<UBarkBubbleWidget>(GetOwningPlayer(), BubbleWidgetClass);

		if (!Bubble)
		{
			return nullptr;
		}

		// top-left anchored, sized to its own contents, placed by position alone - RefreshBubbles
		// works out that position from the speaker every frame
		if (UCanvasPanelSlot* CanvasSlot = BubbleCanvas->AddChildToCanvas(Bubble))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetAutoSize(true);
		}

		Bubble->SetVisibility(ESlateVisibility::Collapsed);

		BubbleWidgets.Add(Bubble);
	}

	return BubbleWidgets[Index];
}

FVector UBarkBubbleLayerWidget::GetBubbleAnchor(const AActor* Speaker) const
{
	// a unit's location is the middle of its capsule, so the top of its head is half the capsule up
	return Speaker->GetActorLocation() + FVector(0.0, 0.0, Speaker->GetSimpleCollisionHalfHeight() + HeadClearance);
}
