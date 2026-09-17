# HUD and Panels

## Purpose

The always-on heads-up display and the floating panels the player opens from it: what each
region of the screen is, who owns it in C++, and the rules that keep a click on the HUD from
also ordering the squad somewhere.

This is the permanent record. The multi-slice plan that builds the rest of it — the time-pace
strip, the target panel, the squad portrait bar, the activity feed — lives in
`Docs/roadmaps/hud-roadmap.md` and is a temporary working document.

> **Status: Slices 1 and 2 of the HUD roadmap have shipped.** Four of the six regions are real:
> the nav rail, the resource strip, the time-pace strip and the target panel. The squad portrait
> bar and the activity feed are still labelled placeholder boxes in `UI_Strategy` — they are there
> so the layout can be judged whole, and Slice 3 replaces each with its real widget.

## Player Surface

### The frame

Six regions, positioned to the wireframe at `tmp/Squad HUD Wireframes v3-selection.png`:

| Region | Where | Today |
|---|---|---|
| Nav rail | Left, top-anchored | **Live.** Five buttons: Squad, Inventory, Map, Research, Help |
| Time pace | Top centre | **Live.** Pause / 1× / 2× / 4× buttons and a readout of the tier actually running |
| Resource strip | Top right | **Live.** Gold balance |
| Target panel | Top right, below the strip | **Live.** Name, what it is, how far away, health, and what you may do to it |
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

### Time pace

Four buttons — pause, 1×, 2×, 4× — and a readout. `Space` toggles pause; `-` and `=` step a
longer ladder than the buttons cover (paused, 1/3×, 1/2×, 3/4×, 1×, 2×, 4×, 8×). The controls are
real from day one: they change the simulation, not a number on screen.

**The readout is the source of truth, not the buttons.** Step onto a tier that has no button —
3/4×, say — and no button is lit while the readout says "3/4x". That is honest and costs nothing;
lighting the nearest button would be a quiet lie about what the world is doing.

Pausing remembers the speed it interrupted, so unpausing returns there rather than to 1×.

### Target panel

Whatever the player last clicked: its name, what it is and how it feels about you
(`PERSON - HOSTILE - 12m`), a health bar if it has health, and a row of the things you may
actually do to it. With nothing targeted the panel hides rather than sitting there empty.

| Target | Row |
|---|---|
| Container | Open |
| Body (Downed or Dead) | Loot |
| Person on their feet | Talk, Attack |
| One of your own squad | *nothing* |

**The row is real, not decorative.** Each button is assembled from what the player controller's
own gating helpers would permit right now, and clicking one runs the same code the equivalent key
runs. A **disabled action still shows, with its reason beside it**: "Talk" greyed out and reading
*They won't deal with you* because the person is hostile teaches the rule, where a missing button
teaches nothing. Attack is the one action with no range gate, correctly — the squad walks over.

Your own squad gets an empty row rather than a row of disabled buttons, because every verb that
applies to your own pawn already has a route that isn't this panel.

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
- **Pace is shared world state; which panels you have open is not.** Everything else on this HUD
  is per-local-player, so the contrast is worth stating. The pace lives on a `UTimePaceComponent`
  on the **GameState** — one per session, replicated to every client — and is only ever mutated
  under `HasAuthority()`. A client can't RPC the GameState because it doesn't own it, so the ask
  routes through the player controller it *does* own (`Server_RequestPace`). Applying it is
  `UGameplayStatics::SetGlobalTimeDilation` on the server; `AWorldSettings::TimeDilation`
  replicates on its own, so there is no multicast and there shouldn't be one.
  - **Pause is a tier, not `SetPause`.** `AGameModeBase::SetPause` is a single-player mechanism
    that doesn't survive co-op. Pause is the bottom rung of the same dilation ladder, at 0.0001
    rather than 0 — `AWorldSettings` clamps whatever it is handed to `MinGlobalTimeDilation`,
    which is 0.0001 by default, so passing 0 would silently become that anyway. Naming the real
    number means the tier the label claims is the tier the world runs at.
  - **Stepping clamps, it never wraps.** `=` held down at 8× stays at 8×. A ladder that wrapped
    would drop the player from top speed to frozen, which is the kind of bug nobody reproduces on
    purpose.
  - **Any player may change it.** That is what the code does with no extra work, and it is
    deliberately provisional — see `Docs/roadmaps/hud-roadmap.md`'s Open Questions. Host-only and
    slowest-request-wins are each one `if` in `SetPace` away, and want a real co-op session to
    judge.
  - **The tier to resume to lives on the component, not the controller that paused.** In co-op one
    player can pause and another unpause; they have to arrive at the same speed, and they would
    not if each controller remembered its own.
