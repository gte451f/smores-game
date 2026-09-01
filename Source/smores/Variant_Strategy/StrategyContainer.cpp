// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyContainer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "StrategyUnit.h"

AStrategyContainer::AStrategyContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the mesh and make it the root component
	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Container Mesh"));
	SetRootComponent(ContainerMesh);

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(250.0f);
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

bool AStrategyContainer::IsUnitInRange(const AStrategyUnit* Unit) const
{
	return Unit && FVector::Dist(GetActorLocation(), Unit->GetActorLocation()) <= InteractionRange->GetScaledSphereRadius();
}

void AStrategyContainer::NotifyOpened()
{
	BP_ContainerOpened();
}

void AStrategyContainer::SetSelected(bool bSelected)
{
	ContainerMesh->SetMaterial(0, bSelected ? SelectedMaterial : NormalMaterial);
}
