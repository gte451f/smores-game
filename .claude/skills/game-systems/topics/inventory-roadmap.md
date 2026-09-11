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

## Grid-Based Storage (Bulk) and Stacking

Replaces the current flat `NumSlots` 1D array with a genuine 2D grid:

- Every holder (pawn backpack, world container, storefront shelf) has its own authored
  grid width × height — **sized per holder type**, not a single fixed size everywhere. A
  pawn's personal pack is meaningfully smaller than a storefront's or a warehouse chest's.
- An item occupies a rectangular footprint of cells (from its definition), not necessarily
  1×1 — a bulkier item takes more grid space, which is the game's stand-in for
  volume/bulk independent of weight (see Weight & Encumbrance below).
- **Rotation is supported.** Auto-placement (e.g. picking up a loose world item, "take all"
  from a loot panel) tries both orientations to find a fit; the player can also manually
  force-rotate an item while dragging, to pack deliberately ("Tetris" play is an intended,
  supported style, not just tolerated).
- Placement validates against overlap — no two placed footprints may share a cell.
- **Stacking**: items with a matching definition and stackable category combine into one
  entry. **Decided:** effective max stack = the definition's base max stack × the holder's
  stack multiplier (a per-`UInventoryComponent` value, default 1.0 for a pawn's pack;
  storefronts/warehouse chests set it higher). One number per holder type covers "a shelf
  stacks deeper than a backpack" without per-transfer special cases.
- This replaces `UInventoryComponent::AddItem`/`SetItemAt`/`MoveItem`'s current
  single-index model with real placement/collision logic, and the current
  `IInventoryMoveHost::Server_MoveInventoryItem` RPC needs to carry a target cell and
  rotation (and eventually a quantity, for partial-stack moves) instead of a single
  destination index.

## Weight & Encumbrance

Two independent measures, deliberately not conflated:

- **Weight** — the sum of carried item weights (unit weight × quantity), a density figure
  that will eventually feed movement/stealth effects per `characters-and-squads.md` (a thief
  carrying stolen goods moves slower and noisier). Pure numeric accumulation; no grid/UI
  footprint of its own.
- **Grid footprint (bulk/volume)** — see above; how much *space* an item takes, independent
  of how heavy it is. A large but light item (a bundle of cloth) and a small but heavy one
  (an ingot) stress the two measures differently, which is the point.
- **Decided: track only, no effects this round.** Total carried weight and a per-pawn weight
  capacity are computed, replicated, and shown in the UI; movement-speed and stealth-noise
  penalties are deliberately deferred to a later characters/combat pass, since those systems
  own the effects. Nothing in this round blocks a pickup on weight grounds.

## Equipment / Worn Slots

- A separate equipped-slot set per pawn, distinct from the general carried grid — a
  paperdoll, not just another grid region.
- Slot type must match item category (a weapon only fits a weapon slot, etc.).
- **Right-click a carried item to auto-equip** it into its matching slot (swapping out
  whatever's currently equipped there, which returns to the carried grid — auto-placed,
  and the equip fails cleanly if the grid has no room for the displaced item).
