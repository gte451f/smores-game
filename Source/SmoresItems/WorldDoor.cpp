// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WorldDoor.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AWorldDoor::AWorldDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	// open and shut during play, so every machine has to hear about it
	bReplicates = true;

	DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Door Root"));
	SetRootComponent(DoorRoot);

	// Movable throughout: the leaf swings, and a movable component can't hang off a static parent
	DoorRoot->SetMobility(EComponentMobility::Movable);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame Mesh"));
	FrameMesh->SetupAttachment(DoorRoot);
	FrameMesh->SetMobility(EComponentMobility::Movable);

	HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge Pivot"));
	HingePivot->SetupAttachment(DoorRoot);
	HingePivot->SetMobility(EComponentMobility::Movable);

	LeafMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaf Mesh"));
	LeafMesh->SetupAttachment(HingePivot);
	LeafMesh->SetMobility(EComponentMobility::Movable);

	// the leaf is the wall: it blocks pawns and it cuts the navmesh while it stands in the doorway
	LeafMesh->SetCollisionProfileName(FName("BlockAll"));
	LeafMesh->SetCanEverAffectNavigation(true);

	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(DoorRoot);

	// a doorway is wide, and the squad member reaching for the handle stands to one side of the
	// middle of it - a little more than a unit's 250 so a walk that stops at the leaf is in reach
	InteractionRange->SetSphereRadius(300.0f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	// never a navigation obstacle itself, however it's set up in the Blueprint
	InteractionRange->SetCanEverAffectNavigation(false);
}

void AWorldDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWorldDoor, bOpen);
}

void AWorldDoor::BeginPlay()
{
	Super::BeginPlay();

	ApplyOpenState(/*bAnnounce*/ false);
}

void AWorldDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// so a door authored as starting open shows open in the editor viewport, not only once the level runs
	ApplyOpenState(/*bAnnounce*/ false);
}

bool AWorldDoor::IsInRangeOf(const AActor* Other) const
{
	return ISmoresInteractable::IsActorWithinSphere(this, InteractionRange, Other);
}

void AWorldDoor::SetOpen(bool bNewOpen)
{
	// shared world state - only the server may swing it
	if (!HasAuthority() || bOpen == bNewOpen)
	{
		return;
	}

	bOpen = bNewOpen;

	// the server doesn't get an OnRep, so it applies the change itself
	ApplyOpenState(/*bAnnounce*/ true);
}

void AWorldDoor::OnRep_Open()
{
	ApplyOpenState(/*bAnnounce*/ true);
}

void AWorldDoor::ApplyOpenState(bool bAnnounce)
{
	if (HingePivot)
	{
		// instant, deliberately: the swing is what re-opens or cuts the path, and a leaf halfway
		// round would leave the navmesh half-rebuilt for as long as it took
		HingePivot->SetRelativeRotation(FRotator(0.0f, bOpen ? OpenYaw : 0.0f, 0.0f));
	}

	if (!bAnnounce)
	{
		return;
	}

	if (bOpen)
	{
		BP_DoorOpened();
	}
	else
	{
		BP_DoorClosed();
	}
}
