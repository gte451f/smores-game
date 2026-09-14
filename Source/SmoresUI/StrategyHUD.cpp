// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyHUD.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "StrategySelectionHost.h"
#include "WalletComponent.h"
#include "StrategyUI.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void AStrategyHUD::BeginPlay()
{
	Super::BeginPlay();

	// spawn the UI widget
	UIWidget = CreateWidget<UStrategyUI>(GetOwningPlayerController(), UIWidgetClass);
	check(UIWidget);

	// add the UI widget to the screen
	UIWidget->AddToViewport(0);
}

void AStrategyHUD::DragSelectUpdate(FVector2D Start, FVector2D WidthAndHeight, FVector2D CurrentPosition, bool bDraw)
{
	// copy the selection box data
	bDrawBox = bDraw;
	BoxStart = Start;
	BoxSize = WidthAndHeight;
	BoxCurrentPosition = CurrentPosition;

}

void AStrategyHUD::DrawHUD()
{
	// draw all debug information, etc.
	Super::DrawHUD();

	// ensure we have a valid player controller that hosts unit selection
	APlayerController* PC = GetOwningPlayerController();
	IStrategySelectionHost* SelectionHost = Cast<IStrategySelectionHost>(PC);

	if (PC && SelectionHost)
	{
		// draw the selection box
		if (bDrawBox)
		{
			DrawRect(SelectionBoxColor, BoxStart.X, BoxStart.Y, BoxSize.X, BoxSize.Y);

			// get all the player-controlled units in the selection box (NPC units are not selectable)
			TArray<AStrategyPlayerUnit*> BoxedPlayerUnits;
			GetActorsInSelectionRectangle(BoxStart, BoxCurrentPosition, BoxedPlayerUnits, true);

			// GetActorsInSelectionRectangle doesn't clip against the near plane, so an actor behind
			// the camera can still satisfy the 2D screen-rect test once the camera can pitch/rotate freely
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);

			BoxedPlayerUnits.RemoveAll([&CamLoc, &CamRot, PC](AStrategyPlayerUnit* Unit)
			{
				// only this controller's own squad is drag-selectable - see AStrategyPlayerUnit::ClaimForController
				return !IsValid(Unit) || Unit->GetOwningController() != PC
					|| FVector::DotProduct(CamRot.Vector(), Unit->GetActorLocation() - CamLoc) <= 0.0f;
			});

			// widen to the base type expected by the player controller
			TArray<AStrategyUnit*> BoxedUnits;
			BoxedUnits.Reserve(BoxedPlayerUnits.Num());
			for (AStrategyPlayerUnit* CurrentUnit : BoxedPlayerUnits)
			{
				BoxedUnits.Add(CurrentUnit);
			}

			// update the unit selection on the player controller
			SelectionHost->DragSelectUnits(BoxedUnits);
		}

		// get the currently selected units
		TArray<AStrategyUnit*> SelectedUnits = SelectionHost->GetSelectedUnits();

		// update the selection count on the UI widget
		if (UIWidget)
		{
			UIWidget->SetSelectedUnitsCount(SelectedUnits.Num());
			UIWidget->SetSelectionTargetLabel(SelectionHost->GetSelectionTargetLabel());

			// the quick-access resource readout, read straight off this player's own wallet -
			// APlayerState is an engine type, so no narrow interface into `smores` is needed for it
			if (const UWalletComponent* Wallet = GetWallet())
			{
				UIWidget->SetGold(Wallet->GetGold());
			}
		}

		// process each selected unit
		for (AStrategyUnit* CurrentUnit : SelectedUnits)
		{
			if (IsValid(CurrentUnit))
			{
				// project the unit's location to screen coordinates
				FVector2D ScreenCoords;

				if (PC->ProjectWorldLocationToScreen(CurrentUnit->GetActorLocation(), ScreenCoords, true))
				{
					// draw a selection string near the unit
					const FString SelectionString = "Selected";
					DrawText(SelectionString, FColor::White, ScreenCoords.X - 25.0f, ScreenCoords.Y + 25.0f, nullptr, 1.5f);
				}
			}
			
		}
	}

}

UWalletComponent* AStrategyHUD::GetWallet()
{
	if (UWalletComponent* Wallet = CachedWallet.Get())
	{
		return Wallet;
	}

	// keyed off this HUD's own player, never a global - there is no single "the" player in a
	// co-op session. The player state can arrive late on a client, so a miss is normal early on
	// and simply retries next frame rather than being cached as a negative.
	const APlayerController* PC = GetOwningPlayerController();
	const APlayerState* OwningPlayerState = PC ? PC->PlayerState : nullptr;

	if (!OwningPlayerState)
	{
		return nullptr;
	}

	UWalletComponent* Wallet = OwningPlayerState->FindComponentByClass<UWalletComponent>();

	CachedWallet = Wallet;

	return Wallet;
}
