# HUD and Panels

## Purpose

The always-on heads-up display and the floating panels the player opens from it: what each
region of the screen is, who owns it in C++, and the rules that keep a click on the HUD from
also ordering the squad somewhere.

This is the permanent record. The multi-slice plan that builds the rest of it — the time-pace
strip, the target panel, the squad portrait bar, the activity feed — lives in
`Docs/roadmaps/hud-roadmap.md` and is a temporary working document.

> **Status: Slice 1 of the HUD roadmap has shipped.** The frame exists: the HUD root hosts
> anchored regions, the nav rail opens four panel windows, and the gold readout has moved into a
> resource strip. The other four regions are labelled placeholder boxes in `UI_Strategy` — they
> are there so the layout can be judged whole, and Slices 2 and 3 replace each with its real
> widget.

## Player Surface

### The frame

Six regions, positioned to the wireframe at `tmp/Squad HUD Wireframes v3-selection.png`:

| Region | Where | Today |
|---|---|---|
| Nav rail | Left, top-anchored | **Live.** Five buttons: Squad, Inventory, Map, Research, Help |
| Time pace | Top centre | Placeholder box (Slice 2) |
| Resource strip | Top right | **Live.** Gold balance |
| Target panel | Top right, below the strip | Placeholder box; the old one-line selection-target label sits here (Slice 2) |
| Squad portraits | Bottom left | Placeholder box (Slice 3) |
| Activity feed | Bottom right | Placeholder box (Slice 3) |

### Panels

Every panel is a floating window with the same chrome as the inventory windows — a title bar you
can drag, a corner you can resize from, and a close button. Each opens from its nav-rail button
**or** its key, and pressing either again closes it.

| Panel | Key | What it does today |
|---|---|---|
| Squad | `P` | Stub. Names what will live there and which design topic owns it |
| Inventory | `I` | The existing pack + paperdoll windows — the rail button runs the same path as the key |
| Map | `M` | Stub |
| Research | `U` | Stub |
| Help | `F1` | **Real.** Lists the current keybindings |

The three stubs are deliberate. An empty panel the player can open is better than a HUD that
grows a new corner every time a system ships, because the layout is the thing that has to be
lived with — and a fake tech tree would teach the player a shape the real system may not take.

Help is not a stub because it is the cheapest panel here and the only immediately useful one, and
because it is what a player actually presses `F1` for.

### What the rail is not

The rail mirrors keys; it does not add capability. Asking for the inventory with no pawn selected
does the same nothing whether you pressed `I` or clicked the button — by construction, not by
coincidence (see Core Rules).

## Core Rules

- **A rail button and its key run the same code.** Both arrive at
  `IStrategyHUDCommands::RequestPanel`, and `RequestPanel` is the only thing that decides what
  opening a panel means. A button that duplicated the controller's rules would eventually
  disagree with the key, and a refusal would come out differently depending on how the player
  asked.
- **The HUD swallows mouse presses that land on it.** The persistent HUD sits at Z-order 0,
  *below* every floating window, and unlike a window it inherits none of `UWindowWidget`'s
  click-eating behaviour. `UHUDRegionWidget` supplies it: every region's rectangle consumes the
  press of every mouse button, button-agnostically, for the same reason `UWindowWidget` does —
  right-click means "move order" in the world, and a right-click on the gold readout should mean
  nothing rather than sending the squad to whatever is behind it.
  - **Real `UButton`s inside a region already handle themselves.** Slate's `SButton` consumes both
    the press and the double-click (`SButton::OnMouseButtonDoubleClick` routes into
    `OnMouseButtonDown`), so a rail button needs nothing extra. The region's shield only ever
    catches what nothing inside it claimed — the gaps between the buttons.
  - **The double-click needs its own override, and `UHUDRegionWidget` has one.** Claiming the
    press does not claim the second click of a rapid pair; without the override it reaches the
    viewport and fires `IA_Strategy_SelectAllDoubleClick`. See `input-and-keybinds.md`.
  - **The shield only works if the region hit-tests**, so `bBlockWorldClicks` forces
    `ESlateVisibility::Visible` on construct. A region left at the designer's
    `SelfHitTestInvisible` would pass every click through, and that failure is invisible until
    someone notices their squad walking to a spot under the HUD.
- **Anything drawn on the HUD that reads as a panel must be a `UHUDRegionWidget` — placeholder or
  not.** This is the rule that the click shield above actually depends on, and it was learned the
  expensive way: the four unbuilt regions shipped as plain `UBorder`s dropped into `UI_Strategy`,
  and a right-click over the bottom-right box issued a move order to the selected squad. A
  `UBorder` returns Unhandled from `OnMouseButtonDown`, so Slate bubbles the press up past the
  canvas to the game viewport — being *visible* is not the same as *consuming*. `UHUDRegionWidget`
  is the only thing in the HUD that consumes, so a box on the HUD has to be one.
  `UHUDPlaceholderRegionWidget` exists solely so that an empty region is still a real region.
