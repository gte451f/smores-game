// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "SmoresItems.h"

#define LOCTEXT_NAMESPACE "SmoresInventory"

namespace
{
	/**
	 *  Word order for a modifier whose asset left NamePattern blank. English, and deliberately
	 *  a last resort - the pattern lives on the modifier precisely so a translator can reorder
	 *  it, and this fallback can't be reordered per modifier. It exists only so an unfilled
	 *  asset shows its modifier rather than silently dropping it from the name.
	 */
	FText GetModifierNamePattern(const UItemModifierDefinition* Modifier)
	{
		return Modifier->NamePattern.IsEmpty()
			? LOCTEXT("DefaultModifierNamePattern", "{Modifier} {Item}")
			: Modifier->NamePattern;
	}
}

FText FInventoryItem::GetDisplayName() const
{
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	FText Composed = Definition->DisplayName;

	// slot order, not array order: a copy holding its quality first must still read
	// "Masterwork Bronze Spear", because the array order is an accident of how it was built
	for (const EItemModifierSlot Slot : UItemModifierDefinition::GetAllModifierSlots())
	{
		if (const UItemModifierDefinition* Modifier = GetModifier(Slot))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Modifier"), Modifier->DisplayName);
			Args.Add(TEXT("Item"), Composed);

			Composed = FText::Format(GetModifierNamePattern(Modifier), Args);
		}
	}

	return Composed;
}

FLinearColor FInventoryItem::GetTint() const
{
	FLinearColor Tint = FLinearColor::White;

	for (const TObjectPtr<UItemModifierDefinition>& Modifier : Modifiers)
	{
		if (Modifier)
		{
			// multiplied rather than "last one wins", so a quality left at White lets the
			// material's colour through instead of washing it out
			Tint *= Modifier->Tint;
		}
	}

	return Tint;
}

float FInventoryItem::GetUnitWeight() const
{
	if (!Definition)
	{
		return 0.0f;
	}

	float Weight = Definition->Weight;

	for (const TObjectPtr<UItemModifierDefinition>& Modifier : Modifiers)
	{
		if (Modifier)
		{
			Weight *= Modifier->WeightMultiplier;
		}
	}

	return Weight;
}

int32 FInventoryItem::GetUnitBaseValue() const
{
	if (!Definition)
	{
		return 0;
	}

	double Value = Definition->BaseValue;

	for (const TObjectPtr<UItemModifierDefinition>& Modifier : Modifiers)
	{
		if (Modifier)
		{
			Value *= Modifier->ValueMultiplier;
		}
	}

	// a definition priced at nothing is genuinely worthless rather than cheap, and no multiplier
	// makes it worth something; anything the designer did price never rounds away to nothing
	if (Definition->BaseValue <= 0)
	{
		return 0;
	}

	return FMath::Max(1, FMath::RoundToInt32(Value));
}

float FInventoryItem::GetConditionScale() const
{
	float Scale = 1.0f;

	for (const TObjectPtr<UItemModifierDefinition>& Modifier : Modifiers)
	{
		if (Modifier)
		{
			Scale *= Modifier->ConditionMultiplier;
		}
	}

	return Scale;
}

UItemModifierDefinition* FInventoryItem::GetModifier(EItemModifierSlot Slot) const
{
	for (const TObjectPtr<UItemModifierDefinition>& Modifier : Modifiers)
	{
		if (Modifier && Modifier->Slot == Slot)
		{
			return Modifier;
		}
	}

	return nullptr;
}

bool FInventoryItem::AddModifier(UItemModifierDefinition* Modifier)
{
	if (!Modifier || HasModifier(Modifier->Slot))
	{
		return false;
	}

	Modifiers.Add(Modifier);

	return true;
}

bool FInventoryItem::SetModifier(UItemModifierDefinition* Modifier)
{
	if (!Modifier)
	{
		return false;
	}

	RemoveModifier(Modifier->Slot);
	Modifiers.Add(Modifier);

	return true;
}

