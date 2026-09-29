# Action Menu Roadmap — Right-Click Actions, Walking Over to Act, Doors and Examine

## Purpose

This is a **roadmap**, not a system reference. The permanent record of how this works is the
`game-systems` skill — `action-menu.md` for the rules table, who acts, the walk-over order, the
menu, the hover and the door, with `strategy-unit-commands.md`, `hud-and-panels.md`,
`input-and-keybinds.md`, `refusals-and-feedback.md`, `combat.md`, `game-data.md`, `inventory.md`
and `testing.md` updated alongside. The design intent is `game-design`'s `player-interface.md`
("Right-Click Action Menu", and the known-vs-hidden numbers rule).

> **Status: the one slice is DONE - the roadmap is complete.** Built 2026-09-29 and PIE-checked by
> Jim the same day: right-click opens the menu, choosing an action walks the squad member over,
> Examine works, a door opens and closes from a double-click and from the menu, Pawn 2's Nimble
> Settler description shows in Examine, and the placeholders appear under their rules (Kidnap only
> once the NPC is down). His checkpoint calls are below. 216 automated tests pass (19 of them from
> this slice).

## Slice 1 — The Action Menu — SHIPPED

- **The rules** moved out of the controller into `FStrategyTargetActions`
  (`Variant_Strategy/StrategyTargetActions.*`): one table of what each kind of thing offers, the
  "who acts" rule, and the placeholder odds. The target panel and the right-click menu are two
  views of it.
- **Walking over** (`UActionOrderComponent`, `AStrategyUnit::MoveToActor`, `IActionOrderHost`):
  every route - menu, panel, `T`, `O`, double-click - sends the nearest selected squad member, who
  re-checks the rules on arrival. The player's own new order cancels silently; going down,
  fighting back, the target vanishing and no path are all said.
- **The menu and the hover**: one resolver for the hover, the right-click and the double-click;
  an amber overlay rim (`M_HoverRim`); `UActionMenuWidget` on its own Z-50 layer.
- **Groundwork**: `ISmoresInteractable` (reach, name, examine text) with `IInventoryHolder` deriving
  from it; `ECharacterKind` on character definitions; `Disposition` and a copy of the record's
  `Attributes` replicated; `NoOneSelected` and `CannotReach` refusals.
- **The door** (`AWorldDoor`, `BP_WorldDoor`) and **Examine** (`UExamineWidget`, `WBP_Examine`), and
  a test storeroom in `LVL_Strategy` (folder `ActionMenuTest`) with a chest behind a door.
- **Placeholders**: Heal, Kidnap, Pickpocket and Knock out, offered under real rules, walked over
  to, and announced "not built yet" on arrival.

### What changed from the plan while building

- The order hears only **its own walk's** end, by move request id, rather than listening to
  `OnMoveCompleted` - the delegate fires for aborts too, synchronously, inside the call that
  replaced the move.
- The hover resolver gained a **middle step** - the cursor's ray through each thing's mesh bounds -
  because the selection trace may not see pawns and dropped items have no collision.
- `IInventoryHolder` kept `GetHolderDisplayName`, which answers the new
  `GetInteractionDisplayName`, rather than renaming 34 call sites.
- A refusal with no reason on arrival still gets a feed line, rather than a squad member walking
  over and doing nothing.

### Jim's checkpoint calls (2026-09-29)

- **Hover highlight, not hold-to-open** - kept. It lights up on every target the squad could act on,
  as intended.
- **Knock out added** as a fourth placeholder, on people on their feet only (not creatures, not a
  squad) - after Pickpocket, greyed out on someone already hostile, with placeholder odds (Strength
  against Endurance). Jim is adding placeholders on purpose, to judge the menu's spacing full.
