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
to fit, and merge into stacks capped per holder. This system does **not** yet handle:
equipment/worn slots, weight/encumbrance, currency, trading/purchase, theft, or world-loose
item pickups. See `inventory-roadmap.md` for the target design covering all of that — this
topic only documents what's actually built.

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
- **Interim visuals.** Until the grid UI slice lands, each grid cell is its own widget and a
  multi-cell item draws its label only on its top-left cell, so a 1×3 sword reads as one
  labelled cell followed by two blank ones. There's no rotate-while-dragging key yet either —
  a dragged item keeps whatever orientation it was placed at. Rotation *is* applied
  automatically when an item is auto-placed and only fits sideways.
- Deselecting all units, or having no player pawn selected/in range, closes any open
  inventory window.

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
- All mutation (`AddItem`, `AddItemAt`, `RemoveEntry`, `SetEntryQuantity`, `RepositionEntry`,
  `SetGridSize`) is authority-only; called on a non-authority machine, each is a silent no-op.
  `Entries`, `GridWidth`, `GridHeight` and `StackMultiplier` all replicate; `OnRep_Entries`
  re-broadcasts `OnInventoryChanged` on clients (authority already broadcasts it directly from
  the mutators, so it isn't double-fired there).
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
  directly — the slot widget's drop handler instead calls into `IInventoryMoveHost`
  (implemented by `AStrategyPlayerController`), whose `Server_MoveInventoryItem` RPC is the
  only path that actually calls `UInventoryComponent::MoveItem`.

## C++ Implementation

- **Primary classes:**
  - `SmoresItems`: `UItemDefinition` (+ `EItemCategory` / `EEquipSlot`), `FInventoryItem`,
    `UInventoryComponent`, `AStrategyContainer` (abstract base for world containers),
    `AStrategyChest` (first concrete container type — no added behavior of its own, exists
    so future chest-specific behavior like locks/keys has a home)
  - `SmoresUI`: `UWindowWidget` (reusable floating-window chrome), `UInventoryWidget`,
    `UInventorySlotWidget`, `UInventoryDragDropOperation`, `IInventoryMoveHost`
  - `SmoresCharacters`: `AStrategyUnit` (owns the `Inventory` subobject shared by NPCs and
    player units alike), `AStrategyPlayerUnit`
  - `smores` (`Variant_Strategy`): `AStrategyPlayerController`
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
  - `UInventoryComponent::AddItem` / `AddItemAt` / `RemoveEntry` / `SetEntryQuantity` /
    `RepositionEntry` / `SetGridSize` — authority-only mutators, all broadcast
    `OnInventoryChanged`
  - `UInventoryComponent::MoveItem` (static) — the single move/merge entry point for both
    repacking and cross-inventory transfer
  - `AStrategyPlayerController::SmoresDumpInventory` / `SmoresAddItem` (console execs) —
    debug-only. `SmoresDumpInventory` logs the selected pawn's **server-side** grid as an
    ASCII occupancy map plus a per-entry list (id, quantity, anchor, footprint, rotation,
    effective stack cap); `SmoresAddItem <Count>` adds `Count` more of whatever the pawn's
    first entry holds and then dumps, exercising stack-merge, auto-placement and the rotation
    fallback. Both hop to the server via `Server_DebugInventory`, since the local replicated
    copy isn't the authoritative one
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
  - `AStrategyPlayerController::Server_MoveInventoryItem_Implementation` — the sole
    authoritative caller of `UInventoryComponent::MoveItem` from UI drag-drop. Carries entry
    id, destination cell, rotation and quantity; it does no validation of its own, since
    `MoveItem` does all of it
  - `UInventoryWidget::SetInventory` / `ClearInventory` — binds/unbinds an inventory,
    subscribes to `OnInventoryChanged`
  - `UInventorySlotWidget::SetCell` / `NativeOnDragDetected` / `NativeOnDrop` — binds one
    grid cell to whichever entry covers it, then creates/consumes a
    `UInventoryDragDropOperation`, routing the move through the owning player controller's
    `IInventoryMoveHost`
- **Runtime ownership:** `UInventoryComponent` is a default subobject of `AStrategyUnit`
  (every unit, NPC or player-controlled) and of `AStrategyContainer` (every world
  container). The two `UInventoryWidget` instances (`InventoryWidget`, `ContainerWidget`)
  are lazy-created and owned by `AStrategyPlayerController` directly — **not** by
  `AStrategyHUD`, which today only spawns the general `UStrategyUI` widget and draws the
  drag-selection box; it has no inventory role.
- **Data flow (drag-and-drop transfer):** `UInventorySlotWidget::NativeOnDragDetected`
  (source cell) → `UInventoryDragDropOperation` payload (source inventory + entry id +
  rotation) → target `UInventorySlotWidget::NativeOnDrop` (supplies its own cell coordinate)
  → `IInventoryMoveHost::Server_MoveInventoryItem` (client → server RPC via the owning
  `AStrategyPlayerController`) → `UInventoryComponent::MoveItem` → `Entries` replicates back
  down → `OnRep_Entries` → `OnInventoryChanged` → `UInventoryWidget` refreshes on every
  observing client. A rejected move mutates nothing, so the refresh redraws the unchanged
  state and the item appears to snap back — the client is never told "no" explicitly.

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
- **`WBP_InventorySlot`** — `UInventorySlotWidget` subclass, assigned to
  `UInventoryWidget::SlotWidgetClass`. One instance is spawned per **grid cell**. The widget
  needs a `UPanelWidget` named `SlotContainer` on the owning
  `WBP_Inventory`/`WBP_ContainerInventory`; a `UUniformGridPanel` lays the cells out as the
  bound inventory's actual `GridWidth` × `GridHeight` grid (the old `GridColumns` property is
  gone — the width comes from the inventory now), and any other `UPanelWidget` renders a flat
  list. A plain `SlotListText` text block is the fallback default visual if
  `SlotContainer`/`SlotWidgetClass` aren't set; it lists each placed entry with its quantity,
  anchor cell, footprint and rotation.
