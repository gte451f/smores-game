// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyContainer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AStrategyContainer::AStrategyContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	// the Inventory component below is replicated, which does nothing unless the actor carrying
	// it is too. A placed container gets away without this today only because placed actors
	// exist on every machine regardless; a spawned one (or a client joining mid-session) would
	// not - see AWorldItem, which has always set it because it appears and vanishes during play
	bReplicates = true;

	// create the mesh and make it the root component
	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Container Mesh"));
	SetRootComponent(ContainerMesh);

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(312.5f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// create the inventory component
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
}

void AStrategyContainer::BeginPlay()
{
	Super::BeginPlay();

	// start out showing the unselected material
	if (NormalMaterial)
	{
		ContainerMesh->SetMaterial(0, NormalMaterial);
	}

	// stock the inventory with the starting items
	for (const FInventoryItem& Item : StartingItems)
	{
		Inventory->AddItem(Item);
	}
}

bool AStrategyContainer::IsInRangeOf(const AActor* Other) const
{
	return IInventoryHolder::IsActorWithinSphere(this, InteractionRange, Other);
}

void AStrategyContainer::NotifyOpened()
{
	BP_ContainerOpened();
}

void AStrategyContainer::SetSelected(bool bSelected)
{
	ContainerMesh->SetMaterial(0, bSelected ? SelectedMaterial : NormalMaterial);
}
