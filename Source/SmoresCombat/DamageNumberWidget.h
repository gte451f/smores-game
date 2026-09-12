// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

/**
 *  A single floating combat-damage number's visual. Spawned and driven by
 *  ADamageNumberActor, which owns the world placement, rise, and fade animation - this
 *  widget only ever displays the amount.
 */
UCLASS(abstract)
class SMORESCOMBAT_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Text block showing the damage amount. Name it "AmountText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AmountText;

public:

	/** Displays Amount as a negative red number (e.g. 25 damage taken shows as "-25") */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetDamageAmount(float Amount);
};
