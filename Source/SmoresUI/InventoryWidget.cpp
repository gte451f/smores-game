// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "InventoryDragDropOperation.h"
#include "InventoryMoveHost.h"
#include "SmoresUI.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"

#define LOCTEXT_NAMESPACE "InventoryWidget"

void UInventoryWidget::SetInventory(UInventoryComponent* InInventory)
{
	// drop any previous binding
	ClearInventory();

	BoundInventory = InInventory;

	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.AddDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	RefreshDisplay();
}

void UInventoryWidget::ClearInventory()
{
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryWidget::HandleInventoryChanged);
	}

	BoundInventory.Reset();

	// cleared with the inventory so a window reused for a different holder (the container window
	// serves both chests and loot) can never carry the previous pawn's paperdoll across
	EquipmentTarget.Reset();
}

void UInventoryWidget::SetEquipmentTarget(UEquipmentComponent* InEquipment)
{
	EquipmentTarget = InEquipment;
}

FIntPoint UInventoryWidget::GetGridSize() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetGridSize() : FIntPoint::ZeroValue;
}

TArray<FInventoryEntry> UInventoryWidget::GetEntries() const
{
	return BoundInventory.IsValid() ? BoundInventory->GetEntries() : TArray<FInventoryEntry>();
}

FText UInventoryWidget::GetContentsSummary() const
{
	if (!BoundInventory.IsValid())
	{
		return FText::GetEmpty();
	}

	const TArray<FInventoryEntry>& Entries = BoundInventory->GetEntries();
	const FIntPoint GridSize = BoundInventory->GetGridSize();

	TArray<FString> Lines;
	Lines.Reserve(Entries.Num() + 1);

	Lines.Add(FString::Printf(TEXT("%dx%d grid, %d/%d cells free"),
		GridSize.X, GridSize.Y, BoundInventory->GetFreeCellCount(), GridSize.X * GridSize.Y));

	// the same labelled readout WeightText shows, so a WBP wiring only one of the two still reports weight
	Lines.Add(GetWeightSummary().ToString());

	for (const FInventoryEntry& Entry : Entries)
	{
		// display data lives on the shared item definition, not on the carried instance
		const FIntPoint Footprint = Entry.GetFootprint();

		Lines.Add(FString::Printf(TEXT("- %s @ (%d,%d) %dx%d%s"),
			*UInventoryWidget::GetItemLabel(Entry.Item).ToString(),
			Entry.AnchorCell.X, Entry.AnchorCell.Y,
			Footprint.X, Footprint.Y,
			Entry.bRotated ? TEXT(" rotated") : TEXT("")));
	}

	if (Entries.IsEmpty())
	{
		Lines.Add(LOCTEXT("EmptyInventory", "(empty)").ToString());
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FText UInventoryWidget::GetWeightSummary() const
{
	if (!BoundInventory.IsValid())
	{
		return FText::GetEmpty();
	}

	// one decimal place is enough resolution for a figure the player only ever reads, never solves against
	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 1;
	Format.MaximumFractionalDigits = 1;

	const FText CarriedText = FText::AsNumber(BoundInventory->GetTotalWeight(), &Format);

	// a holder with no capacity authored has no denominator to show - see UInventoryComponent::HasWeightLimit
	if (!BoundInventory->HasWeightLimit())
	{
		return FText::Format(LOCTEXT("WeightNoCapacity", "Weight: {0}"), CarriedText);
	}

	return FText::Format(LOCTEXT("WeightWithCapacity", "Weight: {0} / {1}"),
		CarriedText, FText::AsNumber(BoundInventory->GetWeightCapacity(), &Format));
}

bool UInventoryWidget::IsOverWeightCapacity() const
{
	return BoundInventory.IsValid() && BoundInventory->IsOverWeightCapacity();
}

FText UInventoryWidget::GetItemLabel(const FInventoryItem& Item)
{
	if (Item.IsEmpty())
	{
		return FText::GetEmpty();
	}

	// suffix the count only for an actual stack - a lone item reads as just its name
	if (Item.Quantity > 1)
	{
		return FText::Format(LOCTEXT("StackedItemLabel", "{0} x{1}"), Item.GetDisplayName(), FText::AsNumber(Item.Quantity));
	}

	return Item.GetDisplayName();
}

void UInventoryWidget::HandleInventoryChanged()
{
	RefreshDisplay();
}

void UInventoryWidget::HandleDragEnded(UDragDropOperation* Operation)
{
	ClearDragPreview();
}

void UInventoryWidget::RefreshDisplay()
{
	if (SlotListText)
	{
		SlotListText->SetText(GetContentsSummary());
	}

	if (WeightText)
	{
		WeightText->SetText(GetWeightSummary());

		// over capacity is purely informational today - colour is the whole of the consequence
		WeightText->SetColorAndOpacity(FSlateColor(IsOverWeightCapacity() ? WeightOverCapacityColor : WeightNormalColor));
	}

	RebuildGrid();

	BP_InventoryUpdated();
}

void UInventoryWidget::RebuildGrid()
{
	if (!SlotContainer)
	{
		return;
	}

	SlotContainer->ClearChildren();
	CellWidgets.Reset();
	ItemWidgets.Reset();

	const FIntPoint GridSize = GetGridSize();

	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return;
	}

	// UUniformGridSlot has no span, so a uniform grid cannot host a widget covering a
	// multi-cell footprint at all - the two-layer grid needs a real UGridPanel
	UGridPanel* GridPanel = Cast<UGridPanel>(SlotContainer.Get());

	if (GridPanel)
	{
		// Equal fill across every row and column is what keeps cells uniform, which the
		// screen-position-to-cell conversion relies on. A filled column takes its size purely
		// from the coefficient and ignores its children's desired size, so a long item label
		// cannot stretch the column it sits in.
		for (int32 Column = 0; Column < GridSize.X; ++Column)
		{
			GridPanel->SetColumnFill(Column, 1.0f);
		}

		for (int32 Row = 0; Row < GridSize.Y; ++Row)
		{
			GridPanel->SetRowFill(Row, 1.0f);
		}
	}

	if (GridPanel && CellWidgetClass)
	{
		CellWidgets.Reserve(GridSize.X * GridSize.Y);

		for (int32 Row = 0; Row < GridSize.Y; ++Row)
		{
			for (int32 Column = 0; Column < GridSize.X; ++Column)
			{
				UInventoryCellWidget* CellWidget = CreateWidget<UInventoryCellWidget>(this, CellWidgetClass);

				if (!CellWidget)
				{
					continue;
				}

				CellWidget->SetCell(FIntPoint(Column, Row));

				if (UGridSlot* GridSlot = GridPanel->AddChildToGrid(CellWidget, Row, Column))
				{
					GridSlot->SetLayer(0);
					GridSlot->SetHorizontalAlignment(HAlign_Fill);
					GridSlot->SetVerticalAlignment(VAlign_Fill);
				}

				CellWidgets.Add(CellWidget);
			}
		}
	}
	else if (GridPanel)
	{
		// the cell layer is also what gives the panel its full column and row count - without it
		// the grid collapses to whatever the items happen to span, and every drop then resolves
		// to the wrong cell. Worth saying out loud rather than degrading silently.
		UE_LOG(LogSmoresUI, Warning, TEXT("%s has a grid panel but no CellWidgetClass - the grid will not lay out or accept drops correctly."), *GetName());
	}

	if (!ItemWidgetClass)
	{
		return;
	}

	for (const FInventoryEntry& Entry : GetEntries())
	{
		if (!Entry.IsValidEntry())
		{
			continue;
		}

		UInventoryItemWidget* ItemWidget = CreateWidget<UInventoryItemWidget>(this, ItemWidgetClass);

		if (!ItemWidget)
		{
			continue;
		}

		ItemWidget->SetEntry(BoundInventory.Get(), Entry);

		if (GridPanel)
		{
			const FIntPoint Footprint = Entry.GetFootprint();

			if (UGridSlot* GridSlot = GridPanel->AddChildToGrid(ItemWidget, Entry.AnchorCell.Y, Entry.AnchorCell.X))
			{
				// one widget over the whole footprint, on the layer above the cells - that is
				// what gives a multi-cell item a single border and a centred label instead of
				// a name stranded in its top-left cell
				GridSlot->SetColumnSpan(FMath::Max(1, Footprint.X));
				GridSlot->SetRowSpan(FMath::Max(1, Footprint.Y));
				GridSlot->SetLayer(1);
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		else
		{
			SlotContainer->AddChild(ItemWidget);
		}

		ItemWidgets.Add(ItemWidget);
	}
}

bool UInventoryWidget::ScreenPositionToCell(const FVector2D& ScreenPosition, FIntPoint& OutCell) const
{
	const FIntPoint GridSize = GetGridSize();

	// only a real grid panel lays cells out where this maths expects them; the flat-list
	// fallback has no cell coordinates to recover, so it accepts no drops rather than
	// scattering items across cells it never drew
	const UGridPanel* GridPanel = Cast<UGridPanel>(SlotContainer.Get());

	if (!GridPanel || GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return false;
	}

	const FGeometry& PanelGeometry = GridPanel->GetCachedGeometry();
	const FVector2D PanelSize = PanelGeometry.GetLocalSize();

	if (PanelSize.X <= 0.0f || PanelSize.Y <= 0.0f)
	{
		return false;
	}

	const FVector2D LocalPosition = PanelGeometry.AbsoluteToLocal(ScreenPosition);

	// deliberately unclamped: a pointer outside the grid produces an out-of-bounds cell, which
	// reads as a rejected drop rather than silently snapping to the nearest legal one
	OutCell = FIntPoint(
		FMath::FloorToInt32(LocalPosition.X / (PanelSize.X / GridSize.X)),
		FMath::FloorToInt32(LocalPosition.Y / (PanelSize.Y / GridSize.Y)));

	return true;
}

bool UInventoryWidget::GetDropAnchorCell(const UInventoryDragDropOperation* DragOperation, const FVector2D& ScreenPosition, FIntPoint& OutAnchorCell) const
{
	FIntPoint HoveredCell;

	if (!DragOperation || !ScreenPositionToCell(ScreenPosition, HoveredCell))
	{
		return false;
	}

	// the anchor is where the ghost's top-left sits, not where the cursor is - the player
	// grabbed the item somewhere in the middle of its footprint and expects it to land there
	OutAnchorCell = HoveredCell - DragOperation->GrabOffset;

	return true;
}

bool UInventoryWidget::WouldAcceptDrop(const UInventoryDragDropOperation* DragOperation, FIntPoint AnchorCell) const
{
	if (!DragOperation || !BoundInventory.IsValid() || !DragOperation->SourceInventory.IsValid())
	{
		return false;
	}

	const bool bSameInventory = (DragOperation->SourceInventory.Get() == BoundInventory.Get());

	// mirrors UInventoryComponent::MoveItem: an occupied anchor cell only ever resolves to a
	// merge, and only an entry moving whole within its own grid may reuse its own cells
	const int32 TargetEntryId = BoundInventory->GetEntryIdAtCell(AnchorCell);

	if (TargetEntryId != INDEX_NONE && !(bSameInventory && TargetEntryId == DragOperation->SourceEntryId))
	{
		const FInventoryEntry TargetEntry = BoundInventory->GetEntry(TargetEntryId);

		return TargetEntry.Item.CanStackWith(DragOperation->DraggedItem)
			&& TargetEntry.Item.Quantity < BoundInventory->GetEffectiveMaxStack(TargetEntry.Item.Definition);
	}

	const int32 IgnoreEntryId = bSameInventory ? DragOperation->SourceEntryId : INDEX_NONE;

	return BoundInventory->CanPlaceAt(DragOperation->DraggedItem, AnchorCell, DragOperation->bRotated, IgnoreEntryId);
}

void UInventoryWidget::SetHoveringDrag(UInventoryDragDropOperation* DragOperation)
{
	if (HoveringDrag == DragOperation)
	{
		return;
	}

	ClearDragPreview();

	HoveringDrag = DragOperation;

	if (!HoveringDrag)
	{
		return;
	}

	// a mid-drag rotate changes the preview without the pointer moving, so the grid has to be
	// told rather than waiting for the next drag-over
	HoveringDragRotatedHandle = HoveringDrag->OnRotated.AddUObject(this, &UInventoryWidget::UpdateDragPreview);

	// the drag can also end somewhere that never sends this widget a drag-leave (dropped on
	// another window, cancelled with Escape), which would otherwise strand the highlight
	HoveringDrag->OnDrop.AddUniqueDynamic(this, &UInventoryWidget::HandleDragEnded);
	HoveringDrag->OnDragCancelled.AddUniqueDynamic(this, &UInventoryWidget::HandleDragEnded);
}

void UInventoryWidget::UpdateDragPreview()
{
	if (!HoveringDrag)
	{
		return;
	}

	FIntPoint AnchorCell;
	const bool bHasCell = GetDropAnchorCell(HoveringDrag, LastDragScreenPosition, AnchorCell);

	// the drop is deliberately literal - an item that does not fit is rejected rather than
	// quietly auto-rotated, since turning an item the player did not ask to turn works against
	// the packing this design is built around. The red footprint is what says "press rotate".
	const EInventoryCellHighlight CoveredHighlight = (bHasCell && WouldAcceptDrop(HoveringDrag, AnchorCell))
		? EInventoryCellHighlight::Valid
		: EInventoryCellHighlight::Invalid;

	const FIntPoint Footprint = HoveringDrag->GetFootprint();

	for (UInventoryCellWidget* CellWidget : CellWidgets)
	{
		if (!CellWidget)
		{
			continue;
		}

		const FIntPoint Cell = CellWidget->GetCellCoord();

		const bool bCovered = bHasCell
			&& Cell.X >= AnchorCell.X && Cell.X < AnchorCell.X + Footprint.X
			&& Cell.Y >= AnchorCell.Y && Cell.Y < AnchorCell.Y + Footprint.Y;

		CellWidget->SetHighlight(bCovered ? CoveredHighlight : EInventoryCellHighlight::None);
	}
}

void UInventoryWidget::ClearDragPreview()
{
	if (HoveringDrag)
	{
		HoveringDrag->OnRotated.Remove(HoveringDragRotatedHandle);
		HoveringDrag->OnDrop.RemoveDynamic(this, &UInventoryWidget::HandleDragEnded);
		HoveringDrag->OnDragCancelled.RemoveDynamic(this, &UInventoryWidget::HandleDragEnded);

		HoveringDrag = nullptr;
	}

	HoveringDragRotatedHandle.Reset();

	for (UInventoryCellWidget* CellWidget : CellWidgets)
	{
		if (CellWidget)
		{
			CellWidget->SetHighlight(EInventoryCellHighlight::None);
		}
	}
}

void UInventoryWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation))
	{
		SetHoveringDrag(DragOperation);

		LastDragScreenPosition = InDragDropEvent.GetScreenSpacePosition();

		UpdateDragPreview();
	}
}

void UInventoryWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	if (InOperation == HoveringDrag)
	{
		ClearDragPreview();
	}
}

bool UInventoryWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation))
	{
		SetHoveringDrag(DragOperation);

		LastDragScreenPosition = InDragDropEvent.GetScreenSpacePosition();

		UpdateDragPreview();

		return true;
	}

	return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
}

bool UInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation);

	if (!DragOperation || !BoundInventory.IsValid() || !DragOperation->SourceInventory.IsValid())
	{
		return false;
	}

	FIntPoint AnchorCell;
	const bool bHasCell = GetDropAnchorCell(DragOperation, InDragDropEvent.GetScreenSpacePosition(), AnchorCell);

	ClearDragPreview();

	if (!bHasCell)
	{
		return false;
	}

	// the actual move is shared-world state, owned by the server - this widget cannot mutate
	// inventory contents directly (UInventoryComponent's mutators are authority-only), so
	// dispatch through the owning PlayerController instead. A rejected move simply changes
	// nothing, and the replicated state the UI redraws from is unchanged, so the item visually
	// snaps back.
	if (IInventoryMoveHost* MoveHost = Cast<IInventoryMoveHost>(GetOwningPlayer()))
	{
		// quantity 0 means "the whole stack" - partial-stack drags are a later slice
		MoveHost->Server_MoveInventoryItem(DragOperation->SourceInventory.Get(), DragOperation->SourceEntryId, BoundInventory.Get(), AnchorCell, DragOperation->bRotated, 0);

		return true;
	}

	return false;
}

void UInventoryWidget::NativeDestruct()
{
	ClearDragPreview();
	ClearInventory();

	Super::NativeDestruct();
}

void UInventoryWidget::RequestClose_Implementation()
{
	ClearDragPreview();
	ClearInventory();

	if (IsInViewport())
	{
		RemoveFromParent();
	}

	// after the window is actually gone, so a listener reacting to this sees it that way
	Super::RequestClose_Implementation();
}

#undef LOCTEXT_NAMESPACE
