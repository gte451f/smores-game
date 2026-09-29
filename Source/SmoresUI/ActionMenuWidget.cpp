// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ActionMenuWidget.h"
#include "TargetActionWidget.h"
#include "StrategyHUDCommands.h"
#include "SmoresUI.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "ActionMenuWidget"

void UActionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// nothing to show until something is right-clicked. Collapsed, not merely hidden, so the click
	// catcher is not hit-testable at all while the menu is closed - otherwise it would eat every
	// click in the game.
	if (!bOpen)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UActionMenuWidget::OpenAt(AActor* Target)
{
	if (!IsValid(Target))
	{
		Close();
		return;
	}

	MenuTarget = Target;
	bOpen = true;

	// forget the last menu's rows, so the first push always draws - and keep the panel hidden until
	// it has, or the last menu's rows would show for a frame at the new spot
	Info = FStrategyTargetInfo();

	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Hidden);
	}

	// the cursor, in this widget's space - mouse positions are viewport pixels, and at any DPI
	// scale other than 1.0 those differ from a canvas slot's units
	OpenPosition = FVector2D::ZeroVector;

	if (const APlayerController* OwningPlayer = GetOwningPlayer())
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

		if (OwningPlayer->GetMousePosition(MouseX, MouseY) && ViewportScale > 0.0f)
		{
			OpenPosition = FVector2D(MouseX, MouseY) / ViewportScale;
		}
	}

	// Visible, so the whole screen catches the next click - see the class comment
	SetVisibility(ESlateVisibility::Visible);

	PlaceAtOpenPosition();
}

void UActionMenuWidget::Close()
{
	if (!bOpen)
	{
		return;
	}

	bOpen = false;
	MenuTarget = nullptr;

	SetVisibility(ESlateVisibility::Collapsed);
}

void UActionMenuWidget::SetTargetInfo(const FStrategyTargetInfo& NewInfo)
{
	if (!bOpen)
	{
		return;
	}

	// the thing picked up, destroyed, or otherwise gone - there is nothing left to act on
	if (!MenuTarget.IsValid() || !NewInfo.HasTarget() || NewInfo.Target.Get() != MenuTarget.Get())
	{
		Close();
		return;
	}

	if (Info.HasTarget() && Info.DrawsIdenticallyTo(NewInfo))
	{
		return;
	}

	Info = NewInfo;

	RefreshMenuDisplay();
}

void UActionMenuWidget::HandleEntryClicked(FName ActionId)
{
	AActor* Target = MenuTarget.Get();

	// closed first: the action may well open a window, and the menu has done its job
	Close();

	if (IStrategyHUDCommands* Commands = Cast<IStrategyHUDCommands>(GetOwningPlayer()))
	{
		// the controller rebuilds the entry and refuses if it has gone grey in the meantime
		Commands->RequestTargetAction(Target, ActionId);
	}
}

void UActionMenuWidget::ResizeEntries(int32 EntryCount)
{
	if (!EntryBox)
	{
		return;
	}

	// shrink first, so the rows that survive keep their place
	while (EntryWidgets.Num() > EntryCount)
	{
		if (UTargetActionWidget* Removed = EntryWidgets.Pop())
		{
			Removed->OnActionClicked.RemoveAll(this);
			Removed->RemoveFromParent();
		}
	}

	if (EntryCount > EntryWidgets.Num() && !EntryWidgetClass)
	{
		// the usual silent single point of failure: without it the menu opens with a header and no rows
		if (!bWarnedNoEntryClass)
		{
			UE_LOG(LogSmoresUI, Warning, TEXT("UActionMenuWidget has no EntryWidgetClass set; the right-click menu will show no actions."));
			bWarnedNoEntryClass = true;
		}

		return;
	}

	while (EntryWidgets.Num() < EntryCount)
	{
		UTargetActionWidget* Widget = CreateWidget<UTargetActionWidget>(this, EntryWidgetClass);

		if (!Widget)
		{
			break;
		}

		Widget->OnActionClicked.AddUObject(this, &UActionMenuWidget::HandleEntryClicked);

		EntryBox->AddChild(Widget);
		EntryWidgets.Add(Widget);
	}
}

