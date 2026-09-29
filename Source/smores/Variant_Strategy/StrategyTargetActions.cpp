// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyTargetActions.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "StrategyContainer.h"
#include "WorldItem.h"
#include "WorldDoor.h"
#include "HealthComponent.h"
#include "TraderComponent.h"
#include "SmoresInteractable.h"

#define LOCTEXT_NAMESPACE "StrategyTargetActions"

/*
 *  The helpers below are uniquely prefixed rather than living in an anonymous namespace: unity
 *  builds concatenate .cpp files, and two anonymous namespaces merging would make a shared name a
 *  redefinition. Same convention as the test files - see testing.md.
 */

/** An entry as the target's own state allows it, before anyone is chosen to carry it out */
static FTargetAction StrategyTargetActions_MakeEntry(FName Id, const FText& Label, const FText& KeyHint, bool bEnabled, ESmoresRefusalReason DisabledReason)
{
	FTargetAction Action;
	Action.Id = Id;
	Action.Label = Label;
	Action.KeyHint = KeyHint;
	Action.bEnabled = bEnabled;
	Action.DisabledReason = bEnabled ? ESmoresRefusalReason::None : DisabledReason;

	return Action;
}

/** True if Unit has taken any harm - the Heal rule's "not at full health" */
static bool StrategyTargetActions_IsHurt(const AStrategyUnit* Unit)
{
	const UHealthComponent* Health = Unit ? Unit->GetHealth() : nullptr;

	return Health && Health->GetHealth() < Health->MaxHealth;
}

/**
 *  Every action Target offers, in the rules table's order, each enabled or greyed out by the
 *  target's own state alone - hostility, health, a door's position. Who would carry them out is
 *  applied afterwards, so the same list serves the client's menu and the server's check.
 *
 *  The table this is, row for row, lives in game-systems/action-menu.md. Change both together.
 */
