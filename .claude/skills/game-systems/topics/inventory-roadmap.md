# Inventory & Item Systems Roadmap

## Purpose

Unlike `inventory.md`, this topic isn't documenting built behavior — it's a **forward-looking
target design**, the result of a brainstorming pass against the `game-design` skill (mainly
`economy.md`, `characters-and-squads.md`, and `combat.md`) and the actual current
`UInventoryComponent`/`SmoresUI` implementation. It exists so the next several rounds of
inventory programming work have a settled target to build toward instead of re-litigating
the same design questions piecemeal per feature. Treat every section as "what to build
next," not "what exists" — **except** where a heading is marked SHIPPED, which means that
section has moved to `inventory.md` and only its summary line remains here.

The work is deliberately cut into **one slice per clean session** (see "Implementation
Order" below). Each session should start by reading `inventory.md`, this file's slice
entry, and the current source — not the conversation that produced this document. When a
slice ships, move its content into `inventory.md`, delete it from here, and mark the slice
`DONE` in the order list (the same convention `unreal-module-organization.md` uses).

## Item Definitions vs. Item Instances — **SHIPPED (Slice 1)**

The definition/instance split is built; see `inventory.md`. The one piece of it still
outstanding is per-instance **grid anchor cell + rotation**, which arrives with the grid
itself in Slice 2.

## Grid-Based Storage (Bulk) and Stacking — **SHIPPED (Slices 2–3)**

The component-side grid shipped in Slice 2 and its UI — footprint-spanning item widgets, the
rotate-while-dragging key, and the drop preview — in Slice 3. See `inventory.md`. The one
piece still outstanding is **partial-stack drag**: `MoveItem` takes a quantity and splits
correctly, but the UI always passes "whole stack" because there's no designed way for the
player to say how many.

## Weight & Encumbrance — **SHIPPED (Slice 4)**

Weight tracking and the per-holder capacity readout shipped in Slice 4; see `inventory.md`.
The deliberate deferral stands: **effects are not this system's job**. Movement-speed and
stealth-noise penalties for a heavily-laden pawn belong to a later characters/combat pass,
which owns those effects — `IsOverWeightCapacity` is the seam it reads.

## Equipment / Worn Slots — **SHIPPED (Slice 5)**

The paperdoll, right-click-to-equip, drag-onto-a-slot, and the clean-fail swap all shipped in
Slice 5; see `inventory.md`. The deliberate deferrals stand: **no skill gating at equip time**
(slot-type matching is the whole rule, and an untrained equip's penalty belongs to
combat/skill resolution), and **no equipped visual** — attaching the definition's 3D mesh to a
skeletal socket is cosmetic polish nobody has needed yet. Combat reading the equipped weapon
is a combat-system change, not an inventory one; `GetEquippedItem` is the seam it reads.

## Currency — **SHIPPED (Slices 4 and 8)**

The per-player gold balance shipped in Slice 4 and the purchase/sale transaction that moves it
in Slice 8, which also relocated the balance into a `UWalletComponent` in the new
`SmoresEconomy`; see `inventory.md`. Possession restrictions (robbable cash, per-division
budgets) remain explicitly deferred; the shared-pool model is unchanged.

## Unified Transfer Interface

One transfer mechanism — extending today's drag-and-drop `MoveItem`/`IInventoryMoveHost`
plumbing to move a footprint of cells between two grids instead of swapping two indices —
serves every context below. Each context layers additional rules on top of the same
underlying move/copy operation rather than inventing its own UI:

| Context | Direction | Currency? | Extra gating |
|---|---|---|---|
| Pawn ↔ pawn | free move | no | proximity only |
| Loot Downed or dead body | free take | no | proximity; target must be Downed **or dead** — **SHIPPED (Slice 7)**, exactly as decided: same actor, same code path, no corpse container |
| Container transfer | free move | no | proximity (already implemented, `InteractionRange`) |
| Trade with an NPC (buy and sell) | two-way exchange | yes | proximity + not hostile + on its feet — **SHIPPED (Slice 8)**, exactly as decided: `IPricingProvider` with a flat buy markup / sell markdown, behind which the real market system (`economy.md`) drops in later without touching transfer code. The "storefront" row this table used to carry separately collapsed into this one: a shop is an NPC with a `UTraderComponent`, its stock is that component's own grid, and it is distinct from the NPC's personal belongings exactly as required |
| Steal from an unsuspecting NPC | free take | no | proximity; gated by detection/awareness rules per `characters-and-squads.md`'s pickpocketing design (skill vs. target awareness/crowd density, immediate witnessed-failure consequence); sets the stolen flag on the taken item(s). Blocked on NPC awareness systems that don't exist yet |

- **All transfers require proximity to the target** — **SHIPPED (Slice 7)**. The
  `IInventoryHolder` interface (`GetHolderDisplayName` + `IsInRangeOf`, and deliberately no
  grid accessor) is implemented by pawns, containers and world pickups, and every proximity
  path on the controller is now holder-generic. A storefront or a traded-with NPC implements
  the same two methods and inherits the whole thing; see `inventory.md`.
  `characters-and-squads.md` is explicit that inventory transfer during theft requires physical
  proximity too (no handing stolen goods to a mule waiting outside) — the same rule already
  fits every other context, so one proximity check serves all of them.
- **Stolen-item flag** — set on an item taken via theft; recognized within the origin
  faction's territory (sellable at a discount through fences, full price to distant
  factions unaware of the theft), and quietly expires after enough in-game time per
  `characters-and-squads.md`. Territory recognition and expiry depend on faction/world
  systems that don't exist yet — this round only carries the flag.

## World Pickups — **SHIPPED (Slice 6)**

`AWorldItem`, double-click-to-collect, and the proximity gate all shipped in Slice 6; see
`inventory.md`. The deliberate deferral stands: **the player can't yet put an item back**.
`AWorldItem::SpawnWorldItem` and a `WorldItemClass` on the controller exist and are exercised by
the `SmoresDropItem` debug exec, but the drag-an-item-onto-the-world gesture is unbuilt — the
obvious hook (`UDragDropOperation::DragCancelled`) fires for *every* unhandled drop including an
Escape-cancelled one, so it needs a designed confirmation before it's safe to wire.

