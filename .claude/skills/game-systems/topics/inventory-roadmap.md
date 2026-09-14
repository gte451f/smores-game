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

## Currency — **SHIPPED (Slice 4)**

The per-player gold balance on `AStrategyPlayerState` shipped in Slice 4; see `inventory.md`.
Still outstanding is everything that *moves* it — the purchase/sale transaction is Slice 8,
which pairs `TrySpendGold` with the item half in one server-side call. Possession
restrictions (robbable cash, per-division budgets) remain explicitly deferred; the shared-pool
model is unchanged.

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
| Trade with an NPC / sell to a storefront | two-way exchange | yes | proximity; **decided:** price = the definition's base resale value behind a small pricing interface (`IPricingProvider`-style), with a fixed buy markup / sell markdown, so the real market system (`economy.md`) can replace it later without touching transfer code |
| Storefront purchase | one-directional (buy) | yes | proximity; storefront's stock is flagged "for sale" and priced through the same pricing interface; distinct from an NPC's personal belongings |
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

## Sort & Filter (Inventory UI)

- **Sort by**: weight, value, quantity — in a grid, "sort" means a server-side auto-repack
  of the holder's contents ordered by the chosen criterion, not just a visual reorder.
- **Filter by**: category (weapons, armor, food, etc.) — purely visual (dim or hide
  non-matching entries); reads the item type/category field on the definition. No storage
  change.

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
  snap back. If a context ever needs to explain *why* a move failed (insufficient gold,
  Slice 8), that needs a new client RPC — it can't be inferred from the absence of a change.
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
  `unreal-module-organization.md` records for the other three. Slice 8's UI price display will
  want the same seam; extend that interface rather than inventing a second one.
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
- **`Gold` is inline on `AStrategyPlayerState`, and that's a known temporary shape.** One
  `int32` didn't justify a component, but it's the first of several per-player values headed
  for that class (squad roster, faction standing, research progress). Slice 8 moves it into
  a `UWalletComponent` in a new `SmoresEconomy`; see `unreal-module-organization.md`'s
  "Framework Classes vs. Feature Modules" for why per-player state belongs in a component
  rather than on the framework class. Don't add a *second* inline field here in the
  meantime — write the component instead.

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
  **Slice 8's purchase must use `AddItemCounted` too** — the same trap, with gold attached.
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

### Slice 8 — Storefront, purchase, and trade

- **Stand up `SmoresEconomy` as part of this slice, and move gold into it.** This is the
  slice where a currency module stops being speculative: it brings `IPricingProvider`, the
  transaction, and the existing balance together into one coherent domain, which is exactly
  the "a real ownership seam appeared in practice" trigger `unreal-module-organization.md`
  requires before cutting a module. Scope it to **value primitives only** — wallet, pricing,
  transactions, depending on `SmoresItems` and not on factions; the market simulation that
  eventually *sets* prices is a separate, much later `SmoresMarkets`.
  - Convert `AStrategyPlayerState::Gold` into a `UWalletComponent` hosted on the player
    state, per that topic's "Framework Classes vs. Feature Modules". Slice 4 put `Gold`
    inline because one `int32` didn't justify a component; by this slice it's carrying a
    transaction API and is no longer the only thing headed for that class.
  - Doing so lets `IStrategyResourceHost` be **deleted**: with the wallet in a module
    `SmoresUI` already depends on, `AStrategyHUD` can reach it via
    `PC->PlayerState->FindComponentByClass<UWalletComponent>()` (cache the pointer), since
    `APlayerState` is an engine type visible everywhere. Prefer that to extending the
    interface with price/affordability reads.
- **Build:** `IPricingProvider` (base value × buy markup / sell markdown, flat for now) and
  **`UTraderComponent`** — a component attached to an NPC, holding that trader's stock and
  prices. Server-side transaction: verify proximity, verify `TrySpendGold`, move the item,
  debit/credit — all-or-nothing. UI: price shown on hover; dragging store → pawn triggers
  purchase, pawn → store triggers sale.
- **The trader is a component on a character, not a shop actor — settled, see Resolved Design
  Decisions.** This supersedes the earlier `AStrategyStorefront : AStrategyContainer` sketch.
  Two reasons, both load-bearing:
  - `economy.md` wants **caravans**: traders that physically travel between towns, that patrols
    escort and bandits raid, and that the player can intercept or follow to an undiscovered
    market. A caravan is a trader that walks. As a component, a caravan NPC gets trading for
    free; as a building, caravans need the whole thing implemented a second time.
  - **Presence of the component *is* the "is this a trader?" flag.** No separate bool, so there
    is nothing that can disagree with reality — no NPC flagged as a merchant with no stock, and
    no stocked NPC who refuses to trade. Same single-source-of-truth reasoning as Slice 7's
    `IInventoryHolder`.
