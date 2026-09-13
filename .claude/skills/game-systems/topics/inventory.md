# Inventory System

## Purpose

`UInventoryComponent` gives any Actor a 2D cell grid of item storage, server-authoritative
and replicated, with a shared drag-and-drop UI for moving items within one inventory or
between two. The same component and UI back three different holders today: a squad unit's
own carried items, a world container's contents, and a Downed NPC's loot.

What an item *is* lives in a shared `UItemDefinition` data asset; what a carried copy *is
like* lives in the `FInventoryItem` instance that references it; *where that copy sits* in a
particular holder's grid lives in the `FInventoryEntry` placement that wraps it.

Items occupy a rectangular footprint of cells rather than one uniform slot, may be rotated 90°
to fit, and merge into stacks capped per holder. Carried weight is tracked and displayed
alongside the grid, and the player has a gold balance on their `AStrategyPlayerState` — but
neither *does* anything yet: weight applies no penalty, and nothing spends gold.

Alongside the carried grid, every unit has a **paperdoll**: `UEquipmentComponent`, a small set
of named worn slots that is deliberately *not* a region of the grid. An item goes in only if
its definition says that's its slot; nothing else gates an equip.

Items also exist *outside* any grid: an `AWorldItem` is a single item instance lying on the
ground, drawn with its definition's 3D mesh and collected by double-clicking it with a pawn in
range. It is the one holder shape with no grid and no window behind it. This system does
**not** yet handle: trading/purchase or theft. See `inventory-roadmap.md` for the target design
covering both — this topic only documents what's actually built.

## Player Surface

- Press the inventory key (`IA_Strategy_Inventory`) with **exactly one** player-controlled
  unit selected to open that unit's inventory window; pressing it again closes the window.
  Selecting more than one player unit and pressing the key does nothing. Cycling to a
  different pawn while a window is open closes the (now stale) window rather than switching
  it to the new pawn.
- Press the container key (`IA_Strategy_ToggleContainer`) to open the nearest world
  container within interaction range of a selected unit; pressing it again closes it. If no
  container is in range, the same key opens a Downed NPC's loot instead — never a
  Passive/Aggressive NPC (not yet Downed), and never a player-controlled unit.
- Double-clicking a container or a Downed NPC in the world selects and highlights it, and
  opens it immediately if **any** player-controlled pawn (not just the current selection) is
  within interaction range. This rides on the same gesture as the normal select-all
  double-click, not a separate input.
- Opening a container or loot target also opens the nearest player pawn's own inventory
  window alongside it, so both panels are visible at once for drag-and-drop transfer between
  them.
- Each inventory/container window is a floating panel (optional title bar, drag-to-move,
  resize-from-corner, close button) showing that holder's grid — dragging an item onto
  another cell moves it there, whether that's repacking within one panel or transferring
  between two different ones. Dropping onto a matching stackable item merges the two; a drop
  that doesn't fit, or lands on something that can't stack, is rejected and the item stays
  where it was. There is no swap: an item never displaces another by landing on it.
- Each placed item draws as **one bordered region spanning its whole footprint**, with its
  label centred in it — and turned 90° when the footprint is taller than it is wide, so a
  1×3 sword's name reads down the blade rather than spilling across its neighbours.
- **Picking an item up grabs it where you clicked.** The floating ghost keeps that grip, so a
  sword grabbed by its tip lands with its tip where you dropped it, rather than jumping to put
  its top-left corner under the cursor.
- **Press the rotate key while dragging** (R by default, and player-rebindable like every other
  binding) to turn the held item 90°. The ghost turns with it, as does the drop preview.
  Rotating a square item does nothing — a square turns into itself.
- While a drag hovers a grid, **the cells it would claim light up green or red**: green means
  the drop will be accepted, red that it won't (no room, or a stack it can't merge into). That
  preview is the only thing that tells the player whether the rotate key helped.
- **A drop is literal.** An item that doesn't fit is rejected, never quietly turned sideways to
  make it fit — auto-placement still tries both orientations on its own, but that's a different
  path, and turning an item the player didn't ask to turn works against the deliberate packing
  this design is built around.
- Every inventory window shows a **carried-weight readout** above its grid — `Weight: 12.4 /
  30.0` for a pawn, and just `Weight: 8.0` for a chest, which has no capacity of its own. It
  updates live as items move in, out, and between windows. Exceeding the capacity turns the
  readout red and does **nothing else**: the drop still lands, the transfer still completes,
  and the pawn moves exactly as fast as before.
- The HUD carries a **gold readout** ("Gold: 250") next to the selection count. Gold isn't an
  item — it has no weight, no footprint, and never appears in a grid — and today nothing in the
  game spends or earns it outside the debug console.
- The **inventory key** also opens that pawn's **Equipment window** beside the pack — a second
  floating panel listing its five worn slots (Main Hand, Off Hand, Head, Body, Feet) and the
  combined weight of what's in them. The two open and close together and are moved and resized
  independently. Only the inventory key brings it up: a pack opened as a transfer partner for a
  container or a corpse comes on its own, and closes any paperdoll already showing — that pack
  may have just rebound to the nearest pawn rather than the selected one, which would leave the
  paperdoll describing somebody else.
- **Right-click a carried item to wear it.** It goes into whichever slot its definition names,
  and whatever was already in that slot comes back to the grid. If the grid has no room for the
  displaced item, nothing happens at all — the swap never half-lands. Right-clicking an item
  that isn't wearable does nothing, and so does right-clicking inside a chest or a Downed NPC's
  loot panel: only a pawn's own inventory window equips.
- **Dragging a carried item onto a paperdoll slot** does the same thing, and the slot lights up
  green or red on hover exactly like a grid cell does — red for the wrong kind of item, or for
  a swap whose displaced item wouldn't fit back in the grid.
- **Right-click a filled paperdoll slot to take the item off.** It returns to that pawn's own
  pack, never to whatever other window happens to be open. A full pack means it stays worn.
- **Wearing one out of a stack takes one.** Equipping from a stack of five knives leaves four
  in the grid, so one stack can arm several pawns.
- A worn item **leaves the carried grid**, so the pawn's `Weight:` readout drops when something
  is equipped; the paperdoll window reports the worn weight separately. Neither figure does
  anything yet.
- **Clicks no longer fall through an open window** — any mouse button, single or double.
  Previously only a single left-click was consumed, so right-clicking a window also marched the
  squad to whatever was behind it, and a *fast second* click of any button leaked through even
  after the first was caught.
- **Double-click a loose item lying in the world to pick it up.** It goes into the grid of
  whichever player pawn is nearest and close enough, trying both orientations to find room, and
  the item disappears from the ground. No pawn in range means nothing happens at all — there's
  no "walk over and get it" order, and no auto-pickup radius: the player has to have somebody
  standing there already.
