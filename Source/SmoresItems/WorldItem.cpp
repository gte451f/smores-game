// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WorldItem.h"
#include "SmoresItems.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AWorldItem::AWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;

	// a loose item is spawned and destroyed during play, so it has to exist on clients too
	bReplicates = true;

	// create the mesh and make it the root component
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	SetRootComponent(ItemMesh);

	// purely decorative - a dropped item should never shove a pawn around or block a selection
	// trace, and the interaction sphere below is what actually gates reaching it
	ItemMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(312.5f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));
}

void AWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWorldItem, Item);
}

void AWorldItem::BeginPlay()
{
	Super::BeginPlay();

	RefreshMesh();
}

void AWorldItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshMesh();
}

void AWorldItem::OnRep_Item()
{
	RefreshMesh();
}

void AWorldItem::RefreshMesh()
{
	ItemMesh->SetStaticMesh(Item.Definition ? Item.Definition->WorldMesh : nullptr);
}

bool AWorldItem::IsInRangeOf(const AActor* Other) const
{
	return IInventoryHolder::IsActorWithinSphere(this, InteractionRange, Other);
}

void AWorldItem::SetItem(const FInventoryItem& NewItem)
{
	// shared world state - only the server may mutate it
	if (!HasAuthority())
	{
		return;
	}

	Item = NewItem;

	// OnRep only fires on the other machines, so the server refreshes its own mesh here
	RefreshMesh();
}

bool AWorldItem::TryPickUp(UInventoryComponent* DestInventory)
{
	// shared world state - only the server may mutate it
	if (!HasAuthority())
	{
		return false;
	}

	if (!DestInventory || Item.IsEmpty())
	{
		return false;
	}

	int32 QuantityTaken = 0;
	DestInventory->AddItemCounted(Item, QuantityTaken);

	// nothing fit: leave the item exactly where it was rather than half-consuming it
	if (QuantityTaken <= 0)
	{
		UE_LOG(LogSmoresItems, Verbose, TEXT("WorldItem %s: no room in %s for '%s'."),
			*GetName(), *GetNameSafe(DestInventory->GetOwner()), *GetNameSafe(Item.Definition));

		return false;
	}

	if (QuantityTaken >= Item.Quantity)
	{
		Destroy();

		return true;
	}

	// only part of the stack fit - the rest stays on the ground as a smaller pile
	Item.Quantity -= QuantityTaken;

	UE_LOG(LogSmoresItems, Verbose, TEXT("WorldItem %s: %s took %d x '%s', %d left on the ground."),
		*GetName(), *GetNameSafe(DestInventory->GetOwner()), QuantityTaken, *GetNameSafe(Item.Definition), Item.Quantity);

	return true;
}

AWorldItem* AWorldItem::SpawnWorldItem(const UObject* WorldContextObject, TSubclassOf<AWorldItem> WorldItemClass, const FInventoryItem& InItem, const FVector& Location, const FRotator& Rotation)
{
	if (!WorldItemClass || InItem.IsEmpty())
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;

	if (!World)
	{
		return nullptr;
	}

	// the actor replicates, so a client-spawned copy would exist on that client alone
	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogSmoresItems, Warning, TEXT("SpawnWorldItem called on a client - world pickups may only be spawned with authority."));

		return nullptr;
	}

	// spawn deferred so the item is in place before OnConstruction picks its mesh
	AWorldItem* Spawned = World->SpawnActorDeferred<AWorldItem>(WorldItemClass, FTransform(Rotation, Location), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Spawned)
	{
		return nullptr;
	}

	Spawned->Item = InItem;
	Spawned->FinishSpawning(FTransform(Rotation, Location));

	return Spawned;
}
