# Action Menu Roadmap — Right-Click Actions, Walking Over to Act, Doors and Examine

## Purpose

This is a **roadmap**, not a system reference: read it while implementing its slice, or when Jim
points at it. The permanent record of how this works belongs in the `game-systems` skill — a new
`action-menu.md` topic, plus the existing topics listed under Task H — and this file is trimmed to
its status and decision log once the slice ships.

The design intent is settled and lives in the `game-design` skill: **read `player-interface.md`'s
"Right-Click Action Menu" section before starting**, and the "numbers about your own squad are
known; numbers about the world are not" rule under Diegetic vs. Non-Diegetic Philosophy on the
same page. Between them they own what the menu is for, the hover-before-click rule, who acts, and
why the odds text may be a number. Don't reopen those here.

> **Status: planned 2026-09-29, not started.** One slice.

## The Shape, in One Paragraph

Right-click on empty ground still moves the squad. Right-click on a **thing** — a person, a
creature, a body, a container, a door, an item on the ground — opens a small menu at the cursor
listing what the squad can do to it. The thing under the cursor **lights up before any click**, so
the player always knows which right-click they're about to make. The menu is not a new set of
rules: it is a second view of the list the target panel already shows, built by the same function.
Choosing an action sends **one squad member** (the selected one nearest the target) to walk over and
do it; Attack still commits everyone selected. Every other route to the same action — the target
panel's buttons, the `T` and `O` keys, a double-click — now walks over as well, so there is one
behaviour to learn. Heal, Kidnap and Pickpocket appear as **visible placeholders**: real rules for
when they're offered, placeholder odds text that changes with who would go, and on arrival a feed
line saying the action isn't built yet. A simple **door** and an **Examine** window arrive with it.

## Decided With Jim (2026-09-29)