## Sort & Filter (Inventory UI) — **SHIPPED (Slice 9)**

The server-side repack and the client-side category filter both shipped in Slice 9; see
`inventory.md`. The deliberate omission stands: **no name or category sort** — the three criteria
are the three *figures* an item carries, and an alphabetical sort only earns its place once the
grid has icons to make names worth scanning past.

## Explicitly Out of Scope for This Round

- **Save/storage backend** (SQLite vs. UE's native `SaveGame`, or anything else) — this is
  a save-system-wide decision per `save-system.md` ("exact schema/serialization format is
  an implementation decision, not a design one"), not something to bake into the item
  system specifically. This system should stay plain `UPROPERTY`-reflected data (as it is
  today) so whatever the save system eventually decides can serialize it unchanged.
- **Item degradation/condition** — the `Condition` field exists on the item instance as a
  placeholder only; no functional durability/upkeep mechanics yet (`combat.md` calls for
  this eventually).
- **Encumbrance effects** — weight is tracked and displayed, but speed/noise penalties
  belong to a later characters/combat pass (see Weight & Encumbrance).
- **The crafting system itself** — out of scope here, but kept in mind: a "Material/
  Component" item category exists specifically to feed it, and needs to support large
  stack sizes for bulk crafting inputs (`tech-and-crafting.md`).
- **Currency possession restrictions** — robbable cash, division-level budgets. Noted as a
  possible future layer on top of the simple shared-pool model above, not designed now.
- **Theft detection/awareness** and **stolen-flag territory recognition/expiry** — the
  flag is carried; everything that reads it waits on faction/NPC-awareness systems.

## Relationship to the Current Implementation

What carries forward largely unchanged: authority-gated mutation + replication, the
`OnInventoryChanged` delegate pattern (Slice 5 cloned it wholesale for `OnEquipmentChanged`),
`UWindowWidget` floating-panel chrome, and the
overall "drag from one slot widget, drop on another, server validates and applies" shape —
and now the narrow-interface seam between `smores` and `SmoresUI` (Slice 4 added a fourth,
`IStrategyResourceHost`, rather than moving a gameplay class down a module).
Already reworked in Slice 2: `UInventoryComponent`'s flat index model (now a 2D grid with
placement/collision) and `IInventoryMoveHost::Server_MoveInventoryItem`'s signature (now
entry id + cell + rotation + quantity). Already reworked in Slice 3: the interim
one-widget-per-cell UI (now a `UGridPanel` with a cell layer and a footprint-spanning item
layer, one grid-level drop target, and a drop preview). Already reworked in Slice 7: the three
duplicated `IsUnitInRange` distance tests and three
differently-named display-name getters (→ one `IInventoryHolder` with one shared static test),
the controller's six ad-hoc finder methods (→ one `FindHolderActorAtLocation` plus
`FindPlayerPawnInRangeOfHolder`/`IsHolderInRangeOfSelection` and thin per-type wrappers), and
`UHealthComponent`'s `bIsDowned` bool (→ an `EHealthState` enum, so a body can be Dead).
Already reworked in Slice 8: `AStrategyPlayerState::Gold` (→ a `UWalletComponent` in the new
`SmoresEconomy`, which let `IStrategyResourceHost` be deleted rather than extended — the fourth
narrow interface turned out to be the one that didn't need to exist), `MoveItem`'s bool return
(→ `MoveItemCounted`, so a caller can charge for what actually moved), `FindLootableNPCAtLocation`
(→ `FindNPCAtLocation` plus a branch, so one sweep serves both a body and a living NPC), and the
click-an-Aggressive-NPC-to-attack shortcut (removed, per `input-and-keybinds.md`).
Already reworked in Slice 9: `CanPlaceAt`/`FindFreePlacement` (→ thin forwarders over
`CanPlaceAgainst`/`FindFreePlacementAgainst`, which take the placement array to test against, so
a repack can ask "would this fit?" about an arrangement that isn't live yet) and the inventory
debug exec's grid dump (→ a shared `LogInventoryGrid`, so the sort exec reports identically).
Nothing structural is left needing rework — what remains in the list below is new building.

## Implementation Order

Dependency-ordered slices, **one per clean session**. Every slice follows the same
protocol, so it isn't repeated per entry:

1. Read `inventory.md`, this slice's entry, and the current source files it names.
2. Write the C++ first (CLAUDE.md's "C++ first" rule). Every slice here adds new
   `UCLASS`/`USTRUCT` types, so expect a **cold Visual Studio build** (close the editor,
   build, reopen) — never Live Coding.
3. Wire Blueprints/assets via `unreal-mcp` (`mcp-workflow` skill). After any reparent or
   moved property, diff the affected Blueprint's class defaults *and* placed level instances
   against expectations — see `unreal-module-organization.md`'s per-move mechanics for the
   two known silent-failure modes.
4. The user PIE-tests the player-facing behavior (drag/rotate feel, window flow); the agent
   verifies compile, wiring, and property state.
5. Commit the slice on its own, then move its shipped content from this file into
   `inventory.md` and mark the slice `DONE` below — in the same commit or the next.

Slice status is tracked inline; update it when a slice ships.

### Slice 1 — Item definitions and instances — **DONE**

Shipped; see `inventory.md`. Two notes worth carrying forward:

- `StartingItems` no longer has any C++-authored defaults — hard-coding content paths in a
  constructor is exactly what CLAUDE.md's "C++ first, Blueprints for wiring" rule rules out.
  Every holder's starting contents are Blueprint/instance-authored now.
- Reshaping `FInventoryItem` silently voided **per-placed-instance** `StartingItems`
  overrides on four actors in `LVL_Strategy` — "Chest 2" held three entries that
  deserialized to null definitions, and "Chest 1" plus both placed `BP_PlayerUnit` actors
  held *empty*-array overrides that quietly shadowed the new Blueprint defaults. Slice 2 did
  **not** reshape `FInventoryItem` (it added `FInventoryEntry` around it instead), so those
  overrides survived untouched — but Slice 5 may still touch it: grep
  `Content/__ExternalActors__/` for `StartingItems` first, and don't assume Blueprint class
  defaults are the only authored copies.

### Slice 2 — Grid storage, footprint, rotation, stacking (component side) — **DONE**

Shipped; see `inventory.md`. Notes worth carrying forward:

- **`MoveItem` deliberately has no swap path.** Two differently-shaped footprints have no
  well-defined exchange, so a drop resolves to merge / reposition / place, and anything else
  is rejected whole. Later slices that add a transfer context (purchase, loot, steal) should
  layer their gating *in front of* `MoveItem` rather than adding a fourth resolution to it.
- **Rejection is silent.** The server mutates nothing and sends nothing back; the client's
  next refresh redraws unchanged replicated state, which is what makes the item appear to
  snap back. Slice 8 proved the cost: a purchase refused for insufficient gold is
  indistinguishable on screen from one that simply didn't fit, and there is not even a red
  preview to hint at it, since the cells were fine. Explaining *why* still needs a new client
  RPC — it can't be inferred from the absence of a change.
- **Rotation is only half-shipped.** `FindFreePlacement` uses it; the player can't yet
  invoke it. Slice 3 owns the rotate key and the footprint-spanning item widget.
- Verify with the `SmoresDumpInventory` / `SmoresAddItem <Count>` console execs on
  `AStrategyPlayerController` — they run server-side and log an ASCII occupancy map.

### Slice 3 — Grid UI: cells, footprints, drag with rotate — **DONE**

Shipped; see `inventory.md`. Notes worth carrying forward:

- **The open question is settled: a drop stays literal.** An item that doesn't fit is
  rejected, never auto-rotated to make it fit. What made it safe to settle that way is the
  drop preview — the covered cells turn red, so "press R" is visible rather than something
  the player has to guess. Without the preview, literal would just read as broken.
- **The rotate key is a normal Enhanced Input action** (`IA_Strategy_RotateDraggedItem`),
  mapped in `IMC_Strategy_Inventory`, which `AStrategyPlayerController` adds at priority 1 only
  while an inventory window is open and removes when the last one closes. It shipped as a Slate
  input pre-processor and was moved the same day, once player-rebindable keybinds became a
  commitment — a pre-processor is invisible to Unreal's player key-mapping system, so it could
  never appear in a settings screen. **Any later mid-drag key (split-stack, quick-transfer)
  should follow this pattern**, not the pre-processor.
  - What makes it work: a `NativeOnKeyDown` on the window really would never fire (the window
    doesn't hold keyboard focus), but Enhanced Input sits at the *end* of the focus path, and
    on a click Slate walks up from the non-focusable inventory widgets to the game viewport,
    which does take focus. So the viewport holds the keyboard throughout a drag.
  - The controller reaches the in-flight drag through `UInventoryDragDropOperation::GetActiveDrag()`,
    which asks Slate for the current drag content. Slate owns the drag — nothing else holds a
    reference to it — so that static is the only bridge, and it deliberately lives in `SmoresUI`
    so the UMG drag plumbing doesn't leak into the controller.
- **`SlotContainer` must be a `UGridPanel`.** `UUniformGridSlot` has no row/column span, so a
  uniform grid cannot host a footprint-spanning child at all. The C++ casts and silently
  degrades to a flat list if the cast fails — if a new holder's WBP shows items in a list and
  drops do nothing, that's the cause.
- **Ghost and drop agree by construction**, not by coincidence: the drag carries the footprint
  cell the pointer grabbed, the anchor is the hovered cell minus it, and the decorator is
  positioned by the matching fraction against `EDragPivot::TopLeft`. A mid-drag rotate
  transposes both together. Changing either half alone will desync them.
- Partial-stack drag is still unbuilt — `MoveItem` supports the split, the UI always sends 0
  ("whole stack"). It needs a player-facing way to choose a quantity, which is a design
  question, not a plumbing one.
- Slice 5 touches the item widget again (right-click to equip). The right-click hook goes on
  `UInventoryItemWidget`, which is now the per-entry widget — there's no longer any per-cell
  widget that knows what item it's under.

### Slice 4 — Weight tracking and currency — **DONE**

Shipped; see `inventory.md`. Notes worth carrying forward:

- **`AStrategyPlayerState` went in `smores/Variant_Strategy/`, not `SmoresCore`.** It's a
  Strategy-variant framework class like `StrategyGameMode`/`StrategyPawn`/
  `StrategyPlayerController`, and `SmoresCore`'s charter is explicitly "no gameplay-specific
  logic". The cost of that choice is that `SmoresUI` can't see the type, which is paid with a
  fourth narrow interface, `IStrategyResourceHost` (`GetPlayerGold`), declared in `SmoresUI`
  and implemented by `AStrategyPlayerController` — the same pattern
  `unreal-module-organization.md` records for the other three. **Slice 8 deleted that interface
  again** by moving the balance into a `UWalletComponent` in `SmoresEconomy` — a component in a
  module `SmoresUI` already depends on needs no seam at all. The lesson is worth keeping: an
  `I*Host` interface is the right answer for *behavior* on the controller and the wrong one for
  per-player *state*, which wants a component.
- **`StrategyGameMode.*` needed no C++ change.** The roadmap predicted one, but the project's
  abstract-C++-class/BP-subclass convention means `PlayerStateClass` is assigned on
  `BP_StrategyGameMode` instead. Nothing in C++ references `AStrategyPlayerState::StaticClass()`
  — so if that Blueprint assignment is ever lost, the failure is silent at compile time and
  shows up only as a permanent `Gold: 0` plus a warning from the gold execs.
- **Setting a class default on a Blueprint whose placed instances predate the property writes
  a stale per-instance override.** Changing `BP_Chest`'s `WeightCapacity` 30 → 0 and compiling
  re-instanced both placed chests in `LVL_Strategy`, and the re-instancing copied their *old*
  inherited value (30) forward as a genuine override, silently shadowing the new class default.
  Caught by `grep -arl "WeightCapacity" Content/__ExternalActors__/` returning two files. Fixed
  by setting the intended value explicitly per instance and re-saving — the delta then matches
  the CDO, so nothing serializes and the grep comes back clean. This is the `mcp-workflow`
  skill's documented hazard firing on a plain class-default edit, not just on a module move:
  **run that grep after every class-default change to a property placed actors also carry.**
- The player-facing readouts are `BindWidgetOptional` text blocks (`GoldText` in `UI_Strategy`,
  `WeightText` in both inventory WBPs) that C++ fills directly — no Blueprint property
  bindings. That's a deliberate departure from `UI_Strategy`'s older
  `SelectionCount`/`SelectionTargetText` pattern, which binds BP-side to a `BlueprintPure`
  getter; the C++-fills-it shape matches `UInventoryWidget`'s and needs no graph work to wire.
- `SmoresAddGold <Amount>` / `SmoresSpendGold <Amount>` are the console execs, and
  `SmoresDumpInventory` now logs carried weight against capacity too.
- **`Gold` was inline on `AStrategyPlayerState` until Slice 8 moved it into a
  `UWalletComponent`.** One `int32` didn't justify a component for the first piece of per-player
  state and would have been indefensible for the fourth; the squad roster, faction standing and
  research progress still to come each want a component of their own. See
  `unreal-module-organization.md`'s "Framework Classes vs. Feature Modules". **Don't add an
  inline field to that class — write the component instead.**

### Slice 5 — Equipment component and paperdoll — **DONE**

Shipped; see `inventory.md`. Notes worth carrying forward:

- **The `UWindowWidget` blocker is cleared, and the fix is asymmetric on purpose.** The *press*
  is now swallowed for every mouse button; the *release* is deliberately still let through.
  Swallowing the release too looks tidier and is actively dangerous: if the press reached the
  viewport (a drag-select started on the world and ended over a window), eating the release
  leaves that button stuck down in `UPlayerInput` forever. Letting it through is safe because
  Enhanced Input never saw the press this window ate, so an action bound on release — which is
  most of them — has nothing to complete.
- **Claiming a mouse button takes two overrides, not one.** `NativeOnMouseButtonDown` does not
  see the second click of a rapid pair — Windows sends `WM_xBUTTONDBLCLK`, which Slate routes as
  `OnMouseButtonDoubleClick` on a separate pass. Found via a double right-click on an item
  leaking a move order to the world *after* the press fix was already in. Every widget claiming
  a button now routes its double-click handler into its press handler. Worth remembering because
  the symptom is intermittent by construction: it only fires inside the OS double-click time and
  slop rectangle, so a slightly slower repeat looks like the bug fixing itself.
- **Right-click is routed by *which window* the item is in, not by what the item is.**
  `UInventoryWidget::SetEquipmentTarget` is set by the controller only for a pawn's own window,
  so right-clicking in a chest or a Downed NPC's loot panel does nothing rather than dressing
  the holder. Every later right-click/modifier transfer gesture wants the same shape: the
  controller decides what a window *is*, the widget only routes.
- **The paperdoll is its own `UWindowWidget`, not a panel inside `WBP_Inventory`.** That avoided
  widget-tree surgery on an existing WBP, and it matches how a container and a pawn inventory
  already open side by side. Cross-window drag needs nothing special — Slate's drag is global.
- **The paperdoll is scoped to the inventory key.** It first shipped opening with *any* pawn
  inventory window, which meant opening a chest put three panels on screen. `OpenInventoryForPawn`
  now takes `bOpenEquipment`, true only from `ToggleInventory`; the container and loot paths pass
  false and *close* any open paperdoll, because those paths rebind the pack to the nearest pawn
  rather than the selected one and would otherwise leave the paperdoll describing somebody else.
- **A second window forced `UWindowWidget::OnWindowClosed` into existence.** With two windows
  opening together, the X button became a real bug rather than a latent one: closing the pack
  left the paperdoll floating alone, and neither close re-scoped the inventory input context.
  The delegate is the general fix — a window announces its own close, the controller decides
  what that means. Any future companion window should ride it rather than adding a second
  mechanism.
- **`Slot` is a reserved member name on any `UWidget`** (`UWidget::Slot`, its layout slot). UHT
  rejects a shadowing `UPROPERTY` outright, and MSVC rejects even a loop variable named `Slot`
  in a widget method as C4458-as-error. Name equipment-slot members/locals `EquipSlot`.
- **Equipped weight is tracked but not folded into the carried total.** Wearing an item takes it
  out of the grid, so the pawn's `Weight:` readout drops; `UEquipmentComponent::GetTotalWeight`
  is reported separately in the paperdoll window. Harmless while weight is inert, but the
  characters/combat pass that gives weight a *consequence* has to sum both — that's the moment
  to decide whether the two readouts merge.
- No new keybind was needed: right-click inside a widget reads straight off the click event and
  is explicitly "not a binding" per `input-and-keybinds.md`.
- Verify with the `SmoresDumpEquipment` / `SmoresEquipItem <EntryIndex>` / `SmoresUnequipItem
  <SlotIndex>` console execs on `AStrategyPlayerController` — all server-side, like the grid ones.

### Slice 6 — World pickups — **DONE**

Shipped; see `inventory.md`. Notes worth carrying forward:

- **`AddItem`'s bool return is a trap for any caller that still holds the source.** It returns
  true only when the *entire* quantity landed, but a partial add keeps what fit — so
  "`if (AddItem(...)) Destroy();`" silently deletes the units that didn't make it, and
  "`else` leave it alone" duplicates the ones that did. Slice 6 split out `AddItemCounted`,
  which reports the quantity actually taken, and `AddItem` is now a one-line forwarder.
  Slice 8's purchase hit the same trap one level up and needed `MoveItemCounted` for it, since a
  drag lands at a named cell rather than auto-placing. **Assume any other "did it work?" bool in
  this system is hiding a quantity until checked.**
- **The stretch was deliberately left out, and the reason is the hook, not the effort.**
  Dragging an item onto the world would ride `UDragDropOperation::DragCancelled`, which fires
  for *every* unhandled drop — a drag cancelled with Escape, or one whose window closed
  mid-drag, is indistinguishable from a deliberate release over the ground. Wiring it naively
  turns a stray click into a dropped item. The authority-side half (`SpawnWorldItem`,
  `WorldItemClass`, spawn-then-remove ordering) is built and exercised by `SmoresDropItem`, so
  whenever the gesture is designed only the front half remains.
- **Double-click now means four things, resolved by type.** The order is world item →
  container → Downed NPC → select-all-on-screen, and the world item goes first because it's the
  smallest target and the only one with no selection state to set. It also gets its own tighter
  `WorldItemSelectionRadius` (100 vs. the container's 250): sharing one radius let an item lying
  near a chest swallow every double-click meant for the chest. **Any later double-click meaning
  needs both a position in that order and a radius sized to the thing it hits.**
- **`Server_PickUpWorldItem` is the first inventory RPC that validates anything.**
  `Server_MoveInventoryItem`/`Server_EquipItem` deliberately do none, deferring to
  `MoveItem`/`Equip`; a pickup has no such validator behind it, because proximity *is* the rule
  and `TryPickUp` can't know how far away the asking pawn was. It re-checks range and that the
  destination is a player pawn's own pack. Slice 7's holder-generic transfers inherit that
  shape.
- **A spawned actor has to replicate; a placed one gets away without it.** `AWorldItem` sets
  `bReplicates` because it appears and vanishes during play. `AStrategyContainer` never has —
  which nobody has noticed because placed actors exist on clients regardless, and multiplayer
  isn't testable yet. Noted in `inventory.md`'s Known Gaps as a holder-wide multiplayer pass,
  not a Slice 6 fix.
- **A world item can't be scaled down without breaking its own reach.** `ItemMesh` is the root
  and `InteractionRange` is attached to it, so shrinking the mesh shrinks the pickup sphere with
  it — silently, since nothing reports a reduced radius. The placeholder spheres are therefore
  left at full size. If world items ever need to be smaller than their reach, `AWorldItem` needs
  a plain `USceneComponent` root with the mesh and sphere as siblings; doing that after instances
  are placed moves the root out from under them, so it's cheapest to do before there's much
  placed content.
- **Colour rides on the component, not the definition.** `UItemDefinition` has a `WorldMesh` and
  deliberately no material field; `BP_WorldItem` sets `OverrideMaterials[0]` in its class defaults
  instead, and that override survives `RefreshMesh`'s runtime `SetStaticMesh`. Any later
  per-item-type visual (a rarity tint, a stolen-goods shader) should ask whether it really needs a
  new definition field before adding one.
  - The catch, found the hard way: **a material override won't stick to a component whose
    `StaticMesh` is null.** Zero mesh means zero material slots, so the engine drops the entry and
    the write reports success anyway — the same silent-write shape `mcp-workflow` records for
    clearing an IMC's mappings. The fix is to give the component template a default mesh purely so
    the slot exists; `RefreshMesh` overwrites the mesh itself on construction, so the default's
    *value* never matters and the whole thing reads as dead weight to anyone tidying up later.
- **`OnConstruction`, not just `BeginPlay`, drives the mesh.** A placed `AWorldItem` picks its
  mesh from whatever definition the instance holds, so authoring `Item` in the level has to show
  up in the viewport immediately — otherwise every placed instance looks identical until PIE.
- Verify with the `SmoresDropItem <EntryIndex>` console exec, which drops a carried entry on the
  ground in front of the pawn — server-side, like the other item execs. It spawns *first* and
  removes the grid entry only on success, so a refused spawn can't destroy the item.

### Slice 7 — `IInventoryHolder` and loot-dead — **DONE**

Shipped; see `inventory.md` (the holder interface and the finder collapse) and `combat.md` (the
`EHealthState` enum). Notes worth carrying forward:

- **The interface shipped exactly as designed: two methods, no grid accessor, no click radius.**
  Both exclusions earned their keep immediately. `AWorldItem` still has no `UInventoryComponent`,
  so a `GetInventory` would have had to lie from one of the three implementers; and the click
  radius stayed on the controller, which is what let `FindHolderActorAtLocation` take it as a
  parameter rather than every holder type having to carry an input-tuning field it has no
  business owning.
- **"They share a shape, not a policy" was the whole difficulty, and the fix is a predicate
  parameter.** Six finder methods collapsed into one shared body plus thin wrappers, with each
  method's own rule surviving as a lambda or an explicit branch: the container sweep still
  prefers `SelectedContainer`; `FindLootableNPCInRange` still refuses to sweep at all (only
  `SelectedNPC` is ever a candidate, so a key press can't open whichever corpse happened to be
  nearest); the world-item finder still skips definition-less items. **A future holder that
  needs a seventh rule adds a predicate, not a seventh method.**
- **`FindClosestPlayerPawn` was correctly left out of the collapse.** It answers "who opens this
  panel", not "who may touch this" — no proximity gate at all. It looks like the others and is
  not one of them.
- **`IsLootableNPC` is the one place "lootable" is written down** (not a player pawn, Downed or
  Dead). Slice 10's theft context and any faction-based restriction extend that predicate rather
  than each `Find*` method.
- **The enum was the right call over a second bool, and the cost landed where predicted:** a
  replicated property's type changed *and* its `OnRep` signature changed (`OnRep_IsDowned(bool)`
  → `OnRep_HealthState(EHealthState)`), which is a cold-build-only change. The payoff is
  `IsIncapacitated()` — every existing `IsDowned()` guard across `StrategyUnit`,
  `CombatComponent` and the controller became an `IsIncapacitated()` guard, so Dead inherited
  the entire inert-unit ruleset for free instead of needing a parallel check bolted on at each
  of the eight call sites.
- **Nothing in the damage path kills, deliberately.** `Kill()` exists, is authority-only and
  terminal, and is reached only by the `SmoresKillNPC` exec. Wiring a lethal hit to it would
  have meant deciding what kills a unit in play — a combat/characters question that
  `character-death-and-permadeath.md` hasn't answered. This slice needed "a body can be
  looted", and that's what it built.
  - The one trap found while building it: a kill landing on an **already-Downed** unit must
    cancel the recovery timer already in flight, or the corpse stands back up a few seconds
    later. `Kill()` clears it and `Recover()` also refuses to run while Dead — two guards
    because the failure is silent and delayed, which is the worst kind to debug.
- **Downed and Dead are told apart in exactly one place: the loot window's title.** Everywhere
  else they're deliberately indistinguishable. If a second place ever needs the distinction,
  that's worth noticing — it probably means a real rule is being added, not a cosmetic one.
- **The `AStrategyContainer::bReplicates` one-liner was taken** while the file was open, as the
  entry allowed. It's no longer in `inventory.md`'s Known Gaps.
- Verify with `SmoresKillNPC` (click an NPC to target it first) — server-side, like the other
  execs. The killed NPC drops to the grounded pose, never recovers, and double-clicking it
  opens "<Name> (Dead)" with its inventory intact.

**Left undone on purpose:** a dead unit never despawns and keeps its pack forever. Body lifetime
is a combat/characters design question, recorded in `combat.md`'s Known Gaps.

### Slice 8 — Storefront, purchase, and trade — **DONE**

Shipped; see `inventory.md` (the trade system, the wallet, the pricing interface) and
`unreal-module-organization.md` (the `SmoresEconomy` cut). It shipped whole rather than split in
two, which the entry had allowed for — the module + wallet half turned out to be genuinely
mechanical, and doing it first is what made the storefront half cheap. Notes worth carrying
forward:

- **Standing the module up cost almost nothing; what it *bought* was the interface deletion.**
  `SmoresEconomy` is the first module cut for an ownership reason rather than as part of the
  original migration, and the trigger held up exactly as `unreal-module-organization.md`
  predicted: a wallet, a price and a transaction are one coherent domain that needs `SmoresItems`
  and nothing else. Moving gold into it let `IStrategyResourceHost` be **deleted** rather than
  extended — `AStrategyHUD` now reaches the balance through
  `PlayerState->FindComponentByClass<UWalletComponent>()`, because `APlayerState` is an engine
  type every module can see. **Weigh that trade whenever a fifth `I*Host` interface is about to
  be added: if the thing the UI wants is *state* rather than *behavior*, a component in a module
  `SmoresUI` already depends on beats an interface.**
- **`UTraderComponent` derives from `UInventoryComponent`, and that decision paid for the whole
  slice.** A shop shelf *is* a grid, so the stock replicates, opens into the existing window, and
  is dragged in and out of through the existing `MoveItem` with no new UI, no new RPC and no new
  drag payload. The client half of a purchase is byte-for-byte the client half of moving an item
  into a chest. **The transaction is recognised server-side**, where the money is: exactly one
  end of a move being a `UTraderComponent` is what turns it into a purchase or a sale. That is
  the "layer gating in front of `MoveItem`, don't add a fourth resolution to it" rule from Slice
  2, and it is why the UI needed no gesture of its own.
- **`MoveItem`'s bool return was the trap Slice 6 warned about, one level up.** Slice 6 recorded
  that `AddItem` returning true doesn't mean the whole quantity landed; `MoveItem` has the same
  hole in its merge branch (`Merged = min(Space, MoveQuantity)`, then `return true`). A purchase
  charging off the *requested* quantity would overcharge whenever a destination stack cap
  truncated the move. Fixed the same way: `MoveItemCounted` reports what actually moved and
  `MoveItem` forwards to it. **Assume any other "did it work?" bool in this system is hiding a
  quantity until checked.**
- **The all-or-nothing ordering is the load-bearing detail of the transaction.** A purchase
  prices the *whole requested quantity* and refuses up front if the balance won't cover it; only
  then does the item move; the debit is then for what actually moved, which can only be less. So
  the debit can never fail after the goods have changed hands. Checking the smaller (actual)
  figure first would have been wrong in the other direction — it would silently sell a player a
  partial stack they never asked for. The cost is recorded in `inventory.md`'s Known Gaps: a
  player who can half-afford a stack is refused outright, and the honest fix is partial-stack
  drag, not a cleverer transaction.
- **The double-click order grew a fourth meaning without growing a fourth sweep.** The entry
  called for "one `AStrategyUnit` lookup that then branches on state" and that's what shipped:
  `FindLootableNPCAtLocation` became `FindNPCAtLocation` (filter: not a player pawn) and the
  caller branches on `IsLootableNPC`. Two sweeps with two filters would have had to agree about
  which NPC was nearer, which is a bug waiting for two NPCs standing together.
- **The hostility rule lives in a predicate, not in a guard.** `IsInteractableNPC` ("not a player
  pawn, on its feet, not hostile") mirrors `IsLootableNPC`, and the hostility clause is *inside*
  it rather than at each call site — so dialog inherits it for free, the same way Dead inherited
  the whole inert-unit ruleset from Slice 7's `IsIncapacitated()`.
- **Click-to-attack was removed in the same slice**, as `input-and-keybinds.md` scheduled. It had
  to be: the guard stops a hostile trader's shop from opening, but the select click that fires
  alongside the double-click would still have started the fight. A single click now only ever
  picks a target; `H` attacks it.
- **Prices are on hover, and that needed no asset work at all** — `SetToolTipText` on the item
  widget, driven by the *window* it sits in rather than by the item. Same routing rule as
  right-click-to-equip: the controller decides what a window is, the widget only asks. A pack
  opened beside a trader quotes sell prices; the same pack opened beside a chest quotes nothing,
  because `ClearInventory` drops the pricing along with the binding.
- **A `UPROPERTY` moving onto a new component has no `CoreRedirects` equivalent** — Slice 4
  recorded this and it fired again here: `StartingGold` moved from `AStrategyPlayerState` to the
  wallet, silently voiding the 250 authored on `BP_StrategyPlayerState`. Re-author it on the
  component. Player states are spawned rather than placed, so there were no level instances to
  grep this time; there will be for the next component that takes a property off a placed actor.
- Verify with `SmoresDumpTrader` / `SmoresBuyItem <EntryIndex>` / `SmoresSellItem <EntryIndex>`
  (click an NPC to target it first) — server-side, like the other execs, and routed through the
  real `TryTradeItem` rather than a parallel debug path.

**Left undone on purpose:** dialog. `InteractWithNPC` is the seam and its ordering is settled, but
there is deliberately no stub — an empty hook nobody implements against is clutter, and the
branch is one `if` away from existing when dialog is real.

### Slice 9 — Sort and filter — **DONE**

Shipped; see `inventory.md`. It landed exactly as scoped, including on `WBP_ContainerInventory`
as well as `WBP_Inventory` — same C++ class, same four widget names, so a chest, a corpse and a
trader's shelf got the toolbar for the cost of doing the wiring twice. Notes worth carrying
forward:

- **The slice's real content was noticing that its two halves are different kinds of thing.**
  A sort changes the world and must replicate; a filter changes one player's view and must not
  leave their machine. They arrived in one entry and read as one feature, but the sort needed an
  authority-only mutator, an `IInventoryMoveHost` method and an RPC, while the filter needed
  none of those and would have been actively wrong with them. **Ask which of the two a new
  inventory feature is before writing it** — it decides the entire shape.
- **`MoveItem`'s "no swap" reasoning applies to the repack too, and the fix is a scratch array.**
  First-fit packing in criterion order can strand an item the *previous* arrangement had room
  for, so a naive in-place repack would silently drop things. Building into a scratch array and
  committing only on full success makes the failure mode "the button did nothing" instead of
  "the button ate my sword". Biggest-footprint-first among equal keys makes it rarer still.
- **Sorting merges stacks, and that was safe to add only because it introduces no new rule.**
  It reuses `CanStackWith` and `GetEffectiveMaxStack` exactly as `AddItem` does, so a sorted grid
  is always something the player could have produced by hand: the stolen flag still blocks a
  merge, `Condition` is still ignored. A merge rule invented *for* sort would have been a second
  source of truth about stacking.
- **`TArray::Sort` is not stable, which turns "deterministic" into a real requirement rather than
  a nicety.** Without a tiebreak chain ending at `EntryId`, two equal entries can swap places on a
  repack that changed nothing else — so pressing Sort twice would keep moving items and "sorted"
  would be a state the grid never settles into. Sort keys are also compared *exactly* rather than
  with `IsNearlyEqual`: a tolerance-based comparator isn't a strict weak ordering, and `Sort` is
  entitled to misbehave on one that isn't.
- **Dim, don't hide — and the reason is the cell layer, not taste.** A hidden item widget exposes
  the empty-looking cells underneath it, so the player reads occupied space as free, drops
  something there, and gets a silent rejection. Dimmed items also stay draggable, which means the
  filter never has to be cleared to act on something it dimmed.
- **`EItemCategory::None` became the "All" row.** It was already a sentinel nothing authored
  (definitions default to `Misc`, the real catch-all), so reusing it avoided a parallel filter
  enum that would have had to be kept in step with `EItemCategory` forever. Recorded in
  `inventory.md`'s Core Rules, because an item actually authored as `None` would now be visible
  only while no filter is set.
- **Four more `BindWidgetOptional` widgets and still no Blueprint graph work.** C++ binds
  `OnClicked`/`OnSelectionChanged` in `NativeConstruct` and fills the combo box from the enum
  itself, so wiring was "place four widgets, name them exactly" — the Slice 4 `GoldText`/
  `WeightText` shape extended from text to interactive controls. Two things that shape forced:
  `NativeConstruct` runs on **every** open (the window is re-added to the viewport, not
  respawned), so the option list must be cleared first or it grows duplicates; and the filter has
  to be re-applied at the end of `RefreshDisplay`, since `RebuildGrid` respawns every item widget
  at full opacity and the filter would otherwise lift itself the first time anything moved.
- **The dropdown maps back to a category by row index, not by display string.** A localised
  "Weapon" would break a string lookup and not an index one, and `FilterBoxCategories` is
  populated in the same pass that adds the rows, so the two can't disagree.
- Verify with `SmoresSortInventory <0|1|2>` (0 = weight, 1 = value, 2 = quantity) — server-side
  like the other execs, and it dumps the occupancy map afterwards, so a before/after pair of
  `SmoresDumpInventory` calls shows the repack without the window open.

### Slice 10 — Theft (blocked)

- Only the `Steal` transfer context wrapper that sets `bStolen` on items taken from a
  non-Downed NPC. Detection/awareness, standing loss, and fence pricing all wait on NPC
  awareness and faction systems. **Do not start** until those exist; listed so the seam is
  known.

## Resolved Design Decisions

Recorded so future sessions don't reopen them:

- **Stack cap** — definition base max stack × per-holder `StackMultiplier`. (Rejected:
  definition-only, holder-only.)
- **Pricing before the economy exists** — flat base resale value behind a pricing interface
  with fixed buy/sell markups; the market simulation swaps in behind that interface later.
  Shipped in Slice 8 as `IPricingProvider`, with totals derived rather than virtual and the whole
  `FInventoryItem` passed in so `Condition`/`bStolen` can matter later without a signature
  change. (Rejected: stubbing a market table now, deferring trade entirely.)
- **Loot dead vs. Downed** — same actor, same code path; lootable = Downed or Dead. Shipped in
  Slice 7, along with the `EHealthState { Alive, Downed, Dead }` enum it required. (Rejected:
  corpse-container actor; a second bool alongside `bIsDowned`.)
- **`IInventoryHolder` carries proximity, not grid access** — shipped in Slice 7. The interface
  promises `GetHolderDisplayName` and `IsInRangeOf(const AActor*)` only; a caller that needs a
  `UInventoryComponent` casts to the concrete holder type. `AWorldItem` holds a bare
  `FInventoryItem` and no grid, so a `GetInventory` on the interface would be unimplementable by
  one of its three implementers. Proximity is where the real duplication is (three copies of the
  same distance test); grid access has two implementers and no duplication problem, so putting it
  on the interface buys nothing and costs honesty. (Rejected: `GetInventory` returning null for
  world items — a method that sometimes lies, leaving every caller to remember a null check
  someone eventually won't; and leaving `AWorldItem` off the interface entirely, which preserves
  one of the three duplicate proximity checks the interface exists to delete.)
- **Traders are a component on a character, not a shop actor** — `UTraderComponent` attaches
  to an NPC and carries that trader's stock and prices, and its *presence* is the only "is this
  a trader?" flag there is. Chosen so that `economy.md`'s travelling caravans are the same
  system rather than a second implementation, and so a merchant can't be half-configured.
  Shipped in Slice 8, and as a `UInventoryComponent` **subclass** — a shelf is a grid, so the
  stock replicates, opens into the existing window and moves through the existing `MoveItem`
  with no new UI at all. (Rejected: `AStrategyStorefront : AStrategyContainer`, a stationary
  shop building; a separate `bIsTrader` bool alongside the stock, which can disagree with
  itself; a plain `UInventoryComponent` owned *by* the trader component, which would have needed
  its own registration and bought nothing.)
- **Double-click a living NPC is the "interact with this person" verb** — a trader opens
  trade, a non-trader is where dialog goes when it exists. It swallows the gesture either way,
  the same as an out-of-range container or body already does. Double-clicking *empty ground*
  still selects all on screen. Never fires on a hostile NPC. (Rejected: letting a living NPC
  fall through to select-all-on-screen, which would mean changing the gesture's meaning twice —
  once now and again when dialog lands; and building a dialog stub now, which is clutter
  nobody would implement against.) Shipped in Slice 8, including the deliberate absence of the
  dialog stub.
- **A single click selects the actor under the cursor and does nothing else** — clicking is
  for picking a target, never for issuing an order against it. The click-an-Aggressive-NPC-to-
  attack shortcut that contradicted this was removed in Slice 8, which is when it started
  actually mattering: the select click fires alongside the double-click, so it would have
  started a fight every time the player tried to talk to a hostile. `H` covers attacking a
  target. (Rejected: keeping click-to-attack as a convenience, which makes a single click mean
  different things depending on the target's mood.)
- **Encumbrance** — tracked and displayed only; effects deferred to a characters/combat
  pass. (Rejected: soft slowdown now, hard cap.)
- **World pickup range** — double-click gated by the shared proximity check; no auto-pickup
  radius. Shipped in Slice 6, along with a *click*-precision radius separate from the
  container's (`WorldItemSelectionRadius`), which is a different question from reach.
- **Drop orientation** — a drag lands at the orientation the player is holding; an ill-fitting
  drop is rejected, not auto-rotated. The rotate key plus a red drop preview is the answer.
  (Rejected: falling back to the other orientation when the literal one doesn't fit.)
- **Item definition storage** — `UPrimaryDataAsset` per item, not a `UDataTable`.
- **Equipping takes one unit, not the whole entry** — wearing from a stack splits a single unit
  off and leaves the rest in the grid, so one stack of knives can arm several pawns; unequip
  returns one. (Rejected: moving the whole entry into the slot, which would make a slot hold a
  stack.)
- **Right-click's meaning comes from the window, not the item** — only a pawn's own inventory
  window carries an equipment target, so right-click is inert in a chest or loot panel.
  (Rejected: resolving the equipment component off the item's holder, which let the player
  dress a corpse.)
- **The paperdoll is a separate floating window**, opened and closed with the pawn inventory
  window rather than embedded in it. (Rejected: a panel inside `WBP_Inventory`.)
- **Currency ownership** — per-player, shared across divisions, no possession restrictions yet.
  Shipped in Slice 4 inline on `AStrategyPlayerState` (with an `IStrategyResourceHost` seam to
  reach it from `SmoresUI`), and relocated in Slice 8 into a `UWalletComponent` in
  `SmoresEconomy` — which deleted that seam rather than extending it, since a component in a
  module `SmoresUI` already depends on needs no interface at all.
- **Weight capacity of 0 means unlimited**, which is what every static holder (chest,
  storefront shelf, warehouse) should use — weight is a carried-density figure and nothing
  static carries. (Rejected: a sentinel `bHasWeightLimit` flag, or a huge placeholder number.)
- **Save backend** — not this system's decision; stay `UPROPERTY`-reflected.
- **A sort replicates; a filter doesn't** — the repack is an authority-only mutator behind an
  `IInventoryMoveHost` RPC, so every player watching that holder sees it; the category filter is
  pure client state with no RPC and no authority check, because nothing about the holder changed.
  Shipped in Slice 9. (Rejected: replicating the filter as per-player view state, which would put
  screen state on the wire for no gain; and doing the sort client-side, which can't work at all —
  the mutators are authority-only.)
- **A repack is all-or-nothing** — built in a scratch array and committed only if every entry
  re-places, because first-fit in criterion order can strand an item the previous arrangement had
  room for. Shipped in Slice 9. (Rejected: dropping what won't fit, which would make a tidy-up
  button occasionally eat an item; and abandoning the criterion to pack purely by footprint,
  which isn't the sort the player asked for — footprint area is a *tiebreak* instead.)
- **Sorting consolidates stacks** — using the same `CanStackWith` and effective cap `AddItem`
  uses, so nothing a sort produces is unreachable by dragging stacks together by hand. Shipped in
  Slice 9. (Rejected: leaving partial stacks alone, which makes a freshly sorted grid still look
  untidy; and a merge rule written for sort, which would be a second source of truth about
  stacking.)
- **A filter dims, it never hides** — a hidden item widget exposes the empty cell layer beneath
  it, so the player would read occupied cells as free space and get a silent rejection on the
  drop. Dimmed items stay fully draggable. Shipped in Slice 9. (Rejected: collapsing/hiding
  non-matching entries, which makes the grid lie about what it holds.)
- **Sort criteria are the three figures, not the labels** — weight, value, quantity; no
  alphabetical or category sort, since category is what the *filter* is for and a name sort only
  earns its place once the grid draws icons. Shipped in Slice 9, with a tiebreak chain
  (footprint area → display name → entry id) that gives alphabetical order among equals for
  free. (Rejected: a Name criterion now.)