- **The release is deliberately not swallowed**, the same as for windows. Enhanced Input never saw
  the press the HUD ate, so an action bound on release has nothing to complete; eating a release
  whose press the viewport *did* see would leave that button stuck down in `UPlayerInput`.
- **A HUD panel must never touch `UpdateInventoryInputContext`.** That context is the inventory's
  business. A stub research window that re-scoped it would quietly give `R` a second meaning while
  it was open. `HandleWindowClosed` returns early for a `UHUDPanelWidget` for exactly this reason.
- **Readouts live in a region, never on the HUD root.** `UStrategyUI` hosts regions and forwards
  what the HUD pushes into it; it does not hold readouts itself. Gold moved out for this reason,
  and the next at-a-glance figure has an obvious home instead of being wedged in beside a number
  that was already there.
- **Every region is optional.** `UStrategyUI` binds its children with `BindWidgetOptional`, which
  is what lets the regions arrive one slice at a time without the HUD breaking in between.
- **Panel windows are kept, not rebuilt.** `PanelWidgets` holds a panel's widget after it closes,
  so re-opening reuses it. Closing goes through the window's own `RequestClose` so it broadcasts
  `OnWindowClosed` exactly as it would have if the player had clicked its X.
- **All of this is per-local-player UI state.** Which panels a player has open is nobody else's
  business: nothing here is replicated or authority-gated, and in co-op every player's rail
  answers only for their own windows. `OpenPanel` early-outs on a non-local controller.
- **The help panel lists only keys that do something.** A help screen naming a key that does
  nothing is worse than one that is incomplete — the player tries it, nothing happens, and they
  stop trusting the list. Slices 2 and 3 add their keys to `UHelpPanelWidget::GetDefaultBodyText`
  in the same change that makes them work.
- **Layout now, styling later — but legible is not styling.** Plain UMG defaults and correct
  positions, deliberately, so the layout can be judged in PIE before anything is worth restyling,
  and so a layout change doesn't cost restyling twice. The wireframe's cream palette, fonts and
  portrait art are a later pass. **The exception is contrast**, because a layout nobody can read
  can't be judged: UMG's default `UBorder` brush is white and its default `UTextBlock` colour is
  white, so every region shipped with invisible text until the backgrounds were set dark
  translucent and the text near-white. That minimum — dark panel, light text — is the floor, not a
  style choice, and a later styling pass replaces it wholesale.

## C++ Implementation

All of it in `SmoresUI`, except the controller side.

### The HUD root

- **`UStrategyUI`** — the always-on widget spawned by `AStrategyHUD::BeginPlay` at Z-order 0. Its
  WBP is a full-screen canvas with an anchored slot per region; each region binds by name through
  `BindWidgetOptional` (`NavRail`, `ResourceStrip` today). It keeps the selected-unit count and
  the selection-target label it always had, and forwards `SetGold` to the resource strip.
- **`AStrategyHUD::DrawHUD`** — already pushed selection count, target label and gold every frame;
  it now also calls `UStrategyUI::RefreshNavRail()`. That per-frame push is the pattern every new
  region follows, and it is why regions read state rather than subscribing to it.

### The regions

- **`UHUDRegionWidget`** — base for every HUD region. Two things, neither free: the click shield
  described in Core Rules, and `GetHUDCommands()`, the one place the cast from the owning player
  controller to `IStrategyHUDCommands` lives.
- **`UNavRailWidget`** — five `BindWidgetOptional` `UButton`s, each calling `RequestPanel`. Its key
  hints and labels are static text in the WBP; they are layout, not behaviour.
  `RefreshPanelStates()` repaints the active-panel tint and is called every frame, so it compares
  a bitmask of open panels against the last one and returns immediately unless it changed.
- **`UHUDPlaceholderRegionWidget`** — a labelled empty box standing in for a region that hasn't
  been built. One WBP serves all four; each instance sets its own `RegionLabel`, which is why the
  label is an `EditAnywhere` property rather than a virtual override like the panel windows' body
  copy. It pushes the label through `SynchronizeProperties` as well as `NativeConstruct`, so the
  box reads correctly in the UMG designer too — half of judging a layout happens there.
- **`UResourceStripWidget`** — the gold readout. The balance still arrives the way it always did:
  `AStrategyHUD` resolves `UWalletComponent` off its own player state (with a late-arrival retry)
  and pushes it through `UStrategyUI::SetGold`. Only the display moved.

