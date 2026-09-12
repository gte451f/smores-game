// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DamageNumberActor.generated.h"

class UWidgetComponent;
class UDamageNumberWidget;

/**
 *  A single floating combat-damage number. Spawned above a damaged unit by
 *  UHealthComponent::TakeDamage; rises a fixed distance, fades out over the back portion
 *  of its lifetime, then self-destroys. Spawning several of these in quick succession (e.g.
 *  a unit taking repeated hits) naturally stacks them in the order they were received, since
 *  each rises independently from its own spawn time - no extra bookkeeping needed.
 */
UCLASS(abstract)
class SMORESCOMBAT_API ADamageNumberActor : public AActor
{
	GENERATED_BODY()

public:

	/** Constructor */
	ADamageNumberActor();

	/** Sets the displayed amount and starts the rise/fade animation */
	void Initialize(float DamageAmount);

protected:

	//~ Begin AActor interface
	virtual void Tick(float DeltaSeconds) override;
	//~ End AActor interface

	/** Hosts this number's visual. Assign a UDamageNumberWidget-derived WBP as its Widget Class. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Number", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> WidgetComponent;

	/** Total distance this number rises over its lifetime */
	UPROPERTY(EditAnywhere, Category = "Damage Number", meta = (ClampMin = 0, Units = "cm"))
	float FloatDistance = 100.0f;

	/** How long the number takes to rise and fade before it's destroyed */
	UPROPERTY(EditAnywhere, Category = "Damage Number", meta = (ClampMin = 0.1, Units = "s"))
	float Lifetime = 1.2f;

	/** Fraction of Lifetime (0-1) at which the number starts fading out */
	UPROPERTY(EditAnywhere, Category = "Damage Number", meta = (ClampMin = 0, ClampMax = 1))
	float FadeStartFraction = 0.5f;

private:

	/** Resolved once in Initialize(); null if the assigned Widget Class isn't a UDamageNumberWidget */
	TWeakObjectPtr<UDamageNumberWidget> CachedWidget;

	/** Time elapsed since Initialize() was called */
	float ElapsedTime = 0.0f;
};
