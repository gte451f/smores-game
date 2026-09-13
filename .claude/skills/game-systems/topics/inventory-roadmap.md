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
overall "drag from one slot widget, drop on another, server validates and applies" shape —
and now the narrow-interface seam between `smores` and `SmoresUI` (Slice 4 added a fourth,
`IStrategyResourceHost`, rather than moving a gameplay class down a module).
Already reworked in Slice 2: `UInventoryComponent`'s flat index model (now a 2D grid with
placement/collision) and `IInventoryMoveHost::Server_MoveInventoryItem`'s signature (now
entry id + cell + rotation + quantity). Already reworked in Slice 3: the interim
one-widget-per-cell UI (now a `UGridPanel` with a cell layer and a footprint-spanning item
layer, one grid-level drop target, and a drop preview). What still needs real rework:
`AStrategyContainer`/loot's `InteractionRange` proximity pattern (→ generalized to every
transfer context via `IInventoryHolder`, not reimplemented per context).

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
- **The rotate key is a Slate input pre-processor**, registered by the drag operation and torn
  down with it. This isn't stylistic: a drag captures the pointer but not keyboard focus, and
  Slate routes key events along the *focus* path (the game viewport), so a `NativeOnKeyDown` on
  the inventory window never fires during a drag. Any later slice wanting a mid-drag modifier
  (split-stack, say) should reuse that hook rather than rediscovering this.
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
- **Drop orientation** — a drag lands at the orientation the player is holding; an ill-fitting
  drop is rejected, not auto-rotated. The rotate key plus a red drop preview is the answer.
  (Rejected: falling back to the other orientation when the literal one doesn't fit.)
- **Item definition storage** — `UPrimaryDataAsset` per item, not a `UDataTable`.
- **Currency ownership** — per-player on `AStrategyPlayerState`, shared across divisions;
  no possession restrictions yet. Shipped in Slice 4, including the module placement
  (`smores`, not `SmoresCore`) and the `IStrategyResourceHost` seam it required.
- **Weight capacity of 0 means unlimited**, which is what every static holder (chest,
  storefront shelf, warehouse) should use — weight is a carried-density figure and nothing
  static carries. (Rejected: a sentinel `bHasWeightLimit` flag, or a huge placeholder number.)
- **Save backend** — not this system's decision; stay `UPROPERTY`-reflected.
