Draggable Items, 8x8 Grid, and Paired Pawn/Container Panels (Variant_Strategy)

Context

The inventory system currently has real window chrome and enumerated, index-stable
slots (per Design/plans/9-31-inventory-gui.md, already implemented): `UWindowWidget`
(drag/resize/close chrome), `UInventorySlotWidget` (one enumerated slot, index-stable,
`BP_SlotClicked` hook already anticipates future drag-and-drop but implements none of
it), and `UInventoryWidget` (spawns slot widgets into a `SlotContainer`, rendering as a
grid when that container is a `UUniformGridPanel`). `UInventoryComponent::Items` is
always exactly `NumSlots` entries; `RemoveItemAt` clears in place rather than shifting,
specifically so slot indices stay stable for this kind of work.

Today `AStrategyPlayerController` owns two independent singleton widgets -
`InventoryWidget` (`ToggleInventory`, StrategyPlayerController.cpp:384) and
`ContainerWidget` (`ToggleContainer`/`OpenContainer`, StrategyPlayerController.cpp:447
and :483) - that never interact. `ToggleInventory` only opens a pawn's panel when
exactly one player-controlled pawn is currently selected. Window titles are static
per-WBP text (`WindowTitle`, set once in `UWindowWidget::NativeConstruct`) even though
`SetWindowTitle` already exists as a runtime setter nobody calls. `NumSlots` defaults
to 4 and `GridColumns` defaults to 4.

This plan stays text-only (no icons) and adds:
1. An 8x8 grid, locked for every inventory panel (containers and pawns alike).
2. Drag-and-drop: reorder within a panel, and transfer between a pawn's inventory and
   an open container's inventory, in both directions, swapping when the drop target
   is occupied.
3. Opening a container also opens the inventory of whichever player-controlled pawn
   is closest to it (not gated by current selection - mirrors the existing
   all-player-pawns range check already used by the double-click-to-open path at
   StrategyPlayerController.cpp:578-590), shown side by side with the container.
4. Dynamic per-instance window titles - "Chest 1 Contents", "Pawn 2 Inventory" - built
   from the existing `AStrategyContainer::ContainerDisplayName` /
   `AStrategyUnit::UnitDisplayName` fields via the already-existing `SetWindowTitle`.

Resolved during brainstorming:
- Which pawn's inventory opens alongside a container: the closest player-controlled
  pawn to the container, regardless of selection (not "exactly one selected").
- Drag direction: bidirectional (pawn<->container), not just pawn->container.
- Drop onto an occupied slot: swap the two items (same rule within one panel and
  across panels).
- Closing one panel does not force-close the other - once open, each panel closes
  independently via its own X button / toggle key, same as today.

Approach

1. Lock the grid to 8x8 (C++ defaults, still designer-editable per
   [[project_inventory_system_roadmap]] - not hardcoded constants, since a future
   container type may want a different size)

- `Inventory/InventoryComponent.h`: change `NumSlots`'s default from `4` to `64`
  (ClampMax is already 64, no clamp change needed).
- `UI/InventoryWidget.h`: change `GridColumns`'s default from `4` to `8`.
- Existing per-instance Blueprint overrides of either value (if any exist on
  BP_Chest instances or player pawn BPs) need clearing/updating in the content pass
  below so they inherit the new defaults instead of a stale smaller grid.

2. Item moves - `UInventoryComponent`

- Add `bool SetItemAt(int32 Index, const FInventoryItem& Item)`: validates the
  index, assigns `Items[Index] = Item`, broadcasts `OnInventoryChanged`, returns
  false on an invalid index (same contract as `RemoveItemAt`).
- Reimplement `RemoveItemAt(Index)` as `return SetItemAt(Index, FInventoryItem());`
  - identical behavior, one less code path. (Confirmed via grep: `RemoveItemAt` has
  no external callers today, zero blast radius.)
- Add `static bool MoveItem(UInventoryComponent* SourceInventory, int32 SourceIndex, UInventoryComponent* DestInventory, int32 DestIndex)`:
  validates both components and indices; no-ops (returns false) if
  `SourceInventory == DestInventory && SourceIndex == DestIndex` (dropped on itself)
  or if the source slot is empty; otherwise reads `FromItem`/`ToItem`, calls
  `DestInventory->SetItemAt(DestIndex, FromItem)` then
  `SourceInventory->SetItemAt(SourceIndex, ToItem)`. This is the single code path
  for reorder-within-a-panel (`Source == Dest`), move-to-empty (`ToItem` is empty),
  and swap (`ToItem` is non-empty) - no special-casing needed. Each component
  broadcasts its own `OnInventoryChanged`, so both widgets refresh independently
  even on a cross-panel move.

