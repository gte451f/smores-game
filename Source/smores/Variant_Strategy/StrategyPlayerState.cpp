// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyPlayerState.h"
#include "WalletComponent.h"
#include "PlayerStandingComponent.h"

AStrategyPlayerState::AStrategyPlayerState()
{
	Wallet = CreateDefaultSubobject<UWalletComponent>(TEXT("Wallet"));
	Standing = CreateDefaultSubobject<UPlayerStandingComponent>(TEXT("Standing"));
}
