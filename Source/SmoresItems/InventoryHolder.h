// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InventoryHolder.generated.h"

class USphereComponent;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInventoryHolder : public UInterface
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
class SMORESITEMS_API IInventoryHolder
{
	GENERATED_BODY()

public:

	/** Player-facing name for this holder, used in window titles and the selection label */
	virtual FText GetHolderDisplayName() const = 0;

	/** True if Other is close enough to transfer items with this holder. The one proximity gate
	 *  every transfer context shares - opening a chest, looting a body, collecting a pickup. */
	virtual bool IsInRangeOf(const AActor* Other) const = 0;

	/** Shared body for every implementer's IsInRangeOf: a plain centre-to-centre distance test
	 *  against the holder's own interaction sphere. Each holder sizes that sphere itself, so
	 *  reach stays per-type while the test itself is written once. */
	static bool IsActorWithinSphere(const AActor* HolderActor, const USphereComponent* RangeSphere, const AActor* Other);
};