- **A widget never guesses at the result of a request.** The pace buttons don't repaint
  themselves on click — they ask, and the next frame's push moves the readout. A button that
  updated optimistically would show a tier the world wasn't running at whenever the request was
  refused, which is precisely when the player most needs to be told the truth.
- **The target panel's action row is assembled from the rules, not alongside them.** Every
  action comes out of `AStrategyPlayerController::BuildTargetInfo`, which calls the same
  `IsLootableNPC` / `IsInteractableNPC` / reach predicates the keys call. **And the click
  re-derives the row rather than trusting the button**: the panel the player clicked is a frame
  old, and a frame is long enough for the squad to have walked out of range, so
  `RequestTargetAction` rebuilds the row, finds the action in it, and refuses with the same
  refusal line the key would have raised if it has gone disabled in between.
- **The target label and the action row are one thing, not two.** `GetSelectionTargetInfo()`
  *replaced* `GetSelectionTargetLabel()` rather than joining it. Two paths would eventually let
  the name on screen and the actions offered describe different things.
- **Every per-frame push is guarded at the region that receives it.** `DrawHUD` pushes gold, the
  pace, the selection count and the whole target struct every frame, and each region compares
  what it was handed against what it is already showing before touching Slate. The target panel
  compares what would be *drawn* — distance to the nearest metre, health to the nearest percent —
  rather than the raw floats, because otherwise a stationary squad invalidates layout sixty times
  a second over sub-millimetre jitter.
- **Layout now, styling later — but legible is not styling.** Plain UMG defaults and correct
  positions, deliberately, so the layout can be judged in PIE before anything is worth restyling,
  and so a layout change doesn't cost restyling twice. The wireframe's cream palette, fonts and
  portrait art are a later pass. **The exception is contrast**, because a layout nobody can read
  can't be judged: UMG's default `UBorder` brush is white and its default `UTextBlock` colour is
  white, so every region shipped with invisible text until the backgrounds were set dark
  translucent and the text near-white. That minimum — dark panel, light text — is the floor, not a
  style choice, and a later styling pass replaces it wholesale.

## C++ Implementation

All of it in `SmoresUI`, except the controller side and the two pieces of shared state the HUD
reads (`UTimePaceComponent` in `SmoresCore`, `AStrategyGameState` in `smores`).

### The HUD root

- **`UStrategyUI`** — the always-on widget spawned by `AStrategyHUD::BeginPlay` at Z-order 0. Its
  WBP is a full-screen canvas with an anchored slot per region; each region binds by name through
  `BindWidgetOptional` (`NavRail`, `ResourceStrip` today). It keeps the selected-unit count and
  the selection-target label it always had, and forwards `SetGold` to the resource strip.
- **`AStrategyHUD::DrawHUD`** — pushes the selection count, the whole `FStrategyTargetInfo`, gold
  and the current `EGamePace` every frame, and calls `UStrategyUI::RefreshNavRail()`. That
  per-frame push is the pattern every new region follows, and it is why regions read state rather
  than subscribing to it.