- **The template's arrival animation is removed.** It never played: the unit Blueprints only ran it
  when a `ResponseAnimation` was assigned, and none ever was. What Jim did see - the NPC turning to
  face a squad member who stopped beside them - was the C++ half of the same leftover. The whole
  thing is gone (`Interact`, `BP_InteractionBehavior`, `BP_StopAnimation`, `bInteractOnArrival`,
  `InteractionRadius`, and the three Blueprint events plus the unused variable on `BP_StrategyUnit`
  and `BP_PlayerUnit`). The turn survives where it means something: an NPC now turns to face the
  squad member who comes to **talk or trade**. The lead unit of a move order still takes the best
  EQS point (`MoveToLocation`'s `bLeadUnit`).
- **Examine is worth having as a window**, not just a feed line.
- **Pickpocket's odds differ between the two pawns**, as intended. Knock out's don't, also as
  intended: it reads Strength, and only Pawn 2's Agility was raised.
- **Found after the close: reach went through walls.** A shut door blocked the walk as it should,
  so the pawn stopped outside the storeroom's back wall - still within the chest's 3 m reach, which
  was distance only, so the chest opened through the wall. Reach now also needs a clear line
  (`ISmoresInteractable::HasClearReach`, a world-static trace), so that walk ends with *Can't get
  there* until the door is opened. One test added (216).

## Not in This Roadmap

- **Esc to close.** Esc is PIE's own stop key, and closing things with it wants to be one "back"
  stack (menu → window → system menu), designed once. Reserved in `input-and-keybinds.md`.
- **The real Heal, Kidnap, Pickpocket and Knock out.** Heal waits on injuries and medicine
  (`characters-and-squads.md`). Kidnap waits on carrying a body. Pickpocket and Knock out wait on NPC
  awareness, which is also what unblocks `inventory-roadmap.md` Slice 10. Each replaces its
  placeholder line and arrival branch; the menu doesn't change. The real rolls come from the same
  function as the displayed odds and follow the save-scum rule (`UWorldSeedComponent`).
- **Queueing orders with `Shift`** - it arrives with `orders-and-jobs.md`.
- **Locked doors, and units opening doors on their own.** `Locked` is the likely next refusal reason.
- **Word-based descriptions of a target's ability** (*a difficult lock*, *looks alert*) - they
  arrive with locks and NPC awareness, as a line in Examine.
- **Giving items to a squad member, or trading between players.**
- **Touch**, **a cursor change** alongside the hover, and **door state surviving a save**.

## Resolved Design Decisions

- **The menu is a view of the target panel's list, not a second list.** One rules function builds
  both. (Rejected: menu-specific rules; they would drift.)
- **Visible placeholders for Heal, Kidnap, Pickpocket** — Jim, 2026-09-29; **Knock out** added at the
  checkpoint. (Rejected: leaving them off, which would leave the per-member text with nothing on
  screen to judge; building simple real versions, which would reopen parked theft.)
- **Every route walks over, double-click included** — Jim. (Rejected: menu, panel and keys only;
  menu only.)
- **Nearest selected acts; Attack is everyone** — Jim. (Rejected: choosing the member inside the
  menu, which makes the menu long.)
- **A simple door now** — Jim. (Rejected: doors later with base building.)
- **Hover highlight over hold-for-menu** — recommended 2026-09-29, **confirmed by Jim in PIE**. Hold
  is slow, undiscoverable, and on PC often means camera; the RTS smart right-click with a visible
  hover is the common convention.
- **The actor updates live, not locked when the menu opens** — a locked actor would be wrong by the
  time of the click.
- **Walking over uses a goal-actor move, not `MoveToLocation`** — EQS's random best-25% can end out
  of reach, and a goal actor follows a moving target for free.
- **Container's action renamed Loot; Open/Close is doors only** — Jim's own vocabulary.
- **Odds shown as a number; a target's stats never shown** — Jim, 2026-09-29, relaxing the diegetic
  rule for numbers the player already knows. (Rejected: words only.)
- **The template's arrival interaction is retired** — Jim, at the checkpoint; the NPC's turn to face
  kept for talk and trade only.

## Open Questions Worth Tracking

Carried into `action-menu.md`'s Known Gaps, which is where they live now:

- **Should a single click share the hover resolver too**, so what's lit is also what a left click
  picks? Not changed - selection's generous sweep exists for picking small moving units.
- **Should the right-click that closes the menu also open a new one** at the new spot? It is
  swallowed today - simplest and predictable.
- **The target panel's row is horizontal**, and a person now offers up to seven actions.
