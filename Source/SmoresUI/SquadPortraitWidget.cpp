// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SquadPortraitWidget.h"
#include "StrategyUnit.h"
#include "HealthComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void USquadPortraitWidget::SetUnit(AStrategyUnit* InUnit, bool bInSelected)
{
	// the danger subscription follows the unit, not the widget - a portrait handed a different
	// unit must stop hearing about the old one, and must not finish flashing on its behalf
	if (Unit.Get() != InUnit)
	{
		WatchUnitCombat(InUnit);
	}

	// ahead of the early-out below: the flash has to keep pulsing and run out on frames where
	// nothing else about the portrait changes, which is nearly all of them
	UpdateDangerFlash();

	float NewHealthFraction = 0.0f;
	bool bNewHasHealth = false;

	if (IsValid(InUnit))
	{
		if (const UHealthComponent* Health = InUnit->GetHealth())
		{
			bNewHasHealth = true;
			NewHealthFraction = Health->MaxHealth > 0.0f
				? FMath::Clamp(Health->GetHealth() / Health->MaxHealth, 0.0f, 1.0f)
				: 0.0f;
		}
	}

	// The bar refreshes every frame, so the common case is being handed the same unit in the same
	// state. Health is compared to the nearest percent - what would actually be *drawn* - for the
	// same reason the target panel compares its distance to the nearest metre: a unit regenerating
	// or taking chip damage would otherwise invalidate Slate layout on every frame of it.
	if (Unit.Get() == InUnit
		&& bSelected == bInSelected
		&& bHasHealth == bNewHasHealth
		&& FMath::RoundToInt(HealthFraction * 100.0f) == FMath::RoundToInt(NewHealthFraction * 100.0f))
	{
		return;
	}

	Unit = InUnit;
	bSelected = bInSelected;
	bHasHealth = bNewHasHealth;
	HealthFraction = NewHealthFraction;

	RefreshPortraitDisplay();
}

FText USquadPortraitWidget::GetUnitName() const
{
	const AStrategyUnit* CurrentUnit = Unit.Get();

	return IsValid(CurrentUnit) ? CurrentUnit->GetHolderDisplayName() : FText::GetEmpty();
}

FText USquadPortraitWidget::GetInitials() const
{
	const FString Name = GetUnitName().ToString();

	FString Initials;

	// first letter of each whitespace-separated word, up to two - "Pawn 1" reads as "P1", which is
	// exactly what the wireframe's placeholder discs show
	bool bAtWordStart = true;

	for (const TCHAR Character : Name)
	{
		if (FChar::IsWhitespace(Character))
		{
			bAtWordStart = true;
			continue;
		}

		if (bAtWordStart)
		{
			Initials.AppendChar(FChar::ToUpper(Character));
			bAtWordStart = false;

			if (Initials.Len() >= 2)
			{
				break;
			}
		}
	}

	return FText::FromString(Initials);
}

void USquadPortraitWidget::HandleClicked()
{
	AStrategyUnit* CurrentUnit = Unit.Get();

	if (!IsValid(CurrentUnit))
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	// A rapid pair arrives as two ordinary clicks, not as a Slate double-click - SButton routes
	// OnMouseButtonDoubleClick straight into its press handler. See the class comment.
	const bool bFocusCamera = LastClickTime >= 0.0 && (Now - LastClickTime) <= DoubleClickSeconds;

	// reset rather than keep counting, so three fast clicks are "select, focus, select" rather
	// than a camera cut on every click after the first
	LastClickTime = bFocusCamera ? -1.0 : Now;

	OnPortraitClicked.Broadcast(CurrentUnit, bFocusCamera);
}

