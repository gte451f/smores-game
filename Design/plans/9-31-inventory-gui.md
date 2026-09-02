 Inventory Widget GUI: Window Chrome + Enumerated Slots

 Context

 The inventory widget (UInventoryWidget) currently has no real GUI — it's a single
 UTextBlock fed by a joined "1. ItemName\n2. (empty)..." string
 (InventoryWidget.cpp: GetSlotSummary()), with no button, no border, no window feel,
 and no per-slot addressability. UInventoryComponent::Items is also a packed array
 (items shift down on removal), so a given item's slot index isn't stable — a blocker
 for any future drag-and-drop.

 This plan gives the widget a basic but real GUI: a close ("X") button, window-style
 chrome (title bar, and — since it's genuinely cheap given UE5.8's viewport-slot API —
 drag and resize), and enumerated, index-stable slots rendered as a list or grid of
 placeholder text-in-a-box widgets. This is explicitly a visual/structural pass, not a
 drag-and-drop implementation — it sets up the slot addressing and click hooks that
 future drag-and-drop work will need, per [[project_inventory_system_roadmap]].

 Per CLAUDE.md's "C++ first" rule and [[feedback_anticipate_reusability]] (user has
 previously confirmed preferring a reusable base + thin subclass over one monolithic
 class), the window-chrome behavior (title bar, close button, drag, resize) goes into
 a new reusable UWindowWidget base class rather than being bolted only onto
 UInventoryWidget — nothing else uses it today, but it's the kind of thing other
 future UI (e.g. a crafting screen) would want too.

 Scope decision: drag & resize

 Included now, not deferred. UE5.8 exposes UUserWidget::SetPositionInViewport /
 SetDesiredSizeInViewport, backed by a BlueprintCallable UGameViewportSubsystem::GetWidgetSlot()
 getter that lets drag/resize math always read the live slot back rather than tracking
 its own state. Hit-testing which sub-widget (title bar vs. resize handle) was clicked
 is the standard GetCachedGeometry().IsUnderLocation(ScreenPos) idiom. This is
 self-contained in one base class, not duplicated per-widget, so the "if it's easy"
 bar is met. One required detail: AddToViewport() full-stretches the widget by
 default, so UWindowWidget::NativeConstruct() must call SetPositionInViewport/
 SetDesiredSizeInViewport once up front to switch the slot into a fixed-rect window —
 without this, drag/resize would have nothing meaningful to move.

 New files

 Source/smores/Variant_Strategy/UI/WindowWidget.h/.cpp (new, UCLASS(abstract))

 Reusable floating-window base for UUserWidget. Optional BindWidgetOptional parts —
 all no-ops if a subclass's WBP doesn't provide them:
 - TitleText (UTextBlock) — set from WindowTitle (EditAnywhere FText) in NativeConstruct.
 - CloseButton (UButton) — OnClicked wired to HandleCloseButtonClicked(), which
   calls RequestClose().
 - TitleBarDragHandle (UWidget) — hit-region for starting a drag.
 - ResizeHandle (UWidget) — hit-region for starting a resize (bottom-right grip).

 RequestClose() is BlueprintNativeEvent; base implementation does nothing —
 subclasses override to actually remove themselves.

 Also: InitialWindowPosition, InitialWindowSize, MinWindowSize (all
 FVector2D, EditAnywhere), bAllowDrag / bAllowResize (bool, EditAnywhere,
 default true).

 Mouse handling — override NativeOnMouseButtonDown/Move/Up:
 - Button-down: if over ResizeHandle (and bAllowResize) → start resize; else if
   over TitleBarDragHandle (and bAllowDrag) → start drag. Cache
   GestureStartScreenPos and GestureStartSlot = UGameViewportSubsystem::Get()->GetWidgetSlot(this),
   return FReply::Handled().CaptureMouse(TakeWidget()).
 - Mouse-move: compute DeltaSlotSpace = (CurrentScreenPos - GestureStartScreenPos) / UWidgetLayoutLibrary::GetViewportScale(this)
   once per move; apply to position (drag) or size (resize, clamped to MinWindowSize)
   relative to GestureStartSlot, via SetPositionInViewport(..., /*bRemoveDPIScale=*/false) /
   SetDesiredSizeInViewport(...). Never re-reads the slot mid-gesture — pure function
   of start-slot + total delta, so no drift.
 - Button-up: clear drag/resize flags, FReply::Handled().ReleaseMouseCapture().

 New includes: Components/Button.h, Components/TextBlock.h,
 Blueprint/WidgetLayoutLibrary.h, Blueprint/GameViewportSubsystem.h. No Build.cs
 change — UMG/Slate are already public deps.

 Source/smores/Variant_Strategy/UI/InventorySlotWidget.h/.cpp (new, UCLASS(abstract))

 One enumerated, index-addressable slot:
 - SlotIndex (BlueprintReadOnly int32, INDEX_NONE until set).
 - Item (BlueprintReadOnly FInventoryItem).
 - SlotText (BindWidgetOptional UTextBlock*) — set to item name or "(empty)".
 - SetSlot(int32 InSlotIndex, const FInventoryItem& InItem) — sets state, refreshes SlotText.
 - GetSlotIndex(), GetItem(), IsSlotEmpty() — BlueprintPure.
 - BP_SlotClicked() — BlueprintImplementableEvent, fired from
   NativeOnMouseButtonUp on left-click. Pure hook for future drag-and-drop; no drag
   logic implemented here.

 Border-around-text is WBP-only (wrap SlotText in a UBorder in the widget tree) —
 no C++ representation needed.

 Modified files

 Source/smores/Variant_Strategy/Inventory/InventoryComponent.h/.cpp

 Items becomes "always exactly NumSlots entries," addressed by index; an empty
 slot is a default-constructed FInventoryItem (ItemId == NAME_None). Smallest
 change that gives real index stability — no new struct field, no wrapper type.

 - FInventoryItem: add bool IsEmpty() const { return ItemId.IsNone(); } (plain C++
   helper, not UFUNCTION — structs can't expose those).
 - UInventoryComponent: add BlueprintPure bool IsSlotEmpty(int32 Index) const and
   FInventoryItem GetItemAt(int32 Index) const.
 - Constructor: Items.SetNum(NumSlots); after existing init.
 - New virtual void BeginPlay() override: Super::BeginPlay(); if (Items.Num() != NumSlots) { Items.SetNum(NumSlots); }
   — re-syncs slot count against a Blueprint-overridden NumSlots before any
   StartingItems seeding runs (verified: AStrategyContainer/AStrategyPlayerUnit
   both seed items only after their own Super::BeginPlay()).
 - AddItem: scan for first index where Items[Index].IsEmpty(); if none, log +
   return false (unchanged failure contract); else Items[FreeIndex] = Item; +
   broadcast + return true.
 - RemoveItemAt(int32 Index): Items[Index] = FInventoryItem(); in place — not
   `Items.RemoveAt(Index)**, since that used to shift every later item down a slot,
   silently renumbering indices. (No current caller exists, so zero blast radius today —
   this only matters for future drag-and-drop.)
 - GetFreeSlotCount(): count empties directly (old NumSlots - Items.Num() becomes
   always-zero once Items.Num() == NumSlots invariantly).
 - GetSlotSummary() in InventoryWidget.cpp needs a matching fix — see below.
 - SetNumSlots: unchanged, already correct (Items.SetNum naturally
   drops/empty-fills on shrink/grow).

 Verified via grep: no caller outside these two files touches Items.Num(),
 Items.RemoveAt, or packed-array assumptions — AStrategyContainer::BeginPlay,
 AStrategyChest's constructor, and AStrategyPlayerUnit::BeginPlay all go through
 AddItem/StartingItems only, unaffected by this change.

 Source/smores/Variant_Strategy/UI/InventoryWidget.h/.cpp

 - Inherit UWindowWidget instead of UUserWidget; #include "WindowWidget.h".
 - Keep SlotListText/GetSlotSummary() exactly as a harmless fallback (no
   SlotContainer set → behaves as today).
 - Add SlotContainer (BindWidgetOptional UPanelWidget*), SlotWidgetClass
   (EditAnywhere TSubclassOf<UInventorySlotWidget>), GridColumns (EditAnywhere int32, default 4, used only when SlotContainer is a UUniformGridPanel), and a
   transient TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets bookkeeping array.
 - RefreshDisplay(): after the existing SlotListText update, if SlotContainer && SlotWidgetClass: ClearChildren(), then for each slot index spawn/SetSlot() an
   UInventorySlotWidget and add it — Cast<UUniformGridPanel>(SlotContainer)
   determines whether to call AddChildToUniformGrid(Widget, Index/GridColumns, Index%GridColumns) or plain AddChild(Widget) (so a UVerticalBox reads as a
   list, a UUniformGridPanel as a grid, with one shared code path).
 - Required correctness fix: GetSlotSummary()'s empty check must switch from
   Items.IsValidIndex(SlotIndex) (now always true) to !Items[SlotIndex].IsEmpty(),
   or every "empty" slot would print a blank line instead of "(empty)".
 - Override RequestClose_Implementation(): ClearInventory(); if (IsInViewport()) { RemoveFromParent(); }
   — byte-for-byte what AStrategyPlayerController::CloseInventory()/CloseContainer()
   already do. Zero changes needed to StrategyPlayerController — confirmed both
   ToggleInventory()/ToggleContainer() gate purely on IsInViewport(), so once the
   X button removes the widget, the next toggle naturally sees it as closed.
 - New includes: Components/PanelWidget.h, Components/UniformGridPanel.h,
   InventorySlotWidget.h.

 Files confirmed unchanged

 StrategyPlayerController.h/.cpp, StrategyContainer.h/.cpp, StrategyChest.h/.cpp,
 StrategyPlayerUnit.h/.cpp, smores.Build.cs.

 Implementation order

 1. WindowWidget.h/.cpp
 2. InventorySlotWidget.h/.cpp
 3. InventoryComponent.h/.cpp
 4. InventoryWidget.h/.cpp

 All in one cold build — UWindowWidget/UInventorySlotWidget are new UCLASSes,
 so Live Coding won't register them (per CLAUDE.md: close editor, build from VS,
 reopen).

 Follow-up editor/MCP wiring (after cold build succeeds)

 New UCLASSes aren't usable from Blueprints until the cold build completes and the
 editor is reopened. Per CLAUDE.md, run this wiring pass in a fork/subagent using
 unreal-mcp so the ~20-30 call payloads don't pollute this session.

 - New asset: WBP_InventorySlot (child of UInventorySlotWidget) — a SlotText
   UTextBlock wrapped in a UBorder for the placeholder "text in a box" look.
 - WBP_Inventory and WBP_ContainerInventory (existing, children of
   UInventoryWidget) each need, by exact widget name: TitleText, CloseButton
   (a UButton containing an "X"), TitleBarDragHandle (e.g. a UBorder sized to
   the title strip), ResizeHandle (small corner grip widget), and SlotContainer
   (a UVerticalBox for a list layout, or UUniformGridPanel for a grid — either
   is fine per the user's ask). Set SlotWidgetClass → WBP_InventorySlot,
   GridColumns as appropriate, and leave bAllowDrag/bAllowResize at their
   true defaults.
 - Get user confirmation before creating/modifying these project assets, per
   CLAUDE.md's MCP caveats.

 Verification

 1. Cold build compiles clean (editor closed, build via smores.slnx, Development
    Editor config).
 2. Grep-confirm no leftover packed-array assumptions:
    Items\.Num\(\)|Items\.RemoveAt|Items\.IsValidIndex outside
    InventoryComponent.cpp/InventoryWidget.cpp.
 3. Reopen editor; before any WBP rewiring, PIE-test that WBP_Inventory/
    WBP_ContainerInventory still open/close via the existing toggle inputs and
    still show the text fallback — expect it now correctly prints "(empty)" for
    empty slots, and the window will appear shrunk to InitialWindowSize at
    InitialWindowPosition rather than full-screen (expected, since
    NativeConstruct now switches the slot to a fixed rect).
 4. After the MCP wiring pass: PIE-test opening a unit's inventory and a chest's
    inventory — confirm the "X" button closes each, the title bar drags the window,
    the corner handle resizes it (respecting MinWindowSize), and slots render with
    correct enumerated contents (including any empty slots as bordered "(empty)"
    boxes) in whichever layout (list/grid) was chosen per widget.