- **No skill gating at equip time.** Any pawn can equip any weapon or armor regardless of
  training — this system's only job is slot-type matching. Performance consequences of an
  untrained equip (per `combat.md`'s weapon-class mismatch penalty) are entirely
  combat/skill-resolution's concern, never an inventory-system restriction.

## Currency

- Placeholder name **"gold"** — no weight, not part of the grid/inventory at all (not an
  item, not a stack, doesn't occupy a cell).
- **Owned per-player, not per-pawn or per-division.** Every sub-squad/division under one
  player draws from the same pool regardless of physical location or distance from each
  other, per `characters-and-squads.md`'s Squad Divisions (an organization layer, not a
  separate economy). This means it lives outside `UInventoryComponent` entirely — a
  replicated, authority-gated value on a new `AStrategyPlayerState` (no custom
  `PlayerState` exists in the project yet) — and the trade/purchase transfer context
  (below) touches both a pawn's `UInventoryComponent` *and* the player's currency value in
  the same server-side transaction.
- No possession restrictions yet (no robbable cash, no per-division budgets) — explicitly
  deferred; noted as a future extension point only.

## Unified Transfer Interface

One transfer mechanism — extending today's drag-and-drop `MoveItem`/`IInventoryMoveHost`
plumbing to move a footprint of cells between two grids instead of swapping two indices —
serves every context below. Each context layers additional rules on top of the same
underlying move/copy operation rather than inventing its own UI:

| Context | Direction | Currency? | Extra gating |
|---|---|---|---|
| Pawn ↔ pawn | free move | no | proximity only |
| Loot Downed or dead body | free take | no | proximity; target must be Downed **or dead** — **decided:** same code path and same actor, the lootable check just becomes "Downed or Dead"; no corpse-container actor. Requires a Dead state in `UHealthComponent` first (only Downed exists today) |
| Container transfer | free move | no | proximity (already implemented, `InteractionRange`) |
| Trade with an NPC / sell to a storefront | two-way exchange | yes | proximity; **decided:** price = the definition's base resale value behind a small pricing interface (`IPricingProvider`-style), with a fixed buy markup / sell markdown, so the real market system (`economy.md`) can replace it later without touching transfer code |
| Storefront purchase | one-directional (buy) | yes | proximity; storefront's stock is flagged "for sale" and priced through the same pricing interface; distinct from an NPC's personal belongings |
| Steal from an unsuspecting NPC | free take | no | proximity; gated by detection/awareness rules per `characters-and-squads.md`'s pickpocketing design (skill vs. target awareness/crowd density, immediate witnessed-failure consequence); sets the stolen flag on the taken item(s). Blocked on NPC awareness systems that don't exist yet |

- **All transfers require proximity to the target** — this generalizes the existing
  container `InteractionRange`/`IsUnitInRange` pattern (currently container- and
  Downed-NPC-specific) to pawns, live NPCs, storefronts, bodies, and world pickups
  uniformly, through one `IInventoryHolder` interface (inventory pointer, display name,
  in-range check) implemented by every holder type. `characters-and-squads.md` is explicit
  that inventory transfer during theft requires physical proximity too (no handing stolen
  goods to a mule waiting outside) — the same rule already fits every other context, so one
  proximity check serves all of them.
- **Stolen-item flag** — set on an item taken via theft; recognized within the origin
  faction's territory (sellable at a discount through fences, full price to distant
  factions unaware of the theft), and quietly expires after enough in-game time per
  `characters-and-squads.md`. Territory recognition and expiry depend on faction/world
  systems that don't exist yet — this round only carries the flag.

## World Pickups

- Loose item actors scattered in the world, collected via **double-click**, distinct from
  the existing container-actor interaction pattern (`AStrategyContainer` is a whole
  inventory holder with a mesh and interaction sphere; a world pickup is a single item
  instance on a much lighter-weight actor).
- Uses the item definition's **3D representation**, not its 2D icon, for its world
  appearance.
- **Decided:** double-click is gated by the same proximity check as every other transfer
  (nearest player pawn must be in range); no separate auto-pickup radius. The item
  auto-places into the nearest in-range player pawn's grid (trying both rotations) and the
  actor is destroyed on success; if nothing fits, the pickup fails and the item stays in
  the world.

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
`OnInventoryChanged` delegate pattern, `UWindowWidget` floating-panel chrome, and the
overall "drag from one slot widget, drop on another, server validates and applies" shape.
What needs real rework: `UInventoryComponent`'s flat index model (→ 2D grid + placement),
`IInventoryMoveHost::Server_MoveInventoryItem`'s single-index signature (→ cell + rotation,
and possibly quantity for partial-stack moves), and `AStrategyContainer`/loot's
`InteractionRange` proximity pattern (→ generalized to every transfer context via
`IInventoryHolder`, not reimplemented per context).

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
  held *empty*-array overrides that quietly shadowed the new Blueprint defaults. Slices 2
  and 5 reshape this struct again: grep `Content/__ExternalActors__/` for `StartingItems`
  first, and don't assume Blueprint class defaults are the only authored copies.