### The panels

- **`EHUDPanel`** (`HUDPanel.h`) — the shared vocabulary between the rail, the controller and the
  windows. In its own header because both the interface and the widgets need it and neither
  should have to include the other.
- **`IStrategyHUDCommands`** (`StrategyHUDCommands.h`) — the things the HUD needs the controller
  to *do*, implemented by `AStrategyPlayerController` alongside `IStrategySelectionHost`,
  `IStrategyCameraCommands` and `IInventoryMoveHost`. One interface, not four: everything on it is
  a request only the controller can service. Data the HUD only needs to *read* goes the other way,
  through `DrawHUD`'s push or a component lookup. `RequestPanel` and `IsPanelOpen` are live;
  `RequestSelectUnit` and `RequestTargetAction` are declared and stubbed for Slices 3 and 2.
- **`UHUDPanelWidget`** — base for every panel window, a `UWindowWidget` subclass so drag, resize,
  the title bar, the close button and the press-swallowing all come for free. It adds an
  `EHUDPanel` id (so a panel is trackable without a cast per class) and one `BodyText` block
  filled from `GetBodyText()`.
- **`USquadPanelWidget` / `UMapPanelWidget` / `UResearchPanelWidget` / `UHelpPanelWidget`** — thin
  subclasses. Each constructor sets its `PanelId` and `WindowTitle`; each overrides
  `GetDefaultBodyText()`. That is the whole difference between them, which is why they are
  subclasses rather than four copies of the base. Body copy lives in C++ rather than in a WBP so a
  stub's "what will live here, and what owns it" line is versioned with the code and shows up in a
  grep; `BodyOverride` on the WBP can replace it if a designer ever needs to.

### The controller

`AStrategyPlayerController` (`smores`, `Variant_Strategy/`):

- **`PanelWidgetClasses`** — a `TMap<EHUDPanel, TSubclassOf<UHUDPanelWidget>>` rather than one
  property per panel, so adding a panel later is a map entry plus an `EHUDPanel` value.
  `EHUDPanel::Inventory` is deliberately absent: the inventory has its own window path that
  predates the rail, and `RequestPanel` routes to it.
- **`PanelWidgets`** — the spawned windows, keyed the same way. An entry survives closing.
- **`ToggleInventoryPanel()`** — the inventory key's behaviour with the input plumbing stripped
  off, so the rail's Inventory button runs exactly the same path rather than a lookalike of it.
  `ToggleInventory(const FInputActionValue&)` is now a one-line forwarder.
- **`HandleWindowClosed`** — returns early for any `UHUDPanelWidget`, before the inventory
  bookkeeping.

## Blueprint / Asset Dependencies

All under `Content/Variant_Strategy/UI/`:

| Asset | Parent | Bound names |
|---|---|---|
| `UI_Strategy` | `UStrategyUI` | `NavRail`, `ResourceStrip`; plus the pre-existing `SelectionTargetBorder` / `SelectionTargetText` and the units-count display. Four placeholder boxes — `TimePaceRegion`, `TargetPanelRegion`, `SquadBarRegion`, `ActivityFeedRegion` — hold the unbuilt regions' space |
| `WBP_NavRail` | `UNavRailWidget` | `SquadButton`, `InventoryButton`, `MapButton`, `ResearchButton`, `HelpButton` |
| `WBP_ResourceStrip` | `UResourceStripWidget` | `GoldText` |
| `WBP_HUDPlaceholderRegion` | `UHUDPlaceholderRegionWidget` | `LabelText`; four instances in `UI_Strategy`, one per unbuilt region |
| `WBP_SquadPanel` | `USquadPanelWidget` | `TitleText`, `CloseButton`, `TitleBarDragHandle`, `ResizeHandle`, `BodyText` |
| `WBP_MapPanel` | `UMapPanelWidget` | as above |
| `WBP_ResearchPanel` | `UResearchPanelWidget` | as above |
| `WBP_HelpPanel` | `UHelpPanelWidget` | as above |

`BP_StrategyPlayerController` holds the four `PanelWidgetClasses` entries and the eight new
`UInputAction` references. `BP_StrategyHUD` holds `UIWidgetClass` (`UI_Strategy`) and
`RefusalWidgetClass`, unchanged.

**The `GoldText` widget moved out of `UI_Strategy` into `WBP_ResourceStrip`.** The C++ property
moved with it; nothing else in `UI_Strategy` referenced it.

## Extension Points

### Adding a panel

1. Add a value to `EHUDPanel`.
2. Add a `UHUDPanelWidget` subclass: constructor sets `PanelId` and `WindowTitle`, override
   `GetDefaultBodyText()`.