- **Gesture: double-click a living NPC — settled.** Double-click is the "interact with this
  person" verb. A trader opens the trade window; a non-trader is where **dialog** will go when
  it exists (out of scope here, and **deliberately no stub** — an empty hook nobody implements
  is clutter; the settled *ordering* is what makes dialog drop in later with no rework).
  - **A living NPC becomes a double-click target**, taking that gesture over from
    select-all-on-screen, which the user has confirmed doesn't work long term for NPCs. It
    swallows the gesture either way, matching the existing rule for an out-of-range container
    or body ("the gesture means *that thing*, not *everyone*"). Double-clicking **empty ground**
    still selects all on screen — that half is unchanged.
  - **New position in `SelectAllDoubleClick`'s order:** world item → container → body →
    **living NPC** → empty ground/select-all. Body and living NPC are the same actor type
    differing only by health state, so prefer one `AStrategyUnit` lookup at
    `ContainerSelectionRadius` that then branches on state, rather than a second sweep.
  - **Guard: never trade (or later, talk) with a hostile NPC.** A double-click also fires the
    normal select click, and a select click on an already-Aggressive NPC currently issues a
    squad attack order — so without this guard, double-clicking a hostile trader would open
    their shop and start a fight at once. (That click-to-attack behavior is itself scheduled
    for removal; see `input-and-keybinds.md`'s "Existing defaults worth revisiting".)
- **Keyboard route: `T`** — talk/trade with the currently targeted NPC, the same shape as `H`
  (attack the targeted NPC) and reading the same `SelectedNPC`. Reserved in
  `input-and-keybinds.md`. Not required for the slice to ship, but it is the accessible
  alternative to a double-click and costs one binding; add it via that topic's checklist.
- **Proximity comes free.** `AStrategyUnit` already implements `IInventoryHolder` as of Slice 7,
  so "is a pawn close enough to trade with this NPC" needs no new distance code — reuse
  `FindPlayerPawnInRangeOfHolder`/`IsHolderInRangeOfSelection`.
- **Touches:** new `SmoresEconomy` module (`WalletComponent.*`, `PricingProvider.h`,
  `TraderComponent.*`), `StrategyPlayerState.*` (loses `Gold`, gains the component),
  `StrategyHUD.*`/`StrategyUI.*` (read through the component), `InventoryMoveHost.h`,
  `StrategyPlayerController.*` (the double-click branch, the trade window, the hostility
  guard, optionally the `T` binding), `InventoryWidget.*`/item widget (price display), and a
  `BP_` NPC subclass carrying the trader component. Deletes `StrategyResourceHost.h`.
- **Note:** standing up a module plus moving replicated state off a framework class is a
  lot to carry alongside the storefront itself. If it turns out too big for one clean
  session, split it — the module + wallet move first (mechanical, verifiable via the
  existing gold execs), the storefront second.
- **Done when:** double-clicking a trader NPC in range opens trade; buying debits gold and
  moves the item; insufficient gold rejects with no state change; selling credits gold; all
  replicated. Double-clicking a non-trader, or any hostile NPC, does nothing at all.

### Slice 9 — Sort and filter

- **Build:** `Server_SortInventory(criterion)` → server auto-repacks the grid ordered by
  weight/value/quantity; client-side category filter dims/hides non-matching item widgets.
  Buttons/dropdown on `WBP_Inventory`.
- **Touches:** `InventoryComponent.*`, `InventoryMoveHost.h`, `InventoryWidget.*`,
  `WBP_Inventory`. Can be done any time after Slice 3.
- **Done when:** sort repacks deterministically and replicates; filter toggles visibility.

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
  (Rejected: stubbing a market table now, deferring trade entirely.)
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
  (Rejected: `AStrategyStorefront : AStrategyContainer`, a stationary shop building; a
  separate `bIsTrader` bool alongside the stock, which can disagree with itself.)
- **Double-click a living NPC is the "interact with this person" verb** — a trader opens
  trade, a non-trader is where dialog goes when it exists. It swallows the gesture either way,
  the same as an out-of-range container or body already does. Double-clicking *empty ground*
  still selects all on screen. Never fires on a hostile NPC. (Rejected: letting a living NPC
  fall through to select-all-on-screen, which would mean changing the gesture's meaning twice —
  once now and again when dialog lands; and building a dialog stub now, which is clutter
  nobody would implement against.)
- **A single click selects the actor under the cursor and does nothing else** — clicking is
  for picking a target, never for issuing an order against it. The existing
  click-an-Aggressive-NPC-to-attack shortcut contradicts this and is scheduled for removal;
  `H` already covers attacking a target. (Rejected: keeping click-to-attack as a convenience,
  which makes a single click mean different things depending on the target's mood.)
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
- **Currency ownership** — per-player on `AStrategyPlayerState`, shared across divisions;
  no possession restrictions yet. Shipped in Slice 4, including the module placement
  (`smores`, not `SmoresCore`) and the `IStrategyResourceHost` seam it required.
- **Weight capacity of 0 means unlimited**, which is what every static holder (chest,
  storefront shelf, warehouse) should use — weight is a carried-density figure and nothing
  static carries. (Rejected: a sentinel `bHasWeightLimit` flag, or a huge placeholder number.)
- **Save backend** — not this system's decision; stay `UPROPERTY`-reflected.