void USquadPortraitWidget::RefreshPortraitDisplay()
{
	const AStrategyUnit* CurrentUnit = Unit.Get();
	const bool bHasUnit = IsValid(CurrentUnit);

	// a portrait with no unit is a spare the bar is holding onto between refreshes - see
	// USquadBarWidget::ResizePortraitRow
	SetVisibility(bHasUnit ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	UTexture2D* Portrait = bHasUnit ? CurrentUnit->GetPortraitTexture() : nullptr;

	if (PortraitImage)
	{
		if (Portrait)
		{
			PortraitImage->SetBrushFromTexture(Portrait);
		}

		PortraitImage->SetVisibility(Portrait ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (InitialsText)
	{
		// the fallback the wireframe itself draws - initials on a plain disc. Shown only when
		// there is no face, so the two never stack on top of each other.
		InitialsText->SetText(GetInitials());
		InitialsText->SetVisibility(Portrait ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (NameText)
	{
		NameText->SetText(GetUnitName());
	}

	if (HealthBar)
	{
		HealthBar->SetVisibility(bHasHealth ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		HealthBar->SetPercent(HealthFraction);
	}

	if (SelectionRing)
	{
		// Hidden rather than Collapsed: the ring sits on top of the disc and its absence must not
		// change the portrait's size, or the whole bar would shuffle sideways on every selection
		SelectionRing->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	if (DangerMarker)
	{
		// Hidden for the same reason as the ring, and never hit-testable: the badge sits over the
		// button, and a click on a flashing portrait still has to select the unit
		DangerMarker->SetVisibility(IsDangerFlashing() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	BP_UpdatePortrait();
}

float USquadPortraitWidget::GetDangerFlashProgress() const
{
	if (!IsDangerFlashing() || DangerFlashSeconds <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(static_cast<float>((FPlatformTime::Seconds() - DangerFlashStartTime) / DangerFlashSeconds), 0.0f, 1.0f);
}

void USquadPortraitWidget::WatchUnitCombat(AStrategyUnit* InUnit)
{
	if (UCombatComponent* OldCombat = WatchedCombat.Get())
	{
		OldCombat->OnDangerSignal.Remove(DangerSignalHandle);
	}

	DangerSignalHandle.Reset();
	WatchedCombat = nullptr;

	StopDangerFlash();

	UCombatComponent* NewCombat = IsValid(InUnit) ? InUnit->GetCombat() : nullptr;

	if (!NewCombat)
	{
		return;
	}

	DangerSignalHandle = NewCombat->OnDangerSignal.AddUObject(this, &USquadPortraitWidget::HandleDangerSignal);
	WatchedCombat = NewCombat;
}

void USquadPortraitWidget::HandleDangerSignal(AActor* SignalUnit, EDangerSignal Signal)
{
	// every signal restarts the flash, whether or not one is still running - an escalation landing
	// mid-flash is new information, and has to be seen as a fresh start rather than lost in the tail
	// of the last one
	DangerFlashStartTime = FPlatformTime::Seconds();
	LastDangerSignal = Signal;

	if (DangerMarker)
	{
		DangerMarker->SetRenderOpacity(1.0f);
		DangerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	BP_DangerFlash(Signal);
}

void USquadPortraitWidget::UpdateDangerFlash()
{
	if (!IsDangerFlashing())
	{
		return;
	}

	const double Elapsed = FPlatformTime::Seconds() - DangerFlashStartTime;

	if (Elapsed >= DangerFlashSeconds)
	{
		StopDangerFlash();
		return;
	}

	if (DangerMarker && DangerPulseHz > 0.0f)
	{
		// a cosine so each pulse starts at full strength - the moment the signal lands is the moment
		// the badge is most visible, rather than half a pulse later
		const float Wave = 0.5f + 0.5f * FMath::Cos(static_cast<float>(Elapsed) * DangerPulseHz * UE_TWO_PI);

		DangerMarker->SetRenderOpacity(FMath::Lerp(DangerPulseMinOpacity, 1.0f, Wave));
	}
}

void USquadPortraitWidget::StopDangerFlash()
{
	if (!IsDangerFlashing())
	{
		return;
	}

	DangerFlashStartTime = -1.0;

	if (DangerMarker)
	{
		DangerMarker->SetRenderOpacity(1.0f);
		DangerMarker->SetVisibility(ESlateVisibility::Hidden);
	}

	BP_DangerFlashEnded();
}

void USquadPortraitWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PortraitButton)
	{
		PortraitButton->OnClicked.AddUniqueDynamic(this, &USquadPortraitWidget::HandleClicked);
	}

	// SetUnit may well have run before this widget was constructed
	RefreshPortraitDisplay();
}

void USquadPortraitWidget::NativeDestruct()
{
	// stop listening, and forget the unit so that if this widget is ever re-added the next SetUnit
	// counts as a new unit and subscribes again
	WatchUnitCombat(nullptr);
	Unit = nullptr;

	Super::NativeDestruct();
}