3. Create its WBP with that native parent and the five chrome/body names above.
4. Add an entry to `PanelWidgetClasses` on `BP_StrategyPlayerController`.
5. If it gets a rail button: add a `BindWidgetOptional` `UButton` and its handler to
   `UNavRailWidget`, extend `GetButtonForPanel` and `GetRailPanels`, and add the button to
   `WBP_NavRail`. `GetRailPanels` is a `uint8` bitmask today, so it holds eight panels before that
   needs widening.
6. If it gets a key: follow `input-and-keybinds.md`'s checklist, and route the handler through
   `RequestPanel` so the key and the button cannot drift apart.
7. Add its key to `UHelpPanelWidget::GetDefaultBodyText()` — **in the same change that makes it
   work**, not before.

### Adding a HUD region

Subclass `UHUDRegionWidget` (never `UUserWidget` directly — that is how the click shield is
inherited), add a `BindWidgetOptional` property for it on `UStrategyUI`, and push whatever it
needs from `AStrategyHUD::DrawHUD`. Replace that region's placeholder box in `UI_Strategy` with
`UMGToolSet.ReplaceWidgetWithTemplate`, which keeps the widget's name, its parent slot and that
slot's anchors/offsets — delete-and-re-add loses the `BindWidgetOptional` name binding.

**If the region isn't ready yet, use `WBP_HUDPlaceholderRegion` rather than a bare `UBorder`.**
An empty box still has to consume clicks; see Core Rules for the bug that proved it.

## Known Gaps

- **Four of the six regions are placeholder boxes** (`WBP_HUDPlaceholderRegion` instances, so they
  shield clicks like any other region). Time pace and the target panel are Slice 2; the squad
  portrait bar and the activity feed are Slice 3. See `Docs/roadmaps/hud-roadmap.md`.
- **`Space`, `-`, `=` and `L` are mapped and bound but do nothing yet.** Each logs a line naming
  the slice that implements it. They were mapped early so all eight key mappings could be authored
  and verified in one editor pass — the one manual editor step in Slice 1. A key that logs is
  deliberately not a key that silently does nothing: it is how a mistyped mapping is told apart
  from an unimplemented one.
- **No styling pass.** Plain UMG defaults throughout — see Core Rules for why that is on purpose.
  The active-panel state on the rail is a background tint, which is the cheapest thing that
  survives restyling.
- **Two legacy readouts still leak clicks.** `SelectionTargetBorder` (+ `SelectionTargetText`) and
  `Border_0` (+ `SelectionCount`, the "N selected" readout) are plain `UMG.Border`s sitting
  directly on `HUDCanvas`, predating the region system. A right-click *directly on one of them*
  still issues a move order to the world, for the reason in Core Rules. They can't simply be
  reparented into a region: a `UHUDRegionWidget` is a `UUserWidget`, not a `UPanelWidget`, so
  `MoveWidget` refuses it, and `SetNamedSlotContent` creates a *new* widget rather than moving an
  existing one — either route risks the property bindings to `GetSelectionTargetLabel` and
  `GetSelectedUnitsCount`, which must stay in `UI_Strategy`'s own tree to resolve at all.
  **The natural fix is Slices 2 and 3**, which replace both with real `UHUDRegionWidget`s (the
  target panel and the squad portrait bar) and delete these. Worth doing sooner only if the leak
  proves annoying in play.
- **`Border_361` is a collapsed leftover**, not a live widget — an empty decorative `Border` from
  the template HUD, sitting bottom-right in the root `Overlay` with no bindings and no C++
  reference. It was set `Collapsed` rather than deleted because it predates this work. Delete it
  when the activity feed lands and that corner is genuinely owned.
- **No tooltips anywhere.** `RefusalWidget.h` already notes that a tooltip escaping its panel will
  need its own top-most layer when it arrives.
- **The rail's key hints are static WBP text.** They can't drift from the C++ (nothing generates
  them), but they *can* drift from `IMC_Strategy_Mouse` if a key is ever remapped. There is no
  keybind settings screen yet, so nothing makes them wrong today.
- **`UHelpPanelWidget`'s list is hand-maintained.** It is the in-game copy of this skill's
  `input-and-keybinds.md`; keeping the two in step is a discipline, not a mechanism. The thing
  that would replace it is a real keybind settings screen reading
  `PlayerMappableKeySettings`, which is its own piece of work.
- **No panel remembers its position or size between sessions**, or even between closes — every
  open uses the WBP's `InitialWindowPosition` / `InitialWindowSize`. Shared with the inventory
  windows; it belongs with a settings/save pass rather than here.