static void StrategyTargetActions_BuildOffered(const AActor* Target, const TArray<AStrategyUnit*>& Squad, TArray<FTargetAction>& OutActions)
{
	const FText KeyO = LOCTEXT("KeyO", "O");
	const FText KeyT = LOCTEXT("KeyT", "T");
	const FText KeyH = LOCTEXT("KeyH", "H");

	const FTargetAction Examine = StrategyTargetActions_MakeEntry(StrategyTargetAction::Examine(), LOCTEXT("ActionExamine", "Examine"), FText::GetEmpty(), true, ESmoresRefusalReason::None);

	if (Cast<AStrategyContainer>(Target))
	{
		OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Loot(), LOCTEXT("ActionLoot", "Loot"), KeyO, true, ESmoresRefusalReason::None));
		OutActions.Add(Examine);
		return;
	}

	if (const AWorldDoor* Door = Cast<AWorldDoor>(Target))
	{
		// one entry, never both: the one that would change it
		OutActions.Add(Door->IsOpen()
			? StrategyTargetActions_MakeEntry(StrategyTargetAction::Close(), LOCTEXT("ActionClose", "Close"), KeyO, true, ESmoresRefusalReason::None)
			: StrategyTargetActions_MakeEntry(StrategyTargetAction::Open(), LOCTEXT("ActionOpen", "Open"), KeyO, true, ESmoresRefusalReason::None));
		OutActions.Add(Examine);
		return;
	}

	if (const AWorldItem* Item = Cast<AWorldItem>(Target))
	{
		// an item holding nothing is about to be destroyed - look at it by all means
		if (!Item->GetItem().IsEmpty())
		{
			OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::PickUp(), LOCTEXT("ActionPickUp", "Pick up"), FText::GetEmpty(), true, ESmoresRefusalReason::None));
		}

		OutActions.Add(Examine);
		return;
	}

	const AStrategyUnit* Unit = Cast<AStrategyUnit>(Target);

	if (!Unit)
	{
		// something interactable that is none of the above - a name and a look, nothing wrong
		OutActions.Add(Examine);
		return;
	}

	const FTargetAction Heal = StrategyTargetActions_MakeEntry(StrategyTargetAction::Heal(), LOCTEXT("ActionHeal", "Heal"), FText::GetEmpty(),
		StrategyTargetActions_IsHurt(Unit), ESmoresRefusalReason::None);

	// AStrategyPlayerUnit derives from AStrategyUnit, so it has to be checked first
	if (Cast<AStrategyPlayerUnit>(Unit))
	{
		// your own squad can be patched up; another player's squad is somebody else's to command,
		// and giving or trading between players is not in this round
		if (Squad.Contains(Unit))
		{
			OutActions.Add(Heal);
		}

		OutActions.Add(Examine);
		return;
	}

	const bool bHostile = Unit->IsAggressive();

	if (!Unit->IsPerson())
	{
		// a creature is fought, and butchered once it's down - never spoken to or robbed
		if (FStrategyTargetActions::IsLootableNPC(Unit))
		{
			OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Loot(), LOCTEXT("ActionLoot", "Loot"), KeyO, true, ESmoresRefusalReason::None));
		}
		else
		{
			OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Attack(), LOCTEXT("ActionAttack", "Attack"), KeyH, !bHostile, ESmoresRefusalReason::None));
		}

		OutActions.Add(Examine);
		return;
	}

	if (Unit->IsDead())
	{
		OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Loot(), LOCTEXT("ActionLoot", "Loot"), KeyO, true, ESmoresRefusalReason::None));
		OutActions.Add(Examine);
		return;
	}

	if (Unit->IsDowned())
	{
		// someone down can be gone through, patched up, or carried off - Downed and Dead loot the same
		OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Loot(), LOCTEXT("ActionLoot", "Loot"), KeyO, true, ESmoresRefusalReason::None));
		OutActions.Add(Heal);
		OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Kidnap(), LOCTEXT("ActionKidnap", "Kidnap"), FText::GetEmpty(), true, ESmoresRefusalReason::None));
		OutActions.Add(Examine);
		return;
	}

	// A person on their feet. Talk and Trade are offered whatever their mood - greyed out because
	// they are trying to kill you is the rule made visible, where a missing button teaches nothing.
	const bool bInteractable = FStrategyTargetActions::IsInteractableNPC(Unit);

	OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Talk(), LOCTEXT("ActionTalk", "Talk"), KeyT, bInteractable, ESmoresRefusalReason::NotInteractable));

	// listed on the shop being there at all, not on GetTraderStock - which answers null for a hostile
	// trader and would make the entry vanish instead of greying out
	if (Unit->FindComponentByClass<UTraderComponent>())
	{
		OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Trade(), LOCTEXT("ActionTrade", "Trade"), FText::GetEmpty(), bInteractable, ESmoresRefusalReason::NotInteractable));
	}

	// greyed with no reason on a hostile: the real reason (they're watching you) belongs to the
	// awareness system the stealth work brings, and a made-up one now would be wrong later
	OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Pickpocket(), LOCTEXT("ActionPickpocket", "Pickpocket"), FText::GetEmpty(), !bHostile, ESmoresRefusalReason::None));

	// the same reasoning: a takedown wants a mark that isn't already fighting you, and the real
	// reason (they've seen you coming) belongs to the awareness system
	OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::KnockOut(), LOCTEXT("ActionKnockOut", "Knock out"), FText::GetEmpty(), !bHostile, ESmoresRefusalReason::None));

	OutActions.Add(Heal);

	// no reason on a hostile either: "you are already doing this" is not a refusal
	OutActions.Add(StrategyTargetActions_MakeEntry(StrategyTargetAction::Attack(), LOCTEXT("ActionAttack", "Attack"), KeyH, !bHostile, ESmoresRefusalReason::None));

	OutActions.Add(Examine);
}

/**
 *  Fills in who would carry Action out: disables it with NoOneSelected when nobody would, names
 *  them, and writes the per-actor detail line. OddsActor is whose numbers the odds come from - the
 *  one actor, or null for an attack by several. HeaderActorName is the menu's "who would go", so a
 *  row only repeats a name when it differs from that one.
 */
