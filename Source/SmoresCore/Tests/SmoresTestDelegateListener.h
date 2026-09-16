// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SmoresTestDelegateListener.generated.h"

/**
 *  Counts how many times a zero-parameter dynamic multicast delegate fired, so a test can
 *  assert "broadcast once on success, not at all on a refusal".
 *
 *  **Why this is not wrapped in WITH_DEV_AUTOMATION_TESTS like the rest of the harness.** A
 *  dynamic delegate can only be bound to a UFUNCTION, a UFUNCTION only exists on a UCLASS, and
 *  UHT parses every header in a module regardless of preprocessor conditions it doesn't know
 *  about. Guarding this one would generate reflection code for a class the compiler had been
 *  told to skip. It is an empty UObject with one counter, so the cost of it existing in a
 *  packaged build is a few bytes of class registration and nothing else.
 *
 *  Serves UInventoryComponent::OnInventoryChanged and UEquipmentComponent::OnEquipmentChanged,
 *  which have the same zero-parameter signature. Delegates that carry parameters need their own
 *  handler added here.
 */
UCLASS()
class SMORESCORE_API USmoresTestDelegateListener : public UObject
{
	GENERATED_BODY()

public:

	/** How many broadcasts have landed since construction or the last Reset */
	int32 CallCount = 0;

	/** Bind with AddDynamic to any DECLARE_DYNAMIC_MULTICAST_DELEGATE that takes no parameters */
	UFUNCTION()
	void OnChanged() { ++CallCount; }

	/** Zeroes the count, so one listener can cover several steps of a test */
	void Reset() { CallCount = 0; }
};
