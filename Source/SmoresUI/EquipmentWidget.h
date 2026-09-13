// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WindowWidget.h"
#include "EquipmentComponent.h"
#include "EquipmentSlotWidget.h"
#include "EquipmentWidget.generated.h"

class UTextBlock;
class UPanelWidget;

/**
 *  Paperdoll window for one pawn's worn items - a floating panel like the inventory window,
 *  opened alongside it rather than embedded in it, so the two can be moved and sized
 *  independently and either one can be the drop target for a drag out of the other.
 *
 *  C++ spawns one UEquipmentSlotWidget per slot into SlotContainer, in
 *  UEquipmentComponent::GetAllEquipSlots order. The slot roster is fixed, so unlike the
 *  inventory grid these widgets are built once per bound pawn and merely refreshed when the
 *  equipment changes - a rebuild mid-drag would destroy the widget the pointer is over.
 */
UCLASS(abstract)
class SMORESUI_API UEquipmentWidget : public UWindowWidget
{
	GENERATED_BODY()

protected:

	/** Equipment component this widget is currently displaying */
	TWeakObjectPtr<UEquipmentComponent> BoundEquipment;

	/** Container the slot widgets are built into - any UPanelWidget will do, since slots don't overlap.
	 *  Name it "SlotContainer" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SlotContainer;

	/** Optional fallback text listing what's worn, for a WBP with no slot widgets wired. Name it "SlotListText" to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotListText;

	/** Optional worn-weight readout. Name it "WeightText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeightText;

	/** Widget class spawned once per worn slot */
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TSubclassOf<UEquipmentSlotWidget> SlotWidgetClass;

	/** Slot widgets spawned by the last RebuildSlots, in GetAllEquipSlots order */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UEquipmentSlotWidget>> SlotWidgets;

public:

	/** Binds this widget to a pawn's equipment and refreshes the display */
	void SetEquipment(UEquipmentComponent* InEquipment);

	/** Unbinds this widget from its equipment */
	void ClearEquipment();

	/** Multi-line summary: one line per worn slot */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FText GetContentsSummary() const;

	/** Combined weight of everything worn, as "Equipped: 4.3". Deliberately separate from the grid's
	 *  carried-weight readout - nothing folds the two figures together yet. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	FText GetWeightSummary() const;

protected:

	/** Blueprint handler to rebuild custom paperdoll visuals */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment", meta = (DisplayName = "Equipment Updated"))
	void BP_EquipmentUpdated();

	/** Bound to the equipment's OnEquipmentChanged delegate */
	UFUNCTION()
	void HandleEquipmentChanged();

	/** Pushes current state to the slot widgets, the text blocks and the BP hook */
	void RefreshDisplay();

	/** Builds one slot widget per worn slot into SlotContainer. Called on bind, not on every change. */
	void RebuildSlots();

	//~ Begin UUserWidget interface
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	//~ Begin UWindowWidget interface
	virtual void RequestClose_Implementation() override;
	//~ End UWindowWidget interface
};