- A loose item has to be clicked **more precisely than a chest** — a tighter click radius, so an
  apple lying beside a chest doesn't swallow every double-click meant for the chest. As with a
  chest or a corpse, a double-click that lands on one means *that item*, so it never falls
  through to the select-all-on-screen gesture even when the pickup fails.
- **A pickup that only partly fits takes what fits.** Double-clicking a pile of 20 apples with
  room for 8 leaves 12 on the ground rather than refusing the whole pile or quietly destroying
  the rest. A grid with no room at all leaves the pile untouched.
- Loose items show their **3D mesh**, not their inventory icon — and since no `DA_Item_*` has a
  `WorldMesh` authored yet, one is currently an invisible actor you can still click.
- Deselecting all units, or having no player pawn selected/in range, closes any open
  inventory window — and the equipment window with it.

## Core Rules

- **Item definitions are shared; item instances are per-copy; placements are per-holder.** A
  `UItemDefinition` (`UPrimaryDataAsset`, one asset per item type under `Content/Items/`)
  carries everything every copy of that item has in common — stable `ItemId`, display name,
  description, `EItemCategory`, 2D `Icon`, 3D `WorldMesh`, `Weight`, `BaseValue`,
  `FootprintWidth`/`FootprintHeight`, `MaxStackSize`, `EEquipSlot`. An `FInventoryItem`
  carries only what varies copy-to-copy: a `Definition` pointer plus `Quantity`, `Condition`,
  and `bStolen`. An `FInventoryEntry` wraps one `FInventoryItem` with where it sits in *this*
  holder: a stable `EntryId`, an `AnchorCell`, and a `bRotated` flag. All display data is read
  through the definition; nothing is duplicated per instance.
- **Storage is a `GridWidth` × `GridHeight` cell grid** (both clamped 1–32, defaulting to
  8×8 and sized per holder type in Blueprint — a pawn's pack is 6×4, a chest 8×6). `Entries`
  holds only the items actually placed, in no particular order; there is no per-cell array and
  no "empty slot" object. A cell is free exactly when no entry's footprint covers it.
- **An item occupies a rectangular footprint**, `FootprintWidth` × `FootprintHeight` from its
  definition, which is the game's stand-in for bulk/volume — deliberately independent of
  `Weight`. `bRotated` swaps those two dimensions; that single 90° turn is the only rotation
  there is. Placement validates against overlap: no two footprints may share a cell, and the
  whole footprint (not just the anchor) must lie inside the grid.
- **`EntryId` is the handle callers use**, not an array index — it survives other entries
  being added or removed, which matters because the UI holds a reference across a
  client→server round trip. Ids come from a server-side `NextEntryId` counter and are unique
  per holder, not globally.
- **Stacking is capped at the definition's `MaxStackSize` × the holder's `StackMultiplier`**
  (`GetEffectiveMaxStack`, never below 1). One number per holder covers "a shelf stacks deeper
  than a backpack" without per-transfer special cases; it's 1.0 everywhere today. Two entries
  merge only if `FInventoryItem::CanStackWith` agrees: same definition, the definition is
  actually stackable (`MaxStackSize > 1`), and both carry the same `bStolen` flag — so theft
  can't be laundered by merging. `Condition` is deliberately *not* compared, since splitting a
  bulk-material stack per wear value would fragment it uselessly.
- **Weight and footprint are two independent measures, and are meant to disagree.** Footprint
  is bulk/volume — how much *space* an item claims. Weight is density — `Definition->Weight` ×
  quantity, summed over every placed entry by `GetTotalWeight`, with no relation to how many
  cells those entries cover. A bundle of cloth stresses the grid; an ingot stresses the scale.
- **`WeightCapacity` is per holder and deliberately inert.** It's the denominator of the
  window's readout and nothing else — `IsOverWeightCapacity` is consulted only to colour that
  text. No mutator consults it, so being over capacity never blocks a placement, a transfer or
  a pickup. The movement-speed and stealth-noise penalties this figure eventually feeds belong
  to a later characters/combat pass, which owns those effects. A capacity of 0 means *no limit*
  (`HasWeightLimit`), which is what a static holder like a chest wants — nothing static carries
  anything anywhere.
- **Currency is per-player, not per-pawn, and lives outside the inventory entirely.** `Gold`
  is a replicated `int32` on `AStrategyPlayerState`, not an `FInventoryItem` — it has no
  weight, no footprint, and occupies no cell. Every sub-squad/division under one player draws
  from the same balance regardless of where in the world it is, because divisions are an
  organization layer rather than a separate economy. `AddGold` credits and `TrySpendGold`
  debits-if-affordable, both authority-only; `TrySpendGold` returning false is what a later
  purchase flow checks *before* touching any item, so a transaction can't half-apply.
- **Worn slots are a paperdoll, not a region of the grid.** `UEquipmentComponent` holds an
  `FEquippedItem` per *occupied* slot — a slot name plus one `FInventoryItem`, with no anchor
  cell and no rotation, because a slot is a named place rather than a rectangle. An absent slot
  is an empty one; there is no "empty slot" object, the same way `Entries` has no empty cells.
  A worn item is always quantity 1.
- **Slot-type matching is the only equip gate there is.** `CanEquipItem` is one comparison:
  the item's `Definition->EquipSlot` against the slot being filled, with `EEquipSlot::None`
  meaning "not wearable at all". No skill, attribute or condition check belongs here — any pawn
  may wear any weapon regardless of training, and the performance consequence of an untrained
  equip is combat/skill resolution's business (`combat.md`'s weapon-class mismatch penalty),
  never an inventory-side restriction.
- **`Equip` and `Unequip` are all-or-nothing.** Both find the displaced item a home with
  `FindFreePlacement` *before* mutating anything, and bail out having changed nothing if there
  isn't one — so running out of grid room can never destroy an item. `Equip` splits one unit off
  the source entry (removing it outright when that empties it) rather than moving the whole
  stack, so a stack of five knives arms five pawns. When the whole entry *is* consumed, the
  displaced item may be placed back onto the cells that entry is vacating; when it isn't, it
  can't.
- **A slot is filled from `EEquipSlot::None` or from a named slot, and both end up in the same
  place.** `None` means "wherever this item belongs", which is what right-click-to-equip sends;
  a drop on a specific paperdoll slot names that slot and still has to match it. There is no
  path that forces an item into a slot it doesn't belong in.
- **Equipped weight is tracked separately and folded into nothing.** A worn item leaves the grid,
  so `UInventoryComponent::GetTotalWeight` no longer counts it and
  `UEquipmentComponent::GetTotalWeight` does. Harmless while weight is inert; the pass that
  gives weight a consequence has to sum both.
- **A world pickup is one item instance, not a holder.** `AWorldItem` carries a single
  `FInventoryItem` and no `UInventoryComponent` at all — deliberately *not* an
  `AStrategyContainer`, which owns a whole grid and opens into a window. The whole interaction is
  "double-click it and it's yours", so there is nothing to open and nothing to drag out of. Its
  mesh is read from the held definition's `WorldMesh` rather than authored on the actor, which is
  what lets one Blueprint subclass serve every item type: contents are set per placed instance
  (or at spawn time), and the mesh follows.
