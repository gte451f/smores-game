// Copyright Epic Games, Inc. All Rights Reserved.


#include "DamageNumberWidget.h"
#include "Components/TextBlock.h"

void UDamageNumberWidget::SetDamageAmount(float Amount)
{
	if (!AmountText)
	{
		return;
	}

	AmountText->SetText(FText::FromString(FString::Printf(TEXT("-%d"), FMath::RoundToInt(Amount))));
	AmountText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
}