3. Drag source/target - `UInventorySlotWidget`, plus a new drag-payload class

New file `Source/smores/Variant_Strategy/UI/InventoryDragDropOperation.h/.cpp`
(new `UCLASS`, cold build required):

```
UCLASS()
class UInventoryDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<UInventoryComponent> SourceInventory;
    UPROPERTY(BlueprintReadOnly) int32 SourceSlotIndex = INDEX_NONE;
};
```

`UI/InventorySlotWidget.h/.cpp`:

- Add `TWeakObjectPtr<UInventoryComponent> OwningInventory` alongside the existing
  `SlotIndex`/`Item` state.
- Change `SetSlot(int32, const FInventoryItem&)` to
  `SetSlot(UInventoryComponent* InOwningInventory, int32 InSlotIndex, const FInventoryItem& InItem)`,
  storing `OwningInventory` too. Update the one call site in
  `UInventoryWidget::RefreshDisplay` (InventoryWidget.cpp:96-104) to pass
  `BoundInventory.Get()`.
- Add `virtual FReply NativeOnMouseButtonDown(...)`: if left button and
  `!IsSlotEmpty()`, return `FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton)`;
  otherwise `Super::NativeOnMouseButtonDown(...)`. An empty slot has nothing to
  drag, so its mouse-down falls through unhandled to `UWindowWidget`'s own handler
  underneath (unchanged behavior: clicks inside the window body are swallowed so
  they don't fall through to world/selection). `DetectDrag` doesn't suppress a
  same-widget click: a plain click (no movement past the drag threshold) still
  delivers a normal `NativeOnMouseButtonUp`, so the existing `BP_SlotClicked` hook
  is unaffected.
- Add `virtual void NativeOnDragDetected(const FGeometry&, const FPointerEvent&, UDragDropOperation*&)`:
  construct a `UInventoryDragDropOperation`, set `SourceInventory = OwningInventory`,
  `SourceSlotIndex = SlotIndex`; build a floating drag visual by creating a second
  instance of this same slot widget's class (`CreateWidget<UInventorySlotWidget>(this, GetClass())`)
  and calling `SetSlot` on it with the same item - reuses the existing WBP
  (text-in-a-border) as the drag ghost with no new content asset; assign it to
  `OutOperation->DefaultDragVisual` and set `OutOperation->Pivot = EDragPivot::MouseDown`.
- Add `virtual bool NativeOnDrop(const FGeometry&, const FPointerEvent&, UDragDropOperation* InOperation)`:
  cast to `UInventoryDragDropOperation`; if valid and `OwningInventory.IsValid()`,
  call `UInventoryComponent::MoveItem(Op->SourceInventory.Get(), Op->SourceSlotIndex, OwningInventory.Get(), SlotIndex)`
  and return true. This fires regardless of whether this slot is empty or occupied
  (empty-target = move, occupied-target = swap, both handled uniformly by
  `MoveItem`), and regardless of whether the drop is within the same panel or
  across the paired pawn/container panels - `OwningInventory` is whatever
  component this slot widget is currently bound to, set fresh by `RefreshDisplay`
  each time either panel's `SetInventory` is called.
- New includes: `Components/DragDropOperation.h` (actually `Blueprint/DragDropOperation.h`),
  `InventoryDragDropOperation.h`.

4. Paired opening + dynamic titles - `AStrategyPlayerController`

- New helper `AStrategyPlayerUnit* FindClosestPlayerPawn(const FVector& Location) const`:
  calls `RefreshPlayerPawns()`, then returns the pawn in `PlayerPawns` with the
  smallest `FVector::DistSquared` to `Location` (nullptr if `PlayerPawns` is empty).
  Deliberately checks every player pawn, not just `ControlledUnits` - mirrors the
  existing all-player-pawns range check at StrategyPlayerController.cpp:578-590.
- Extract the widget-creation/bind/show body currently inlined in `ToggleInventory`
  (StrategyPlayerController.cpp:415-431) into a new helper
  `void OpenInventoryForPawn(AStrategyPlayerUnit* PlayerUnit)`: lazily creates
  `InventoryWidget` from `InventoryWidgetClass` (unchanged early-out if unset),
  then `InventoryWidget->SetWindowTitle(...)` (see title format below),
  `InventoryWidget->SetInventory(PlayerUnit->GetInventory())`,
  `InventoryWidget->AddToViewport(0)`. `ToggleInventory` calls this in place of its
  old inline body; its close-if-open check and "exactly one selected" gate are
  otherwise unchanged (that gate still governs the "I" key specifically).
- `OpenContainer(Container)` (StrategyPlayerController.cpp:483): after the existing
  widget-creation/bind/show body, add
  `ContainerWidget->SetWindowTitle(...)` (see below), then
  `if (AStrategyPlayerUnit* ClosestPawn = FindClosestPlayerPawn(Container->GetActorLocation())) { OpenInventoryForPawn(ClosestPawn); }`.
  No change to `ToggleContainer`/`CloseContainer`/`CloseInventory` - per the
  "independent once open" decision, closing either panel still only closes that
  one panel.
- Title format: `ContainerWidget->SetWindowTitle(FText::Format(LOCTEXT("ContainerTitle", "{0} Contents"), Container->GetContainerDisplayName()))`
  and `InventoryWidget->SetWindowTitle(FText::Format(LOCTEXT("PawnTitle", "{0} Inventory"), PlayerUnit->GetUnitDisplayName()))`.
  Reuses the existing `ContainerDisplayName`/`UnitDisplayName` EditAnywhere fields
  as-is (e.g. "Chest 1", "Pawn 2") - no new properties.

5. Content / editor wiring (fork, per CLAUDE.md MCP token-discipline guidance)

- Verify `WBP_Inventory` and `WBP_ContainerInventory`'s `SlotContainer` is a
  `UUniformGridPanel` (not a list panel) and `GridColumns` reads 8 (now the C++
  default - clear any stale BP override that pins it lower).
- Verify the bound inventories (player pawn BPs, BP_Chest and its placed instances)
  don't carry a stale `NumSlots` override below 64; clear so they inherit the new
  default, or set explicitly to 64.
- Set distinct `InitialWindowPosition` on `WBP_Inventory` vs `WBP_ContainerInventory`
  (e.g. container at x=100, pawn inventory at x=560, same y) so the two panels
  render side by side rather than stacked when both open together - purely a
  per-WBP-class default, no C++ layout code needed since only these two window
  types are ever shown paired.
- Verify `ContainerDisplayName` is set on each placed BP_Chest instance (e.g.
  "Chest 1", "Chest 2") and `UnitDisplayName` is set on each player pawn (e.g.
  "Pawn 1", "Pawn 2") - both fields already exist; this only fills gaps.
- Get user confirmation before changing these project assets, per CLAUDE.md.

Build order

1. Write `InventoryDragDropOperation.h/.cpp` (new UCLASS), and the
   `InventoryComponent.h/.cpp`, `InventorySlotWidget.h/.cpp`, `InventoryWidget.h/.cpp`,
   `StrategyPlayerController.h/.cpp` changes above.
2. Close the editor, cold build via Visual Studio (new UCLASS - Live Coding won't
   register it).
3. Reopen the editor, do the content/MCP wiring pass (step 5) in one fork.
4. /compact if the MCP fork's output bloated context.

Verification (PIE)

- Open a pawn's inventory alone ("I"): still an 8x8 grid, title reads
  "<UnitDisplayName> Inventory".
- Walk a pawn near a chest, open it ("O" or double-click): both the container panel
  ("<ContainerDisplayName> Contents") and the closest pawn's inventory panel open
  side by side, without requiring that pawn to be the selected one.
- Drag an item within one panel onto an empty slot: it moves, index-stable slots
  update correctly (no shifting of unrelated items).
- Drag an item within one panel onto an occupied slot: the two items swap.
- Drag an item from the pawn panel onto an empty container slot: it transfers;
  drag one from the container onto an empty pawn slot: it transfers back.
- Drag an item from one panel onto an occupied slot in the other panel: the two
  items swap across the two inventories.
- Drop an item onto its own slot: no-op, nothing changes.
- Close the container only (its X or "O" again): the pawn panel stays open,
  unaffected, and vice versa.
- With no player-controlled pawn anywhere in the level, opening a container shows
  only the container panel - no crash.
