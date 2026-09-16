// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
 *  handler added here - OnIntChanged serves UWalletComponent::OnGoldChanged and OnActorChanged
 *  serves UHealthComponent::OnDamaged. All three share CallCount, so a listener bound to one
 *  delegate counts the same way whatever that delegate's shape is.
 */
UCLASS()
class SMORESCORE_API USmoresTestDelegateListener : public UObject
{
	GENERATED_BODY()

public:

	/** How many broadcasts have landed since construction or the last Reset */
	int32 CallCount = 0;

	/** The value carried by the most recent OnIntChanged broadcast */
	int32 LastInt = 0;

	/**
	 *  The actor carried by the most recent OnActorChanged broadcast.
	 *
	 *  Deliberately weak, and deliberately not a UPROPERTY. A listener is kept alive by
	 *  FSmoresTestWorld for the whole test, so a strong reference here would keep the recorded
	 *  actor alive too - and through it the world the actor was spawned into, which then fails
	 *  to collect when the test world is torn down ("Previously active world not cleaned up by
	 *  garbage collection"). A listener records what it saw; it has no business keeping it.
	 */
	TWeakObjectPtr<AActor> LastActor = nullptr;

	/** Bind with AddDynamic to any DECLARE_DYNAMIC_MULTICAST_DELEGATE that takes no parameters */
	UFUNCTION()
	void OnChanged() { ++CallCount; }

	/** Bind to a ..._OneParam delegate carrying an int32 - a new balance, a count, a quantity */
	UFUNCTION()
	void OnIntChanged(int32 NewValue) { ++CallCount; LastInt = NewValue; }

	/** Bind to a ..._OneParam delegate carrying an actor - an instigator, a target */
	UFUNCTION()
	void OnActorChanged(AActor* Actor) { ++CallCount; LastActor = Actor; }

	/** Zeroes the count and the recorded payloads, so one listener can cover several steps of a test */
	void Reset()
	{
		CallCount = 0;
		LastInt = 0;
		LastActor = nullptr;
	}
};
