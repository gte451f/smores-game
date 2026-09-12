// Copyright Epic Games, Inc. All Rights Reserved.


#include "InventoryDragDropOperation.h"
#include "InventoryItemWidget.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

/**
 *  Slate input pre-processor that watches for the rotate key while a drag is in flight. See
 *  UInventoryDragDropOperation::BeginRotateInput for why a widget key handler will not do.
 */
class FInventoryDragRotateProcessor : public IInputProcessor
{
public:

	FInventoryDragRotateProcessor(UInventoryDragDropOperation* InOwner, const FKey& InRotateKey)
		: Owner(InOwner)
		, RotateKey(InRotateKey)
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		// auto-repeat would spin the item while the key is held down; one turn per press
		if (InKeyEvent.IsRepeat() || InKeyEvent.GetKey() != RotateKey)
		{
			return false;
		}

		if (UInventoryDragDropOperation* DragOperation = Owner.Get())
		{
			DragOperation->ToggleRotation();

			// swallowed, so the rotate key can double as a gameplay binding without firing it
			// every time the player turns an item
			return true;
		}

		return false;
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("InventoryDragRotate"); }

private:

	TWeakObjectPtr<UInventoryDragDropOperation> Owner;

	FKey RotateKey;
};

void UInventoryDragDropOperation::BeginRotateInput(const FKey& InRotateKey)
{
	EndRotateInput();

	if (!InRotateKey.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}

	RotateProcessor = MakeShared<FInventoryDragRotateProcessor>(this, InRotateKey);

	FSlateApplication::Get().RegisterInputPreProcessor(RotateProcessor);
}

void UInventoryDragDropOperation::EndRotateInput()
{
	if (!RotateProcessor.IsValid())
	{
		return;
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(RotateProcessor);
	}

	RotateProcessor.Reset();
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

void UInventoryDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	EndRotateInput();

	Super::Drop_Implementation(PointerEvent);
}

void UInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	EndRotateInput();

	Super::DragCancelled_Implementation(PointerEvent);
}

void UInventoryDragDropOperation::BeginDestroy()
{
	// belt and braces - a drag that ends some way this class did not anticipate still cannot
	// leave a pre-processor holding a stale pointer inside Slate
	EndRotateInput();

	Super::BeginDestroy();
}
