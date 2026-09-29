// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresInteractable.h"
#include "InventoryHolder.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInventoryHolder : public USmoresInteractable
{
	GENERATED_BODY()
};

/**
 *  Anything the player can transfer items to or from: a pawn's pack, a container, a loose
 *  world pickup - and later a storefront, a live NPC being traded with, or a corpse.
 *
 *  Every transfer context in the game shares exactly one gating rule, physical proximity, and
 *  one piece of presentation, a player-facing name for whatever is being opened. Before this
 *  interface existed each holder type carried its own private copy of both: three identical
 *  distance tests named IsUnitInRange (one of which took a narrower parameter type than the
 *  other two) and three differently-named display-name getters. This is those six methods
 *  collapsed into two.
 *
 *  **It derives from ISmoresInteractable**, which is where reach (IsInRangeOf) and the shared
 *  sphere test now live - a door is something the squad walks up to and acts on without being an
 *  inventory holder, so reach had to move up a layer for the two to share it. Every holder is
 *  therefore interactable for free: GetHolderDisplayName answers GetInteractionDisplayName, below.
 *
 *  Deliberately no GetInventory(). AWorldItem holds a single FInventoryItem and no
 *  UInventoryComponent at all - a pickup is one item on a light actor, not a grid - so a grid
 *  accessor here would either be unimplementable by one of the three implementers or would
 *  have to return null from it, leaving every caller to remember a null check someone
 *  eventually won't. Code that needs a grid casts to the concrete holder type instead; only
 *  two types have one, and there is no duplication there to collapse.
 *
 *  Nor is the player's click radius here. How precisely the player has to aim at a thing is a
 *  property of the input gesture, not of the thing - it lives on AStrategyPlayerController
 *  next to the other input tuning, per holder type (ContainerSelectionRadius vs. the tighter
 *  WorldItemSelectionRadius), and is passed into the holder-generic finders.
 */
class SMORESITEMS_API IInventoryHolder : public ISmoresInteractable
{
	GENERATED_BODY()

public:

	/** Player-facing name for this holder, used in window titles and the selection label */
	virtual FText GetHolderDisplayName() const = 0;

	//~ Begin ISmoresInteractable interface

	/** A holder's interaction name is its holder name - one name, not two that could disagree */
	virtual FText GetInteractionDisplayName() const override { return GetHolderDisplayName(); }

	//~ End ISmoresInteractable interface
};
