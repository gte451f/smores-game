# Inventory System

## Purpose

`UInventoryComponent` gives any Actor a fixed-size grid of item slots, server-authoritative
and replicated, with a shared drag-and-drop UI for moving items within one inventory or
between two. The same component and UI back three different holders today: a squad unit's
own carried items, a world container's contents, and a Downed NPC's loot.

This system does **not** yet handle: stacking/quantities, item definitions/data tables,
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
  resize-from-corner, close button) — dragging an item from one panel's slot onto another
  slot moves/swaps it, whether that's reordering within one panel or transferring between
  two different ones.
- Deselecting all units, or having no player pawn selected/in range, closes any open
  inventory window.

## Core Rules

- Every `UInventoryComponent` has a fixed slot count (`NumSlots`, default 64, clamped
  0–64); `Items` always has exactly that many entries, and an entry with
  `ItemId == NAME_None` marks an empty slot.
- No stacking or quantities exist — every non-empty slot holds exactly one `FInventoryItem`,
  however small the item conceptually is.
- All mutation (`AddItem`, `SetItemAt`, `RemoveItemAt`, `SetNumSlots`) is authority-only;
  called on a non-authority machine, each is a silent no-op. `Items` and `NumSlots` are both
  replicated; `OnRep_Items` re-broadcasts `OnInventoryChanged` on clients (authority already
  broadcasts it directly from the mutators, so it isn't double-fired there).
- `AddItem` fills the first free slot and fails (returns `false`, logs a warning) if none is
  free — there's no partial/overflow handling since there's nothing to stack into.
- `RemoveItemAt` clears the slot in place rather than compacting the array, so slot indices
  stay stable across removals.
- `MoveItem` is a static swap: it exchanges whatever occupies `SourceIndex`/`DestIndex`,
  works identically within one inventory (reordering) or across two different inventories
  (transfer), and no-ops if either component is null, either index is invalid, the source
  slot is empty, or source and destination are the same slot. This single method is what
  every current transfer path (reorder, pawn↔container, pawn↔loot) actually calls.
- A container's `InteractionRange` (a `USphereComponent`, default 312.5 units) gates whether
  a given unit may open it; the same proximity check shape (`IsUnitInRange`) is reused for
  Downed-NPC loot.
- A client-side drag-and-drop widget can't mutate a replicated, authority-only inventory
  directly — the slot widget's drop handler instead calls into `IInventoryMoveHost`
  (implemented by `AStrategyPlayerController`), whose `Server_MoveInventoryItem` RPC is the
  only path that actually calls `UInventoryComponent::MoveItem`.

## C++ Implementation

- **Primary classes:**
  - `SmoresItems`: `FInventoryItem`, `UInventoryComponent`, `AStrategyContainer` (abstract
    base for world containers), `AStrategyChest` (first concrete container type — no added
    behavior yet beyond its own `StartingItems`, exists so future chest-specific behavior
    like locks/keys has a home)
  - `SmoresUI`: `UWindowWidget` (reusable floating-window chrome), `UInventoryWidget`,
    `UInventorySlotWidget`, `UInventoryDragDropOperation`, `IInventoryMoveHost`
  - `SmoresCharacters`: `AStrategyUnit` (owns the `Inventory` subobject shared by NPCs and
    player units alike), `AStrategyPlayerUnit`
  - `smores` (`Variant_Strategy`): `AStrategyPlayerController`
- **Important methods:**
  - `UInventoryComponent::AddItem` / `SetItemAt` / `RemoveItemAt` / `SetNumSlots` —
    authority-only mutators, all broadcast `OnInventoryChanged`
  - `UInventoryComponent::MoveItem` (static) — the single move/swap entry point for both
    reordering and cross-inventory transfer
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
    authoritative caller of `UInventoryComponent::MoveItem` from UI drag-drop
  - `UInventoryWidget::SetInventory` / `ClearInventory` — binds/unbinds an inventory,
    subscribes to `OnInventoryChanged`
  - `UInventorySlotWidget::NativeOnDragDetected` / `NativeOnDrop` — creates/consumes a
    `UInventoryDragDropOperation`, routing the move through the owning player controller's
    `IInventoryMoveHost`
- **Runtime ownership:** `UInventoryComponent` is a default subobject of `AStrategyUnit`
  (every unit, NPC or player-controlled) and of `AStrategyContainer` (every world
  container). The two `UInventoryWidget` instances (`InventoryWidget`, `ContainerWidget`)
  are lazy-created and owned by `AStrategyPlayerController` directly — **not** by
  `AStrategyHUD`, which today only spawns the general `UStrategyUI` widget and draws the
  drag-selection box; it has no inventory role.
- **Data flow (drag-and-drop transfer):** `UInventorySlotWidget::NativeOnDragDetected`
  (source slot) → `UInventoryDragDropOperation` payload (source inventory + index) → target
  `UInventorySlotWidget::NativeOnDrop` → `IInventoryMoveHost::Server_MoveInventoryItem`
  (client → server RPC via the owning `AStrategyPlayerController`) →
  `UInventoryComponent::MoveItem` → `Items` replicates back down → `OnRep_Items` →
  `OnInventoryChanged` → `UInventoryWidget` refreshes on every observing client.

## Blueprint / Asset Dependencies

- **`WBP_Inventory`** (`Content/Variant_Strategy/UI/`) — `UInventoryWidget` subclass,
  assigned to `AStrategyPlayerController::InventoryWidgetClass`. Shows the selected pawn's
  own inventory.
- **`WBP_ContainerInventory`** — a second `UInventoryWidget` subclass, assigned to
  `ContainerWidgetClass`. Reused for both world containers and Downed-NPC loot; only the
  window title differs at open time.
- **`WBP_InventorySlot`** — `UInventorySlotWidget` subclass, assigned to
  `UInventoryWidget::SlotWidgetClass`. For the per-slot spawn path, the widget needs a
  `UPanelWidget` named `SlotContainer` on the owning `WBP_Inventory`/`WBP_ContainerInventory`
  (`UUniformGridPanel` renders as a grid using `GridColumns`; any other `UPanelWidget`
  renders as a list). A plain `SlotListText` text block is the fallback default visual if
  `SlotContainer`/`SlotWidgetClass` aren't set.
- **`IA_Strategy_Inventory`** (`Content/Variant_Strategy/Input/Actions/`) — bound to
  `ToggleInventoryAction`. Desktop-only; not mapped in the touch `InputMappingContext`.
- **`IA_Strategy_ToggleContainer`** — bound to `ToggleContainerAction`. Also desktop-only.
- **`AStrategyContainer` / `AStrategyChest` Blueprint subclasses** — assign `ContainerMesh`'s
  materials (`NormalMaterial`/`SelectedMaterial`), `ContainerDisplayName`, and populate
  `StartingItems`.
- **Unit Blueprints** — may pre-populate `Items` on their `Inventory` subobject for testing;
  `NumSlots` can be overridden per-Blueprint (re-synced against `Items.Num()` in
  `UInventoryComponent::BeginPlay`).

## Extension Points

- **Item definitions / data table** — `FInventoryItem` is a fully inline, hand-authored
  struct today; a `UDataTable` of shared item rows would let every holder reference common
  icon/name/base-stat data instead of duplicating it per instance.
- **Stacking, equipment, currency, trading, theft, world pickups, grid/bulk placement,
  sort/filter** — none of this exists in code yet. Full target design and rationale live in
  `inventory-roadmap.md`.
- **Save/load** — `FInventoryItem` and `UInventoryComponent`'s state are fully
  `UPROPERTY`-reflected; no struct changes are needed for whatever serialization approach
  the save system eventually adopts.

## Known Gaps

- No stacking/quantity — every item, however small, consumes a full slot.
- No item definitions — icon, name, and description are duplicated on every instance rather
  than shared.
- No equipment/worn slots distinct from the general carried grid.
- No currency, trading, or purchase flow.
- No weight/encumbrance tracking.
- No world-loose item pickups — only container- and unit-held inventories exist; nothing
  can be dropped in or picked up directly from the world today.
- No stolen-item flag or theft/detection mechanics.
