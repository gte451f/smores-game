// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WindowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UWindowWidget::SetWindowTitle(const FText& NewTitle)
{
	WindowTitle = NewTitle;

	if (TitleText)
	{
		TitleText->SetText(WindowTitle);
	}
}

void UWindowWidget::RequestClose_Implementation()
{
	// the base doesn't close anything itself - subclasses do that, then call up to here so
	// whoever opened this window finds out it went away
	OnWindowClosed.Broadcast(this);
}

void UWindowWidget::HandleCloseButtonClicked()
{
	RequestClose();
}

void UWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TitleText)
	{
		TitleText->SetText(WindowTitle);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UWindowWidget::HandleCloseButtonClicked);
	}

	// switch this widget from the default full-screen-stretch viewport slot into a fixed-rect
	// "floating window" slot - required before dragging/resizing means anything
	SetPositionInViewport(InitialWindowPosition, false);
	SetDesiredSizeInViewport(InitialWindowSize);
}

void UWindowWidget::NativeDestruct()
{
	bIsDragging = false;
	bIsResizing = false;

	Super::NativeDestruct();
}

bool UWindowWidget::IsUnderWidget(const UWidget* Widget, const FVector2D& ScreenSpacePosition)
{
	return Widget && Widget->GetCachedGeometry().IsUnderLocation(ScreenSpacePosition);
}

FReply UWindowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// swallow *every* button that lands on this window so none of them can fall through to
	// world/selection input underneath - only clicks outside the window are unaffected. This is
	// deliberately button-agnostic rather than a list of special cases: right-click means
	// "equip" over an item widget and "move order" over the world, and any future modifier +
	// button transfer gesture has the same problem. A child widget that wants a button still
	// gets it first - Slate bubbles up from the deepest widget, so this only ever catches what
	// nothing inside the window claimed.
	const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
	UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get();

	// dragging and resizing are left-button gestures; other buttons are consumed and no more
	if (Subsystem && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bAllowResize && IsUnderWidget(ResizeHandle, ScreenPos))
		{
			bIsResizing = true;
		}
		else if (bAllowDrag && IsUnderWidget(TitleBarDragHandle, ScreenPos))
		{
			bIsDragging = true;
		}

		if (bIsDragging || bIsResizing)
		{
			GestureStartScreenPos = ScreenPos;
			GestureStartSlot = Subsystem->GetWidgetSlot(this);

			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	// not a drag/resize - still consume the click so it can't fall through to the world,
	// but don't capture the mouse since there's no gesture to track
	return FReply::Handled();
}

FReply UWindowWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// The second click of a rapid pair is NOT a button-down as far as Slate is concerned: Windows
	// sends WM_xBUTTONDBLCLK instead of WM_xBUTTONDOWN, which arrives as OnMouseButtonDoubleClick -
	// a separate event with its own routing. Handling only the press therefore leaves a hole that
	// every button falls through: a double right-click on an inventory window swallows the first
	// click and lets the second reach the viewport, where it registers as a press and issues a move
	// order on release. Treating it exactly like a press closes the hole and makes a fast second
	// click mean what a slow one does.
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWindowWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsDragging && !bIsResizing)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	const FVector2D DeltaSlotSpace = (InMouseEvent.GetScreenSpacePosition() - GestureStartScreenPos) / Scale;

	if (bIsResizing)
	{
		FVector2D NewSize = FVector2D(GestureStartSlot.Offsets.Right, GestureStartSlot.Offsets.Bottom) + DeltaSlotSpace;
		NewSize.X = FMath::Max(NewSize.X, MinWindowSize.X);
		NewSize.Y = FMath::Max(NewSize.Y, MinWindowSize.Y);

		SetDesiredSizeInViewport(NewSize);
	}
	else if (bIsDragging)
	{
		const FVector2D NewPosition = FVector2D(GestureStartSlot.Offsets.Left, GestureStartSlot.Offsets.Top) + DeltaSlotSpace;

		SetPositionInViewport(NewPosition, false);
	}

	return FReply::Handled();
}

FReply UWindowWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging || bIsResizing)
	{
		bIsDragging = false;
		bIsResizing = false;

		return FReply::Handled().ReleaseMouseCapture();
	}

	// deliberately *not* swallowed, unlike the press. Enhanced Input never saw the press this
	// window ate, so an action bound on release (most of them are) has nothing to complete and
	// stays silent anyway - whereas eating a release whose press the viewport *did* see would
	// leave that button stuck down in UPlayerInput for good.
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
