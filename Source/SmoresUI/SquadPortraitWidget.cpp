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

	BP_UpdatePortrait();
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