- **`AStrategyHUD::GetTimePace()`** — the pace component, resolved once and held in a
  `TWeakObjectPtr`, retrying while null. Identical shape to `GetWallet()` and for the identical
  reason: the GameState is one of the last things to arrive on a joining client, so an early miss
  is normal and must not be cached as a negative. It finds the component **by class on
  `AGameStateBase`** rather than casting to `AStrategyGameState`, because `SmoresUI` cannot
  include anything from `smores` — and doesn't need to, since `AGameStateBase` is an engine type
  exactly like the `APlayerState` the wallet hangs off.

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
- **`UTimePaceWidget`** — four `BindWidgetOptional` `UButton`s and a `PaceText` readout.
  `GetStripPaces()` is the four tiers that get buttons; the label comes from
  `UTimePaceComponent::GetPaceLabel`, so a tier is named in one place. Nothing here reads the
  component directly — the tier arrives through the HUD's push, which is what lets a client whose
  GameState hasn't replicated in yet simply show nothing rather than needing a null check of its
  own.
- **`UTargetPanelWidget`** — name, classification line, health bar and the action row, rebuilt
  from `FStrategyTargetInfo`. It keeps its action buttons in `ActionWidgets` and resizes that row
  rather than rebuilding it, since the row usually keeps its shape while its contents change. It
  hides itself (`Collapsed`) when there is no target — whether that's right is a PIE call, see the
  roadmap's Open Questions.
- **`UTargetActionWidget`** — one button on that row, spawned from `ActionWidgetClass`. It is
  deliberately **not** a `UHUDRegionWidget`: it lives inside one. A real `UButton` consumes its own
  press, and when the action is disabled the button doesn't handle the click at all, so it bubbles
  to the panel's own shield rather than reaching the world — either way nothing falls through. Its
  disabled reason is worded by `URefusalWidget::GetRefusalText`, so the same reason reads
  identically here and on the refusal line.
- **`UResourceStripWidget`** — the gold readout. The balance still arrives the way it always did:
  `AStrategyHUD` resolves `UWalletComponent` off its own player state (with a late-arrival retry)
  and pushes it through `UStrategyUI::SetGold`. Only the display moved.

### The shared state the HUD reads

- **`EGamePace`** (`GamePace.h`, `SmoresCore`) — the eight tiers, **declared slowest-to-fastest on
  purpose**. Stepping is ordinal arithmetic over this enum, so the declaration order *is* the
  ladder; a new tier goes in its speed position, never at the end.
- **`UTimePaceComponent`** (`SmoresCore`) — the current tier, the tier to resume to, and the
  static ladder helpers (`GetPaceLadder`, `GetDilationForPace`, `GetPaceLabel`, `StepPace`). The
  helpers are static and world-free deliberately: the tier arithmetic is the part that can be
  wrong in a way nobody notices, so it is the part that gets tested. Both stored tiers are
  `UPROPERTY(Replicated)`; `SetPace` is authority-gated.
- **`AStrategyGameState`** (`smores`, `Variant_Strategy/`) — hosts that component and owns no
  state of its own, the same shape as `AStrategyPlayerState`. The GameState is Unreal's
  composition root for state shared by everyone in a session, which is exactly what pace is.
  **`BP_StrategyGameState` must be set as `GameStateClass` on `BP_StrategyGameMode`** or nothing
  spawns it and the pace controls silently do nothing.
- **`FStrategyTargetInfo` / `FTargetAction`** (`StrategyTargetInfo.h`, `SmoresUI`) — the target
  panel's whole contents, rebuilt every frame. `bHasTarget` is a flag of its own rather than "is
  the name empty", because a target's name is authored data and an actor nobody got round to
  naming would otherwise make the whole panel vanish. `FTargetAction::DisabledReason` is an
  `ESmoresRefusalReason`, not a sentence, for the same reason every refusal in this project is.
- **`StrategyTargetAction::Open()` / `Loot()` / `Talk()` / `Attack()`** — the action ids, as
  functions rather than header constants because an `FName` built during static initialisation
  runs before the name pool is guaranteed to exist.

### The panels

