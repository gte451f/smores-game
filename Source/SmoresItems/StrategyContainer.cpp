// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyContainer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LootTableDefinition.h"
#include "Misc/Crc.h"
#include "SmoresItems.h"
#include "WorldSeedComponent.h"

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

	// contents are shared state, so only the server stocks them; clients receive the grid through
	// the inventory's own replication
	if (!HasAuthority())
	{
		return;
	}

	// the hand-authored items first, then the roll on top of them
	for (const FInventoryItem& Item : StartingItems)
	{
		Inventory->AddItem(Item);
	}

	RollLootTable();
}

void AStrategyContainer::PostActorCreated()
{
	Super::PostActorCreated();

#if WITH_EDITOR
	// a container newly placed in the editor gets its roll key now, so nobody has to remember to
	// author one. Game worlds are skipped - the same rule AStrategyUnit's PlacedRecordId follows
	const UWorld* World = GetWorld();

	if (World && !World->IsGameWorld() && !PlacedContainerId.IsValid() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		PlacedContainerId = FGuid::NewGuid();
	}
#endif
}

#if WITH_EDITOR
void AStrategyContainer::PostEditImport()
{
	Super::PostEditImport();

	// a pasted or alt-dragged copy is a different chest - sharing a key would make the two roll
	// identical contents, which reads as a bug the moment a player opens both
	PlacedContainerId = FGuid::NewGuid();
}
#endif

void AStrategyContainer::RollLootTable()
{
	if (!LootTable || !HasAuthority())
	{
		return;
	}

	FGuid Key = PlacedContainerId;

	if (!Key.IsValid())
	{
		// still deterministic - the actor name is the same every time this version of the level
		// loads - but it changes the moment someone edits the level, and the chest re-rolls with it
		UE_LOG(LogSmoresItems, Warning, TEXT("%s has a LootTable but no PlacedContainerId, so its roll is keyed off its actor name and changes whenever the level is edited. Author one."),
			*GetName());

		Key = FGuid(0, 0, 0, FCrc::StrCrc32(*GetName()));
	}

	FRandomStream Stream = UWeightedTableDefinition::MakeRollStream(UWorldSeedComponent::GetWorldSeedFor(this), Key);

	TArray<FInventoryItem> Rolled;
	LootTable->RollLoot(Stream, Rolled);

	// all-or-nothing, so a roll too big for the grid is refused as a whole rather than trimmed to
	// whatever happened to land first - AddItemsAllOrNothing has already said what didn't fit
	if (!Inventory->AddItemsAllOrNothing(Rolled))
	{
		UE_LOG(LogSmoresItems, Warning, TEXT("%s rolled %d item(s) from %s and couldn't fit them all, so it holds only its StartingItems. Make the grid bigger or the table smaller."),
			*GetName(), Rolled.Num(), *GetNameSafe(LootTable));
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
