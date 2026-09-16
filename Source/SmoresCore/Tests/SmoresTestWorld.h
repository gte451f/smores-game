// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/GCObject.h"

/**
 *  A throwaway world for one automation test, plus the handful of things a test hangs off it.
 *  Construct one on the stack at the top of RunTest and forget about it - the destructor tears
 *  the world down and forces a collect.
 *
 *  **Why a world at all, rather than a bare NewObject.** Nearly every mutator worth testing in
 *  this project is authority-gated - UInventoryComponent, UEquipmentComponent, UWalletComponent
 *  and UHealthComponent all no-op silently off-authority, by design and per
 *  multiplayer-discipline.md. A component created with NewObject and no owner has no authority,
 *  so every mutator returns having done nothing, and a test written as "call RemoveEntry, assert
 *  the grid is unchanged" passes for entirely the wrong reason. An actor spawned into a world
 *  with no net driver holds ROLE_Authority, so the real path runs. SmoresTestWorldTest.cpp
 *  asserts exactly that, once, so the whole suite isn't quietly testing nothing.
 *
 *  Lives in SmoresCore because three modules want it and SmoresCore is the only one all of them
 *  already depend on.
 */
struct FSmoresTestWorld : public FGCObject
{
	explicit FSmoresTestWorld(EWorldType::Type WorldType = EWorldType::Game)
	{
		bCreated = Wrapper.CreateTestWorld(WorldType);
	}

	virtual ~FSmoresTestWorld()
	{
		if (bCreated)
		{
			// runs before ~FGCObject, so anything handed to KeepAlive is still referenced
			// while the forced collect sweeps
			Wrapper.DestroyTestWorld(true);
		}
	}

	FSmoresTestWorld(const FSmoresTestWorld&) = delete;
	FSmoresTestWorld& operator=(const FSmoresTestWorld&) = delete;

	/** True if the world was actually created - a test should bail rather than assert against null */
	bool IsValid() const { return bCreated && Wrapper.GetTestWorld() != nullptr; }

	UWorld* GetWorld() const { return Wrapper.GetTestWorld(); }

	/**
	 *  A bare actor to own components under test. Holds ROLE_Authority, which is the entire
	 *  point - see the class comment.
	 */
	AActor* SpawnOwner()
	{
		UWorld* World = GetWorld();

		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		return World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParameters);
	}

	/** Attaches a component to an already-spawned owner and registers it */
	template <typename TComponent>
	TComponent* AddComponent(AActor* Owner)
	{
		if (!Owner)
		{
			return nullptr;
		}

		TComponent* Component = NewObject<TComponent>(Owner);
		Component->RegisterComponent();

		return Component;
	}

	/** The common case: one fresh owner actor carrying one component under test */
	template <typename TComponent>
	TComponent* SpawnComponent()
	{
		return AddComponent<TComponent>(SpawnOwner());
	}

	/**
	 *  A UObject that stays reachable until this world is torn down. Test-built item
	 *  definitions and delegate listeners have no owner to keep them alive, and a collect
	 *  triggered mid-test would otherwise pull them out from under the assertions.
	 */
	template <typename TObject>
	TObject* NewKeptObject()
	{
		TObject* Object = NewObject<TObject>();
		KeepAlive(Object);

		return Object;
	}

	void KeepAlive(UObject* Object)
	{
		if (Object)
		{
			KeptObjects.Add(Object);
		}
	}

	/** Runs BeginPlay - needed only by components that read a starting value there */
	bool BeginPlay() { return Wrapper.BeginPlayInTestWorld(); }

	/** Advances one frame - needed only by components with a timer */
	bool Tick(float DeltaSeconds = 0.01f) { return Wrapper.TickTestWorld(DeltaSeconds); }

	/**
	 *  Advances simulated time far enough for a timer to fire. Lower the duration property
	 *  under test first - ticking through a 15 second default at 100fps is 1500 iterations
	 *  for no benefit.
	 */
	bool TickFor(float Seconds, float DeltaSeconds = 0.01f)
	{
		const int32 Steps = FMath::CeilToInt32(Seconds / FMath::Max(DeltaSeconds, KINDA_SMALL_NUMBER));

		for (int32 Step = 0; Step < Steps; ++Step)
		{
			if (!Tick(DeltaSeconds))
			{
				return false;
			}
		}

		return true;
	}

	/** Hands any world-level failures to the automation framework. Call before the test returns. */
	void ForwardErrors(FAutomationTestBase* Test) { Wrapper.ForwardErrorMessages(Test); }

	//~ Begin FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObjects(KeptObjects);
	}

	virtual FString GetReferencerName() const override { return TEXT("FSmoresTestWorld"); }
	//~ End FGCObject interface

private:

	FTestWorldWrapper Wrapper;

	TArray<TObjectPtr<UObject>> KeptObjects;

	bool bCreated = false;
};

#endif // WITH_DEV_AUTOMATION_TESTS
