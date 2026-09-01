# Inventory System

## Purpose

The inventory system stores named item stacks on any Actor via `UInventoryComponent`. It surfaces item data to the player through `UInventoryWidget`. `AStrategyHUD` manages the widget's lifecycle, and `AStrategyPlayerController` connects inventory access to the unit selection state.

This system does **not** handle: save/load, equipment slots, drag-and-drop, networking, or item definitions beyond in-memory data.

## Player Surface

With one or more units selected, press the inventory key to open the inventory panel for the first selected unit. Pressing the key again while the same unit is selected closes the panel. Selecting a different unit and pressing the key switches the panel to show that unit's inventory.

## Core Rules

- Only `ControlledUnits[0]` is inspected when multiple units are selected.
- Toggle behavior: open if closed, close if showing the same inventory, switch if showing a different one.
- Items are in-memory only; they do not persist across sessions.
- `MaxSlots = -1` means unlimited item slots.
- `AddItem` stacks quantities for entries with a matching `ItemID`. New unique item types are rejected when `MaxSlots` is exceeded.
- `RemoveItem` removes the array entry when `Quantity` reaches zero or below.
- `TransferItemTo` is best-effort; items that cannot fit in the target remain in the source.
- The inventory widget renders at z-order 10, above the base HUD at 0.
- Pressing the inventory key with no units selected does nothing.
- If `InventoryWidgetClass` is not assigned on the HUD Blueprint, `OpenInventory` returns safely with no crash.

## C++ Implementation

- **Primary classes:** `FInventoryItem`, `UInventoryComponent`, `UInventoryWidget`, `AStrategyHUD`, `AStrategyPlayerController`, `AStrategyUnit`
- **Important methods:**
  - `UInventoryComponent::AddItem` — stacks or appends; respects `MaxSlots`
  - `UInventoryComponent::RemoveItem` — decrements quantity; removes entry at zero
  - `UInventoryComponent::TransferItemTo` — moves all items to another component
  - `AStrategyHUD::OpenInventory` — lazy-creates widget, calls `SetInventory`, adds to viewport
  - `AStrategyHUD::CloseInventory` — removes widget from parent
  - `AStrategyHUD::ToggleInventory` — open/switch/close logic
  - `UInventoryWidget::SetInventory` — wires data, calls `BP_RefreshDisplay`
  - `UInventoryWidget::BP_RefreshDisplay_Implementation` — default C++ loop: clears `ItemSlotContainer`, adds a `UTextBlock` per item showing `"DisplayName xQuantity"`
  - `AStrategyPlayerController::OpenInventoryPressed` — entry point from input
- **Runtime ownership:** `UInventoryComponent` is a default subobject of `AStrategyUnit`. `UInventoryWidget` is lazy-created and owned by `AStrategyHUD` via `UPROPERTY()`.
- **Data flow:** Input → `OpenInventoryPressed()` → `StrategyHUD->ToggleInventory(component)` → `InventoryWidget->SetInventory()` → `BP_RefreshDisplay()` → C++ default iterates `BoundInventory->GetItems()` and populates `ItemSlotContainer`; Blueprint override is optional.

## Blueprint / Asset Dependencies

- **Blueprint subclass of `AStrategyHUD`:** assign `InventoryWidgetClass` to `WBP_InventoryWidget`.
- **`WBP_InventoryWidget`:** Blueprint subclass of `UInventoryWidget`. Must contain a `UPanelWidget` (e.g. `VerticalBox` or `WrapBox`) named exactly `ItemSlotContainer` — the C++ default loop populates it automatically. Overriding `BP_RefreshDisplay` in the Blueprint graph is optional and allows full custom slot visuals; calling `Super` from the override runs the C++ default first.
- **`IA_OpenInventory`:** Boolean `UInputAction` asset mapped to the `I` key in both `MouseMappingContext` and `TouchMappingContext`.
- **Player controller Blueprint:** assign `OpenInventoryAction` to `IA_OpenInventory`.
- **Unit Blueprint:** optionally set `Inventory → InventoryLabel` (e.g. `"Backpack"`) and pre-populate `Items` for testing.

## Extension Points

- **Storage containers** — attach `UInventoryComponent` to any non-unit actor (chests, crates, etc.). Add an `IInventoryHolder` interface to expose a display name and inventory pointer so the controller can open non-unit inventories without coupling to `AStrategyUnit`.
- **Drag-and-drop** — override `NativeOnDrop` in a `UInventoryWidget` subclass and call `TransferItemTo` or swap logic.
- **Equipment / worn slots** — add a parallel `TArray<FInventoryItem> EquipmentSlots` to `UInventoryComponent`, or create a dedicated `UEquipmentComponent` for worn items and paper-doll rendering.
- **Save / load** — `FInventoryItem` is fully reflected via `UPROPERTY`; serialize `Items` with UE's property reflection system. No struct changes are needed.
- **Item definitions** — introduce a `UDataTable` of `FInventoryItem` rows to share icon, display name, and stack rules across all inventory holders.

## Known Gaps

- Items are in-memory only and not persisted across sessions.
- Only `ControlledUnits[0]` is opened when multiple units are selected.
- No close button or Escape binding is implemented in C++; Blueprint can wire `RemoveFromParent` to a close button.
- No network replication; items are client-authoritative (single-player prototype).
- Interaction overlap in `OnMoveCompleted` casts to `AStrategyUnit` even though the query targets `ECC_WorldDynamic`; future interactables (including storage containers) will need a dedicated interface.
