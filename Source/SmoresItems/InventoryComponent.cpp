// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "SmoresItems.h"

namespace
{
	/** True if two axis-aligned cell rectangles share at least one cell */
	bool DoFootprintsOverlap(FIntPoint CellA, FIntPoint SizeA, FIntPoint CellB, FIntPoint SizeB)
	{
		return CellA.X < CellB.X + SizeB.X
			&& CellB.X < CellA.X + SizeA.X
			&& CellA.Y < CellB.Y + SizeB.Y
			&& CellB.Y < CellA.Y + SizeA.Y;
	}
}

UInventoryComponent::UInventoryComponent()
{
	// inventory is pure state - it never needs to tick
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Entries);
	DOREPLIFETIME(UInventoryComponent, GridWidth);
	DOREPLIFETIME(UInventoryComponent, GridHeight);
	DOREPLIFETIME(UInventoryComponent, StackMultiplier);
}

bool UInventoryComponent::HasOwnerAuthority() const
{
	return GetOwner() != nullptr && GetOwner()->HasAuthority();
}

int32 UInventoryComponent::IndexOfEntry(int32 EntryId) const
{
	if (EntryId == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	return Entries.IndexOfByPredicate([EntryId](const FInventoryEntry& Entry) { return Entry.EntryId == EntryId; });
}

bool UInventoryComponent::IsCellInBounds(FIntPoint Cell) const
{
	return Cell.X >= 0 && Cell.X < GridWidth && Cell.Y >= 0 && Cell.Y < GridHeight;
}

int32 UInventoryComponent::GetFreeCellCount() const
{
	int32 OccupiedCells = 0;

	for (const FInventoryEntry& Entry : Entries)
	{
		const FIntPoint Size = Entry.GetFootprint();
		OccupiedCells += Size.X * Size.Y;
	}

	return FMath::Max(GridWidth * GridHeight - OccupiedCells, 0);
}

int32 UInventoryComponent::GetEffectiveMaxStack(const UItemDefinition* Definition) const
{
	if (!Definition)
	{
		return 0;
	}

	// one number per holder type covers "a shelf stacks deeper than a backpack" without
	// per-transfer special cases; a multiplier can never scale a stack below a single item
	return FMath::Max(FMath::FloorToInt32(Definition->MaxStackSize * StackMultiplier), 1);
}

FInventoryEntry UInventoryComponent::GetEntry(int32 EntryId) const
{
	const int32 Index = IndexOfEntry(EntryId);

	return Entries.IsValidIndex(Index) ? Entries[Index] : FInventoryEntry();
}

int32 UInventoryComponent::GetEntryIdAtCell(FIntPoint Cell) const
{
	if (!IsCellInBounds(Cell))
	{
		return INDEX_NONE;
	}

	for (const FInventoryEntry& Entry : Entries)
	{
		if (Entry.CoversCell(Cell))
		{
			return Entry.EntryId;
		}
	}

	return INDEX_NONE;
}

bool UInventoryComponent::CanPlaceAt(const FInventoryItem& Item, FIntPoint Cell, bool bRotated, int32 IgnoreEntryId) const
{
	if (Item.IsEmpty())
	{
		return false;
	}

	const FIntPoint Size = Item.GetFootprint(bRotated);

	if (Size.X <= 0 || Size.Y <= 0)
	{
		return false;
	}

	// the whole footprint, not just the anchor, has to be inside the grid
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X + Size.X > GridWidth || Cell.Y + Size.Y > GridHeight)
	{
		return false;
	}

	for (const FInventoryEntry& Entry : Entries)
	{
		// the entry being re-anchored doesn't collide with where it currently sits
		if (Entry.EntryId == IgnoreEntryId)
		{
			continue;
		}

		if (DoFootprintsOverlap(Cell, Size, Entry.AnchorCell, Entry.GetFootprint()))
		{
			return false;
		}
	}

	return true;
}

bool UInventoryComponent::FindFreePlacement(const FInventoryItem& Item, FIntPoint& OutCell, bool& bOutRotated, int32 IgnoreEntryId) const
{
	if (Item.IsEmpty())
	{
		return false;
	}

	const FIntPoint NaturalSize = Item.GetFootprint(false);

	// a square footprint reads identically either way, so there's no second pass to run
	const int32 NumOrientations = (NaturalSize.X == NaturalSize.Y) ? 1 : 2;

	// sweep the whole grid in the natural orientation before turning anything sideways
	for (int32 Orientation = 0; Orientation < NumOrientations; ++Orientation)
	{
		const bool bRotated = (Orientation == 1);

		for (int32 Row = 0; Row < GridHeight; ++Row)
		{
			for (int32 Column = 0; Column < GridWidth; ++Column)
			{
				const FIntPoint Cell(Column, Row);

				if (CanPlaceAt(Item, Cell, bRotated, IgnoreEntryId))
				{
					OutCell = Cell;
					bOutRotated = bRotated;

					return true;
				}
			}
		}
	}

	return false;
}

