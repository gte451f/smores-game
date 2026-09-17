// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "NavRailWidget.h"
#include "StrategyHUDCommands.h"
#include "Components/Button.h"

const TArray<EHUDPanel>& UNavRailWidget::GetRailPanels()
{
	static const TArray<EHUDPanel> RailPanels = {
		EHUDPanel::Squad,
		EHUDPanel::Inventory,
		EHUDPanel::Map,
		EHUDPanel::Research,
		EHUDPanel::Help
	};

	return RailPanels;
}

void UNavRailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SquadButton)
	{
		SquadButton->OnClicked.AddUniqueDynamic(this, &UNavRailWidget::HandleSquadClicked);
	}

	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddUniqueDynamic(this, &UNavRailWidget::HandleInventoryClicked);
	}

	if (MapButton)
	{
		MapButton->OnClicked.AddUniqueDynamic(this, &UNavRailWidget::HandleMapClicked);
	}

	if (ResearchButton)
	{
		ResearchButton->OnClicked.AddUniqueDynamic(this, &UNavRailWidget::HandleResearchClicked);
	}

	if (HelpButton)
	{
		HelpButton->OnClicked.AddUniqueDynamic(this, &UNavRailWidget::HandleHelpClicked);
	}

	// nothing is open yet, but the tints have never been applied either
	RefreshPanelStates();
}

UButton* UNavRailWidget::GetButtonForPanel(EHUDPanel Panel) const
{
	switch (Panel)
	{
	case EHUDPanel::Squad:     return SquadButton;
	case EHUDPanel::Inventory: return InventoryButton;
	case EHUDPanel::Map:       return MapButton;
	case EHUDPanel::Research:  return ResearchButton;
	case EHUDPanel::Help:      return HelpButton;
	default:                   return nullptr;
	}
}

void UNavRailWidget::RequestPanel(EHUDPanel Panel)
{
	if (IStrategyHUDCommands* Commands = GetHUDCommands())
	{
		Commands->RequestPanel(Panel);
	}
}

void UNavRailWidget::RefreshPanelStates()
{
	const IStrategyHUDCommands* Commands = GetHUDCommands();
	const TArray<EHUDPanel>& RailPanels = GetRailPanels();

	uint8 OpenMask = 0;

	if (Commands)
	{
		for (int32 Index = 0; Index < RailPanels.Num(); ++Index)
		{
			if (Commands->IsPanelOpen(RailPanels[Index]))
			{
				OpenMask |= (1 << Index);
			}
		}
	}

	// this is pushed every frame, so do nothing at all in the overwhelmingly common case
	if (OpenMask == LastOpenMask)
	{
		return;
	}

	LastOpenMask = OpenMask;

	for (int32 Index = 0; Index < RailPanels.Num(); ++Index)
	{
		if (UButton* Button = GetButtonForPanel(RailPanels[Index]))
		{
			const bool bOpen = (OpenMask & (1 << Index)) != 0;

			Button->SetBackgroundColor(bOpen ? ActiveButtonTint : InactiveButtonTint);
		}
	}
}

void UNavRailWidget::HandleSquadClicked()
{
	RequestPanel(EHUDPanel::Squad);
}

void UNavRailWidget::HandleInventoryClicked()
{
	RequestPanel(EHUDPanel::Inventory);
}

void UNavRailWidget::HandleMapClicked()
{
	RequestPanel(EHUDPanel::Map);
}

void UNavRailWidget::HandleResearchClicked()
{
	RequestPanel(EHUDPanel::Research);
}

void UNavRailWidget::HandleHelpClicked()
{
	RequestPanel(EHUDPanel::Help);
}