- **`EHUDPanel`** (`HUDPanel.h`) — the shared vocabulary between the rail, the controller and the
  windows. In its own header because both the interface and the widgets need it and neither
  should have to include the other.
- **`IStrategyHUDCommands`** (`StrategyHUDCommands.h`) — the things the HUD needs the controller
  to *do*, implemented by `AStrategyPlayerController` alongside `IStrategySelectionHost`,
  `IStrategyCameraCommands` and `IInventoryMoveHost`. One interface, not four: everything on it is
  a request only the controller can service. Data the HUD only needs to *read* goes the other way,
  through `DrawHUD`'s push or a component lookup. `RequestPanel`, `IsPanelOpen`, `RequestPace` and
  `RequestTargetAction` are live; `RequestSelectUnit` is declared and stubbed for Slice 3.
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
- **`BuildTargetInfo(const AActor*, const TArray<AStrategyUnit*>&)`** — a **static** that takes
  the selection explicitly rather than reading it off the instance. Two reasons, and both are the
  point: it is the one piece of this controller worth a test (an action row that quietly offers
  something the rules forbid is exactly the silent kind of wrong), and a static taking a unit
  array can be called from a test world with no controller in it at all. It also makes the
  dependency honest — the row depends on the target and the selection, and on nothing else. It is
  `public` for the test's sake; the gating predicates it calls stay `protected`.
- **`IsHolderInRangeOfUnits`** — the body of `IsHolderInRangeOfSelection` with the selection
  passed in, so `BuildTargetInfo` applies the identical reach rule. One rule, one place; a second
  copy would eventually disagree about what "in range" means.
- **`Server_RequestPace`** — the client's ask, carried to the server by the one actor the client
  owns. `RequestPaceStep` is the shared body of the `-` and `=` keys, and steps from the
  *replicated* current tier rather than a local guess, so holding the key can't run the client's
  idea of the pace ahead of the server's.
- **`GetTimePace()`** — `GetWorld()->GetGameState<AStrategyGameState>()->GetTimePace()`. Unlike
  the HUD's version this may cast, because the controller is in `smores`.

## Blueprint / Asset Dependencies

All under `Content/Variant_Strategy/UI/`:

| Asset | Parent | Bound names |
|---|---|---|
| `UI_Strategy` | `UStrategyUI` | `NavRail`, `ResourceStrip`, `TimePaceRegion`, `TargetPanelRegion`; plus the units-count display. Two placeholder boxes — `SquadBarRegion`, `ActivityFeedRegion` — hold Slice 3's space |
| `WBP_NavRail` | `UNavRailWidget` | `SquadButton`, `InventoryButton`, `MapButton`, `ResearchButton`, `HelpButton` |
| `WBP_ResourceStrip` | `UResourceStripWidget` | `GoldText` |
| `WBP_TimePace` | `UTimePaceWidget` | `PauseButton`, `NormalButton`, `DoubleButton`, `QuadrupleButton`, `PaceText` |
| `WBP_TargetPanel` | `UTargetPanelWidget` | `NameText`, `ClassificationText`, `HealthBar`, `ActionBox`; plus the `ActionWidgetClass` property, which must point at `WBP_TargetAction` or the row silently shows nothing |
| `WBP_TargetAction` | `UTargetActionWidget` | `ActionButton`, `LabelText`, `ReasonText` |
| `WBP_HUDPlaceholderRegion` | `UHUDPlaceholderRegionWidget` | `LabelText`; two instances left in `UI_Strategy`, one per unbuilt region |
| `WBP_SquadPanel` | `USquadPanelWidget` | `TitleText`, `CloseButton`, `TitleBarDragHandle`, `ResizeHandle`, `BodyText` |
| `WBP_MapPanel` | `UMapPanelWidget` | as above |
| `WBP_ResearchPanel` | `UResearchPanelWidget` | as above |
| `WBP_HelpPanel` | `UHelpPanelWidget` | as above |