- **Pickup is gated by proximity and nothing else**, through the same `InteractionRange` sphere
  and default radius a container uses — the roadmap's one proximity rule for every transfer
  context. The client picks the nearest in-range pawn, and the server re-checks both that
  proximity and that the named inventory really belongs to an `AStrategyPlayerUnit` before
  touching anything; range is the whole gate, so it can't be left on the requesting machine.
- **A pickup takes what fits and leaves the rest.** `AddItem` is already allowed to place part of
  a stack, so a pickup that read only its bool return would silently destroy the units that
  didn't land. `AddItemCounted` reports the quantity actually taken: the actor is destroyed only
  when the whole stack moved, and otherwise just shrinks. Taking nothing leaves it untouched.
- All mutation (`AddItem`, `AddItemAt`, `RemoveEntry`, `SetEntryQuantity`, `RepositionEntry`,
  `SetGridSize`, `Equip`, `Unequip`, `AWorldItem::SetItem`/`TryPickUp`) is authority-only; called on a non-authority machine, each is a silent no-op.
  `Entries`, `GridWidth`, `GridHeight` and `StackMultiplier` all replicate; `OnRep_Entries`
  re-broadcasts `OnInventoryChanged` on clients (authority already broadcasts it directly from
  the mutators, so it isn't double-fired there). `UEquipmentComponent` is the same shape one
  level down: `EquippedItems` replicates, `OnRep_EquippedItems` re-broadcasts
  `OnEquipmentChanged` on non-authority machines only.
- `AddItem` **merges before it places**: it fills existing stacks of the same type up to the
  effective cap first, then auto-places whatever's left via `FindFreePlacement`, splitting into
  as many entries as the cap requires. It returns `true` only if the *entire* quantity was
  taken — a partial add keeps what fit and warns about the rest. It rejects an item with no
  `Definition`, so a blank `StartingItems` entry places nothing.
- `FindFreePlacement` sweeps the whole grid in the item's natural orientation **before** trying
  the rotated one, so nothing gets turned sideways that didn't need to be; a square footprint
  skips the second pass entirely.
- `SetEntryQuantity` clamps up to the effective stack cap, and a new quantity of 0 or less
  removes the entry outright rather than leaving a zero-count ghost.
- **`MoveItem` never swaps.** It's the static entry point for both repositioning within one
  grid (`SourceInventory == DestInventory`) and transferring between two, and it resolves a
  drop in exactly one of three ways: merge into a stackable entry already covering the
  destination cell; reposition the existing entry (keeping its `EntryId`) when the whole stack
  moves within one grid; or place the item at the destination cell. A drop onto something that
  can't stack, or a footprint that doesn't fit, is **rejected whole** — two differently-shaped
  footprints have no well-defined exchange, so there is deliberately no swap path, and the UI
  simply redraws from unchanged replicated state (the item "snaps back"). All validation runs
  before anything is mutated, so a cross-inventory move can never half-apply; both components'
  authority is checked up front for the same reason.
- `MoveItem`'s `Quantity` parameter takes 0 or less to mean "the whole stack"; a smaller value
  splits it. A split leaves the source entry in place, so the new piece must find room *around*
  it — only a whole-entry move may reuse the cells it currently occupies. The UI always passes
  0 today; partial-stack drags are a later slice.
- This single method is what every current transfer path (reposition, pawn↔container,
  pawn↔loot) actually calls.
- A container's `InteractionRange` (a `USphereComponent`, default 312.5 units) gates whether
  a given unit may open it; the same proximity check shape (`IsUnitInRange`) is reused for
  Downed-NPC loot.
- A client-side drag-and-drop widget can't mutate a replicated, authority-only inventory
  directly — the inventory window's drop handler instead calls into `IInventoryMoveHost`
  (implemented by `AStrategyPlayerController`), whose `Server_MoveInventoryItem` RPC is the
  only path that actually calls `UInventoryComponent::MoveItem`.
- **The grid UI is two layers in one `UGridPanel`**: a `UInventoryCellWidget` per cell
  underneath (background and drop-preview highlight only, no item, no mouse handling), and a
  `UInventoryItemWidget` per placed entry above it, spanning its footprint via the grid slot's
  row/column spans. A `UUniformGridPanel` can't host the upper layer at all — `UUniformGridSlot`
  has no span — which is why the panel type is a hard requirement rather than a preference.
- **A window swallows the press of every mouse button that lands on it — including a
  double-click — and lets the release through.** The double-click half is a separate override,
  not a consequence of the press one: Windows sends `WM_xBUTTONDBLCLK` rather than
  `WM_xBUTTONDOWN` for the second click of a rapid pair, which reaches Slate as
  `OnMouseButtonDoubleClick`, an event with its own routing. A widget that overrides only
  `NativeOnMouseButtonDown` therefore catches the first click of a double-click and lets the
  second through to the viewport, where it registers as a press and completes whatever world
  action is bound to that button on release. Every widget in the stack that claims a button
  (`UWindowWidget`, `UInventoryItemWidget`, `UEquipmentSlotWidget`) routes its double-click
  handler straight into its press handler, so a fast second click means exactly what a slow one
  does. The release asymmetry is separate, and also deliberate. Swallowing the press is what stops a right-click on
  an inventory panel from also issuing a move order to the squad. Swallowing the *release* as
  well would be worse than useless: Enhanced Input never saw the press this window ate, so an
  action bound on release (most of them are) has nothing to complete anyway — whereas eating a
  release whose press the viewport *did* see (a drag-select begun on the world and ended over a
  window) would leave that button stuck down in `UPlayerInput` permanently. Slate bubbles up
  from the deepest widget, so the window only ever catches what nothing inside it claimed.
- **A window that closes itself says so.** `UWindowWidget::OnWindowClosed` fires after the
  window has removed itself, and the controller subscribes to every window it spawns. A close
  button can only remove its own widget; it can't take the companions that were opened with it
  (a pawn's paperdoll beside its pack), and it can't drop the input context scoped to a window
  being open. That's what this delegate is for, and it's why both window subclasses call
  `Super::RequestClose_Implementation()` *after* removing themselves.
- **Paperdoll slots handle their own drops, one per slot** — the opposite of the grid's rule
  below, and for the reason that rule exists: grid item widgets overlap grid cell widgets, so a
  per-cell handler can't tell which cell was hit. Equipment slots never overlap, so there's
  nothing to disambiguate.
- **One drop target serves the whole grid**, not one per cell. With item widgets sitting on top
  of cell widgets, per-cell drop handlers get ambiguous about which cell was actually hit; the
  drop bubbles up to the window instead, which recovers the cell from the panel's own geometry.
  That also means the cell size is never hardcoded — it falls out of the panel's arranged size,
  so a resized window still drops where it looks like it will.
- **A drag carries a grab offset**, the footprint cell the pointer came down on, and the drop
  anchor is the hovered cell *minus* that. The floating ghost is positioned by the matching
  fraction (`Offset = -GrabOffset / Footprint` against `EDragPivot::TopLeft`), so ghost and drop
  agree by construction — including after a mid-drag rotate, which transposes both together.
- **The rotate key is an Enhanced Input action on the player controller**, not a widget key
  handler. A `NativeOnKeyDown` on the inventory window would never fire: Slate routes key events
  along the *keyboard focus* path, and the window never takes focus. Enhanced Input works because
  it sits at the **end** of that path — on a click, Slate walks up from the (non-focusable)
  inventory widgets and focuses the game viewport, which holds the keyboard for the whole drag.
- **Its mapping context is scoped to the window being open.** `InventoryMappingContext` is added
  at priority 1 when an inventory or container window opens and removed when the last one
  closes (`UpdateInventoryInputContext`, called from all five open/close paths). That's what lets
  an inventory key reuse a key that means something else in the world, and it keeps every
  inventory binding player-rebindable alongside the gameplay ones — a key routed any other way
  would be invisible to Unreal's player key-mapping system and so unreachable from a settings
  screen.
- **The controller reaches the drag through `UInventoryDragDropOperation::GetActiveDrag()`.**
  Slate owns the in-flight drag; no widget and no controller holds a reference to it. That
  static is the bridge, and it lives in `SmoresUI` so the UMG drag plumbing stays out of the
  controller. Rotation is purely local UI state — the orientation only reaches the server on
  drop, through `Server_MoveInventoryItem`.

## C++ Implementation

- **Primary classes:**
  - `SmoresItems`: `UItemDefinition` (+ `EItemCategory` / `EEquipSlot`), `FInventoryItem`,
    `UInventoryComponent`, `FEquippedItem` + `UEquipmentComponent` (the paperdoll),
    `AStrategyContainer` (abstract base for world containers),
    `AStrategyChest` (first concrete container type — no added behavior of its own, exists
    so future chest-specific behavior like locks/keys has a home),
    `AWorldItem` (one loose item lying in the world — a replicated actor with a mesh and an
    interaction sphere, and no inventory component at all)
  - `SmoresUI`: `UWindowWidget` (reusable floating-window chrome), `UInventoryWidget`,
    `UInventoryCellWidget` (+ the `EInventoryCellHighlight` enum), `UInventoryItemWidget`,
    `UEquipmentWidget` (the paperdoll window), `UEquipmentSlotWidget`,
    `UInventoryDragDropOperation`, `IInventoryMoveHost`
  - `SmoresCharacters`: `AStrategyUnit` (owns the `Inventory` and `Equipment` subobjects shared
    by NPCs and player units alike), `AStrategyPlayerUnit`
  - `smores` (`Variant_Strategy`): `AStrategyPlayerController`, `AStrategyPlayerState` (the
    per-player gold balance)
- **Important methods:**
  - `FInventoryItem::GetDisplayName` / `GetIcon` / `GetItemId` / `GetDescription` /
    `GetTotalWeight` / `GetTotalBaseValue` / `GetFootprint` / `HasSameDefinitionAs` /
    `CanStackWith` — the read-through accessors that hide the definition indirection from
    callers; `GetFootprint(bRotated)` is the one place width/height get swapped for rotation
  - `FInventoryEntry::GetFootprint` / `CoversCell` / `IsValidEntry` — placement geometry, used
    by every occupancy test in the component and the UI
  - `UInventoryWidget::GetItemLabel` (static) — the one place an item's player-facing label
    is formatted (`"Name"`, or `"Name xN"` for a stack); shared by the summary text block and
    the per-cell widgets
  - `UInventoryComponent::CanPlaceAt` — the single overlap/bounds test. Its `IgnoreEntryId`
    parameter excludes one existing entry from the overlap check, which is what lets an entry
    be re-anchored onto cells it already occupies itself
  - `UInventoryComponent::FindFreePlacement` — auto-placement scan, natural orientation first
  - `UInventoryComponent::GetEntry` / `GetEntryIdAtCell` / `GetEffectiveMaxStack` /
    `GetFreeCellCount` — the read side used by the UI and by `MoveItem`
  - `UInventoryComponent::GetTotalWeight` / `GetWeightCapacity` / `HasWeightLimit` /
    `IsOverWeightCapacity` — the weight read side. Nothing but the UI calls any of them; no
    mutator consults `WeightCapacity`, which is what keeps the figure inert
  - `AStrategyPlayerState::AddGold` / `TrySpendGold` / `CanAfford` / `GetGold` — the currency
    surface. `AddGold`/`TrySpendGold` are authority-only and broadcast `OnGoldChanged`;
    `OnRep_Gold` re-broadcasts it on clients, same split as `UInventoryComponent`'s mutators
    vs. `OnRep_Entries`. `StartingGold` is applied once, on the server, in `BeginPlay`
  - `UInventoryComponent::AddItem` / `AddItemAt` / `RemoveEntry` / `SetEntryQuantity` /
    `RepositionEntry` / `SetGridSize` — authority-only mutators, all broadcast
    `OnInventoryChanged`
  - `UInventoryComponent::MoveItem` (static) — the single move/merge entry point for both
    repacking and cross-inventory transfer
  - `UEquipmentComponent::Equip` / `Unequip` — the two authority-only mutators, both validating
    (authority on *both* components, slot match, room for the displaced item) before touching
    anything, so neither can half-apply. `Equip` takes one unit off the source entry;
    `SetEntryQuantity` removes it outright if that was the last one
  - `UEquipmentComponent::CanEquipItem` — the whole equip gate, one comparison. The client-side
    drop preview and the server both call it, so the green/red slot can't promise a refusal
  - `UEquipmentComponent::GetSlotForItem` / `GetAllEquipSlots` / `GetSlotDisplayName` (all
    static) — the item's own slot, the fixed five-slot paperdoll roster in display order, and
    the slot's player-facing label read straight off the enum's `UMETA(DisplayName)` so there's
    no second name list to keep in step
  - `UEquipmentComponent::GetEquippedItem` / `IsSlotOccupied` / `GetTotalWeight` /
    `GetOwnerInventory` — the read side. `GetOwnerInventory` is the sibling lookup that decides
    where an unequipped item lands; `GetEquippedItem` is the seam a later combat pass reads the
    equipped weapon through
  - `UInventoryWidget::SetEquipmentTarget` / `GetEquipmentTarget` — what makes right-click mean
    "equip" in a pawn's own window and nothing anywhere else. The controller sets it when it
    opens a pawn window and never for a container or loot window, and `ClearInventory` drops it
    along with the inventory binding so a reused window can't carry the previous pawn's
    paperdoll across
  - `UInventoryItemWidget::TryEquip` — the right-click path: reaches the owning window through
    `GetTypedOuter<UInventoryWidget>()`, checks there's a target and that the item is wearable at
    all, then hops to the server. No target (a chest, a corpse) means silently nothing
  - `UEquipmentSlotWidget::WouldAcceptDrop` — the paperdoll's green/red, deliberately mirroring
    `Equip`'s checks *including* the displaced item's placement test, for the same reason
    `UInventoryWidget::WouldAcceptDrop` mirrors `MoveItem`'s
  - `UEquipmentWidget::RebuildSlots` / `RefreshDisplay` — the slot roster is fixed, so the slot
    widgets are built once per bound pawn and merely refreshed afterwards. That's the opposite of
    `RebuildGrid`, which rebuilds wholesale: a rebuild here would destroy the very widget a drag
    is hovering
  - `AStrategyPlayerController::Server_EquipItem` / `Server_UnequipItem` (`IInventoryMoveHost`) —
    the authoritative callers, doing no validation of their own for the same reason
    `Server_MoveInventoryItem` doesn't
  - `AStrategyPlayerController::OpenEquipmentForPawn` / `CloseEquipment` — the paperdoll window's
    lifecycle, driven entirely from `OpenInventoryForPawn`/`CloseInventory` so the two windows
    can't get out of step. `OpenInventoryForPawn`'s `bOpenEquipment` is what scopes the paperdoll
    to the inventory key: true only from `ToggleInventory`, false from the container and loot
    paths, which close any open paperdoll instead of leaving a stale one
  - `AStrategyPlayerController::HandleWindowClosed` — bound to every window this controller
    spawns (`UWindowWidget::OnWindowClosed`). Closing the pack with its X button takes the
    paperdoll with it; closing the paperdoll leaves the pack, which is what the player asked
    for; either way the scoped input context is re-evaluated
  - `AStrategyPlayerController::SmoresDumpEquipment` / `SmoresEquipItem <EntryIndex>` /
    `SmoresUnequipItem <SlotIndex>` (console execs) — debug-only, all three routed through one
    `Server_DebugEquipment` hop and all three ending in a per-slot dump plus the worn weight.
    `SmoresEquipItem` passes `EEquipSlot::None`, so it exercises the same "wherever it belongs"
    path right-click uses
  - `AStrategyPlayerController::SmoresDumpInventory` / `SmoresAddItem` (console execs) —
    debug-only. `SmoresDumpInventory` logs the selected pawn's **server-side** grid as an
    ASCII occupancy map plus a per-entry list (id, quantity, anchor, footprint, rotation,
    effective stack cap); `SmoresAddItem <Count>` adds `Count` more of whatever the pawn's
    first entry holds and then dumps, exercising stack-merge, auto-placement and the rotation
    fallback. Both hop to the server via `Server_DebugInventory`, since the local replicated
    copy isn't the authoritative one
  - `UInventoryComponent::AddItemCounted` — `AddItem` plus the quantity that actually landed.
    `AddItem` is now a one-line forwarder that discards the count. Any caller still holding the
    source copy (a world pickup today; a storefront purchase later) needs the count rather than
    the bool, since a partial add keeps what fit
  - `AWorldItem::TryPickUp` — authority-only; moves as much as will fit into the destination
    grid, destroys the actor when the whole stack moved and shrinks `Item` when only part did.
    Returns false having changed nothing when nothing fit
  - `AWorldItem::SpawnWorldItem` (static) — the authority-only spawn path, deferred so the
    item is in place before `OnConstruction` picks the mesh. The drop debug exec is its only
    caller today; the drag-an-item-onto-the-world gesture will be the second
  - `AWorldItem::SetItem` / `GetItem` / `RefreshMesh` / `OnRep_Item` — the item and its mesh.
    `RefreshMesh` runs from `OnConstruction` as well as `BeginPlay`, so setting `Item` on a placed
    instance updates the editor viewport immediately; `SetItem` refreshes directly because
    `OnRep_Item` only fires on the *other* machines
  - `AWorldItem::IsUnitInRange` — proximity gate, the same shape as the container's
  - `AStrategyPlayerController::FindWorldItemAtLocation` — nearest loose item within
    `WorldItemSelectionRadius` (100, deliberately tighter than `ContainerSelectionRadius`) of the
    double-clicked world location; mirrors `FindContainerAtLocation`
  - `AStrategyPlayerController::FindPlayerPawnInRangeOfWorldItem` — nearest player pawn actually
    close enough to collect it. Unlike `FindClosestPlayerPawn`, range is a filter here rather
    than a tiebreak, since proximity is the whole gate on a pickup
  - `AStrategyPlayerController::Server_PickUpWorldItem` — the authoritative pickup, and the one
    inventory RPC that *does* validate: it re-checks proximity and that the destination is a
    player pawn's own pack before calling `TryPickUp`
  - `AStrategyPlayerController::SmoresDropItem <EntryIndex>` (console exec) — debug-only; spawns
    the pawn's EntryIndex'th grid entry on the ground in front of it via `Server_DebugDropItem`,
    so the pickup path has something to pick up without hand-placing actors. Spawns first and
    removes the entry only on success, so a refused spawn can't destroy the item
  - `AStrategyContainer::IsUnitInRange` — proximity gate, same shape used for loot
  - `AStrategyPlayerController::ToggleInventory` / `OpenInventoryForPawn` / `CloseInventory`
    — pawn inventory window lifecycle; requires exactly one selected `AStrategyPlayerUnit`
  - `AStrategyPlayerController::ToggleContainer` / `OpenContainer` / `OpenLoot` /
    `CloseContainer` — one shared window (`ContainerWidget`) reused for both world
    containers and Downed-NPC loot; only the window title differs, set at open time
  - `AStrategyPlayerController::FindContainerInRange` / `FindLootableNPCInRange` — used by
    the toggle-key path (checks `ControlledUnits`/`SelectedNPC` only)
  - `AStrategyPlayerController::FindContainerAtLocation` / `FindLootableNPCAtLocation` —
    used by the double-click path (checks every player pawn, within `ContainerSelectionRadius`)
  - `AStrategyPlayerController::SelectAllDoubleClick` — the one gesture behind four meanings,
    resolved by type in order: loose world item, container, Downed NPC, then select-all-on-screen.
    The world item goes first because it's the smallest thing under the cursor and the only one
    of the three with no selection state to set — finding one either collects it or does nothing
  - `AStrategyPlayerController::GetPlayerGold` (`IStrategyResourceHost`) —
    `AStrategyHUD::DrawHUD` polls it each frame and pushes the result into
    `UStrategyUI::SetGold`. The interface exists because the balance lives on a `smores`
    gameplay class that `SmoresUI` deliberately can't see; it's the same narrow-interface
    shape as `IStrategySelectionHost`/`IStrategyCameraCommands`
  - `AStrategyPlayerController::SmoresAddGold` / `SmoresSpendGold` (console execs) —
    debug-only, both hopping to the server via `Server_DebugGold` since the balance is
    server-owned. `SmoresSpendGold` past the balance logs `REJECTED` and changes nothing
  - `UStrategyUI::SetGold` / `GetGoldLabel` — `SetGold` early-outs when the balance is
    unchanged (the HUD pushes every frame), so `NativeConstruct` refreshes once on its own to
    cover a widget built after the first push
  - `UInventoryWidget::GetWeightSummary` / `IsOverWeightCapacity` — the window's weight
    readout and the red/normal colour choice; `RefreshDisplay` applies both, so weight tracks
    `OnInventoryChanged` with no separate subscription
  - `AStrategyPlayerController::Server_MoveInventoryItem_Implementation` — the sole
    authoritative caller of `UInventoryComponent::MoveItem` from UI drag-drop. Carries entry
    id, destination cell, rotation and quantity; it does no validation of its own, since
    `MoveItem` does all of it
  - `UInventoryWidget::SetInventory` / `ClearInventory` — binds/unbinds an inventory,
    subscribes to `OnInventoryChanged`
  - `UInventoryWidget::RebuildGrid` — builds the grid's two layers into the `UGridPanel`:
    one `UInventoryCellWidget` per cell at `UGridSlot` layer 0, one `UInventoryItemWidget` per
    placed entry at layer 1 with `SetRowSpan`/`SetColumnSpan` from its footprint. Equal
    `SetColumnFill`/`SetRowFill` across the grid is what keeps cells uniform, which
    `ScreenPositionToCell` depends on
  - `UInventoryWidget::ScreenPositionToCell` / `GetDropAnchorCell` — the geometry half of the
    single grid-level drop target: panel-relative pointer position → cell, then minus the
    drag's `GrabOffset` to get the anchor the item's top-left lands on
  - `UInventoryWidget::WouldAcceptDrop` — the preview's yes/no, deliberately mirroring
    `MoveItem`'s resolution (merge into a stackable entry at the anchor, else `CanPlaceAt`)
    so the highlight can't promise a drop the server will reject
  - `UInventoryWidget::UpdateDragPreview` / `ClearDragPreview` — marks the covered cells
    Valid/Invalid and tears the preview down again; subscribed to the drag's `OnRotated` so a
    mid-drag rotate redraws without waiting for the pointer to move
  - `UInventoryItemWidget::SetEntry` / `SetPreviewOrientation` / `NativeOnDragDetected` —
    binds one placed entry, re-draws the floating ghost at a new orientation, and starts the
    drag (recording which footprint cell the pointer grabbed)
  - `UInventoryDragDropOperation::GetActiveDrag` / `ToggleRotation` / `ApplyPreview` — the
    rotate path: the static that finds the in-flight drag through Slate, the transpose it
    applies to `bRotated` + `GrabOffset`, and the decorator repositioning that keeps ghost and
    drop agreeing afterwards
  - `AStrategyPlayerController::RotateDraggedItem` / `UpdateInventoryInputContext` — the
    `IA_Strategy_RotateDraggedItem` handler, and the add/remove of `InventoryMappingContext`
    that scopes every inventory key to a window actually being open
- **Runtime ownership:** `UInventoryComponent` is a default subobject of `AStrategyUnit`
  (every unit, NPC or player-controlled) and of `AStrategyContainer` (every world
  container); `UEquipmentComponent` is a default subobject of `AStrategyUnit` only — a chest
  wears nothing. The two `UInventoryWidget` instances (`InventoryWidget`, `ContainerWidget`)
  and the one `UEquipmentWidget` (`EquipmentWidget`)
  are lazy-created and owned by `AStrategyPlayerController` directly — **not** by
  `AStrategyHUD`, which today only spawns the general `UStrategyUI` widget, pushes the
  selection count / target label / gold balance into it each frame, and draws the
  drag-selection box; it has no inventory role. `AStrategyPlayerState` is spawned per player
  by the game mode (`PlayerStateClass`), so the gold balance is scoped to one player and
  never to the world.
- **Data flow (drag-and-drop transfer):** `UInventoryItemWidget::NativeOnDragDetected`
  (source item) → `UInventoryDragDropOperation` payload (source inventory + entry id + a copy
  of the item + rotation + grab offset + cell size) → the target window's
  `UInventoryWidget::NativeOnDrop` (converts the pointer position to a cell, minus the grab
  offset) → `IInventoryMoveHost::Server_MoveInventoryItem` (client → server RPC via the owning
  `AStrategyPlayerController`) → `UInventoryComponent::MoveItem` → `Entries` replicates back
  down → `OnRep_Entries` → `OnInventoryChanged` → `UInventoryWidget` refreshes on every
  observing client. A rejected move mutates nothing, so the refresh redraws the unchanged
  state and the item appears to snap back — the client is never told "no" explicitly, which
  is exactly why the red drop preview exists.
- **Data flow (equip):** `UInventoryItemWidget::NativeOnMouseButtonDown` (right button) →
  `TryEquip` → the owning window's `GetEquipmentTarget` →
  `IInventoryMoveHost::Server_EquipItem` (client → server RPC) → `UEquipmentComponent::Equip` →
  `EquippedItems` *and* `Entries` both replicate back down → `OnRep_EquippedItems` /
  `OnRep_Entries` → `OnEquipmentChanged` / `OnInventoryChanged` → the paperdoll window and the
  inventory window each refresh independently. A drop onto a paperdoll slot is the same path
  from `UEquipmentSlotWidget::NativeOnDrop`, differing only in naming the slot instead of
  sending `EEquipSlot::None`; unequip is the mirror through `Server_UnequipItem`. A rejected
  equip is silent on the wire, exactly like a rejected move.

## Blueprint / Asset Dependencies

- **`DA_Item_*`** (`Content/Items/`) — `UItemDefinition` assets, one per item type.
  Footprints and stack sizes are authored: `Apple` 1×1 stack 10, `GoldCoin` 1×1 stack 100,
  `HealthPotion` 1×1 stack 5, `PocketKnife` 1×1, `Torch` 1×2, `Rope` 2×2, `TrapKit` 2×2,
  `IronSword` 1×3. `Icon` and `WorldMesh` are unassigned on all of them — no item art exists
  yet. Because a definition is the only thing a carried item references, **deleting one
  empties every entry holding it**, and **changing a footprint doesn't re-validate already
  placed entries** — an item grown larger can leave overlapping placements until something
  moves them.
- **`WBP_Inventory`** (`Content/Variant_Strategy/UI/`) — `UInventoryWidget` subclass,
  assigned to `AStrategyPlayerController::InventoryWidgetClass`. Shows the selected pawn's
  own inventory.
- **`WBP_ContainerInventory`** — a second `UInventoryWidget` subclass, assigned to
  `ContainerWidgetClass`. Reused for both world containers and Downed-NPC loot; only the
  window title differs at open time.
  Both hold a `UGridPanel` named `SlotContainer` — **not** a `UUniformGridPanel`, which has no
  slot span and so cannot host a footprint-spanning item widget at all. C++ casts and logs a
  warning if the panel is the wrong type or `CellWidgetClass` is unset, since either one
  silently breaks the drop maths. A plain `SlotListText` text block is the fallback visual when
  no grid is wired; it lists each placed entry with its quantity, anchor cell, footprint and
  rotation.
- **`GoldText`** — a `UTextBlock` in `UI_Strategy`, sitting beside `SelectionCount` in the
  same horizontal strip. Optional (`BindWidgetOptional`); C++ fills its text, so no Blueprint
  property binding is involved. `WeightText` in `WBP_Inventory`/`WBP_ContainerInventory` works
  the same way, sitting between the title bar and the grid.
- **`BP_StrategyPlayerState`** — `AStrategyPlayerState` subclass holding `StartingGold` (250).
  It is assigned to `BP_StrategyGameMode`'s `PlayerStateClass`; without that assignment the
  game mode spawns a plain engine `APlayerState`, the HUD shows `Gold: 0` forever, and the
  gold execs log "No AStrategyPlayerState".
- **`WBP_InventoryCell`** — `UInventoryCellWidget` subclass, assigned to
  `UInventoryWidget::CellWidgetClass`. One instance per **grid cell**, at grid layer 0. Just a
  `UBorder` named `CellBorder`, whose tint C++ drives from the highlight state; the brush
  itself is a plain filled box. Its root must stay hit-test `Visible` so the grid reads as one
  continuous drop surface.
- **`WBP_InventoryItem`** — `UInventoryItemWidget` subclass, assigned to
  `UInventoryWidget::ItemWidgetClass`. One instance per **placed entry**, at grid layer 1,
  spanning its footprint. Tree is `ItemSizeBox` (`USizeBox`, **no** width/height overrides
  authored — C++ clears them for grid instances and sets them for the drag ghost) → a border →
  `ItemLabel` (`UTextBlock`, centred, `AutoWrapText` off). Nothing in that tree may clip to
  bounds: C++ turns the label 90° with a render transform, which doesn't affect layout, so the
  label has to be free to overflow its box.
- **`IA_Strategy_Inventory`** (`Content/Variant_Strategy/Input/Actions/`) — bound to
  `ToggleInventoryAction`. Desktop-only; not mapped in the touch `InputMappingContext`.
- **`IA_Strategy_ToggleContainer`** — bound to `ToggleContainerAction`. Also desktop-only.
- **`IA_Strategy_RotateDraggedItem`** — bound to `RotateDraggedItemAction`; rotates the item
  being dragged. Mapped in `IMC_Strategy_Inventory` (not the always-on mouse context), which the
  controller adds at priority 1 only while an inventory window is open. Bound on
  `ETriggerEvent::Started` so the item turns on key press rather than release — never
  `Triggered`, which for a held key would spin the item once per frame.
- **`AStrategyContainer` / `AStrategyChest` Blueprint subclasses** — assign `ContainerMesh`'s
  materials (`NormalMaterial`/`SelectedMaterial`), `ContainerDisplayName`, and populate
  `StartingItems`. `StartingItems` is authored **entirely in Blueprint** (and per placed
  instance, as the "Chest 2" actor in `LVL_Strategy` does) — no C++ constructor seeds it,
  since C++ shouldn't hard-code content paths. `BP_Chest` sets its `Inventory` subobject to an
  8×6 grid.
- **Unit Blueprints** — `AStrategyPlayerUnit::StartingItems` is likewise Blueprint-authored
  (`BP_PlayerUnit` seeds an Apple and a Pocket Knife) and its `Inventory` subobject is a 6×4
  grid with the default 30 `WeightCapacity`. `BP_Chest` sets its capacity to **0** (no limit),
  since a chest doesn't carry anything anywhere.
  `GridWidth`/`GridHeight`/`StackMultiplier`/`WeightCapacity` are the per-holder knobs to
  override on any new holder Blueprint; `Entries` itself is not editable, so starting contents
  always go through `StartingItems` and `AddItem`'s auto-placement.
- **`BP_WorldItem`** — the concrete `AWorldItem` subclass, assigned to
  `AStrategyPlayerController::WorldItemClass` (which the drop debug exec spawns, and which logs a
  warning naming the controller if it's unset). One Blueprint serves every item type: `ItemMesh`
  is driven from the held definition, so the only thing worth authoring per placed instance is
  `Item` (its definition and quantity). The mesh is set to `NoCollision` in C++ — a dropped item
  should never shove a pawn around or block a selection trace, and the interaction sphere is what
  actually gates reaching it.
- **`DA_Item_*` weights and values are authored** (Apple 0.2/2g, GoldCoin 0.01/1g,
  HealthPotion 0.5/25g, PocketKnife 0.3/15g, Torch 0.8/5g, Rope 2.0/12g, IronSword 3.5/90g,
  TrapKit 4.0/60g) — a definition with a zero `Weight` contributes nothing to the readout, so
  a new item type that forgets to set one looks weightless rather than broken.

## Extension Points

- **New item types** — add a `DA_Item_*` asset under `Content/Items/`; no code change
  needed. `UItemDefinition`'s `GetPrimaryAssetId` keys off the authored `ItemId` (falling
  back to the asset name when it's blank), so definitions can be renamed or moved without
  breaking ID-based lookups once an asset-manager path needs them.
- **New holder types** — anything with a `UInventoryComponent` gets the grid for free; size
  it with `GridWidth`/`GridHeight` and set `StackMultiplier` above 1.0 for a holder meant to
  stack deeper than a pawn's pack (a storefront shelf, a warehouse chest). Nothing else needs
  a per-holder code path.
- **Trading, theft, sort/filter** — none of this exists in code yet, though `UItemDefinition`
  already carries the fields they'll read (`BaseValue`, `Category`). Full target design and
  rationale live in `inventory-roadmap.md`.
- **Dropping an item into the world from the UI** — the authority-side half is built and
  verified: `AWorldItem::SpawnWorldItem` plus a `WorldItemClass` on the controller. What's
  missing is only the gesture. Dragging an item out of a window and releasing it over the world
  is the obvious one, and the hook is `UDragDropOperation::DragCancelled` — but that fires for
  *every* unhandled drop, including a drag cancelled with Escape or by a window closing
  mid-drag, so wiring it naively turns a stray click into a dropped item. Design the confirmation
  before the plumbing; `SmoresDropItem` covers testing in the meantime.
- **A holder-generic proximity check** — `AWorldItem::IsUnitInRange`,
  `AStrategyContainer::IsUnitInRange` and `AStrategyUnit::IsUnitInRange` are now three copies of
  the same three-line distance test against an `InteractionRange` sphere. That's the
  `IInventoryHolder` interface the roadmap's Slice 7 collapses them into, not three separate
  things that happen to look alike.
- **New worn slots** — add to `EEquipSlot` *and* to `UEquipmentComponent::GetAllEquipSlots`,
  which is a deliberate hand-written roster rather than an enum iteration: it fixes the
  paperdoll's display order and keeps `None` out of it. Nothing else needs a code change — the
  paperdoll builds one slot widget per entry in that list.
- **Combat reading the equipped weapon** — `UEquipmentComponent::GetEquippedItem(MainHand)` is
  the seam, and it's readable on any machine since `EquippedItems` replicates. That's a
  `SmoresCombat` change, not an inventory one; nothing here should learn what a weapon *does*.
- **Equipped visuals** — `UItemDefinition::WorldMesh` is authored for it and nothing reads it
  yet. Attaching a mesh to a skeletal socket on `OnEquipmentChanged` is purely cosmetic and
  belongs on the character/animation side.
- **Starting equipment** — there's no `StartingEquipment` counterpart to `StartingItems`; a pawn
  starts wearing nothing. Seed it the same way if it's wanted: a Blueprint-authored array, applied
  once server-side in `BeginPlay`, never hard-coded in C++.
- **Spending gold** — `AStrategyPlayerState::TrySpendGold` is the seam a purchase runs
  through: verify proximity, call it, and only move the item if it returned true, all inside
  one server-side call so the transaction can't half-apply. Nothing calls it yet outside the
  debug exec.
- **Encumbrance effects** — `IsOverWeightCapacity` is the seam, and it's deliberately consulted
  by nothing but the readout's colour. A characters/combat pass that wants a speed or noise
  penalty reads it from there rather than reaching into `GetTotalWeight` itself.
- **New inventory input** — three shapes; pick by what already carries the state:
  - *Modifier + mouse button* (Ctrl+click to split a stack, Shift+click to quick-transfer): read
    `IsControlDown()`/`IsShiftDown()` straight off the click event in the widget's own handler.
    No action asset, no mapping context, no controller involvement — the event already carries
    the modifier state.
  - *A key pressed while a window is open or a drag is in flight*: a new `UInputAction`, mapped
    in `IMC_Strategy_Inventory` (the context scoped to a window being open — **not** the
    always-on mouse context), bound on `AStrategyPlayerController`. If it acts on the drag rather
    than the window, reach it with `UInventoryDragDropOperation::GetActiveDrag()`; if it changes
    shared state, it still goes through a server RPC like every other mutation.
  - *Never* a `NativeOnKeyDown` on an inventory widget (these widgets never hold keyboard focus,
    so it cannot fire) and *never* a Slate input pre-processor (invisible to player rebinding).
  The `UInputAction` asset can be made by duplicating an existing Boolean one; the
  `IMC_Strategy_Inventory` mapping must be authored by hand in the editor. **Check
  `input-and-keybinds.md` before picking a key** — it holds every current binding and the
  reserved list, and is where a new one gets recorded.
- **Save/load** — `FInventoryItem` and `UInventoryComponent`'s state are fully
  `UPROPERTY`-reflected; no struct changes are needed for whatever serialization approach
  the save system eventually adopts.

## Known Gaps

- No partial-stack drag — the UI always moves the whole stack even though `MoveItem` already
  takes a quantity and supports the split. Splitting needs a player-facing way to say "how
  many", which hasn't been designed.
- A rejected drop is still silent on the wire. The red preview is computed client-side by
  `WouldAcceptDrop` mirroring `MoveItem`'s rules, so the two can drift apart if only one is
  changed; and a drop rejected for a reason the client can't see (a race against another
  player's move) shows no explanation at all. A context that needs to say *why* — insufficient
  gold, say — will need a new client RPC.
- No re-validation when an item definition's footprint changes under already-placed entries —
  they can end up overlapping until something moves them.
- `SetGridSize` drops entries that no longer fit rather than re-packing them; it's an
  authoring/debug operation, not something gameplay calls.
- **Equipped weight is reported but never summed with carried weight.** Wearing an item takes it
  out of the grid, so a pawn's `Weight:` readout *drops* when it equips something and the worn
  total is shown in a different window. Nothing reads either figure, so it costs nothing today —
  but the pass that gives weight a gameplay consequence has to add the two, and decide whether
  the readouts merge.
- **Equipment has no ownership check on the wire.** `Server_EquipItem`/`Server_UnequipItem` do no
  more validation than `Server_MoveInventoryItem` does — a client could name any pawn's
  equipment component. Consistent with the existing inventory RPCs rather than a new hole, and
  the fix belongs to all of them at once (see the multiplayer-discipline notes in
  `unreal-module-organization.md`), not to equipment alone.
- No equipped visuals — `WorldMesh` is authored but nothing attaches it to a socket, so an
  equipped item disappears from view entirely rather than showing on the pawn.
- No starting equipment — every pawn starts with empty worn slots; only `StartingItems` is
  seeded.
- **A loose world item is invisible.** `AWorldItem` draws the definition's `WorldMesh`, and no
  `DA_Item_*` has one authored — so a placed or dropped item is a clickable actor with nothing to
  look at. The pickup works; there's just no art behind it.
- **A world item can't be reached, only collected.** There's no "go pick that up" order — if no
  pawn is already in range the double-click does nothing, silently. Routing the pickup through a
  move command is a unit-commands change, not an inventory one.
- **`AWorldItem` replicates but `AStrategyContainer` doesn't.** The pickup actor sets
  `bReplicates` because it's spawned and destroyed during play, so a client that never received
  it would have nothing to click. The container class has never set it — harmless while
  multiplayer isn't testable, but it means a chest's replicated `Inventory` currently has no
  replicated actor to ride on. Fixing it belongs to a multiplayer pass over all the holders at
  once, not to this slice.
- No item art — every `DA_Item_*` has a null `Icon` and `WorldMesh`, so the UI shows names
  only. The item widget draws no icon at all yet, which is why the label's 90° turn for tall
  footprints matters as much as it does.
- No trading or purchase flow — gold exists and replicates, but the only things that move it
  are the `SmoresAddGold`/`SmoresSpendGold` debug execs. It also isn't persisted anywhere,
  and a player state re-created mid-session re-seeds from `StartingGold`.
- **Weight has no gameplay consequence.** Nothing reads `IsOverWeightCapacity` but the
  readout's colour — no speed penalty, no noise penalty, and no pickup is ever refused for
  being too heavy. That's a deliberate deferral, not an oversight, but it does mean an
  over-capacity pawn looks warned-about while nothing is actually happening.
- The gold readout is **polled from `DrawHUD` every frame**, not driven by
  `OnGoldChanged`. That delegate exists and fires correctly on both sides, but nothing
  subscribes to it yet — a consumer that needs to *react* to a balance change (a purchase
  confirmation, an alert) should bind it rather than add a second poll.
- `WeightCapacity` replicates but never changes at runtime, so no refresh is wired to it —
  a future system that varies capacity (a pack upgrade, a strength attribute) needs to
  broadcast `OnInventoryChanged` itself, since only entry changes redraw the window today.
- No world-loose item pickups — only container- and unit-held inventories exist; nothing
  can be dropped in or picked up directly from the world today.
- No stolen-item flag or theft/detection mechanics.