### Slice 2 — Grid storage, footprint, rotation, stacking (component side)

- **Build:** replace `NumSlots`/`Items` with `GridWidth`/`GridHeight`, `StackMultiplier`,
  and a replicated array of placed entries (instance + anchor cell + rotation). Placement
  API: `CanPlaceAt(item, cell, rotation)`, `FindFreePlacement(item)` (tries both
  rotations), `AddItem` (merge into an existing stack up to base × multiplier first, then
  auto-place), `RemoveEntry`, and a new static `MoveItem(src, entryId, dest, cell,
  rotation, quantity)`. Change `IInventoryMoveHost::Server_MoveInventoryItem` to match.
  Keep `OnInventoryChanged`/`OnRep` and authority gating exactly as they are.
- **Touches:** `InventoryComponent.*`, `InventoryMoveHost.h`,
  `StrategyPlayerController.*` (RPC signature), and the UI just enough to compile — the
  slot-widget rendering can be temporarily degraded (e.g. the `SlotListText` fallback) until
  Slice 3.
- **Done when:** unit tests or a debug command exercise placement/rotation/stack-merge on
  the server and results replicate; existing chest/unit `StartingItems` auto-place on
  BeginPlay; the text fallback lists entries with quantities.

### Slice 3 — Grid UI: cells, footprints, drag with rotate

- **Build:** `UInventoryWidget` renders a `GridWidth × GridHeight` cell grid with one item
  widget per entry spanning its footprint; drag creates a `UInventoryDragDropOperation`
  carrying entry id + current rotation; a rotate key during drag flips it; drop resolves the
  hovered cell → `Server_MoveInventoryItem(cell, rotation, full quantity)`; dropping onto a
  same-definition stack merges. Rejected drops snap back with no state change (server is
  authoritative).
- **Touches:** `InventoryWidget.*`, `InventorySlotWidget.*` (likely becomes an item widget
  plus a cell widget), `InventoryDragDropOperation.h`, `WBP_Inventory`,
  `WBP_ContainerInventory`, `WBP_InventorySlot`.
- **Done when:** the user can drag, rotate, and pack items between a pawn window and a chest
  window in PIE, and a second client (or listen-server + client in-editor) sees the result.

### Slice 4 — Weight tracking and currency

- **Build:** `UInventoryComponent::GetTotalWeight()` (unit weight × quantity), a replicated
  `WeightCapacity` on the pawn (or its component) shown as "X / Y" in the inventory window;
  no gameplay effect. `AStrategyPlayerState` (new, `smores` or `SmoresCore`) with a
  replicated `Gold`, server-only `AddGold`/`TrySpendGold`, `OnRep_Gold`, and a readout in
  `UStrategyUI` per `player-interface.md`'s quick-access strip. Set it as the Strategy game
  mode's `PlayerStateClass`.
- **Touches:** `InventoryComponent.*`, `StrategyUnit.*`, `StrategyGameMode.*`, new
  `StrategyPlayerState.*`, `StrategyUI.*`, `UI_Strategy`.
- **Done when:** weight updates live as items move; gold shows on the HUD and a debug command
  can add/spend it with correct replication.

### Slice 5 — Equipment component and paperdoll

- **Build:** `EEquipSlot` enum (keep the roster small — e.g. MainHand, OffHand, Head, Body,
  Feet) and `UEquipmentComponent` on `AStrategyUnit` with one replicated instance per slot;
  server-only `Equip(fromInventory, entryId)` / `Unequip(slot)` that swap the displaced item
  back into the grid via `FindFreePlacement` and fail cleanly if it doesn't fit.
  `IInventoryMoveHost` gains `Server_EquipItem`/`Server_UnequipItem`; right-click on an item
  widget calls it. New `UEquipmentWidget` paperdoll (drag from grid onto a slot also works).
  Attaching the 3D mesh to skeletal sockets is optional cosmetic polish in this slice; combat
  reading the equipped weapon is a later combat-system change, not this slice.