static void StrategyTargetActions_ApplyActor(FTargetAction& Action, bool bHasActor, const AStrategyUnit* OddsActor, const FText& ActorName, const AActor* Target, const FText& HeaderActorName)
{
	// looking costs nothing and needs nobody
	if (Action.Id == StrategyTargetAction::Examine())
	{
		return;
	}

	if (!bHasActor)
	{
		// the target's own reason wins when there is one: selecting somebody wouldn't make a hostile
		// talk, and "No one selected" would suggest it might
		if (Action.bEnabled)
		{
			Action.bEnabled = false;
			Action.DisabledReason = ESmoresRefusalReason::NoOneSelected;
		}

		return;
	}

	Action.ActorName = ActorName;

	TArray<FText> DetailParts;

	if (!ActorName.EqualTo(HeaderActorName))
	{
		DetailParts.Add(ActorName);
	}

	// odds only while the action can actually be attempted - a greyed-out pickpocket promises nothing
	const float Chance = Action.bEnabled ? FStrategyTargetActions::GetPlaceholderSuccessChance(Action.Id, OddsActor, Target) : -1.0f;

	if (Chance >= 0.0f)
	{
		DetailParts.Add(FStrategyTargetActions::FormatSuccessChance(Chance));
	}

	Action.Detail = FText::Join(LOCTEXT("DetailSeparator", " - "), DetailParts);
}

/** What a target is and how it feels about the player, for the line under its name */
static FText StrategyTargetActions_Classify(const AActor* Target, const TArray<AStrategyUnit*>& Squad)
{
	if (Cast<AStrategyContainer>(Target))
	{
		return LOCTEXT("ClassContainer", "CONTAINER");
	}

	if (const AWorldDoor* Door = Cast<AWorldDoor>(Target))
	{
		return Door->IsOpen() ? LOCTEXT("ClassDoorOpen", "DOOR - OPEN") : LOCTEXT("ClassDoorShut", "DOOR - SHUT");
	}

	if (Cast<AWorldItem>(Target))
	{
		return LOCTEXT("ClassItem", "ITEM");
	}

	const AStrategyUnit* Unit = Cast<AStrategyUnit>(Target);

	if (!Unit)
	{
		return FText::GetEmpty();
	}

	if (Cast<AStrategyPlayerUnit>(Unit))
	{
		if (!Squad.Contains(Unit))
		{
			return LOCTEXT("ClassOtherSquad", "ANOTHER SQUAD");
		}

		return Unit->IsIncapacitated() ? LOCTEXT("ClassSquadDown", "SQUAD - DOWN") : LOCTEXT("ClassSquad", "SQUAD");
	}

	const bool bPerson = Unit->IsPerson();

	if (Unit->IsDead())
	{
		return bPerson ? LOCTEXT("ClassBody", "BODY") : LOCTEXT("ClassCarcass", "CARCASS");
	}

	if (Unit->IsDowned())
	{
		return bPerson ? LOCTEXT("ClassPersonDown", "PERSON - DOWN") : LOCTEXT("ClassCreatureDown", "CREATURE - DOWN");
	}

	if (Unit->IsAggressive())
	{
		return bPerson ? LOCTEXT("ClassPersonHostile", "PERSON - HOSTILE") : LOCTEXT("ClassCreatureHostile", "CREATURE - HOSTILE");
	}

	return bPerson ? LOCTEXT("ClassPersonNeutral", "PERSON - NEUTRAL") : LOCTEXT("ClassCreature", "CREATURE");
}

/** Who an actor is, as a row names them - "Someone" for a unit nobody named, the feed's own word for it */
static FText StrategyTargetActions_NameOf(const AStrategyUnit* Actor)
{
	if (!Actor)
	{
		return FText::GetEmpty();
	}

	const FText Name = Actor->GetHolderDisplayName();

	return Name.IsEmpty() ? LOCTEXT("UnnamedActor", "Someone") : Name;
}

