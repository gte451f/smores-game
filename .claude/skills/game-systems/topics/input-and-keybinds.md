# Input and Keybinds

## Purpose

The single place to look **before adding any player-facing key or button**. It records what
is bound today, which keys are deliberately held in reserve for systems not built yet, and
the one wiring pattern every binding must follow.

Keybinds are player-configurable by design (see the `game-design` skill's
`input-and-platforms.md`), which is what makes this topic load-bearing rather than
bookkeeping: a binding wired any way other than Enhanced Input can never appear in a
settings screen, and a default that collides with another system's default is a bug the
player has to discover and work around.

Amend this topic in the same change as any new binding.

## Player Surface

### Defaults — world (`IMC_Strategy_Mouse`, always active in mouse mode)

| Key | Action asset | Does |
|---|---|---|
| Left mouse | `IA_Strategy_SelectClick`, `_SelectHold`, `_SelectClickAdditive`, `_SelectAllDoubleClick` | Select; hold to drag a selection box; additive select; double-click a loose world item to pick it up, a container or a body to open it, a *living* NPC to interact with them (trade if they carry a trader component; dialog's future home; never on a hostile one), or empty ground to select all on screen |
| Right mouse | `IA_Strategy_InteractClick` | Move order / interact at the cursor |
| Middle mouse (hold) | `IA_Strategy_InteractHold` | Rotate the camera |
| Mouse wheel | `IA_Strategy_Zoom` | Camera zoom |
| `W` `A` `S` `D` | `IA_Strategy_MoveCamera` | Pan the camera |
| `Q` / `E` | `IA_Strategy_AdjustHeight` | Lower / raise the camera |
| `Shift` (either) | `IA_Strategy_SelectionModifier` | Hold to add to / remove from the selection |
| `Tab` | `IA_Strategy_CyclePawn` | Cycle the selection to the next player pawn |
| `I` | `IA_Strategy_Inventory` | Toggle the selected pawn's inventory window |
| `O` | `IA_Strategy_ToggleContainer` | Open the nearest container, or a Downed NPC's loot |
| `H` | `IA_Strategy_Attack` | Attack the selected NPC |
| `T` | `IA_Strategy_Talk` | Talk to / trade with the selected NPC — the keyboard route to the double-click interact, reading the same targeted NPC `H` does |
| `P` | `IA_Strategy_SquadPanel` | Toggle the squad roster panel (a stub today — see `hud-and-panels.md`) |
| `M` | `IA_Strategy_MapPanel` | Toggle the world map panel (a stub today) |
| `U` | `IA_Strategy_ResearchPanel` | Toggle the research panel (a stub today) |
| `F1` | `IA_Strategy_HelpPanel` | Toggle the keybind list — the in-game copy of this table |
| `Space` | `IA_Strategy_TogglePause` | Freeze the simulation, or return it to the speed it was running at |
| `-` | `IA_Strategy_PaceSlower` | Step the pace ladder one tier slower, clamping at paused |
| `=` | `IA_Strategy_PaceFaster` | Step the pace ladder one tier faster, clamping at 8× |
| `L` | `IA_Strategy_ToggleActivityFeed` | Expand / collapse the activity feed — expanded it shows the history and stops fading |

Each of the four panel keys has a nav-rail button that does the identical thing; both routes run
`IStrategyHUDCommands::RequestPanel`, so they cannot drift apart. `I` (above) gained a rail button
the same way.

The three time keys work the same way against the pace strip's buttons, through
`IStrategyHUDCommands::RequestPace` — but they cover **more** ground than the buttons do. The
ladder is paused, 1/3×, 1/2×, 3/4×, 1×, 2×, 4×, 8×; the strip has buttons for only four of those,
and `-`/`=` walk all eight. That is deliberate, not an omission — see `hud-and-panels.md`.

`T`, `O` and `H` likewise each have a button on the target panel's action row when the current
target is one they apply to.

`L` is the odd one out among the HUD keys: it has **no button anywhere**. Expanding the feed is a
property of the feed, not a command to the controller, so it goes controller ->
`AStrategyHUD::ToggleActivityFeed` -> `UStrategyUI` -> the region rather than through
`IStrategyHUDCommands`. If it ever gains a corner grip or a chevron, that control calls
`UActivityFeedWidget::ToggleExpanded` directly and the key still cannot drift from it, because
there is only the one method.

### Mapped and bound, but not yet implemented

**Nothing.** Every key in the tables above does something. The HUD round's eight keys were mapped
in one editor pass in Slice 1 — `UInputMappingContext` mappings have to be authored by hand, so
doing them in one pass beat doing them in three — and the four that arrived ahead of their
features (`Space`, `-`, `=` in Slice 2, `L` in Slice 3) each logged a line naming the slice that
would implement them until it did.

That pattern is worth reusing, and so is the discipline around it: a key mapped ahead of its
feature must be **treated as taken** in the reserved list, must log rather than silently doing
nothing (which is what distinguishes a mistyped mapping from an unimplemented feature), and must
stay **out of the in-game help panel** until it works.

### Defaults — inventory (`IMC_Strategy_Inventory`, priority 1, only while a window is open)

| Key | Action asset | Does |
|---|---|---|
| `R` | `IA_Strategy_RotateDraggedItem` | Turn the item being dragged 90° |

### Touch (`IMC_Strategy_Touch`)

Two gesture actions only — `IA_Strategy_Touch_Primary` and `IA_Strategy_Touch_Secondary`.
Nothing here can collide with a keyboard default. Touch is incidental to the inherited
control scheme, not a targeted platform.

### Not bindings

Mouse-button interactions **inside a widget** — with or without a modifier — are not keybinds
and take no slot in the tables above; the click event already carries the button and
`IsControlDown()`/`IsShiftDown()`/`IsAltDown()`. They still need to be *consistent*, so they
have their own convention below. In use today:

| Gesture | Where | Does |
|---|---|---|
| Right-click | An item in a pawn's own inventory window | Wear it (into the slot its definition names) |
| Right-click | A filled paperdoll slot | Take it off, back into that pawn's pack |
| Left-click | A `UButton` or dropdown in a window's own chrome (the inventory sort buttons, the category filter) | Whatever that control does |
| Left-click | A nav-rail button on the HUD | Opens or closes that panel — the same path as its key |
| Left-click | A squad portrait | Selects that unit alone |
| Left-click twice, fast | A squad portrait | Selects it **and** cuts the camera to it. The first click has already selected - see Core Rules |
| Left-click | An activity-feed tab | Filters the feed to that tab |

The last row is the ordinary case, not a special one: a button the player clicks *inside* a
window is just a button. It needs no action asset and takes no key. The flip side is that a
control built only as a button has **no keyboard route at all** — the inventory sort and filter
currently have none, and giving them one means adding a real `UInputAction` to
`IMC_Strategy_Inventory` like the rotate key, not a `NativeOnKeyDown`.

Right-click means "move order" in the world and "equip" over a window, which works only
because a window now swallows the *press* of every button that lands on it — see Core Rules.

## Core Rules

- **Every player-facing key or button goes through a `UInputAction`.** Unreal's player
  key-mapping system only sees Enhanced Input actions, so a binding made any other way can
  never be rebound by the player. Two alternatives are specifically ruled out:
  - a `NativeOnKeyDown` on a UMG widget — game widgets don't hold keyboard focus, so it
    never fires at all;
  - a Slate input pre-processor — it *works*, but is invisible both to rebinding and to
    anyone reading the input assets. The inventory rotate key shipped this way and was
    moved; see `inventory.md`.
- **Enhanced Input sits at the end of the keyboard focus path.** Keys reach it only because
  the game viewport holds focus: UMG game widgets are non-focusable, so on a click Slate
  walks up from the widget to the viewport, which does take focus. That is why an inventory
  key still works mid-drag — a drag captures the pointer but not the keyboard. If a future
  screen ever *does* take focus (a modal, full-screen menu), it has to pass through the keys
  it doesn't claim, or gameplay input dies while it's open.
- **Scope a context-specific key with its own `UInputMappingContext`**, added and removed as
  that context opens and closes, rather than adding it to the always-on mouse context.
  That's what lets one key mean different things in different places without a conflict, and
  it's the reason the inventory context sits at priority 1 above the world context's 0.
- **One key, one meaning per context.** Several actions may share a key inside one context
  when their triggers differ (the four left-mouse selection actions do exactly this), but two
  *unrelated* behaviours must not.
- **A floating window swallows the press of every mouse button over it, and lets the release
  through** (`UWindowWidget::NativeOnMouseButtonDown`). Swallowing the press is what lets a
  mouse button mean one thing over a window and another in the world — without it, right-click
  to equip would also issue a move order to whatever is behind the panel. Swallowing the
  *release* would be actively harmful: Enhanced Input never saw the press the window ate, so an
  action bound on `Completed` has nothing to complete anyway, whereas eating a release whose
  press the viewport *did* see (a drag-select begun on the world, ended over a window) leaves
  that button stuck down in `UPlayerInput`. Slate bubbles from the deepest widget up, so a child
  that wants the button still gets it first.
- **The always-on HUD is not a window and gets none of that for free.** It sits at Z-order 0
  *below* every floating window, and its regions are plain `UUserWidget`s. `UHUDRegionWidget`
  (`SmoresUI`) is the base that supplies the same press-swallowing behaviour, and every HUD region
  derives from it — see `hud-and-panels.md`. Two consequences worth knowing here: a clickable
  element on the HUD should be a real `UButton` (Slate's `SButton` consumes its own press *and*
  double-click, so it needs nothing extra), and the shield only works if the region actually
  hit-tests, which is why `bBlockWorldClicks` forces `Visible` on construct.
- **A double-click is a different Slate event, and claiming the press does not claim it.**
  Windows sends `WM_xBUTTONDBLCLK` instead of `WM_xBUTTONDOWN` for the second click of a rapid
  pair; `FWindowsApplication` turns that into `OnMouseDoubleClick`, which Slate routes as
  `OnMouseButtonDoubleClick` on its own bubble pass. A widget overriding only
  `NativeOnMouseButtonDown` therefore eats the first click of a double-click and lets the second
  reach the viewport — where it registers as a press, and the release completes whatever world
  action that button is bound to. **Any widget that claims a mouse button must override
  `NativeOnMouseButtonDoubleClick` too**; routing it straight into the press handler is the
  right default, since it makes a fast second click mean what a slow one does. This is easy to
  miss because it only reproduces inside the OS double-click time *and* slop rectangle — a
  slightly slower repeat comes through as two ordinary presses and behaves correctly.
- **A widget that wants a double-click of its own must time it, not listen for it.** The rule
  above cuts the other way too: because `SButton` routes `OnMouseButtonDoubleClick` into
  `OnMouseButtonDown`, a real `UButton` never reports a double-click - it reports two ordinary
  clicks. So a control that wants "click does X, double-click also does Y" measures the gap
  between two `OnClicked` calls itself, which is what `USquadPortraitWidget` does (0.5s, matching
  `IA_Strategy_SelectAllDoubleClick` and Windows' own double-click speed).
  **Keeping the `UButton` is the point** - it consumes its own press, so the gesture can never
  fall through to the world. The alternative, a non-button root carrying both mouse overrides, is
  re-solving the clickable-HUD trap by hand. One consequence to design around: the first click's
  action has **already happened** by the time the second arrives, so the second click's behaviour
  has to be additive rather than different. Selecting and then also focusing the camera works;
  "click selects, double-click renames" would not.
- **The double-click gesture's own window is measured release-to-release, not press-to-press.**
  `IA_Strategy_SelectAllDoubleClick` uses `UInputTriggerRepeatedTap`, whose `RepeatDelay` clock
  starts when the *first* click is released and must stop by the time the *second* one is
  released — so the budget covers the gap between the clicks **plus** however long the second
  click is held, and frame granularity eats a further ~16 ms at each end. Epic's Strategy
  template shipped this asset with `RepeatDelay` at 0.2, which a comfortable double-click
  (~200 ms gap + ~80 ms hold) misses outright; it was raised to the engine default of 0.5, which
  is also Windows' system-wide double-click speed. The symptom of too short a window is
  distinctive and easy to misread: the gesture fails, the player instinctively clicks faster on
  the retry, and it succeeds — so it presents as "the second or third try works" rather than as
  a timing setting. Each individual click must also be held under `TapReleaseTimeThreshold`
  (0.2s) to count as a tap at all.
- **Modifier conventions** — keep these consistent everywhere so they're learnable:
  `Shift` = whole/all (take all, queue an order), `Ctrl` = part/split (split a stack),
  `Alt` = inspect/info. `Shift` is already the in-world selection modifier, which matches.
- **Defaults matter even though everything is rebindable**, because most players never open
  the keybind screen. A default that fights PC RPG convention is experienced as the game
  being wrong, not as a setting waiting to be changed.

## C++ Implementation

- **All bindings live on `AStrategyPlayerController::SetupInputComponent`**
  (`smores`, `Variant_Strategy/`). Each action is an `EditAnywhere UInputAction*` property;
  the actual asset is assigned on `BP_StrategyPlayerController`.
- **Trigger event choice matters.** Use `ETriggerEvent::Started` for a one-shot that should
  fire on press, `Completed` for one that should fire on release, and `Triggered` only for
  continuous input (camera pan, zoom). `Triggered` on a held *key* fires every frame — that
  would, for instance, spin a dragged item once per tick.
- **`AStrategyPlayerController::UpdateInventoryInputContext`** is the worked example of a
  scoped context: it adds `InventoryMappingContext` at priority 1 when an inventory, container
  or equipment window opens and removes it when the last one closes. Every path that opens or
  closes a window must call it — including `HandleWindowClosed`, which is how a window shut with
  its own close button gets back here at all (`UWindowWidget::OnWindowClosed`).
- **`UInventoryDragDropOperation::GetActiveDrag`** (`SmoresUI`) is how a controller-bound
  action reaches an in-flight drag. Slate owns the drag — no widget or controller holds a
  reference — so this static is the only bridge, and it lives in `SmoresUI` so the UMG drag
  plumbing stays out of the controller.
- **Touch selection** is chosen at `SetupInputComponent` time by `ShouldUseTouchControls()`,
  which picks `TouchMappingContext` over `MouseMappingContext` — they are alternatives, not
  layers.

## Blueprint / Asset Dependencies

- **`IA_Strategy_*`** (`Content/Variant_Strategy/Input/Actions/`) — one `UInputAction` per
  action. A new Boolean action is most easily made by duplicating an existing Boolean one
  (`IA_Strategy_CyclePawn`). The HUD round added eight: `_SquadPanel`, `_MapPanel`,
  `_ResearchPanel`, `_HelpPanel`, `_TogglePause`, `_PaceSlower`, `_PaceFaster`,
  `_ToggleActivityFeed`.
- **`IMC_Strategy_Mouse`** — the always-on world context, added at priority 0.
- **`IMC_Strategy_Inventory`** — added at priority 1 only while an inventory window is open.
- **`IMC_Strategy_Touch`** — the touch alternative to the mouse context.
- **`BP_StrategyPlayerController`** — holds every `UInputAction` and `UInputMappingContext`
  reference as a class default.
- **`UInputMappingContext` key mappings must be authored by hand in the editor.** MCP cannot
  write them safely, and clearing them silently fails — see the `mcp-workflow` skill. The
  workflow for a new binding is: create the action asset and wire the C++, then hand the user
  the single mapping step.

## Extension Points

**Removed, and worth remembering why:** a single click on an already-Aggressive NPC used to
issue a squad attack order. It broke the settled rule that a single click selects the actor
under the cursor and does nothing else, and it only started *mattering* when the double-click
learned to mean "interact with this person" — the select click fires alongside the
double-click, so talking to a hostile would have opened their shop and started a fight at once.
Removed in Slice 8 of `Docs/roadmaps/inventory-roadmap.md`; `H` covers attacking a target.

### Adding a binding — the checklist

1. **Check the tables above and the reserved list below.** If the key is taken in the same
   context, or reserved, pick another.
2. **Decide the context.** Global to gameplay → `IMC_Strategy_Mouse`. Only meaningful while
   some screen or mode is open → its own context, added and removed with that screen.
3. **Create the `UInputAction`**, bind it in `SetupInputComponent` with the right trigger
   event, and add an `EditAnywhere` property for it.
4. **Hand the user the key mapping** (editor-only), then assign the asset on
   `BP_StrategyPlayerController`.
5. **Update this topic** in the same change.

If the interaction is modifier + mouse button *inside a widget*, skip all of this — read the
modifier off the click event — but follow the modifier conventions in Core Rules.

### Reserved keys — do not take these for something else

Held for their conventional PC-RPG meaning, whether or not the system exists yet. Taking one
now means either a conflict later or a default that surprises the player.

| Key | Held for |
|---|---|
| `Esc` | Close window / back / system menu — never bind to gameplay |
| `F5` / `F9` | Quicksave / quickload — see `save-system.md` |
| `J` | Journal / quests |
| `C` | Character sheet for the selected unit |
| `K` | Skills / training |
| `B` | Base / build mode |
| `1`–`9`, `0` | Squad and control-group recall; `Ctrl`+digit to assign |
| `` ` `` | Console |
| `Alt` (hold) | Highlight interactables / show ground item names |

**`T`, `M` and `Space` have left this list** — all three are mapped now, in the tables above. `M`
and `Space` went to the systems they were being held for (the map panel, pause), which is the list
working as intended, and `Space` now actually pauses rather than logging that it will.

**`F1`–`F4` has been dropped rather than shrunk.** It was held for "select squad member N, if
party-slot selection is ever wanted"; `F1` is now the help panel, which is the near-universal PC
convention and worth more than a fourth route to a roster. The other three keys are released
because the thing they were reserved for has two better answers already: `P` opens the roster, and
the squad portrait bar (`hud-roadmap.md` Slice 3) selects a member with one click. Shrinking to
`F2`–`F5` was the other option and is wrong — `F5` is quicksave.

Broadly free today: `F`, `G`, `N`, `V`, `X`, `Y`, `Z`, and `F2`–`F4`. `P`, `U` and `L` have left
this list, and `L` is now a live binding rather than merely a taken one. `R` is used in the
inventory context only — prefer not to give it a second, unrelated meaning in the world
context, since one key meaning two things is exactly what the context system exists to
*avoid* needing.

### Existing defaults worth revisiting

Not bugs, and not scheduled — but they fight convention, and the moment to fix them is before
players build muscle memory:

- **`H` to attack** is unconventional. The RTS convention is `A` (attack-move), which is
  unavailable because `A` pans the camera. Worth settling when combat commands expand beyond
  one key.
- **`O` to open a container** is unconventional; `E` is the usual interact/open key, which is
  unavailable because `E` raises the camera.
- **`Q`/`E` on camera height** spends two premium keys — in most RPGs they're ability or
  quick-slot keys. Camera height is a rarely-touched control holding valuable real estate.

## Known Gaps

- **No keybind settings screen exists**, and no action is marked player-mappable yet. The
  actions are Enhanced Input actions, which is the prerequisite; exposing them means adding
  `PlayerMappableKeySettings` (a display name and unique name per mapping) and a UI over
  Unreal's user-settings API. Until then "rebindable" is a property of the architecture, not
  something a player can do.
- **`IA_Strategy_ResetCamera` is bound in C++ but mapped to no key** in either context, so
  camera reset is currently unreachable. Either map it or drop the action.
- **No gamepad bindings.** Basic controller support is a pre-launch goal per
  `input-and-platforms.md`; nothing is wired yet.
- **No conflict detection anywhere but this document.** Two mappings of the same key in the
  same context will simply both fire. Until a settings screen with conflict checking exists,
  the tables above are the only guard.