`BP_StrategyPlayerController` holds the four `PanelWidgetClasses` entries and the eight
`UInputAction` references. `BP_StrategyHUD` holds `UIWidgetClass` (`UI_Strategy`) and
`RefusalWidgetClass`, unchanged.

**`BP_StrategyGameState` (`Content/Variant_Strategy/Blueprints/`) is set as `GameStateClass` on
`BP_StrategyGameMode`.** It is an empty Blueprint over `AStrategyGameState` and exists only
because the project's C++ gameplay classes are all `UCLASS(abstract)` and an abstract class can't
be spawned. If that property is ever cleared, the pace component never exists, `GetTimePace()`
returns null forever, and the pace keys and buttons do nothing at all — with no error, because a
missing GameState component is a legitimate early-frame state on a client.

**The `GoldText` widget moved out of `UI_Strategy` into `WBP_ResourceStrip`** in Slice 1, and
**`SelectionTargetBorder` / `SelectionTargetText` were deleted from it** in Slice 2 — the target
panel subsumes them, and the `GetSelectionTargetLabel` binding they used no longer exists.

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

- **Two of the six regions are placeholder boxes** (`WBP_HUDPlaceholderRegion` instances, so they
  shield clicks like any other region). The squad portrait bar and the activity feed are Slice 3.
  See `Docs/roadmaps/hud-roadmap.md`.
- **`L` is mapped and bound but does nothing yet**, and logs a line saying the activity feed
  arrives in Slice 3. It was mapped early so all eight of the HUD round's key mappings could be
  authored and verified in one editor pass — the one manual editor step in Slice 1. A key that
  logs is deliberately not a key that silently does nothing: it is how a mistyped mapping is told
  apart from an unimplemented one. `Space`, `-` and `=` left this list in Slice 2.
- **Nothing reacts to a pace change beyond the dilation.** Animation, timers and AI all slow down
  because global time dilation slows everything; no system has an opinion about *being* at 8×.
  Whether high-speed play is readable is a PIE judgement, and the activity feed (Slice 3) is the
  first half of the answer.
- **The pace strip has no tooltip explaining the tiers it has no button for.** A player who only
  ever clicks will never discover 1/3× or 8×; the help panel names `-` and `=` but not the ladder
  they walk. Worth revisiting with the styling pass.
- **The target panel's hostility reading is server-only.** `BuildTargetInfo` calls
  `AStrategyUnit::IsAggressive()`, and `Disposition` is not a replicated property — so on a remote
  client the classification line and the Talk/Attack enable states would be wrong (everyone reads
  as neutral). Health, distance and everything else on the panel are fine. It is invisible today
  because a single-player PIE session is its own authority, and it is a display defect rather than
  an exploit, since the server re-checks every rule. `combat.md`'s Known Gaps holds the detail and
  the one-line fix.
- **The target panel's portrait is missing** — the wireframe shows per-target art, and no target
  has any. `player-interface.md`'s world-space contextual prompts are the other half of this and
  are explicitly out of scope for the HUD round.
- **No styling pass.** Plain UMG defaults throughout — see Core Rules for why that is on purpose.
  The active-panel state on the rail is a background tint, which is the cheapest thing that
  survives restyling.
- **One legacy readout still leaks clicks.** `Border_0` (+ `SelectionCount`, the "N selected"
  readout) is a plain `UMG.Border` sitting directly on `HUDCanvas`, predating the region system. A
  right-click *directly on it* still issues a move order to the world, for the reason in Core
  Rules. It can't simply be reparented into a region: a `UHUDRegionWidget` is a `UUserWidget`, not
  a `UPanelWidget`, so `MoveWidget` refuses it, and `SetNamedSlotContent` creates a *new* widget
  rather than moving an existing one — either route risks the property binding to
  `GetSelectedUnitsCount`, which must stay in `UI_Strategy`'s own tree to resolve at all.
  **The natural fix is Slice 3**, whose squad portrait bar subsumes it. Its twin,
  `SelectionTargetBorder` / `SelectionTargetText`, was deleted in Slice 2 exactly this way.
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