FStrategyTargetInfo FStrategyTargetActions::BuildTargetInfo(const AActor* Target, const TArray<AStrategyUnit*>& Selection, const TArray<AStrategyUnit*>& Squad)
{
	FStrategyTargetInfo Info;

	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(Target);

	if (!IsValid(Target) || !Interactable)
	{
		// bHasTarget stays false, which is how the panel knows to hide itself entirely
		return Info;
	}

	Info.bHasTarget = true;
	Info.Target = const_cast<AActor*>(Target);
	Info.DisplayName = Interactable->GetInteractionDisplayName();
	Info.Classification = StrategyTargetActions_Classify(Target, Squad);

	// distance is measured from the nearest selected unit, because that is the unit that would
	// actually carry out whatever the player asks for. With nothing selected there is nothing to
	// measure from, and the panel says so by leaving the figure negative rather than printing 0m.
	float NearestDistanceSquared = -1.0f;

	for (const AStrategyUnit* CurrentUnit : Selection)
	{
		if (!IsValid(CurrentUnit))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(CurrentUnit->GetActorLocation(), Target->GetActorLocation());

		if (NearestDistanceSquared < 0.0f || DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
		}
	}

	if (NearestDistanceSquared >= 0.0f)
	{
		// Unreal units are centimetres; the wireframe reads in metres
		Info.DistanceMeters = FMath::Sqrt(NearestDistanceSquared) / 100.0f;
	}

	if (const AStrategyUnit* Unit = Cast<AStrategyUnit>(Target))
	{
		if (const UHealthComponent* Health = Unit->GetHealth())
		{
			Info.bHasHealth = true;
			Info.HealthFraction = Health->MaxHealth > 0.0f
				? FMath::Clamp(Health->GetHealth() / Health->MaxHealth, 0.0f, 1.0f)
				: 0.0f;
		}
	}

	StrategyTargetActions_BuildOffered(Target, Squad, Info.Actions);

	// the menu's "who would go" - the general answer, which is who does almost everything
	const AStrategyUnit* HeaderActor = ResolveActor(Target, NAME_None, Selection, Squad);

	Info.ActorName = StrategyTargetActions_NameOf(HeaderActor);

	for (FTargetAction& Action : Info.Actions)
	{
		if (Action.Id == StrategyTargetAction::Attack())
		{
			// an attack commits everyone selected, not one of them - DoAttackCommand's rule
			const TArray<AStrategyUnit*> Attackers = GetAttackers(Target, Selection);

			const FText AttackerName = Attackers.Num() > 1
				? LOCTEXT("EveryoneSelected", "Everyone selected")
				: (Attackers.Num() == 1 ? StrategyTargetActions_NameOf(Attackers[0]) : FText::GetEmpty());

			StrategyTargetActions_ApplyActor(Action, Attackers.Num() > 0, Attackers.Num() == 1 ? Attackers[0] : nullptr, AttackerName, Target, Info.ActorName);
			continue;
		}

		const AStrategyUnit* Actor = ResolveActor(Target, Action.Id, Selection, Squad);

		StrategyTargetActions_ApplyActor(Action, Actor != nullptr, Actor, StrategyTargetActions_NameOf(Actor), Target, Info.ActorName);
	}

	return Info;
}

bool FStrategyTargetActions::FindActionFor(const AActor* Target, const AStrategyUnit* Actor, FName ActionId, const TArray<AStrategyUnit*>& Squad, FTargetAction& OutAction)
{
	if (!IsValid(Target) || !Cast<ISmoresInteractable>(Target))
	{
		return false;
	}

	TArray<FTargetAction> Offered;
	StrategyTargetActions_BuildOffered(Target, Squad, Offered);

	const FTargetAction* Found = Offered.FindByPredicate([ActionId](const FTargetAction& Candidate)
	{
		return Candidate.Id == ActionId;
	});

	if (!Found)
	{
		return false;
	}

	OutAction = *Found;

	const AStrategyUnit* AbleActor = CanAct(Actor) ? Actor : nullptr;

	StrategyTargetActions_ApplyActor(OutAction, AbleActor != nullptr, AbleActor, StrategyTargetActions_NameOf(AbleActor), Target, StrategyTargetActions_NameOf(AbleActor));

	return true;
}

