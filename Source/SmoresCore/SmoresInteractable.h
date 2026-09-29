// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmoresInteractable.generated.h"

class AActor;
class USphereComponent;

/**
 *  Exported in full rather than MinimalAPI, unlike the project's other interfaces, because
 *  UInventoryHolder in SmoresItems derives from it - a UClass in another module that inherits
 *  this one needs its constructor and registration exported, which MinimalAPI leaves out.
 */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class SMORESCORE_API USmoresInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Anything the squad can walk up to and do something to: a person, a creature, a body, a
 *  container, a door, an item lying on the ground.
 *
 *  **Why it exists.** The right-click menu and the walk-over-to-act order treat all of those the
 *  same way - name it, decide whether a squad member is close enough to act, describe it when
 *  examined - and they span three modules (units in SmoresCharacters, containers and items and
 *  doors in SmoresItems). Reach was already written once, in IInventoryHolder; a door is not an
 *  inventory holder, so reach moved up to here, where every one of them can share it.
 *
 *  IInventoryHolder derives from this, so every holder is interactable without restating any of
 *  it: a holder's GetHolderDisplayName answers GetInteractionDisplayName, and IsInRangeOf is the
 *  same method it always was.
 *
 *  **Never a number about the target** comes out of here. GetExamineText is words only - see
 *  game-design's player-interface.md, "numbers about your own squad are known; numbers about the
 *  world are not".
 */
class SMORESCORE_API ISmoresInteractable
{
	GENERATED_BODY()

public:

	/** Player-facing name for this thing - the target panel's title, the menu's title, the Examine window's title */
	virtual FText GetInteractionDisplayName() const = 0;

	/** True if Other is close enough to act on this thing - open it, loot it, talk to it, pick it up */
	virtual bool IsInRangeOf(const AActor* Other) const = 0;

	/**
	 *  What the squad can see of this thing, in words, for the Examine window. Empty is allowed:
	 *  the window then says there is nothing remarkable about it. Never a stat, a skill or a
	 *  health figure.
	 */
	virtual FText GetExamineText() const = 0;

	/**
	 *  Shared body for every implementer's IsInRangeOf: a plain centre-to-centre distance test
	 *  against the thing's own interaction sphere. Each type sizes that sphere itself, so reach
	 *  stays per-type while the test itself is written once.
	 */
	static bool IsActorWithinSphere(const AActor* InteractableActor, const USphereComponent* RangeSphere, const AActor* Other);
};