bool UInventoryComponent::AddItem(const FInventoryItem& Item)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	// an item with no definition is nothing, not something to place
	if (Item.IsEmpty())
	{
		return false;
	}

	const int32 MaxStack = GetEffectiveMaxStack(Item.Definition);

	int32 Remaining = FMath::Max(Item.Quantity, 1);
	bool bChanged = false;

	// merge into existing stacks of the same type before consuming any new grid space
	for (FInventoryEntry& Entry : Entries)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (!Entry.Item.CanStackWith(Item))
		{
			continue;
		}

		const int32 Space = MaxStack - Entry.Item.Quantity;

		if (Space <= 0)
		{
			continue;
		}

		const int32 Merged = FMath::Min(Space, Remaining);

		Entry.Item.Quantity += Merged;
		Remaining -= Merged;
		bChanged = true;
	}

	// place whatever's left, splitting into as many entries as the stack cap requires
	while (Remaining > 0)
	{
		FInventoryItem Chunk = Item;
		Chunk.Quantity = FMath::Min(Remaining, MaxStack);

		FIntPoint Cell = FIntPoint::ZeroValue;
		bool bRotated = false;

		if (!FindFreePlacement(Chunk, Cell, bRotated))
		{
			UE_LOG(LogSmoresItems, Warning, TEXT("InventoryComponent on %s has no room for %d x '%s' (%d/%d cells free)."),
				*GetNameSafe(GetOwner()), Remaining, *GetNameSafe(Item.Definition), GetFreeCellCount(), GridWidth * GridHeight);

			break;
		}

		Entries.Emplace(NextEntryId++, Chunk, Cell, bRotated);
		Remaining -= Chunk.Quantity;
		bChanged = true;
	}

	if (bChanged)
	{
		OnInventoryChanged.Broadcast();
	}

	return Remaining <= 0;
}

