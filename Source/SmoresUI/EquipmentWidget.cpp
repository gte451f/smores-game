// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "EquipmentWidget.h"
#include "InventoryWidget.h"
#include "SmoresUI.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"

#define LOCTEXT_NAMESPACE "EquipmentWidget"

void UEquipmentWidget::SetEquipment(UEquipmentComponent* InEquipment)
{
	// drop any previous binding
	ClearEquipment();

	BoundEquipment = InEquipment;

	if (BoundEquipment.IsValid())
	{
		BoundEquipment->OnEquipmentChanged.AddDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
	}

	// the slot roster never changes, but each slot widget has to be re-pointed at the new component
	RebuildSlots();

	RefreshDisplay();
}

void UEquipmentWidget::ClearEquipment()
{
	if (BoundEquipment.IsValid())
	{
		BoundEquipment->OnEquipmentChanged.RemoveDynamic(this, &UEquipmentWidget::HandleEquipmentChanged);
	}

	BoundEquipment.Reset();
}

FText UEquipmentWidget::GetContentsSummary() const
{
	if (!BoundEquipment.IsValid())
	{
		return FText::GetEmpty();
	}

	TArray<FString> Lines;

	for (const EEquipSlot EquipSlot : UEquipmentComponent::GetAllEquipSlots())
	{
		const FInventoryItem Worn = BoundEquipment->GetEquippedItem(EquipSlot);

		Lines.Add(FString::Printf(TEXT("%s: %s"),
			*UEquipmentComponent::GetSlotDisplayName(EquipSlot).ToString(),
			Worn.IsEmpty() ? *LOCTEXT("EmptySlot", "(empty)").ToString() : *UInventoryWidget::GetItemLabel(Worn).ToString()));
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UEquipmentWidget::GetWeightSummary() const
{
	if (!BoundEquipment.IsValid())
	{
		return FText::GetEmpty();
	}

	// same one-decimal resolution the carried-weight readout uses
	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 1;
	Format.MaximumFractionalDigits = 1;

	return FText::Format(LOCTEXT("EquippedWeight", "Equipped: {0}"), FText::AsNumber(BoundEquipment->GetTotalWeight(), &Format));
}

void UEquipmentWidget::HandleEquipmentChanged()
{
	RefreshDisplay();
}

void UEquipmentWidget::RefreshDisplay()
{
	for (UEquipmentSlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->RefreshVisuals();
		}
	}

	if (SlotListText)
	{
		SlotListText->SetText(GetContentsSummary());
	}

	if (WeightText)
	{
		WeightText->SetText(GetWeightSummary());
	}

	BP_EquipmentUpdated();
}

void UEquipmentWidget::RebuildSlots()
{
	if (!SlotContainer)
	{
		return;
	}

	SlotContainer->ClearChildren();
	SlotWidgets.Reset();

	if (!SlotWidgetClass)
	{
		// without it the window has a panel and nothing in it, which looks like a broken bind
		// rather than a missing class default - worth saying out loud
		UE_LOG(LogSmoresUI, Warning, TEXT("%s has a slot container but no SlotWidgetClass - the paperdoll will be empty."), *GetName());
		return;
	}

	for (const EEquipSlot EquipSlot : UEquipmentComponent::GetAllEquipSlots())
	{
		UEquipmentSlotWidget* SlotWidget = CreateWidget<UEquipmentSlotWidget>(this, SlotWidgetClass);

		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlot(BoundEquipment.Get(), EquipSlot);

		SlotContainer->AddChild(SlotWidget);
		SlotWidgets.Add(SlotWidget);
	}
}

void UEquipmentWidget::NativeDestruct()
{
	ClearEquipment();

	Super::NativeDestruct();
}

void UEquipmentWidget::RequestClose_Implementation()
{
	ClearEquipment();

	if (IsInViewport())
	{
		RemoveFromParent();
	}

	// after the window is actually gone, so a listener reacting to this sees it that way
	Super::RequestClose_Implementation();
}

#undef LOCTEXT_NAMESPACE
