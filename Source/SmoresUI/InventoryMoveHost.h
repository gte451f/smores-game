// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryMoveHost.generated.h"

class UInventoryComponent;

UINTERFACE(MinimalAPI)
class UInventoryMoveHost : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by whichever PlayerController can authoritatively move/swap inventory items on behalf of a drag-drop UI. */
class SMORESUI_API IInventoryMoveHost
{
	GENERATED_BODY()

public:

	/** Server-side entry point for an inventory drag-drop move/swap */
	virtual void Server_MoveInventoryItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* DestInventory, int32 DestIndex) = 0;
};