bool FStrategyTargetActions::CanAct(const AStrategyUnit* Unit)
{
	return IsValid(Unit) && !Unit->IsIncapacitated();
}

AStrategyUnit* FStrategyTargetActions::ResolveActor(const AActor* Target, FName ActionId, const TArray<AStrategyUnit*>& Selection, const TArray<AStrategyUnit*>& Squad)
{
	if (!IsValid(Target))
	{
		return nullptr;
	}

	const FVector TargetLocation = Target->GetActorLocation();

	AStrategyUnit* Nearest = nullptr;
	float NearestDistanceSquared = 0.0f;
	bool bAnySelected = false;
	bool bTargetSelected = false;

	for (AStrategyUnit* CurrentUnit : Selection)
	{
		if (!IsValid(CurrentUnit))
		{
			continue;
		}

		bAnySelected = true;

		if (CurrentUnit == Target)
		{
			bTargetSelected = true;
			continue;
		}

		if (!CanAct(CurrentUnit))
		{
			continue;
		}

		// the same straight-line distance the panel prints, so the name and the figure agree
		const float DistanceSquared = FVector::DistSquared(CurrentUnit->GetActorLocation(), TargetLocation);

		if (!Nearest || DistanceSquared < NearestDistanceSquared)
		{
			Nearest = CurrentUnit;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	if (Nearest)
	{
		return Nearest;
	}

	// treating your own wounds is the one thing the target can do for itself
	if (ActionId == StrategyTargetAction::Heal() && bTargetSelected)
	{
		AStrategyUnit* TargetUnit = const_cast<AStrategyUnit*>(Cast<AStrategyUnit>(Target));

		if (CanAct(TargetUnit))
		{
			return TargetUnit;
		}
	}

	if (bAnySelected)
	{
		// somebody is selected and none of them can do it - the player asked for them, so nobody
		// else is quietly sent instead
		return nullptr;
	}

	// nothing selected: a squad member already standing within reach does it, which is how the
	// double-click on a chest has always worked for a pawn standing beside it
	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(Target);

	for (AStrategyUnit* CurrentUnit : Squad)
	{
		if (!CanAct(CurrentUnit) || CurrentUnit == Target || !Interactable || !Interactable->IsInRangeOf(CurrentUnit))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(CurrentUnit->GetActorLocation(), TargetLocation);

		if (!Nearest || DistanceSquared < NearestDistanceSquared)
		{
			Nearest = CurrentUnit;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	return Nearest;
}

TArray<AStrategyUnit*> FStrategyTargetActions::GetAttackers(const AActor* Target, const TArray<AStrategyUnit*>& Selection)
{
	TArray<AStrategyUnit*> Attackers;

	for (AStrategyUnit* CurrentUnit : Selection)
	{
		if (CanAct(CurrentUnit) && CurrentUnit != Target)
		{
			Attackers.Add(CurrentUnit);
		}
	}

	return Attackers;
}

bool FStrategyTargetActions::ValidateActionOrder(const AStrategyUnit* Actor, const AActor* Target, FName ActionId, const TArray<AStrategyUnit*>& Squad, ESmoresRefusalReason& OutReason)
{
	OutReason = ESmoresRefusalReason::None;

	// the client named the actor and the action, so neither is taken on trust - the same stance
	// RequestSelectUnit takes about which pawns a player may pick
	if (!IsOrderAction(ActionId) || !CanAct(Actor) || !Squad.Contains(Actor))
	{
		return false;
	}

	// only Heal is something you can do to yourself
	if (Actor == Target && ActionId != StrategyTargetAction::Heal())
	{
		return false;
	}

	FTargetAction Entry;

	if (!FindActionFor(Target, Actor, ActionId, Squad, Entry))
	{
		// no longer offered at all - the target changed state between the click and the RPC
		OutReason = ESmoresRefusalReason::NotInteractable;
		return false;
	}

	if (!Entry.bEnabled)
	{
		OutReason = Entry.DisabledReason;
		return false;
	}

	return true;
}

bool FStrategyTargetActions::IsOrderAction(FName ActionId)
{
	return !ActionId.IsNone()
		&& ActionId != StrategyTargetAction::Attack()
		&& ActionId != StrategyTargetAction::Examine();
}

bool FStrategyTargetActions::IsPlaceholderAction(FName ActionId)
{
	// one line each, so the system that replaces one deletes one line
	return ActionId == StrategyTargetAction::Heal()
		|| ActionId == StrategyTargetAction::Kidnap()
		|| ActionId == StrategyTargetAction::Pickpocket()
		|| ActionId == StrategyTargetAction::KnockOut();
}

float FStrategyTargetActions::GetPlaceholderSuccessChance(FName ActionId, const AStrategyUnit* Actor, const AActor* Target)
{
	// PLACEHOLDER - owned by the future stealth (Pickpocket), capture (Kidnap) and takedown
	// (Knock out) systems. The numbers are arbitrary; the shape is not - see the declaration.
	const AStrategyUnit* Mark = Cast<AStrategyUnit>(Target);

	if (!Actor || !Mark)
	{
		return -1.0f;
	}

	const FCharacterAttributes& Mine = Actor->GetAttributes();
	const FCharacterAttributes& Theirs = Mark->GetAttributes();

	float Edge = 0.0f;

	if (ActionId == StrategyTargetAction::Pickpocket())
	{
		Edge = Mine.Agility - Theirs.Perception;
	}
	else if (ActionId == StrategyTargetAction::Kidnap() || ActionId == StrategyTargetAction::KnockOut())
	{
		Edge = Mine.Strength - Theirs.Endurance;
	}
	else
	{
		return -1.0f;
	}

	return FMath::Clamp(0.5f + Edge * 0.03f, 0.05f, 0.95f);
}

FText FStrategyTargetActions::FormatSuccessChance(float Chance)
{
	return FText::Format(LOCTEXT("SuccessChance", "{0}% chance"), FText::AsNumber(FMath::RoundToInt(Chance * 100.0f)));
}

bool FStrategyTargetActions::IsLootableNPC(const AStrategyUnit* Unit)
{
	// never one of the players' own pawns, and never an NPC still on its feet. Downed and Dead
	// both qualify and are treated identically - the inventory roadmap's settled decision: looting
	// a body is the same actor and the same code path as looting a knocked-down one.
	return IsValid(Unit) && !Cast<AStrategyPlayerUnit>(Unit) && Unit->IsIncapacitated();
}

bool FStrategyTargetActions::IsInteractableNPC(const AStrategyUnit* Unit)
{
	// the mirror of IsLootableNPC: never one of the players' own pawns, on its feet rather than
	// Downed or Dead, and not currently hostile. That last clause is the whole "never trade with
	// someone trying to kill you" rule, written down once here so dialog inherits it.
	return IsValid(Unit) && !Cast<AStrategyPlayerUnit>(Unit) && !Unit->IsIncapacitated() && !Unit->IsAggressive();
}

UTraderComponent* FStrategyTargetActions::GetTraderStock(const AStrategyUnit* Unit)
{
	if (!IsInteractableNPC(Unit))
	{
		return nullptr;
	}

	// the component's presence *is* the "is this a trader?" flag - there is no separate bool that
	// could disagree with it
	return Unit->FindComponentByClass<UTraderComponent>();
}

bool FStrategyTargetActions::IsInRangeOfUnits(const AActor* Target, const TArray<AStrategyUnit*>& Units)
{
	const ISmoresInteractable* Interactable = Cast<ISmoresInteractable>(Target);

	if (!Interactable)
	{
		// an actor that isn't interactable has no reach to be inside of, so it is never in range
		return false;
	}

	for (const AStrategyUnit* CurrentUnit : Units)
	{
		if (IsValid(CurrentUnit) && Interactable->IsInRangeOf(CurrentUnit))
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
