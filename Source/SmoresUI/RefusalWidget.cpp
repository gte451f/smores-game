// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "RefusalWidget.h"
#include "StrategyHUD.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "RefusalWidget"

FText URefusalWidget::GetRefusalText(ESmoresRefusalReason Reason)
{
	// The whole point of the reason code is that this switch is the only place any of these is
	// worded. Nothing on the server ever builds one of these strings, so there is no second copy
	// to drift out of step and nothing to re-translate in two places.
	switch (Reason)
	{
	case ESmoresRefusalReason::TooFar:
		return LOCTEXT("RefusalTooFar", "Too far away");

	case ESmoresRefusalReason::NoRoom:
		return LOCTEXT("RefusalNoRoom", "No room for that");

	case ESmoresRefusalReason::CannotAfford:
		return LOCTEXT("RefusalCannotAfford", "Not enough gold");

	case ESmoresRefusalReason::WrongSlot:
		return LOCTEXT("RefusalWrongSlot", "Can't be worn there");

	case ESmoresRefusalReason::NotInteractable:
		return LOCTEXT("RefusalNotInteractable", "They won't deal with you");

	case ESmoresRefusalReason::None:
	default:
		return FText::GetEmpty();
	}
}

void URefusalWidget::RaiseRefusal(const APlayerController* OwningPlayer, ESmoresRefusalReason Reason)
{
	if (!OwningPlayer)
	{
		return;
	}

	// keyed off the asking player's own HUD - in a co-op session a refusal belongs to whoever
	// asked and nobody else should see it
	const AStrategyHUD* StrategyHUD = Cast<AStrategyHUD>(OwningPlayer->GetHUD());

	if (!StrategyHUD)
	{
		return;
	}

	if (URefusalWidget* Widget = StrategyHUD->GetRefusalWidget())
	{
		Widget->ShowRefusal(Reason);
	}
}

void URefusalWidget::ShowRefusal(ESmoresRefusalReason Reason)
{
	if (Reason == ESmoresRefusalReason::None)
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();

	// a repeat of the same refusal inside the window is the same refusal, not a new one: it gets
	// more time on screen and follows the cursor, but doesn't replay the sound. Anything else
	// turns a held key or an impatient double-click into a machine gun.
	const bool bIsQuickRepeat = (Reason == CurrentRefusal)
		&& (Now - LastRefusalTime) < RefusalRepeatSeconds;

	CurrentRefusal = Reason;
	LastRefusalTime = Now;

	if (RefusalText)
	{
		RefusalText->SetText(GetRefusalText(Reason));

		// never HitTestable: this layer sits above every open window, and a refusal that
		// swallowed the player's next click would cost more than the refusal did
		RefusalText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	PositionRefusalAtCursor();

	if (!bIsQuickRepeat)
	{
		if (RefusalSound)
		{
			// the sound is the half that actually lands - it needs no reading and works while
			// the player is looking at the pawn rather than at the text
			UGameplayStatics::PlaySound2D(this, RefusalSound);
		}

		BP_RefusalShown(Reason);
	}

	// restarted on every raise, repeat or not, so the line measures its life from the last time
	// the player was told rather than from the first
	World->GetTimerManager().SetTimer(RefusalClearTimer, this, &URefusalWidget::ClearRefusal, RefusalDisplaySeconds, false);
}

void URefusalWidget::ClearRefusal()
{
	if (CurrentRefusal == ESmoresRefusalReason::None)
	{
		return;
	}

	CurrentRefusal = ESmoresRefusalReason::None;

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefusalClearTimer);
	}

	if (RefusalText)
	{
		RefusalText->SetText(FText::GetEmpty());
		RefusalText->SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_RefusalCleared();
}

void URefusalWidget::PositionRefusalAtCursor()
{
	if (!RefusalText)
	{
		return;
	}

	// only a Canvas Panel child can be moved. Anywhere else the designer placed it deliberately,
	// so leave it there rather than fighting the layout
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(RefusalText->Slot);

	if (!CanvasSlot)
	{
		return;
	}

	const APlayerController* OwningPlayer = GetOwningPlayer();

	if (!OwningPlayer)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	// false when the cursor isn't over the viewport at all (or there is no cursor - touch, pad).
	// Leaving the line where it last sat is the right answer then; it is still readable.
	if (!OwningPlayer->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	// GetMousePosition is in viewport pixels and a Canvas slot is in widget-space units; at any
	// DPI scale other than 1.0 those are different, and skipping this puts the line a long way
	// from the cursor on a 4K screen
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

	if (ViewportScale <= 0.0f)
	{
		return;
	}

	CanvasSlot->SetPosition(FVector2D(MouseX / ViewportScale, MouseY / ViewportScale) + RefusalCursorOffset);
}

void URefusalWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// This widget covers the whole screen and sits above every window, so anything hit-testable
	// about it would block the entire game's mouse input. Forced here rather than trusted to the
	// WBP, because the failure mode is catastrophic and silent-looking: clicks simply stop
	// working everywhere, with nothing visible to blame.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// nothing has been refused yet, and a WBP author shouldn't have to remember to hide it
	if (RefusalText)
	{
		RefusalText->SetText(FText::GetEmpty());
		RefusalText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URefusalWidget::NativeDestruct()
{
	// the timer holds a raw this - it has to go before the widget does
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefusalClearTimer);
	}

	Super::NativeDestruct();
}

#undef LOCTEXT_NAMESPACE