bool FInventoryItem::RemoveModifier(EItemModifierSlot Slot)
{
	// RemoveAll rather than a single removal, so a hand-authored array that got two materials
	// into it is left genuinely empty in that slot rather than one deep
	return Modifiers.RemoveAll([Slot](const TObjectPtr<UItemModifierDefinition>& Modifier)
		{
			return Modifier && Modifier->Slot == Slot;
		}) > 0;
}

bool FInventoryItem::HasSameModifiersAs(const FInventoryItem& Other) const
{
	// compared per slot rather than as two arrays, which is what makes the comparison
	// order-independent: there is at most one modifier per slot, so matching every slot matches
	// the whole set
	for (const EItemModifierSlot Slot : UItemModifierDefinition::GetAllModifierSlots())
	{
		if (GetModifier(Slot) != Other.GetModifier(Slot))
		{
			return false;
		}
	}

	return true;
}

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

	/** The one figure a sort criterion orders by, as a single comparable number */
	double GetSortKey(const FInventoryEntry& Entry, EInventorySortCriterion Criterion)
	{
		switch (Criterion)
		{
		case EInventorySortCriterion::Weight:
			return Entry.Item.GetTotalWeight();

		case EInventorySortCriterion::Value:
			return Entry.Item.GetTotalBaseValue();

		case EInventorySortCriterion::Quantity:
			return Entry.Item.Quantity;
		}

		return 0.0;
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
	DOREPLIFETIME(UInventoryComponent, WeightCapacity);
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

float UInventoryComponent::GetTotalWeight() const
{
	float TotalWeight = 0.0f;

	for (const FInventoryEntry& Entry : Entries)
	{
		// unit weight x quantity, read through the accessor like every other derived figure - so a
		// steel sword weighs what its material says rather than what a bare sword does
		TotalWeight += Entry.Item.GetTotalWeight();
	}

	return TotalWeight;
}

bool UInventoryComponent::IsOverWeightCapacity() const
{
	// a holder with no capacity authored is unlimited rather than permanently overloaded
	return HasWeightLimit() && GetTotalWeight() > WeightCapacity;
}

int32 UInventoryComponent::ScaleStack(int32 BaseMaxStackSize) const
{
	if (BaseMaxStackSize <= 0)
	{
		return 0;
	}

	// one number per holder type covers "a shelf stacks deeper than a backpack" without
	// per-transfer special cases; a multiplier can never scale a stack below a single item
	return FMath::Max(FMath::FloorToInt32(BaseMaxStackSize * StackMultiplier), 1);
}

int32 UInventoryComponent::GetEffectiveMaxStackForItem(const FInventoryItem& Item) const
{
	return ScaleStack(Item.GetBaseMaxStackSize());
}

int32 UInventoryComponent::GetEffectiveMaxStack(const UItemDefinition* Definition) const
{
	return ScaleStack(Definition ? Definition->MaxStackSize : 0);
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
	return CanPlaceAgainst(Entries, Item, Cell, bRotated, IgnoreEntryId);
}

bool UInventoryComponent::CanPlaceAgainst(const TArray<FInventoryEntry>& Placements, const FInventoryItem& Item, FIntPoint Cell, bool bRotated, int32 IgnoreEntryId) const
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

	for (const FInventoryEntry& Entry : Placements)
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
	return FindFreePlacementAgainst(Entries, Item, OutCell, bOutRotated, IgnoreEntryId);
}

bool UInventoryComponent::FindFreePlacementAgainst(const TArray<FInventoryEntry>& Placements, const FInventoryItem& Item, FIntPoint& OutCell, bool& bOutRotated, int32 IgnoreEntryId) const
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

				if (CanPlaceAgainst(Placements, Item, Cell, bRotated, IgnoreEntryId))
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
	int32 QuantityAdded = 0;

	return AddItemCounted(Item, QuantityAdded);
}

