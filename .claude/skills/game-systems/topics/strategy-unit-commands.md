# Strategy Unit Commands

## Purpose

How the player's orders turn into units moving: a move order to a spot on the ground, an attack
order on a target, and an action order - walk over to something and do something to it. This topic
owns the movement side of all three. What each kind of target *offers*, and the menu that offers it,
belong to `action-menu.md`; the fight itself belongs to `combat.md`.

## Player Surface

- **Right-click on empty ground** moves the selected squad there. The selected unit nearest the spot
  goes to it; the rest spread out around it. A right-click on something lit up opens the action
  menu instead - see `action-menu.md`.
- **Attack** (`H`, the panel, the menu) sends everyone selected at the target.
- **Any other action** (loot, talk, trade, pick up, open a door, and the placeholders) sends one
  squad member - the selected one nearest the target - to walk over and do it.
- A new order replaces whatever a unit was doing, without a word. Anything else that stops a unit
  short of an action order (going down, fighting back, the target vanishing, no way through) is said.

## Core Rules

- **Commands are client-side input, server-side movement.** `ControlledUnits` (the selection) exists
  only on the owning client's controller, so every order carries the units it applies to over a
  `Server_` RPC: `Server_MoveUnits(Units, Goal, ClosestUnit)`, `Server_AttackCommand(Units, Target)`,
  `Server_RequestActionOrder(Actor, Target, ActionId)`. All three movement entry points on the unit
  are authority-gated and refuse a Downed or Dead unit.
- **A move order** (`DoMoveUnitsCommand` → `AStrategyUnit::MoveToLocation`): the lead unit is the
  selected one nearest the goal in 2D; each unit runs an EQS query to pick its own destination
  (`InteractionQuery` for the lead with a single best result, `NoInteractionQuery` for the others,
  picking randomly from the best 25%), then moves there through its `AAIController` with partial
  paths allowed and the goal projected onto the navmesh. `BP_CursorFeedback` shows at once on the
  client, positive with a selection and negative without.
- **An attack order** moves through combat's own approach: `UCombatComponent::AttackTarget` swings
  if in range, otherwise broadcasts `OnTargetOutOfRange`, and the unit walks to the target with
  `MoveToLocation` and attacks again on arrival (`bAttackOnArrival`). See `combat.md`.
- **An action order** is a `UActionOrderComponent` on the unit, walking with
  `AStrategyUnit::MoveToActor` - a goal-actor move that follows the target, never an EQS point that
  might land outside reach. Retries, arrival, cancellation and the re-check on arrival are in
  `action-menu.md`'s "Walking Over to Act".
- **A new move cancels whatever came before.** `MoveToLocation` is the catch-all: it cancels a
  pending action order (silently, before stopping the unit), drops an earlier move's EQS query still
  in flight, and clears any attack. `AttackTarget` cancels an action order silently too.
- **The move-finished callback fires for every request's end, aborts included**, and an abort fires
  synchronously inside whatever call replaced the move. `AStrategyUnit::OnMoveFinished` therefore
  only passes an ending on to the action order when it is that order's own walk
  (`ActionMoveRequestId`), and every call that starts or stops that walk clears the id first.
  `OnEQSFinished` ignores any query that is no longer the current one.
- **Arrival writes the record.** `HandleMoveFinished` writes the unit's `LastKnownLocation` back to its
  character record (see `game-data.md`), broadcasts `OnMoveCompleted`, and re-issues a pending
  attack (`bAttackOnArrival`).
- **Nothing else happens on arrival.** The Strategy template's arrival interaction (the lead unit
  turning a unit it stopped beside to face it, and both playing a "response animation") was
  removed at the action menu's PIE checkpoint - no animation was ever assigned, so it only ever
  turned people. Turning to face now happens where it means something: an action order's squad
  member faces what it acts on, and an NPC faces whoever comes to talk or trade
  (`AStrategyUnit::FaceToward`).

## C++ Implementation

- `AStrategyPlayerController` (`smores`)
  - `InteractClick` (right mouse, on release) → the action menu, or `DoMoveUnitsCommand(CursorLocation)`.
  - `DoMoveUnitsCommand` → `Server_MoveUnits`; `DoAttackCommand` → `Server_AttackCommand`;
    `RequestTargetAction` → `Server_RequestActionOrder` for every order action.
  - `GetClosestSelectedUnitToLocation` picks a move order's lead unit.
- `AStrategyUnit` (`SmoresCharacters`)
  - Auto-possesses AI; caches the `AAIController` and subscribes to its path following's
    `OnRequestFinished` in `NotifyControllerChanged`.
  - `MoveToLocation(Location, bLeadUnit)` (EQS - `InteractionQuery` for the lead, `NoInteractionQuery`
    for the rest; the names are the template's - then `OnEQSFinished` issues the move),
    `MoveToActor` (goal actor, returns `EActionApproachResult`: walking, already there, or failed),
    `StopActionApproach`, `TakeOverForActionOrder`, `StopMoving`, `DropPendingMoveQuery`, `FaceToward`.
  - `OnMoveFinished` → `HandleMoveFinished` → `OnMoveCompleted`, then the action order if it was its walk.
- `UActionOrderComponent` (`SmoresCharacters`) - see `action-menu.md`.

## Blueprint / Asset Dependencies

- Unit Blueprints (`BP_StrategyUnit`, `BP_PlayerUnit`) set `InteractionQuery` (`EnvQuery_MoveUnitClosest`)
  and `NoInteractionQuery` (`EnvQuery_MoveUnitAdditional`). Their event graphs are empty now: the
  only events they implemented served the removed arrival animation. `BP_UnitSelected` /
  `BP_UnitDeselected` remain as hooks for a selection look.
- `BP_StrategyPlayerController` implements `BP_CursorFeedback`.
- A navmesh is required (`RuntimeGeneration=Dynamic`, so doors and other moving obstacles rebuild it
  at runtime), with a `NavMeshBoundsVolume` covering the play area.

## Extension Points

- Formation logic belongs where `Server_MoveUnits` hands each unit its goal.
- A queue of orders (`Shift`, per `orders-and-jobs.md`) grows out of `UActionOrderComponent`, which
  holds one order today.
- Keep presentation in Blueprint events and command authority in C++.

## Known Gaps

- Group formation is randomised by EQS rather than deterministic.
- Nothing queues: every order replaces the last.