- **`IA_Strategy_Inventory`** (`Content/Variant_Strategy/Input/Actions/`) — bound to
  `ToggleInventoryAction`. Desktop-only; not mapped in the touch `InputMappingContext`.
- **`IA_Strategy_ToggleContainer`** — bound to `ToggleContainerAction`. Also desktop-only.
- **`AStrategyContainer` / `AStrategyChest` Blueprint subclasses** — assign `ContainerMesh`'s
  materials (`NormalMaterial`/`SelectedMaterial`), `ContainerDisplayName`, and populate
  `StartingItems`. `StartingItems` is authored **entirely in Blueprint** (and per placed
  instance, as the "Chest 2" actor in `LVL_Strategy` does) — no C++ constructor seeds it,
  since C++ shouldn't hard-code content paths. `BP_Chest` sets its `Inventory` subobject to an
  8×6 grid.
- **Unit Blueprints** — `AStrategyPlayerUnit::StartingItems` is likewise Blueprint-authored
  (`BP_PlayerUnit` seeds an Apple and a Pocket Knife) and its `Inventory` subobject is a 6×4
  grid. `GridWidth`/`GridHeight`/`StackMultiplier` are the per-holder knobs to override on any
  new holder Blueprint; `Entries` itself is not editable, so starting contents always go
  through `StartingItems` and `AddItem`'s auto-placement.

## Extension Points

- **New item types** — add a `DA_Item_*` asset under `Content/Items/`; no code change
  needed. `UItemDefinition`'s `GetPrimaryAssetId` keys off the authored `ItemId` (falling
  back to the asset name when it's blank), so definitions can be renamed or moved without
  breaking ID-based lookups once an asset-manager path needs them.
- **New holder types** — anything with a `UInventoryComponent` gets the grid for free; size
  it with `GridWidth`/`GridHeight` and set `StackMultiplier` above 1.0 for a holder meant to
  stack deeper than a pawn's pack (a storefront shelf, a warehouse chest). Nothing else needs
  a per-holder code path.
- **Equipment, currency, trading, theft, world pickups, sort/filter** — none of this exists in
  code yet, though `UItemDefinition` already carries the fields they'll read (`EquipSlot`,
  `BaseValue`, `WorldMesh`, `Category`). Full target design and rationale live in
  `inventory-roadmap.md`.
- **Save/load** — `FInventoryItem` and `UInventoryComponent`'s state are fully
  `UPROPERTY`-reflected; no struct changes are needed for whatever serialization approach
  the save system eventually adopts.

## Known Gaps

- The grid UI is interim: one widget per cell with the label on the anchor cell only, no item
  widget spanning its footprint, no rotate key while dragging, and no partial-stack drag (the
  UI always moves the whole stack even though `MoveItem` supports a split).
- No re-validation when an item definition's footprint changes under already-placed entries —
  they can end up overlapping until something moves them.
- `SetGridSize` drops entries that no longer fit rather than re-packing them; it's an
  authoring/debug operation, not something gameplay calls.
- No equipment/worn slots distinct from the general carried grid — `EquipSlot` is authored
  but unread.
- No item art — every `DA_Item_*` has a null `Icon` and `WorldMesh`, so the UI shows names
  only.
- No currency, trading, or purchase flow.
- No weight/encumbrance tracking.
- No world-loose item pickups — only container- and unit-held inventories exist; nothing
  can be dropped in or picked up directly from the world today.
- No stolen-item flag or theft/detection mechanics.
