// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemDefinition.h"
#include "InventoryMoveHost.generated.h"

class UInventoryComponent;
class UEquipmentComponent;

UINTERFACE(MinimalAPI)
class UInventoryMoveHost : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by whichever PlayerController can authoritatively move inventory items, and equip or
 *  unequip them, on behalf of an inventory/paperdoll UI. */
class SMORESUI_API IInventoryMoveHost
{
	GENERATED_BODY()

public:

	/**
	 *  Server-side entry point for an inventory drag-drop move. DestCell is the grid cell the
	 *  item's top-left corner lands on and bRotated the orientation it lands at; a Quantity of
	 *  0 or less moves the whole stack.
	 */
	virtual void Server_MoveInventoryItem(UInventoryComponent* SourceInventory, int32 EntryId, UInventoryComponent* DestInventory, FIntPoint DestCell, bool bRotated, int32 Quantity) = 0;

	/**
	 *  Server-side entry point for wearing a placed grid entry. A Slot of EEquipSlot::None means
	 *  "whichever slot the item belongs in", which is what right-click-to-equip sends; a drop on a
	 *  specific paperdoll slot names that slot instead.
	 */
	virtual void Server_EquipItem(UInventoryComponent* SourceInventory, int32 EntryId, UEquipmentComponent* Equipment, EEquipSlot Slot) = 0;

	/** Server-side entry point for taking a worn item off and putting it back in DestInventory */
	virtual void Server_UnequipItem(UEquipmentComponent* Equipment, EEquipSlot Slot, UInventoryComponent* DestInventory) = 0;
};