void UActionMenuWidget::RefreshMenuDisplay()
{
	if (NameText)
	{
		NameText->SetText(Info.DisplayName);
	}

	if (ClassificationText)
	{
		// the same line the target panel shows - what it is, then how far, when there's someone to measure from
		const FText Line = Info.DistanceMeters < 0.0f
			? Info.Classification
			: FText::Format(LOCTEXT("MenuClassificationLine", "{0} - {1}m"), Info.Classification, FText::AsNumber(FMath::RoundToInt(Info.DistanceMeters)));

		ClassificationText->SetText(Line);
	}

	if (ActorText)
	{
		// the menu names who; the rows say why when it's nobody, so an empty line says nothing wrong
		ActorText->SetText(Info.ActorName.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("MenuActorLine", "{0} would go"), Info.ActorName));
		ActorText->SetVisibility(Info.ActorName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	ResizeEntries(Info.Actions.Num());

	for (int32 Index = 0; Index < EntryWidgets.Num() && Index < Info.Actions.Num(); ++Index)
	{
		if (EntryWidgets[Index])
		{
			EntryWidgets[Index]->SetAction(Info.Actions[Index]);
		}
	}

	BP_UpdateActionMenu();

	// the panel may have changed size - a row greyed with a longer reason, a detail line appearing
	PlaceAtOpenPosition();

	if (MenuPanel)
	{
		MenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UActionMenuWidget::PlaceAtOpenPosition()
{
	UCanvasPanelSlot* CanvasSlot = MenuPanel ? Cast<UCanvasPanelSlot>(MenuPanel->Slot) : nullptr;

	if (!CanvasSlot)
	{
		// not in a canvas: the designer placed it deliberately, so leave it there
		return;
	}

	// measure now rather than next frame, so a menu opened at the screen's edge doesn't draw one
	// frame hanging off it
	MenuPanel->ForceLayoutPrepass();

	const FVector2D PanelSize = MenuPanel->GetDesiredSize();
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = ViewportScale > 0.0f ? UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale : FVector2D::ZeroVector;

	FVector2D Position = OpenPosition + CursorOffset;

	// past the right or bottom edge, open on the other side of the cursor instead - still touching
	// it, so the menu reads as belonging to the click
	if (ViewportSize.X > 0.0f && Position.X + PanelSize.X > ViewportSize.X)
	{
		Position.X = OpenPosition.X - CursorOffset.X - PanelSize.X;
	}

	if (ViewportSize.Y > 0.0f && Position.Y + PanelSize.Y > ViewportSize.Y)
	{
		Position.Y = ViewportSize.Y - PanelSize.Y;
	}

	Position.X = FMath::Max(Position.X, 0.0f);
	Position.Y = FMath::Max(Position.Y, 0.0f);

	CanvasSlot->SetPosition(Position);
}

bool UActionMenuWidget::IsOverMenuPanel(const FVector2D& ScreenPosition) const
{
	return MenuPanel && MenuPanel->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

FReply UActionMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// A row's own button has already taken a click on it, and Slate bubbles from the deepest
	// widget up, so anything reaching here is either the panel's background (a greyed row, the
	// header) or the screen around it. The first keeps the menu open; the second closes it. Both
	// are swallowed, whatever the button: the click that closes the menu does nothing else.
	if (!IsOverMenuPanel(InMouseEvent.GetScreenSpacePosition()))
	{
		Close();
	}

	return FReply::Handled();
}

FReply UActionMenuWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// the second click of a fast pair arrives as its own event, not a press - claiming only the
	// press would let it through to the world. See input-and-keybinds.md.
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

#undef LOCTEXT_NAMESPACE