bool UInventoryComponent::AddItemCounted(const FInventoryItem& Item, int32& OutQuantityAdded)
{
	OutQuantityAdded = 0;

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

	const int32 MaxStack = GetEffectiveMaxStackForItem(Item);

	const int32 Requested = FMath::Max(Item.Quantity, 1);

	int32 Remaining = Requested;
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

	OutQuantityAdded = Requested - Remaining;

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
	Placed.Quantity = FMath::Clamp(Placed.Quantity, 1, GetEffectiveMaxStackForItem(Placed));

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

	const int32 ClampedQuantity = FMath::Min(NewQuantity, GetEffectiveMaxStackForItem(Entries[Index].Item));

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

bool UInventoryComponent::SortEntries(EInventorySortCriterion Criterion)
{
	ESmoresRefusalReason UnusedReason = ESmoresRefusalReason::None;

	return SortEntriesWithReason(Criterion, UnusedReason);
}

bool UInventoryComponent::SortEntriesWithReason(EInventorySortCriterion Criterion, ESmoresRefusalReason& OutReason)
{
	// most ways of changing nothing are not worth telling the player about, so the default is
	// silence and only the one case that surprises them sets a reason
	OutReason = ESmoresRefusalReason::None;

	// shared gameplay state - only the server may mutate it
	if (!HasOwnerAuthority())
	{
		return false;
	}

	if (Entries.IsEmpty())
	{
		return false;
	}

	TArray<FInventoryEntry> Working = Entries;

	// pour every later stack into the earliest one that will take it, so a sort can't leave two
	// half-stacks of the same thing sitting side by side looking unsorted. This is exactly the
	// merge AddItem already does - CanStackWith plus this holder's effective cap - so it adds no
	// rule the grid didn't already have, and the stolen flag still can't be laundered by it
	for (int32 Index = 0; Index < Working.Num(); ++Index)
	{
		// an entry already drained into an earlier one is on its way out - refilling it from a
		// later stack would conserve the quantity but resurrect a placement that should just go
		if (Working[Index].Item.Quantity <= 0)
		{
			continue;
		}

		const int32 MaxStack = GetEffectiveMaxStackForItem(Working[Index].Item);

		for (int32 OtherIndex = Index + 1; OtherIndex < Working.Num(); ++OtherIndex)
		{
			const int32 Space = MaxStack - Working[Index].Item.Quantity;

			if (Space <= 0)
			{
				break;
			}

			if (!Working[Index].Item.CanStackWith(Working[OtherIndex].Item))
			{
				continue;
			}

			const int32 Merged = FMath::Min(Space, Working[OtherIndex].Item.Quantity);

			Working[Index].Item.Quantity += Merged;
			Working[OtherIndex].Item.Quantity -= Merged;
		}
	}

	// an entry emptied by that merge is gone rather than left as a zero-count ghost, the same
	// rule SetEntryQuantity keeps
	Working.RemoveAll([](const FInventoryEntry& Entry) { return Entry.Item.Quantity <= 0; });

	Working.Sort([Criterion](const FInventoryEntry& A, const FInventoryEntry& B)
	{
		const double KeyA = GetSortKey(A, Criterion);
		const double KeyB = GetSortKey(B, Criterion);

		// compared exactly rather than with a tolerance: a comparator that calls near values
		// equal isn't a strict ordering, and Sort is entitled to misbehave on one that isn't
		if (KeyA != KeyB)
		{
			return KeyA > KeyB;
		}

		// bigger footprints first among equals - first-fit packing strands a large item far more
		// easily than a small one, so handing it the emptier grid is what keeps a repack from
		// failing outright
		const FIntPoint SizeA = A.Item.GetFootprint(false);
		const FIntPoint SizeB = B.Item.GetFootprint(false);

		const int32 AreaA = SizeA.X * SizeA.Y;
		const int32 AreaB = SizeB.X * SizeB.Y;

		if (AreaA != AreaB)
		{
			return AreaA > AreaB;
		}

		const FString NameA = A.Item.GetDisplayName().ToString();
		const FString NameB = B.Item.GetDisplayName().ToString();

		if (NameA != NameB)
		{
			return NameA < NameB;
		}

		// last resort, and it earns its keep: Sort is not stable, so two entries equal all the
		// way down could otherwise swap places on a repack that changed nothing else - which
		// would make "sorted" a state the grid never settles into
		return A.EntryId < B.EntryId;
	});

	// Built into a scratch array rather than mutated in place. First-fit packing in criterion
	// order can strand an item the *previous* arrangement had room for (a 1x1 taking the corner
	// a 2x2 needed), and a sort that silently drops what it can't re-place would be far worse
	// than one that declines to run.
	TArray<FInventoryEntry> Packed;
	Packed.Reserve(Working.Num());

	for (const FInventoryEntry& Entry : Working)
	{
		FIntPoint Cell = FIntPoint::ZeroValue;
		bool bRotated = false;

		if (!FindFreePlacementAgainst(Packed, Entry.Item, Cell, bRotated, INDEX_NONE))
		{
			UE_LOG(LogSmoresItems, Warning, TEXT("InventoryComponent on %s could not repack '%s' - the sort was abandoned and nothing changed."),
				*GetNameSafe(GetOwner()), *GetNameSafe(Entry.Item.Definition));

			// the one outcome here the player can't work out for themselves: the grid is
			// unchanged and looks exactly like a grid that was already sorted
			OutReason = ESmoresRefusalReason::NoRoom;

			return false;
		}

		// the entry keeps its id through the repack, so a UI holding one across a client->server
		// round trip still resolves to the same item afterwards
		Packed.Emplace(Entry.EntryId, Entry.Item, Cell, bRotated);
	}

	// an already-sorted grid reports no change rather than broadcasting a redraw nobody needs -
	// the same convention SetGridSize and SetEntryQuantity follow
	if (Packed.Num() == Entries.Num())
	{
		bool bIdentical = true;

		for (int32 Index = 0; Index < Packed.Num() && bIdentical; ++Index)
		{
			bIdentical = Packed[Index].EntryId == Entries[Index].EntryId
				&& Packed[Index].AnchorCell == Entries[Index].AnchorCell
				&& Packed[Index].bRotated == Entries[Index].bRotated
				&& Packed[Index].Item.Quantity == Entries[Index].Item.Quantity;
		}

		if (bIdentical)
		{
			return false;
		}
	}

	Entries = MoveTemp(Packed);

	OnInventoryChanged.Broadcast();

	return true;
}

bool UInventoryComponent::MoveItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity)
{
	int32 QuantityMoved = 0;

	return MoveItemCounted(SourceInventory, EntryId, DestInventory, DestCell, bRotated, Quantity, QuantityMoved);
}