| Question | Answer |
|---|---|
| Heal, Kidnap, Pickpocket have no systems behind them. What does the menu do with them? | **Visible placeholders.** Offered under their real rules; Pickpocket and Kidnap show placeholder odds for whoever would go; choosing one walks over, then the feed says it isn't built yet |
| Only Attack walks over today. Should the other routes change to match the menu? | **Everything walks over, double-click included.** A double-clicked chest across the room sends someone to open it |
| Doors? | **Build a simple door** — opens and closes, blocks walking paths when shut — and place one test door in `LVL_Strategy` |
| Several squad members selected — who acts? | **The nearest selected one; Attack is everyone selected.** The menu names who. To pick someone else, select only them |
| Right-click fighting with move orders (Jim's concern) | **Hover highlight**, the RTS "smart right-click" convention, over hold-for-menu. Recommended in-session; Jim to confirm at the checkpoint. Only the input step changes if it flips |
| Can the odds text be a number? | **Yes.** It comes from the squad member's own skill, which the player already knows. What is never shown as a number is the target's side — an NPC's stats, a lock's difficulty — which surfaces only as words. Recorded in `player-interface.md` |

Defaults stated to Jim and not objected to (reopen only if he does):

- Right-clicking a thing also **targets** it, so the target panel and the menu describe the same
  thing.
- The **game keeps running** while the menu is open. Entries refresh every frame; a click
  re-checks the rules before acting, as the panel's buttons already do.
- The menu closes on a pick, on a click anywhere else (that click is **swallowed** — it doesn't
  also move or select), or when its target stops existing.
- **Person vs. creature** is a field on the character definition, defaulting to person. Creatures
  get Attack and Examine (and Loot once down).
- **Trade is its own entry**, opening the shop directly. Talk keeps its current meaning
  (a conversation if one is eligible, else a trader opens trade, else a "nothing to say" bark),
  and so do `T` and the double-click.
- A loose **item on the ground** gets Pick up and Examine.
- **Examine never walks.** It opens a small window: name, what it is, any written description.
- With **nobody selected**, actions that need someone are greyed out with *No one selected*;
  Examine still works.
- A **new move order cancels** the walk-over. A target that walks away is followed. No path gives
  up with a new *Can't get there* line.
- The **server runs the walk and the action**, then tells the ordering player's screen to open
  whatever window it needs.

**One default changed after it was stated:** Jim was told Esc would close the menu. It doesn't in
this roadmap. In PIE, Esc is the editor's own *stop playing* key, so it can't be tested without
changing an editor setting. And Esc is reserved for a general "back" key (close menu → close
window → system menu) that wants designing once, not one screen at a time. Click-away closes the
menu instead. See Not in This Roadmap.

**Vocabulary change, following Jim's wording:** the container's action becomes **Loot** (same as a
body's; they open the same window). **Open / Close** now mean doors only.

## What Exists Today

Read these before touching anything; each one shapes a task below.

- **The target panel's action row is the rules engine this menu reuses.**
  `AStrategyPlayerController::BuildTargetInfo` (static, tested in
  `Source/smores/Tests/TargetInfoTest.cpp`) builds `FStrategyTargetInfo` / `FTargetAction`
  (`SmoresUI/StrategyTargetInfo.h`), including disabled entries with their reason.
  `RequestTargetAction` re-derives the row on click rather than trusting the button.
  `hud-and-panels.md`'s rules — "the row is assembled from the rules, not alongside them" and
  "the target label and the action row are one thing" — now apply to the menu too.
- **Right mouse is `IA_Strategy_InteractClick`**, bound on `Completed` (release), and today always
  runs `DoMoveUnitsCommand`. No new input action or key mapping is needed for the menu.
- **Only Attack walks over.** `UCombatComponent` → `AStrategyUnit::OnCombatTargetOutOfRange` moves
  in with `MoveToLocation` and `bAttackOnArrival`. Talk, Open and Loot refuse *Too far away*
  (`BuildTargetInfo`'s `RangeRefusal`, `InteractWithNPC`, the double-click ladder).
- **`MoveToLocation` goes through EQS** and non-lead units pick a random point in the best 25%, so
  it can't be trusted to end *within reach* of a target. Walking over to act needs its own
  goal-actor move (Task C).
- **The double-click ladder** (`SelectAllDoubleClick`): world item → container → NPC (body or
  standing) → empty ground. It uses per-type click radii (`WorldItemSelectionRadius` 100 cm,
  `ContainerSelectionRadius` 250 cm). **That 250 cm radius is exactly what would make right-click
  fight move orders** — you couldn't right-click the ground beside someone. Task G replaces it for
  these gestures.
- **A single click on a loose world item clears the squad.** `DoSelectCommand` only knows pawns and
  containers, so an item reads as "empty ground". That is why the double-click's pickup branch
  checks *every* pawn instead of the selection. Task D makes items (and doors) targetable, which
  removes the cause.
- **Reach** is `IInventoryHolder::IsInRangeOf` (`SmoresItems`), each holder's own sphere. A door
  isn't an inventory holder, so reach has to move up a layer (Task A).
- **Nothing tells a person from a creature.** Every NPC is an `AStrategyUnit`.
- **No skills exist** (`FCharacterRecord` stores the seven attributes only, deliberately). Records
  are **server-only**: `GetRecord()` is null on a client. So anything a client displays about a
  unit's attributes needs a replicated copy on the unit, the same pattern as `FactionId`.
- **`Disposition` (hostility) is not replicated** — `combat.md`'s Known Gaps. The menu adds three
  more things that read it (Talk, Trade, Pickpocket). A remote client would show all three enabled
  against someone actively attacking them.
- **Theft is parked**: `inventory-roadmap.md` Slice 10 says not to start until NPC awareness exists.
  The Pickpocket placeholder touches nothing it owns — no item moves, no `bStolen`.
- **`strategy-unit-commands.md` is stale.** It describes `OnMoveCompleted` / `bAllowInteraction` /
  `CachedInteraction` handling in the controller. None of that exists there any more; arrival
  handling is `AStrategyUnit::HandleMoveFinished` (`bInteractOnArrival`, the template's
  `BP_InteractionBehavior`). Task H rewrites the topic.
- **`StrategyPlayerController.cpp` is 4,337 lines.** `unreal-module-organization.md` already calls
  the class, not the module, the junk drawer. This roadmap therefore puts the rules in their own
  file and the walk-over state in a component, rather than growing the controller.

## Traps Found While Checking the Plan

Checked against the code on 2026-09-29, after the plan was written. None of them needs a decision
from Jim; each one would cost a debugging session if found the hard way.

1. **"Move finished" also fires when a move is cancelled** (Task C). `AStrategyUnit::OnMoveFinished`
   hands every result to `HandleMoveFinished` (success, blocked and aborted alike), and
   `OnMoveCompleted` carries no result at all. Issuing a new move aborts the old one, which fires
   that callback — **synchronously, inside the new move call**. So the order component will hear
   an "arrival" from its own retry, from `MoveToLocation`'s `StopMoving`, and from any other
   interruption. It must see the result (pass it through, or track the request id) and ignore
   aborts. And `MoveToLocation` must cancel a pending order **before** it calls `StopMoving`, not
   after.
2. **An earlier move's EQS query can land late and hijack the walk-over** (Task C).
   `MoveToLocation` starts an async EQS query and issues the real move from `OnEQSFinished`. If an
   action order arrives while that query is still running, the query finishes afterwards and
   sends the unit to the old point. `MoveToActor` has to drop the pending query
   (`EnvQueryInstance`) first, and `OnEQSFinished` should ignore a query that is no longer current.
3. **`AlreadyAtGoal` returns without any finished callback** (Task C). `OnEQSFinished` already
   handles it by calling `HandleMoveFinished` by hand; `MoveToActor` must do the same, or a unit
   standing next to its target waits forever.
4. **The server has no selection** (Tasks B and C). `ControlledUnits` exists only on the owning
   client. So the rules need two entry points: *who acts* (client-side, from the selection) and
   *is this action available for this actor on this target* (both sides). The server's checks —
   on issue and on arrival — use only the second, with the actor the client sent.
5. **`GetTraderStock` returns null for a hostile trader** (Task B), because it checks
   `IsInteractableNPC` first. Used to decide whether Trade is *listed*, it would make Trade vanish
   on a hostile trader instead of greying out with *They won't deal with you*. List Trade on the
   presence of `UTraderComponent`; decide *enabled* on hostility.
6. **`OpenTradeWith` is an `IDialogHost` method** (Task C), which conversations' `<<OpenTrade>>`
   also calls. Give arrival its listener through an internal helper, not by changing the interface
   signature.
7. **Both player pawns share `DA_Character_Settler`** (Task A), set on `BP_PlayerUnit`, with no
   per-instance override in `LVL_Strategy`. "Give them different Agility" therefore means a second
   definition — a copy of the Settler with its own `DefinitionId` and higher Agility, in
   `Content/Characters/Definitions/` so the Asset Manager scan finds it — set on one placed pawn.
   That is a per-instance override written through MCP, which `mcp-workflow` warns can silently
   vanish on re-instancing. Verify it after save and a level reload, as that skill says.
8. **A double-click on your own pawn** (Task D). Today the ladder never finds one of your own
   pawns, so the gesture falls through to "select all on screen". The new resolver *will* find
   them, because they're hoverable for Heal and Examine. Keep select-all for that case; don't
   send anyone to heal on a double-click.
9. **"Is the cursor over the HUD?"** (Task G) has no one-call answer in UMG. One way: ask Slate for
   the widget path under the cursor and treat anything other than the game viewport as UI. The HUD
   regions and windows are hit-testable, and the refusal line and bark bubbles are not, so they
   correctly don't count.

Verified harmless: nothing in `Content/Variant_Strategy` uses the overlay-material slot, so the
hover can't clobber a selection look. The container highlight swaps material slot 0, and units'
selection look is Blueprint-side. `combat.md` says `AStrategyUnit` has no
`GetLifetimeReplicatedProps`; it does now (for `UnitDisplayName` and `FactionId`), so replicating
`Disposition` really is one line.

## The Rules — What Each Kind of Thing Offers

This table is the contract for `BuildTargetInfo`, and the tests assert it row by row. "Disabled
(reason)" means shown greyed with that refusal's wording beside it; "(none)" means greyed with no
reason, the existing convention for "not a refusal".

| Target | Entries, in order | When disabled |
|---|---|---|
| Person, standing, neutral | Talk · Trade *(traders only)* · Pickpocket · Heal · Attack · Examine | Heal: at full health (none) |
| Person, standing, hostile | the same | Talk, Trade: *They won't deal with you*. Pickpocket: (none) — the stealth system brings a real awareness reason later. Attack: already fighting you (none) |
| Person, Downed | Loot · Heal · Kidnap · Examine | — |
| Person, Dead | Loot · Examine | — |
| Creature, standing | Attack · Examine | Attack: already fighting you (none) |
| Creature, Downed / Dead | Loot · Examine | — |
| Your own squad member | Heal · Examine | Heal: at full health (none) |
| Another player's squad member | Examine | — |
| Container | Loot · Examine | — |
| Door | Open *or* Close · Examine | — |
| Item on the ground | Pick up · Examine | — |

On top of that, **every entry except Examine** is disabled with *No one selected* when the actor
rule below finds nobody.

- **No entry is disabled for distance any more.** That is the point of walking over. *Too far away*
  disappears from the row; the distance figure stays.
- **Placeholder entries are Heal, Kidnap and Pickpocket.** They are flagged as placeholders in the
  rules file, one line each, so the real system flips one flag when it lands.
- **Key hints:** Loot `O`, Open/Close `O`, Talk `T`, Attack `H`. The rest have no key.

### Who acts

One function, used by the menu, the panel, the keys and the double-click, so they can't disagree:

1. **Candidates** are this player's selected units that can act (not Downed or Dead), **excluding
   the target itself**.
2. **The actor** is the candidate nearest the target in a straight line — the same distance the
   panel already shows.
3. **Heal is the one exception to the exclusion:** if the target is selected and nobody else is,
   they treat themselves.
4. **With nothing selected**, a squad member already within reach of the target acts, so a pawn
   standing at a chest can still open it with nothing selected (today's double-click behaviour,
   kept). Otherwise there is no actor → *No one selected*.
5. **Attack uses every candidate**, not one, exactly as `DoAttackCommand` does today.
6. Examine needs no actor.

`FTargetAction` gains the actor's name, so the menu can name who would go. The menu refreshes
live, so if the squad shifts, the named actor updates. It is not locked when the menu opens: a
locked actor would be stale by the time the player clicked.

### The dynamic text

`FTargetAction` gains a **`Detail`** `FText`: the per-actor text. In this roadmap only Pickpocket
and Kidnap fill it, from **one placeholder function** in the rules file. Something like
`clamp(0.5 + (actor Agility − target Perception) × 0.03, 5%, 95%)` for Pickpocket, and Strength
against Endurance for Kidnap. It is labelled in code as a placeholder owned by the future stealth
system. The shape that must survive is:

- **The same function will produce both the displayed text and the roll**, so the menu can never
  promise odds the server doesn't use. There's no roll yet, only display.
- **It reads replicated data only**, so it works on a remote client (Task A adds the replicated
  attributes).
- **It shows a percentage** — settled with Jim, see the decisions table. The target's own numbers
  go into the sum but are never displayed. `Detail` is free text, so a later switch to coarse
  steps (`player-interface.md`'s open question about working the odds backwards) is a one-line
  change in that function.
- For the odds to visibly differ in PIE, the two test pawns need different Agility. They currently
  share one definition, so this takes a second one — see trap 7. The default for every attribute
  is 10.

## Walking Over to Act

The part that is genuinely new. Lives in a component on the unit, per CLAUDE.md's "new per-pawn
state goes in a component in a feature module".

- **`UActionOrderComponent`** (`SmoresCharacters`, a default subobject of `AStrategyUnit` —
  NPCs included, harmlessly, since future AI orders will want the same thing). It holds at most one
  order: target, action id, retry count. It is authority-only; nothing about it needs replicating,
  because what the player sees is the unit walking.
- **`AStrategyUnit::MoveToActor(Target)`** — new. A move request with a **goal actor** rather than an
  EQS-picked point, so a target who walks away is followed by path-following for free. It keeps the
  same guards as `MoveToLocation` (authority, incapacitated) and clears attack state the same way.
- **The lifecycle:**
  1. **Issue.** If the actor is already within reach, act immediately. Otherwise `MoveToActor`.
  2. **Arrive** (`OnMoveCompleted`, already broadcast by `HandleMoveFinished`). Within reach → act.
     Not within reach (partial path, target moved) → re-issue, a small fixed number of times, then
     fail with **`CannotReach`**.
  3. **Act.** Before doing anything, re-derive the entry for (actor, target, action) through the
     same rules function. Disabled now — the target turned hostile, went down, got looted empty —
     means refuse with that entry's own reason. Otherwise hand it to the host (below).
- **Cancellation — the rule to get right:**
  - **The player's own new order cancels silently**: `Server_MoveUnits`, `Server_AttackCommand`,
    or another action order for that unit. The player did it; there's nothing to explain.
  - **Anything else that pulls the unit away ends the order and says so**: going Downed or Dead,
    swinging back at someone who hit it (`OnHealthDamaged`'s retaliation), the target ceasing to
    exist. Per `orders-and-jobs.md`, "a blocked job is visible, never silent". A reason that is a
    refusal goes through `Client_NotifyRefusal`; an interruption that isn't a refusal ("Hana stopped
    to fight") goes to the feed through `Client_NotifyActivity`.
  - `MoveToLocation` called while an order is pending is the catch-all and cancels silently. Every
    caller of it means "the unit is now doing something else".
- **`IActionOrderHost`** (`SmoresCharacters`) — the narrow interface the component calls on
  arrival, implemented by `AStrategyPlayerController`, found through the unit's
  `GetOwningController()`. Nothing depends on `smores`, per the module rules. The controller then
  dispatches by id, server-side:

| Action | On arrival (server) |
|---|---|
| Loot (container or body) | `Client_OpenHolder(Target, Actor)` → the owning client runs `OpenContainer` / `OpenLoot`, and opens **the actor's** pack alongside rather than `FindClosestPlayerPawn`'s |
| Talk | the body of today's `Server_InteractWithNPC`, with **the actor as the listener** instead of whichever pawn is nearest |
| Trade | `OpenTradeWith`, with the actor as the listener |
| Pick up | the body of `Server_PickUpWorldItem`, into the actor's pack |
| Open / Close | `AWorldDoor::SetOpen` |
| Heal, Kidnap, Pickpocket | `Client_NotifyActivity` on the SQUAD tab: *Hana is ready to pickpocket Bandit — not built yet.* Nothing else changes |
| Attack | not an order — still `Server_AttackCommand`, squad-wide, via combat's own approach |
| Examine | not an order — client-side, immediate (Task F) |

- **The client's side** is `Server_RequestActionOrder(Actor, Target, ActionId)` on the controller.
  The server re-checks that the actor is this player's own and able, and that the action is
  available, rather than trusting the menu. That's the same stance `RequestSelectUnit` takes.

## The Menu, the Hover and the Clicks

- **One resolver decides "the thing under the cursor"**: the actor the cursor trace hits, if it is
  interactable; otherwise the nearest interactable within a small, tunable `HoverPickRadius` of the
  hit point, so a flat body or a dropped item can still be caught. Use the double-click ladder's
  priority (item → container → door → unit) to break ties. **Hover, right-click and double-click
  all use it**, so what is lit is what any of those clicks acts on. **Single-click selection keeps
  its own generous 250 cm sweep**: picking your own moving pawns is a different job, and nobody has
  asked for it to change.
- **Hover highlight** — local and cosmetic, never replicated (in co-op each player lights their
  own). Set a `HoverOverlayMaterial` (an `EditAnywhere` on the controller, assigned on
  `BP_StrategyPlayerController`) as the overlay material on the hovered actor's mesh components,
  and clear it when the hover moves. It must read as different from the *selected* look (units'
  `BP_UnitSelected`, containers' dark-green highlight). Hover clears while the cursor is over any
  HUD region or window — a click there never reaches the world — and while the camera is rotating.
  The material is a simple fresnel rim; make it via MCP. If MCP material authoring fights back, it
  is a two-minute hand step for Jim at the checkpoint.
- **Right-click** (`InteractClick`, release): resolver finds something → target it (see Task D) and
  open the menu at the cursor. Nothing → `DoMoveUnitsCommand`, as today. The hover is what tells the
  player which will happen.
- **`UActionMenuWidget`** (`SmoresUI`) — its own top-most layer owned by `AStrategyHUD`, like
  `URefusalWidget`, at **Z-order 50**: above every window (a menu opened over a world spot next to
  an open inventory must not draw behind it) and below the refusal line at 100, which was left room
  for exactly this. Two parts:
  - **A full-screen click catcher** that consumes the press **and the double-click** of every mouse
    button, then closes the menu. Per `input-and-keybinds.md`, a widget claiming a button must
    override `NativeOnMouseButtonDoubleClick` too, or the second click of a pair leaks to the world.
    The release is not swallowed, per the same topic. The catcher is what makes the closing click
    do nothing else.
  - **The panel** at the cursor, clamped to the viewport: the target's name and classification, who
    would go, then one row per entry. Rows reuse `UTargetActionWidget` / `WBP_TargetAction` (same
    bound names), which gains an optional `DetailText`.
- **Refresh** follows `hud-and-panels.md`'s per-frame push-and-compare rule. `DrawHUD` asks the
  selection host for the menu target's info (a new `GetTargetInfoFor(const AActor*)` beside
  `GetSelectionTargetInfo`), and the widget compares what it would draw before touching Slate. The
  menu has **its own target**, not `LastSelectionTarget`, because `Tab` can retarget the panel while
  the menu is open.
- **A pick** calls `IStrategyHUDCommands::RequestTargetAction`, which now takes **the target
  explicitly**. The panel passes its target, the menu passes its own. The controller rebuilds the
  entry, refuses if it has gone disabled, and otherwise issues the order (or the attack, or the
  examine).
- **Real time keeps running.** No pause, no pace change — `notifications-and-alerts.md`'s "never
  changes the simulation on the player's behalf" applies to menus too.

## Module Placement

| Piece | Module | Why there |
|---|---|---|
| `ISmoresInteractable` (display name, reach, examine text) | `SmoresCore` | Doors, containers, items and units all need one reach rule, and they span three modules |
| `ESmoresRefusalReason::NoOneSelected`, `CannotReach` + wording | `SmoresCore` / `SmoresUI` (`GetRefusalText`) | The existing split |
| `ECharacterKind` on `UCharacterDefinition`; `AStrategyUnit::IsPerson()`, `MoveToActor`, replicated `Attributes` and `Disposition` | `SmoresCharacters` | Per-unit, and where the definition lives |
| `UActionOrderComponent`, `IActionOrderHost` | `SmoresCharacters` | Per-pawn state; the interface keeps `smores` out of the dependency chain |
| `AWorldDoor` | `SmoresItems`, beside `AStrategyContainer` | The home of world objects the squad handles today. **Flag it to move** when a world/building module is cut — it isn't an item |
| `FTargetAction` fields, `UActionMenuWidget`, `UExamineWidget`, `UTargetActionWidget::DetailText`, the HUD layer | `SmoresUI` | UI |
| The rules (`BuildTargetInfo`, the gating predicates, the actor rule, the placeholder odds), the arrival dispatch, the hover/right-click routing | `smores` — the rules in a **new `Variant_Strategy/StrategyTargetActions.{h,cpp}`**, not in the controller | It needs every type below it. Its own file keeps the controller from growing |

**`SmoresAI` is still not cut.** `unreal-module-organization.md` names "the first non-combat
behaviour" as the trigger. A walk-over order is arguably that, but it is one small component whose
policy is entirely "go there, then hand back". When a job queue arrives, `UActionOrderComponent` is
its obvious seed and moves with it.

## Why This Is One Slice

Every task below is the same pattern applied across target types. None of them needs a human to
look before the next can start: the hover, the menu layout, the door and the walk-over feel are all
judged together at the end, and a change to any of them afterwards is cheap. No key mapping is
needed (right mouse is already mapped), and the Blueprint work — two WBPs, a door Blueprint, one
material, one placed test door — is MCP-drivable. So the only genuine stop is Jim's PIE pass at the
end. Commit points inside the slice are marked; one session can make several commits.

## Slice 1 — The Action Menu

### Task A: groundwork

- **`ISmoresInteractable`** in `SmoresCore`: `GetInteractionDisplayName()`, `IsInRangeOf(const
  AActor*)`, `GetExamineText()`. **`IInventoryHolder` derives from it**, and its two methods move up,
  so every existing holder is interactable with no call site broken and the reach rule stays written
  once. Verify `Cast<ISmoresInteractable>` resolves through the derived interface on a container, a
  unit and a world item — a test covers it. If interface inheritance misbehaves, fall back to the
  holders implementing both and forwarding, and record why.
- **Examine text:** a unit gives its definition's `Description`, plus `Backstory` for a person, plus
  its condition in words (hurt / downed / dead). **Never a stat** — no attributes, no skills, no
  health number. That's `player-interface.md`'s known-vs-hidden rule, and it applies to your own
  squad's Examine too, whose numbers belong to the character sheet. A container and a door get a
  new `ExamineText` property. A world item gives its item definition's `Description`. An empty
  result falls back to one shared "nothing remarkable" line.
- **`ECharacterKind { Person, Creature }`** on `UCharacterDefinition`, default `Person`.
  `AStrategyUnit::IsPerson()` reads it; a unit with no definition counts as a person. Definitions
  load on every machine, so no replication is needed. Add it to the content sweep if `game-data.md`'s
  sweep checks enums.
- **Replicate `Disposition`** — `combat.md`'s one-line fix (`UPROPERTY(Replicated)` +
  `DOREPLIFETIME`). The menu triples the UI that reads it, so the gap gets closed now rather than
  discovered.
- **Replicated `Attributes`** on `AStrategyUnit`: the working copy of the record's, set wherever
  `FactionId` is. Then give one test pawn a second, higher-Agility definition (trap 7).
- **Refusal reasons** `NoOneSelected` (*No one selected*) and `CannotReach` (*Can't get there*).
  Both are raised by this slice, so they pass the "only add a reason something raises" rule.

### Task B: the rules, moved and extended

- Move `BuildTargetInfo`, `IsLootableNPC`, `IsInteractableNPC`, `GetTraderStock` and
  `IsHolderInRangeOfUnits` into `StrategyTargetActions.{h,cpp}`. The controller calls them.
  `TargetInfoTest` follows them.
- Implement the rules table, the actor rule, `Detail`, the actor name, the placeholder flags and
  the container's rename to Loot. New ids beside the existing ones: `Trade`, `PickUp`, `Open`,
  `Close` (door — `Open` changes meaning from container to door), `Examine`, `Heal`, `Kidnap`,
  `Pickpocket`.
- **Tests** (`testing.md`'s standing rule — this is exactly the "an action row that quietly offers
  something the rules forbid" case the static was made public for): one test per row of the
  rules table; the actor rule's six cases; the odds placeholder's clamp and that it differs by
  actor. Expect today's `TargetInfoTest` assertions about *Too far away* to change — that is the
  decision, not a regression.

**Commit point.**

### Task C: walking over to act

- `UActionOrderComponent`, `AStrategyUnit::MoveToActor`, `IActionOrderHost`,
  `Server_RequestActionOrder`, and the arrival dispatch table above, including the cancellation rules.
- Split what arrival needs out of today's client-side and server-side entry points rather than
  copying them: a server `StartTalk(NPC, Listener)` from `Server_InteractWithNPC`; the listener
  parameter on `OpenTradeWith`; the pickup body from `Server_PickUpWorldItem`; `Client_OpenHolder`.
- **Design the component so its arrival logic is callable from a test** without navmesh. Test
  worlds have none, so a real walk can't happen. Teleport-then-complete is fine.
- **Tests** — it is a state machine with counted outcomes: in reach acts once, immediately;
  arrival in reach acts once; a player move cancels with no host call; going Downed fails with a
  notice; no path fails `CannotReach` after the retry limit; a target turned hostile before arrival
  refuses with *They won't deal with you*; an order for a unit this controller doesn't own is
  rejected server-side.

### Task D: every route goes through it

- `RequestTargetAction(Target, ActionId)` (the panel and the menu): non-Examine, non-Attack actions
  become orders for the resolved actor.
- **`T`**: the targeted NPC's Talk, through the same path. Its toggle-to-close behaviour is
  unchanged.
- **`O`**: the targeted container, body or door, through the same path. With nothing targeted it
  falls back to today's in-reach sweep, then *Too far away* — the one place that refusal is still
  raised by a key.
- **Double-click** (the resolver's hit, in ladder order): item → Pick up, container → Loot,
  door → Open/Close, body → Loot, standing NPC → Talk, empty ground → select all. All walk over. An
  out-of-reach double-click no longer refuses.
- **World items and doors become targetable** by a single click (they set `LastSelectionTarget`,
  and the panel shows them). This also fixes the single click on an item clearing the squad. A
  generic `SetTargetedActor` for types without their own highlight state; containers and NPCs keep
  `SetSelectedContainer` / `SetSelectedNPC`.
- `H` is untouched.

**Commit point.**

### Task E: the door

- **`AWorldDoor`** (`SmoresItems`, abstract): root, an optional frame mesh, a hinge pivot holding the
  leaf mesh, a reach sphere, `UPROPERTY(ReplicatedUsing = OnRep_Open) bool bOpen`, authority-only
  `SetOpen(bool)`, an `OpenYaw` (90°), `DoorDisplayName`, `ExamineText`, and `BP_DoorOpened` /
  `BP_DoorClosed` hooks for sound and animation. Implements `ISmoresInteractable`.
- **Pathfinding:** the leaf's collision affects navigation, and `RuntimeGeneration=Dynamic` rebuilds
  the navmesh as it swings. **Verify in PIE**: a chest behind a shut door gives *Can't get there*;
  opened, the squad walks through. If the rebuild proves unreliable, a nav modifier toggled with
  the door is the fallback.
- **Content (MCP):** `BP_WorldDoor` with engine basic-shape meshes, and in `LVL_Strategy` a short
  wall with the door in it and a chest behind it, so the path-blocking can be seen. Jim may move it.
- **Tests:** `SetOpen` is authority-gated and toggles; reach; the door's rules-table row.

### Task F: Examine

- **`UExamineWidget`** (`SmoresUI`), a `UWindowWidget` (drag, resize, close and click-swallowing come
  free): title, classification, body. **`WBP_Examine`** via MCP with the window-chrome names plus
  `ClassificationText` and `BodyText`. Kept after closing like the panel windows; re-examining
  something else rebinds it. `HandleWindowClosed` returns early for it, exactly as for
  `UHUDPanelWidget` — it must never touch the inventory input context.
- Entirely client-side: definitions and authored text are on every machine, so there's no RPC.

### Task G: the menu and the hover

- The resolver, `HoverPickRadius`, the hover overlay, the right-click routing, `UActionMenuWidget`
  (`WBP_ActionMenu` via MCP), `UTargetActionWidget::DetailText` (add the optional text to
  `WBP_TargetAction`), `GetTargetInfoFor`, and `DrawHUD`'s push.
- **The bindings are the usual silent single point of failure**: the menu's `EntryWidgetClass` and
  the HUD's `ActionMenuWidgetClass`. Warn once when unset, like `ActionWidgetClass` does.
- **Confirm the cursor trace hits pawns.** `SelectionTraceChannel` is what `GetLocationUnderCursor`
  uses and may only see the ground. The small-radius fallback covers it either way, but "exact actor
  first" should actually be exact.

**Commit point.**

### Task H: documents

- **New `game-systems` topic `action-menu.md`**: the rules table, the actor rule, walking over,
  cancellation, the menu layer, the hover resolver, and the placeholder list with how to replace
  one. Add a row to the skill's topic table.
- **Rewrite `strategy-unit-commands.md`** — stale, see What Exists Today — to cover move orders as
  they actually are, plus walking over to act.
- **`input-and-keybinds.md`**: the right-mouse and double-click rows; `O` now covering doors; the
  hover. **`UHelpPanelWidget::GetDefaultBodyText`** in the same change.
- **`hud-and-panels.md`**: the target panel's rows (pointing at `action-menu.md` rather than
  duplicating the table), the Z-50 menu layer, the Examine window.
- **`refusals-and-feedback.md`**: the two new reasons, the player-facing table, and the "where
  refusals are raised" list — several *Too far away* sites are gone.
- **`combat.md`**: `Disposition` is now replicated — close that Known Gap. **`game-data.md`**:
  `ECharacterKind` and the replicated attributes copy. **`inventory.md`** and
  **`strategy-camera-and-selection.md`**: the double-click ladder walking over, and items and doors
  as targets. **`testing.md`**: the new tests.
- Memory: a project memory for this roadmap, and an update when it ships.

**Commit point.**

### Checkpoint — Jim in PIE

1. **Right-click vs. move:** does the hover make it obvious which will happen? Does it ever fight?
   Is `HoverPickRadius` right? And the recommendation to confirm: **hover highlight, or
   hold-for-menu after all?**
2. **The hover look**: readable, and clearly not the selection look.
3. **The menu**: where it opens, how long it gets on a person (up to six rows), click-away closing.
4. **Walking over everywhere, double-click included**: does it feel right, or is an instant
   *Too far away* missed anywhere?
5. **The odds text**: readable, and does it visibly change between the two test pawns?
6. **The door**: opens, closes, blocks the path when shut.
7. **Examine**: is the window worth having, or would one line in the feed do?
8. **The target panel** with its longer rows.
9. **The template's arrival animation**: walk a squad member up *beside* a person (right-click the
   ground next to them) and watch for the leftover interaction animation. Keep it or retire it —
   see Open Questions. Jim chose 2026-09-29 to judge this in PIE rather than decide up front.

Answers get written into the game-design topic or the new `action-menu.md`, whichever owns them.

## Not in This Roadmap

- **Esc to close.** Esc is PIE's own stop key, and closing things with it wants to be one "back"
  stack (menu → window → system menu), designed once. Reserved in `input-and-keybinds.md` already.
- **The real Heal, Kidnap and Pickpocket.** Heal waits on injuries and medicine
  (`characters-and-squads.md`). Kidnap waits on carrying a body, slow and conspicuous. Pickpocket
  waits on NPC awareness, which is also what unblocks `inventory-roadmap.md` Slice 10. Each one
  replaces its placeholder flag and arrival line; the menu doesn't change. **When the real rolls
  land**, they come from the same function as the displayed odds and follow the save-scum rule —
  the same conditions give the same result, seeded from `UWorldSeedComponent`.
- **Queueing orders with `Shift`.** It's the modifier convention for "queue", but there's no queue.
  It arrives with `orders-and-jobs.md`.
- **Locked doors, and units opening doors on their own.** A shut door is a wall to pathfinding
  until someone is ordered to open it. `Locked` is the likely next refusal reason
  (`refusals-and-feedback.md` already predicts it).
- **Word-based descriptions of a target's ability** — *a difficult lock*, *looks alert*. They're
  the known-vs-hidden rule's other half, and they arrive with locks and NPC awareness. Examine is
  where they'll show, so they'll add a line to what Examine displays, not new plumbing.
- **Giving items to a squad member, or trading between players.** Own-squad and other-player
  targets get Heal / Examine only.
- **Touch.** There is no right mouse button on touch, and touch is incidental to the control scheme.
- **A cursor change** (a hand over a chest, a sword over an enemy) alongside the hover. It's the
  other half of the RTS convention and cheap later if the overlay alone doesn't read.
- **Door state surviving a save** — there's no save system yet.

## Resolved Design Decisions

- **The menu is a view of the target panel's list, not a second list.** One rules function builds
  both. (Rejected: menu-specific rules; they would drift.)
- **Visible placeholders for Heal, Kidnap, Pickpocket** — Jim, 2026-09-29. (Rejected: leaving them
  off, which would leave the per-member text with nothing on screen to judge; building simple real
  versions, which would reopen parked theft.)
- **Every route walks over, double-click included** — Jim. (Rejected: menu, panel and keys only;
  menu only.)
- **Nearest selected acts; Attack is everyone** — Jim. (Rejected: choosing the member inside the
  menu, which makes the menu long.)
- **A simple door now** — Jim. (Rejected: doors later with base building.)
- **Hover highlight over hold-for-menu** — recommended 2026-09-29 and pending Jim's confirmation.
  Hold is slow, undiscoverable, and on PC often means camera. The RTS smart right-click with a
  visible hover is the common convention, and RimWorld's right-click options list is the nearest
  match to this design.
- **The actor updates live, not locked when the menu opens** — a locked actor would be wrong by the
  time of the click.
- **Walking over uses a goal-actor move, not `MoveToLocation`** — EQS's random best-25% can end out
  of reach, and a goal actor follows a moving target for free.
- **Container's action renamed Loot; Open/Close is doors only** — Jim's own vocabulary.
- **Odds shown as a number; a target's stats never shown** — Jim, 2026-09-29, relaxing the diegetic
  rule for numbers the player already knows (their own squad's). (Rejected: words only, which
  would hide what the player can already read off the character sheet.)

## Open Questions Worth Tracking

- **Should `bInteractOnArrival` / `BP_InteractionBehavior` be retired?** It's the Strategy
  template's leftover: the lead unit of any move order plays an interaction animation with
  whichever unit it ends up next to. With right-click on a person now opening the menu, it only
  fires when someone walks up *beside* a person, which will read as a glitch. A PIE call for Jim.
- **Should a single-click select share the hover resolver too**, so what's lit is also what a left
  click picks? Deliberately not changed here — selection's generous sweep exists for picking small
  moving units.
- **Should the right-click that closes the menu also open a new one** at the new spot? This
  roadmap swallows it — simplest and predictable. A PIE feel call.
