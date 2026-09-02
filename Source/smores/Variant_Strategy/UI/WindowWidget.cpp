// Copyright Epic Games, Inc. All Rights Reserved.


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
	// base does nothing - subclasses override to actually close themselves
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
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// swallow every left-click that lands on this window so it can't fall through to
	// world/selection input underneath - only clicks outside the window are unaffected
	const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
	UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get();

	if (!Subsystem)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

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

	// not a drag/resize - still consume the click so it can't fall through to the world,
	// but don't capture the mouse since there's no gesture to track
	return FReply::Handled();
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

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
