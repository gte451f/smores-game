// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StrategyTargetInfo.h"

class AActor;
class AStrategyUnit;
class UTraderComponent;

/**
 *  The rules behind the target panel's action row and the right-click menu: what each kind of
 *  thing offers, when an offer is greyed out and why, and which squad member would carry it out.
 *
 *  **One set of rules, two views.** The panel and the menu are both built by BuildTargetInfo, the
 *  controller re-derives an entry through it before acting on a click, and the server checks an
 *  order through FindActionFor - the same function underneath - both when the order is issued
 *  and again on arrival. Nothing that offers an action is allowed to have rules of its own.
 *
 *  **Why it isn't on the controller.** It used to be (BuildTargetInfo was a static there), and
 *  AStrategyPlayerController is already the project's junk drawer - see
 *  unreal-module-organization.md. Everything here is static and takes the selection and the squad
 *  as plain arrays, so a test world with no controller in it can ask any question the menu can.
 *
 *  **Two entry points, because the server has no selection.** ControlledUnits exists only on the
 *  owning client. So "who acts" (ResolveActor) is a client-side question answered from the
 *  selection, and "may this actor do this to that" (FindActionFor, ValidateActionOrder) is asked
 *  on both sides with the actor the client named.
 *
 *  The table this implements, row for row, is in game-systems/action-menu.md.
 */
struct FStrategyTargetActions
{
	//~ The views

	/**
	 *  Everything the target panel and the menu draw about Target: name, kind, distance, health,
	 *  who would go, and every action this kind of thing offers - enabled or greyed out with its
	 *  reason. Selection is this player's selected units; Squad is every unit of theirs, selected or
	 *  not (it tells your own squad from another player's, and finds someone to act when nothing is
	 *  selected). Client-side; reads replicated state only.
	 */
	static FStrategyTargetInfo BuildTargetInfo(const AActor* Target, const TArray<AStrategyUnit*>& Selection, const TArray<AStrategyUnit*>& Squad);

	/**
	 *  The entry ActionId would be for Actor on Target, or false if Target doesn't offer that action
	 *  at all. Actor may be null, which disables everything but Examine with NoOneSelected. The
	 *  server's question, on issue and on arrival - answered by exactly the code the menu drew from.
	 */
	static bool FindActionFor(const AActor* Target, const AStrategyUnit* Actor, FName ActionId, const TArray<AStrategyUnit*>& Squad, FTargetAction& OutAction);

	//~ Who acts

	/**
	 *  Which squad member would carry out ActionId on Target. None-named ActionId asks the general
	 *  question, which is what the menu's header shows.
	 *
	 *  1. Candidates are the selected units able to act (not Downed or Dead), excluding the target.
	 *  2. The actor is the candidate nearest the target, in a straight line.
	 *  3. Heal alone may fall back to the target itself - selected, and nobody else able - who
	 *     treats their own wounds.
	 *  4. With nothing selected at all, a squad member already within reach of the target acts, so
	 *     a pawn standing at a chest can still open it. Otherwise nobody does.
	 *
	 *  Attack is not answered here - it uses every candidate, see GetAttackers.
	 */
	static AStrategyUnit* ResolveActor(const AActor* Target, FName ActionId, const TArray<AStrategyUnit*>& Selection, const TArray<AStrategyUnit*>& Squad);

	/** Every selected unit that would join an attack on Target: able to act, and not the target itself */
	static TArray<AStrategyUnit*> GetAttackers(const AActor* Target, const TArray<AStrategyUnit*>& Selection);

	/** True if Unit is on its feet and could carry something out */
	static bool CanAct(const AStrategyUnit* Unit);

	//~ Orders

	/**
	 *  The server's check on an order a client asked for: Actor is one of this player's own squad
	 *  and able, ActionId is something a unit walks over to do (not Attack, not Examine), and the
	 *  entry is on offer and enabled for Actor right now. False fills OutReason with what to tell
	 *  the player - None when the request was simply not a legal one to make.
	 */
	static bool ValidateActionOrder(const AStrategyUnit* Actor, const AActor* Target, FName ActionId, const TArray<AStrategyUnit*>& Squad, ESmoresRefusalReason& OutReason);

	/** True for an action a squad member walks over to carry out. Attack has combat's own approach, and Examine walks nowhere. */
	static bool IsOrderAction(FName ActionId);

	/**
	 *  True for the actions with no system behind them yet: Heal, Kidnap, Pickpocket, Knock out. They are
	 *  offered under their real rules and walked over to like any other; on arrival the feed says
	 *  they aren't built yet. When the real system lands, it takes its action off this list.
	 */
	static bool IsPlaceholderAction(FName ActionId);

	//~ The odds

	/**
	 *  **PLACEHOLDER, owned by the future stealth and capture systems.** Actor's chance, 0-1, of
	 *  pulling off ActionId on Target, or negative for an action that has no odds.
	 *
	 *  Pickpocket is Agility against the mark's Perception, Kidnap and Knock out Strength against Endurance:
	 *  clamp(0.5 + (edge x 0.03), 5%, 95%). What must survive the real version is the shape - the
	 *  same function produces the displayed chance and (once there is one) the roll, so the menu can
	 *  never promise odds the server doesn't use; and it reads replicated attributes only, so it
	 *  answers the same on a client.
	 */
	static float GetPlaceholderSuccessChance(FName ActionId, const AStrategyUnit* Actor, const AActor* Target);

	/** A chance as the player reads it - "62% chance". The one place to make it coarser, should working the odds backwards ever matter. */
	static FText FormatSuccessChance(float Chance);

	//~ The gating predicates - moved here from the controller, each still the only place its rule is written

	/** True if Unit is an NPC that can be looted - not one of any player's pawns, and Downed or Dead. The two are treated identically. */
	static bool IsLootableNPC(const AStrategyUnit* Unit);

	/**
	 *  True if Unit is an NPC the player may deal with right now - not one of any player's pawns,
	 *  on its feet, and not hostile. "Never trade with (or talk to) someone trying to kill you" is
	 *  this predicate; dialog extends it rather than adding a check of its own.
	 */
	static bool IsInteractableNPC(const AStrategyUnit* Unit);

	/**
	 *  Unit's wares if it is interactable *and* keeps a shop, else null. Right for "may we trade
	 *  now?", wrong for "is this a trader?" - a hostile trader answers null here. BuildTargetInfo
	 *  asks the second question of the component directly, so Trade greys out on a hostile trader
	 *  rather than vanishing.
	 */
	static UTraderComponent* GetTraderStock(const AStrategyUnit* Unit);

	/** True if any of Units is within Target's own reach. An actor that isn't interactable has no reach, so is never in range. */
	static bool IsInRangeOfUnits(const AActor* Target, const TArray<AStrategyUnit*>& Units);
};