bool UInventoryComponent::AddItemAt(const FInventoryItem& Item, FIntPoint Cell, bool bRotated)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	if (!CanPlaceAt(Item, Cell, bRotated))
	{
		return false;
	}

	FInventoryItem Placed = Item;
	Placed.Quantity = FMath::Clamp(Placed.Quantity, 1, GetEffectiveMaxStack(Placed.Definition));

	Entries.Emplace(NextEntryId++, Placed, Cell, bRotated);

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::RemoveEntry(int32 EntryId)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	const int32 Index = IndexOfEntry(EntryId);

	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}

	Entries.RemoveAt(Index);

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::SetEntryQuantity(int32 EntryId, int32 NewQuantity)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	const int32 Index = IndexOfEntry(EntryId);

	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}

	// emptying an entry removes the placement outright rather than leaving a zero-count ghost
	if (NewQuantity <= 0)
	{
		Entries.RemoveAt(Index);

		OnInventoryChanged.Broadcast();

		return true;
	}

	const int32 ClampedQuantity = FMath::Min(NewQuantity, GetEffectiveMaxStack(Entries[Index].Item.Definition));

	if (ClampedQuantity == Entries[Index].Item.Quantity)
	{
		return false;
	}

	Entries[Index].Item.Quantity = ClampedQuantity;

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::RepositionEntry(int32 EntryId, FIntPoint NewCell, bool bRotated)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	const int32 Index = IndexOfEntry(EntryId);

	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}

	// the entry is allowed to land back on cells it already occupies itself
	if (!CanPlaceAt(Entries[Index].Item, NewCell, bRotated, EntryId))
	{
		return false;
	}

	Entries[Index].AnchorCell = NewCell;
	Entries[Index].bRotated = bRotated;

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::SetGridSize(int32 NewWidth, int32 NewHeight)
{
	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	NewWidth = FMath::Clamp(NewWidth, 1, 32);
	NewHeight = FMath::Clamp(NewHeight, 1, 32);

	if (NewWidth == GridWidth && NewHeight == GridHeight)
	{
		return false;
	}

	GridWidth = NewWidth;
	GridHeight = NewHeight;

	// drop anything the smaller grid can no longer hold; no attempt to re-pack, since a resize
	// is an authoring/debug operation rather than something gameplay does routinely
	Entries.RemoveAll([this](const FInventoryEntry& Entry)
	{
		const FIntPoint Size = Entry.GetFootprint();

		const bool bStillFits = Entry.AnchorCell.X + Size.X <= GridWidth && Entry.AnchorCell.Y + Size.Y <= GridHeight;

		if (!bStillFits)
		{
			UE_LOG(LogSmoresItems, Warning, TEXT("InventoryComponent on %s dropped '%s' - it no longer fits the resized %dx%d grid."),
				*GetNameSafe(GetOwner()), *GetNameSafe(Entry.Item.Definition), GridWidth, GridHeight);
		}

		return !bStillFits;
	});

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::MoveItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity)
{
	if (!SourceInventory || !DestInventory)
	{
		return false;
	}

	// checked up front so a cross-inventory move can never half-apply
	if (!SourceInventory->HasOwnerAuthority() || !DestInventory->HasOwnerAuthority())
	{
		return false;
	}

	const FInventoryEntry SourceEntry = SourceInventory->GetEntry(EntryId);

	if (!SourceEntry.IsValidEntry())
	{
		return false;
	}

	const int32 RequestedQuantity = (Quantity <= 0) ? SourceEntry.Item.Quantity : FMath::Min(Quantity, SourceEntry.Item.Quantity);

	// a destination that stacks shallower than the source (a pawn's pack taking from a storefront
	// shelf) can't hold the whole stack in one entry - move what it can and leave the remainder
	// behind, rather than letting the placement clamp quietly destroy it
	const int32 MoveQuantity = FMath::Min(RequestedQuantity, DestInventory->GetEffectiveMaxStack(SourceEntry.Item.Definition));

	if (MoveQuantity <= 0)
	{
		return false;
	}

	const bool bMovingWholeEntry = (MoveQuantity == SourceEntry.Item.Quantity);
	const bool bSameInventory = (SourceInventory == DestInventory);

	// dropped back exactly where it started
	if (bSameInventory && bMovingWholeEntry && SourceEntry.AnchorCell == DestCell && SourceEntry.bRotated == bRotated)
	{
		return false;
	}

	FInventoryItem MovedItem = SourceEntry.Item;
	MovedItem.Quantity = MoveQuantity;

	// dropping onto an occupied cell only ever means "merge into that stack" - there's no
	// swap, because two differently-shaped footprints have no well-defined exchange
	const int32 TargetEntryId = DestInventory->GetEntryIdAtCell(DestCell);

	if (TargetEntryId != INDEX_NONE && !(bSameInventory && TargetEntryId == EntryId))
	{
		const FInventoryEntry TargetEntry = DestInventory->GetEntry(TargetEntryId);

		if (!TargetEntry.Item.CanStackWith(MovedItem))
		{
			return false;
		}

		const int32 Space = DestInventory->GetEffectiveMaxStack(TargetEntry.Item.Definition) - TargetEntry.Item.Quantity;

		if (Space <= 0)
		{
			return false;
		}

		const int32 Merged = FMath::Min(Space, MoveQuantity);

		DestInventory->SetEntryQuantity(TargetEntryId, TargetEntry.Item.Quantity + Merged);
		SourceInventory->SetEntryQuantity(EntryId, SourceEntry.Item.Quantity - Merged);

		return true;
	}

	// a split leaves the original entry in place, so it still occupies its own cells and the
	// new piece has to find room around it; only a whole-entry move may reuse them
	const int32 IgnoreEntryId = (bSameInventory && bMovingWholeEntry) ? EntryId : INDEX_NONE;

	if (!DestInventory->CanPlaceAt(MovedItem, DestCell, bRotated, IgnoreEntryId))
	{
		return false;
	}

	if (bSameInventory && bMovingWholeEntry)
	{
		// a pure reposition - the entry keeps its id rather than being destroyed and recreated
		return SourceInventory->RepositionEntry(EntryId, DestCell, bRotated);
	}

	if (!DestInventory->AddItemAt(MovedItem, DestCell, bRotated))
	{
		return false;
	}

	SourceInventory->SetEntryQuantity(EntryId, SourceEntry.Item.Quantity - MoveQuantity);

	return true;
}

void UInventoryComponent::OnRep_Entries()
{
	// authority already broadcast this directly from whichever mutator it called
	if (HasOwnerAuthority())
	{
		return;
	}

	OnInventoryChanged.Broadcast();
}
