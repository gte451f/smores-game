// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "HUDPlaceholderRegionWidget.h"
#include "Components/TextBlock.h"

void UHUDPlaceholderRegionWidget::SetRegionLabel(const FText& InLabel)
{
	RegionLabel = InLabel;

	RefreshLabel();
}

void UHUDPlaceholderRegionWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	RefreshLabel();
}

void UHUDPlaceholderRegionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshLabel();
}

void UHUDPlaceholderRegionWidget::RefreshLabel()
{
	if (LabelText)
	{
		LabelText->SetText(RegionLabel);
	}
}
