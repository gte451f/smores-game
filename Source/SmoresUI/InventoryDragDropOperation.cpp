// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "InventoryDragDropOperation.h"
#include "InventoryItemWidget.h"
#include "Slate/UMGDragDropOp.h"
#include "Framework/Application/SlateApplication.h"

UInventoryDragDropOperation* UInventoryDragDropOperation::GetActiveDrag()
{
	if (!FSlateApplication::IsInitialized())
	{
		return nullptr;
	}

	const TSharedPtr<FDragDropOperation> SlateOperation = FSlateApplication::Get().GetDragDroppingContent();

	// every UMG drag is wrapped in an FUMGDragDropOp; anything else in flight (an editor drag,
	// a native Slate one) simply is not ours
	if (!SlateOperation.IsValid() || !SlateOperation->IsOfType<FUMGDragDropOp>())
	{
		return nullptr;
	}

	return Cast<UInventoryDragDropOperation>(StaticCastSharedPtr<FUMGDragDropOp>(SlateOperation)->GetOperation());
}

void UInventoryDragDropOperation::ToggleRotation()
{
	const FIntPoint Footprint = GetFootprint();

	// a square turns into itself - leave the flag alone rather than storing a rotation that
	// means nothing
	if (Footprint.X == Footprint.Y)
	{
		return;
	}

	bRotated = !bRotated;

	// the grab offset is expressed in footprint cells, so it has to transpose along with the
	// footprint; without this the ghost and the drop would disagree after a rotate
	GrabOffset = FIntPoint(GrabOffset.Y, GrabOffset.X);

	ApplyPreview();

	OnRotated.Broadcast();
}

void UInventoryDragDropOperation::ApplyPreview()
{
	const FIntPoint Footprint = GetFootprint();

	// Offset is a fraction of the decorator's own size (see UDragDropOperation::Offset), so
	// shifting it back by GrabOffset/Footprint puts the grabbed cell under the cursor whatever
	// size the window is currently at
	Offset = FVector2D(
		Footprint.X > 0 ? -static_cast<float>(GrabOffset.X) / Footprint.X : 0.0f,
		Footprint.Y > 0 ? -static_cast<float>(GrabOffset.Y) / Footprint.Y : 0.0f);

	if (UInventoryItemWidget* DecoratorItem = GetDecoratorItem())
	{
		DecoratorItem->SetPreviewOrientation(bRotated, CellSize);
	}
}

UInventoryItemWidget* UInventoryDragDropOperation::GetDecoratorItem() const
{
	return Cast<UInventoryItemWidget>(DefaultDragVisual);
}
