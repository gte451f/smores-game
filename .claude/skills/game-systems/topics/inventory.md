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
neither *does* anything yet: weight applies no penalty, and nothing spends gold. This system
does **not** yet handle: equipment/worn slots, trading/purchase, theft, or world-loose item
pickups. See `inventory-roadmap.md` for the target design covering all of that — this topic
only documents what's actually built.

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
  directly — the inventory window's drop handler instead calls into `IInventoryMoveHost`
  (implemented by `AStrategyPlayerController`), whose `Server_MoveInventoryItem` RPC is the
  only path that actually calls `UInventoryComponent::MoveItem`.
- **The grid UI is two layers in one `UGridPanel`**: a `UInventoryCellWidget` per cell
  underneath (background and drop-preview highlight only, no item, no mouse handling), and a
  `UInventoryItemWidget` per placed entry above it, spanning its footprint via the grid slot's
  row/column spans. A `UUniformGridPanel` can't host the upper layer at all — `UUniformGridSlot`
  has no span — which is why the panel type is a hard requirement rather than a preference.
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
    `UInventoryComponent`, `AStrategyContainer` (abstract base for world containers),
    `AStrategyChest` (first concrete container type — no added behavior of its own, exists
    so future chest-specific behavior like locks/keys has a home)
  - `SmoresUI`: `UWindowWidget` (reusable floating-window chrome), `UInventoryWidget`,
    `UInventoryCellWidget` (+ the `EInventoryCellHighlight` enum), `UInventoryItemWidget`,
    `UInventoryDragDropOperation`, `IInventoryMoveHost`
  - `SmoresCharacters`: `AStrategyUnit` (owns the `Inventory` subobject shared by NPCs and
    player units alike), `AStrategyPlayerUnit`
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
  container). The two `UInventoryWidget` instances (`InventoryWidget`, `ContainerWidget`)
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
- **Equipment, trading, theft, world pickups, sort/filter** — none of this exists in code
  yet, though `UItemDefinition` already carries the fields they'll read (`EquipSlot`,
  `BaseValue`, `WorldMesh`, `Category`). Full target design and rationale live in
  `inventory-roadmap.md`.
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

- **`UWindowWidget` swallows only left clicks.** Every other mouse button passes through to the
  world underneath, so right-clicking on an open inventory window issues a move order to the
  selected squad. This blocks right-click-to-equip (Slice 5) and any Ctrl/Shift+right-click
  transfer, and should be fixed button-agnostically rather than by special-casing a second
  button.
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
- No equipment/worn slots distinct from the general carried grid — `EquipSlot` is authored
  but unread.
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