- **Touches:** new `EquipmentComponent.*`, `StrategyUnit.*`, `InventoryMoveHost.h`,
  `StrategyPlayerController.*`, new `EquipmentWidget.*`, new `WBP_Equipment`.
- **Done when:** right-click equips into the correct slot, the paperdoll shows it, swapping
  returns the old item to the grid, and it all replicates.

### Slice 6 — World pickups

- **Build:** `AWorldItem` in `SmoresItems`: one `FInventoryItem` instance, a static mesh
  from the definition's 3D representation, implements `IInventoryHolder`-style range check.
  Double-click (extend `SelectAllDoubleClick`'s existing container/Downed branches) → nearest
  in-range player pawn → `Server_PickUpWorldItem` → `AddItem` on the pawn → destroy on
  success, no-op on failure. Optional stretch: dragging an item out of a window onto the
  world spawns an `AWorldItem` at the pawn's feet.
- **Touches:** new `WorldItem.*`, `StrategyPlayerController.*`, a `BP_WorldItem` and a few
  placed instances in `LVL_Strategy`.
- **Done when:** double-clicking a placed item with a pawn in range moves it into that
  pawn's grid; out of range does nothing.

### Slice 7 — `IInventoryHolder` and loot-dead

- **Build:** `IInventoryHolder` interface in `SmoresItems` (`GetInventory`,
  `GetHolderDisplayName`, `IsInRangeOf(const AActor*)`), implemented by `AStrategyUnit`,
  `AStrategyContainer`, and `AWorldItem`; collapse the PC's four `Find*InRange`/
  `Find*AtLocation` methods into holder-generic versions. Add a Dead state to
  `UHealthComponent` (`SmoresCombat` — coordinate with `combat.md`) and make the lootable
  check "Downed or Dead."
- **Touches:** new `InventoryHolder.h`, `StrategyUnit.*`, `StrategyContainer.*`,
  `WorldItem.*`, `StrategyPlayerController.*`, `HealthComponent.*`.
- **Done when:** the PC has one proximity path for every holder type and a killed NPC is
  lootable exactly like a Downed one.

### Slice 8 — Storefront, purchase, and trade

- **Build:** `IPricingProvider` (base value × buy markup / sell markdown, flat for now) and
  `AStrategyStorefront : AStrategyContainer` (or a `UStorefrontComponent`) whose entries are
  for sale. Server-side transaction: verify proximity, verify `TrySpendGold`, move the item,
  debit/credit — all-or-nothing. UI: price shown on hover; dragging store → pawn triggers
  purchase, pawn → store triggers sale. Trading with a live NPC is the same two-way flow
  against the NPC's inventory.
- **Touches:** new `PricingProvider.h`, new `StrategyStorefront.*`,
  `StrategyPlayerState.*`, `InventoryMoveHost.h`, `StrategyPlayerController.*`,
  `InventoryWidget.*`/item widget (price display), a `BP_Storefront`.
- **Done when:** buying debits gold and moves the item; insufficient gold rejects with no
  state change; selling credits gold; all replicated.

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
- **Loot dead vs. Downed** — same actor, same code path; lootable = Downed or Dead.
  Requires a Dead state in `UHealthComponent`. (Rejected: corpse-container actor.)
- **Encumbrance** — tracked and displayed only; effects deferred to a characters/combat
  pass. (Rejected: soft slowdown now, hard cap.)
- **World pickup range** — double-click gated by the shared proximity check; no auto-pickup
  radius.
- **Item definition storage** — `UPrimaryDataAsset` per item, not a `UDataTable`.
- **Currency ownership** — per-player on `AStrategyPlayerState`, shared across divisions;
  no possession restrictions yet.
- **Save backend** — not this system's decision; stay `UPROPERTY`-reflected.