bool UInventoryComponent::MoveItemCounted(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity, int32& OutQuantityMoved)
{
	OutQuantityMoved = 0;

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
	const int32 MoveQuantity = FMath::Min(RequestedQuantity, DestInventory->GetEffectiveMaxStackForItem(SourceEntry.Item));

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

		const int32 Space = DestInventory->GetEffectiveMaxStackForItem(TargetEntry.Item) - TargetEntry.Item.Quantity;

		if (Space <= 0)
		{
			return false;
		}

		const int32 Merged = FMath::Min(Space, MoveQuantity);

		DestInventory->SetEntryQuantity(TargetEntryId, TargetEntry.Item.Quantity + Merged);
		SourceInventory->SetEntryQuantity(EntryId, SourceEntry.Item.Quantity - Merged);

		OutQuantityMoved = Merged;

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
		if (!SourceInventory->RepositionEntry(EntryId, DestCell, bRotated))
		{
			return false;
		}

		OutQuantityMoved = MoveQuantity;

		return true;
	}

	if (!DestInventory->AddItemAt(MovedItem, DestCell, bRotated))
	{
		return false;
	}

	SourceInventory->SetEntryQuantity(EntryId, SourceEntry.Item.Quantity - MoveQuantity);

	OutQuantityMoved = MoveQuantity;

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

#undef LOCTEXT_NAMESPACE
