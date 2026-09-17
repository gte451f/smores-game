// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "TimePaceWidget.h"
#include "StrategyHUDCommands.h"
#include "TimePaceComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

const TArray<EGamePace>& UTimePaceWidget::GetStripPaces()
{
	// four of the eight tiers. The rest are reachable with `-` and `=` and read on the label -
	// see the class comment for why they don't get buttons.
	static const TArray<EGamePace> StripPaces = {
		EGamePace::Paused,
		EGamePace::Normal,
		EGamePace::Double,
		EGamePace::Quadruple
	};

	return StripPaces;
}

void UTimePaceWidget::SetPace(EGamePace NewPace)
{
	// the HUD pushes this every frame, so do nothing at all in the overwhelmingly common case
	if (Pace == NewPace)
	{
		return;
	}

	Pace = NewPace;

	RefreshPaceDisplay();
}

FText UTimePaceWidget::GetPaceLabel() const
{
	// the wording lives with the ladder, not here - one place a tier is named
	return UTimePaceComponent::GetPaceLabel(Pace);
}

UButton* UTimePaceWidget::GetButtonForPace(EGamePace InPace) const
{
	switch (InPace)
	{
	case EGamePace::Paused:    return PauseButton;
	case EGamePace::Normal:    return NormalButton;
	case EGamePace::Double:    return DoubleButton;
	case EGamePace::Quadruple: return QuadrupleButton;
	default:                   return nullptr;
	}
}

void UTimePaceWidget::RequestPace(EGamePace NewPace)
{
	if (IStrategyHUDCommands* Commands = GetHUDCommands())
	{
		// no local guess at what the pace becomes - the server decides, and the next frame's push
		// is what moves the readout. A button that repainted itself optimistically would show a
		// tier the world wasn't running at whenever the request was refused.
		Commands->RequestPace(NewPace);
	}
}

void UTimePaceWidget::HandlePauseClicked()
{
	RequestPace(EGamePace::Paused);
}

void UTimePaceWidget::HandleNormalClicked()
{
	RequestPace(EGamePace::Normal);
}

void UTimePaceWidget::HandleDoubleClicked()
{
	RequestPace(EGamePace::Double);
}

void UTimePaceWidget::HandleQuadrupleClicked()
{
	RequestPace(EGamePace::Quadruple);
}

void UTimePaceWidget::RefreshPaceDisplay()
{
	if (PaceText)
	{
		PaceText->SetText(GetPaceLabel());
	}

	for (EGamePace StripPace : GetStripPaces())
	{
		if (UButton* Button = GetButtonForPace(StripPace))
		{
			Button->SetBackgroundColor(StripPace == Pace ? ActiveButtonTint : InactiveButtonTint);
		}
	}

	BP_UpdatePace();
}

void UTimePaceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PauseButton)
	{
		PauseButton->OnClicked.AddUniqueDynamic(this, &UTimePaceWidget::HandlePauseClicked);
	}

	if (NormalButton)
	{
		NormalButton->OnClicked.AddUniqueDynamic(this, &UTimePaceWidget::HandleNormalClicked);
	}

	if (DoubleButton)
	{
		DoubleButton->OnClicked.AddUniqueDynamic(this, &UTimePaceWidget::HandleDoubleClicked);
	}

	if (QuadrupleButton)
	{
		QuadrupleButton->OnClicked.AddUniqueDynamic(this, &UTimePaceWidget::HandleQuadrupleClicked);
	}

	// the first push may well have happened before this widget existed
	RefreshPaceDisplay();
}
