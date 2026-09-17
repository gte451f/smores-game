// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ResourceStripWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "ResourceStripWidget"

void UResourceStripWidget::SetGold(int32 NewGold)
{
	// the HUD pushes this every frame, so only touch the readout when the balance actually moved
	if (Gold == NewGold)
	{
		return;
	}

	Gold = NewGold;

	RefreshGoldDisplay();
}

FText UResourceStripWidget::GetGoldLabel() const
{
	return FText::Format(LOCTEXT("GoldReadout", "Gold: {0}"), FText::AsNumber(Gold));
}

void UResourceStripWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// the first balance push may well have happened before this widget existed
	RefreshGoldDisplay();
}

void UResourceStripWidget::RefreshGoldDisplay()
{
	if (GoldText)
	{
		GoldText->SetText(GetGoldLabel());
	}

	BP_UpdateGold();
}

#undef LOCTEXT_NAMESPACE
