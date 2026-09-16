# Squad HUD Roadmap

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of what the HUD actually does will live in the
`game-systems` skill — in a new `hud-and-panels.md` topic created by Slice 1, plus additions to
`input-and-keybinds.md` for every key below.

The target is the wireframe at `tmp/Squad HUD Wireframes v3-selection.png`. That image is a
**forward-looking mock**: several of its panels point at systems that don't exist (research,
quests, comms, world map). The point of this round is to **build the frame anyway** — every
region of the screen in its place, with real data behind it wherever real data exists, and an
honest placeholder where it doesn't. Empty panels the player can open are better than a HUD that
grows a new corner every time a system ships, because the layout is the thing that has to be
lived with, and it can't be judged from a picture.

Treat every section as "what to build next," not "what exists" — **except** where a heading is
marked SHIPPED, which means that section has moved into the skill and only its summary line
remains here.

## What the wireframe shows, and what's behind it today

Reading the mock clockwise from the top-left. ("3a Floating islands · sparse" in the image's
corner is the mock's own variant label, not a HUD element.)

| Region | Wireframe | Behind it today |
|---|---|---|
| Left nav rail | `Q SQUAD` / `I INVENTORY` / `M MAP` / `R RESEARCH` / `? HELP` | Only inventory exists (the `I` key). No rail, no other panel. |
| Top centre | Pause / play / 2× / 4× with `SPACE · 1 · 2 · 3` | **Nothing.** No time dilation, no pause, anywhere in the project. |
| Top right | Target panel: portrait, name, `STRUCTURE · NEUTRAL · 61m`, a bar, `[F] SCAVENGE · [T] TAG FOR SQUAD` | `IStrategySelectionHost::GetSelectionTargetLabel()` returns one `FText` string. No classification, distance, bar, or actions. |
| Top right (added) | — | **Gold has no home in the mock.** It lives in `UStrategyUI::GoldText` today and must not be lost. |
| Bottom left | Nine circular portraits, selection ring on the active one | `AStrategyPlayerController::PlayerPawns` is the list; units have no name, no portrait, and no on-screen presence beyond the word "Selected" drawn over them. |
| Bottom right | Tabbed feed: `LOG` / `SQUAD` / `QUESTS` / `COMMS`, colour-coded lines, "fades after 8s · `[TAB]` expand" | **Nothing.** Damage, loot, trade and refusal events all happen and are all forgotten immediately. `URefusalWidget` shows one line for two seconds and is deliberately *not* a log. |

What already exists and should be reused rather than reinvented:

- **`UWindowWidget`** — floating-window chrome (title bar, close button, drag, resize,
  `OnWindowClosed`). Every panel the nav rail opens is one of these.
- **`URefusalWidget`** — the top-most "why not" line at Z-order 100. Still the right home for a
  refusal; the activity feed is the *record*, not the immediate answer. A refused action should
  do both: the line says it now, the feed remembers it.
- **`UStrategyUI`** — the always-on HUD widget at Z-order 0, spawned by `AStrategyHUD::BeginPlay`.
  This becomes the **root that hosts every region below**, not a peer of them.
- **`AStrategyHUD::DrawHUD`** — already pushes selection count, target label and gold into
  `UStrategyUI` every frame, and already resolves the wallet off the owning player state with a
  late-arrival retry. That per-frame push is the pattern the new panels follow.

## Architecture

### Where the code goes

Everything lands in **existing modules** — this round adds no module, and the dependency
direction (`smores` → `SmoresUI` → {`SmoresCharacters`, `SmoresEconomy`} → {`SmoresItems`,
`SmoresCombat`} → `SmoresCore`) is unchanged.

| Piece | Module | Why there |
|---|---|---|
| `UTimePaceComponent`, `EGamePace` | `SmoresCore` | Shared world state with no dependencies. Every module can read the current pace. |
| `USmoresActivityLog` (a `ULocalPlayerSubsystem`) | `SmoresCore` | Every module needs to be able to post to the feed, so it must sit at the bottom. |
| `FStrategyTargetInfo`, `FTargetAction`, `IStrategyHUDCommands` | `SmoresUI` | UI-shaped data and the narrow interface the controller implements. |
| All new widget classes | `SmoresUI` | Where the rest of the UI is. |
| `DisplayName` / `PortraitTexture` on `AStrategyUnit` | `SmoresCharacters` | Per-unit identity data the portrait bar reads. |
| `AStrategyGameState` (hosts the pace component) | `smores`, `Variant_Strategy/` | Game framework, same as the game mode and player state. |

`SmoresUI.Build.cs` gains `SmoresCombat` explicitly (health for the portrait rings). It arrives
transitively through `SmoresCharacters` today, which works but hides a real dependency.

### Reaching the player controller

`SmoresUI` cannot include anything from `smores`, so anything the HUD needs the controller to
*do* goes through one new narrow interface, implemented by `AStrategyPlayerController` alongside
the three it already implements:

```
IStrategyHUDCommands           // SmoresUI
    RequestPace(EGamePace)              -> server RPC, sets the shared pace
    RequestPanel(EHUDPanel)             -> opens/toggles a panel window (incl. the existing inventory path)
    RequestSelectUnit(AStrategyUnit*, bool bFocusCamera)
    RequestTargetAction(FName ActionId) -> runs the action the target panel offered
```

**One interface, not four.** Per CLAUDE.md, a component is preferred over another interface when
the UI wants per-player *state* — all four of these are *behaviour* (a request that only the
controller can service), so an interface is the right shape. Data the HUD only needs to *read*
goes the other way, through the existing per-frame push in `DrawHUD` or through a component
looked up the way `AStrategyHUD::GetWallet()` already looks up the wallet.

`IStrategySelectionHost` grows two members and loses one:

- `GetSelectionTargetInfo()` returning `FStrategyTargetInfo` **replaces** `GetSelectionTargetLabel()`
  — one path, not two.
- `GetControlledPlayerUnits()` returning the player's own pawns, for the portrait bar.

### Multiplayer discipline

Read `multiplayer-discipline.md` before writing any of this. The three calls already made:

- **Pace is shared world state.** It lives on a component on the **GameState**, is
  `UPROPERTY(Replicated)`, and is only ever mutated under `HasAuthority()`. A client can't RPC
  the GameState (it doesn't own it), so the request routes through the player controller
  (`Server_RequestPace`), which the client does own. Applying it is
  `UGameplayStatics::SetGlobalTimeDilation` on the server; `WorldSettings::TimeDilation`
  replicates on its own.
- **Pause is a tier, not `SetPause`.** `AGameModeBase::SetPause` is a single-player
  mechanism. Pause here is the bottom of the same dilation ladder, clamped by
  `MinGlobalTimeDilation` (0.0001 by default — raise the clamp rather than passing 0).
- **The activity feed is per-local-player client-side UI state.** Not replicated, not on the
  player state. A `ULocalPlayerSubsystem` is keyed to a local player by construction, which is
  exactly the no-singleton rule. Server-side events reach it through the client RPCs and
  replicated delegates that already exist (`UHealthComponent`'s damage delegates,
  `Client_NotifyRefusal`), never by reading server state directly.
- **Who may change the pace in co-op is an open question** — see Open Questions. The default this
  roadmap builds is *any player may*, because that's what the code does with no extra work, and
  the feed logs who did it.

### The clickable-HUD trap

The persistent HUD sits at Z-order 0 **below** every floating window (that's deliberate — see the
comment block in `RefusalWidget.h`), but unlike today's readouts these panels are *clicked*.
`UWindowWidget` swallows the press of every mouse button over it precisely so a right-click on a
window doesn't also issue a move order to the world behind it. **The new HUD panels are not
windows and get none of that for free.** Two consequences:

- Build every clickable element as a real `UButton`. Slate's button handles and consumes the
  press itself, which is what stops a portrait click from also drag-selecting the world behind it.
- Anything that claims a mouse button must also override `NativeOnMouseButtonDoubleClick` — the
  documented gotcha in `input-and-keybinds.md`. A fast second click on a portrait would otherwise
  reach the viewport and fire `IA_Strategy_SelectAllDoubleClick`.

## Keybinds

**Decision: no existing binding moves.** The wireframe's letters collide with live bindings
(`Q`/`E` are camera height, `R` rotates a dragged item, `Tab` cycles pawns, and `1`–`9` are
reserved for control groups), and the call was to keep what's bound and pick free keys instead.
The rail's labels change to match; the mock's letters are a suggestion, not a commitment.

| Key | Does | Status today |
|---|---|---|
| `P` | Squad panel | Free |
| `I` | Inventory | **Already bound** — the rail button mirrors the key, same code path |
| `M` | Map panel | Reserved *for* the world map — this is that system |
| `U` | Research panel | Free. The weakest pick of the set; see Open Questions |
| `F1` | Help panel | Conventional. Needs the reserved-key list amended (F1–F4 are currently earmarked for party-slot selection — shrink to F2–F5, or drop, since `P` now covers the roster) |
| `Space` | Toggle pause | Reserved *for* pause — this is that |
| `-` / `=` | Step the pace ladder slower / faster | Free. Digits stay reserved for control groups |
| `L` | Expand / collapse the activity feed | Free. The mock's `Tab` keeps its cycle-pawn meaning |

All eight are ordinary `UInputAction`s in `IMC_Strategy_Mouse`, per the wiring rule — nothing here
is a widget key handler. **`UInputMappingContext` key mappings must be authored by hand in the
editor** (MCP cannot write them safely), so all eight mappings are created in **one manual pass in
Slice 1**, including the ones Slices 2 and 3 use. That single editor hand-off is the main reason
Slice 1 comes first.

## Panels

### Nav rail (left, top-anchored)

A vertical strip of `UButton`s, each with its key hint and label, driving
`IStrategyHUDCommands::RequestPanel`. Pressing the key and clicking the button run the same path,
so a refusal (inventory with no pawn selected) comes out the same way either way. The active
panel's button reads as pressed.

### Panel windows

`USquadPanelWidget`, `UMapPanelWidget`, `UResearchPanelWidget`, `UHelpPanelWidget` — all
`UWindowWidget` subclasses so they inherit drag, resize and close for free. Three of them are
**stubs with honest placeholder bodies** (one line naming what will live there, and the design
topic that owns it). Two notes:

- **Help is not a stub** — it lists the current keybindings as static text. It's the cheapest
  panel here and the only one that's immediately useful, and it's what a player actually presses
  `F1` for.
- **These windows must not touch `UpdateInventoryInputContext`.** That's the inventory context's
  business. A stub panel that added it would silently give `R` a second meaning.

### Time-pace strip (top centre)

Four buttons — pause, 1×, 2×, 4× — plus a text readout of the actual current tier. `EGamePace`
carries the full ladder the design calls for (pause, 1/3×, 1/2×, 3/4×, 1×, 2×, 4×, 8×); `-`/`=`
step through all of it, the buttons jump to the four common ones, and the readout shows where you
actually are. When the current tier has no button, no button is lit — the readout is the source of
truth, which is honest and costs nothing.

### Resource strip (top right, above the target panel)

**Where gold goes.** A one-row strip that holds the gold balance now and has room for whatever
else becomes an at-a-glance figure later. It sits clear of the target panel so both read at once.

The balance itself doesn't change: `AStrategyHUD` already resolves `UWalletComponent` off the
owning player state and pushes it every frame. Slice 1 moves the `GoldText` binding out of
`UStrategyUI` into this widget and leaves the push path alone.

### Target panel (top right)

Driven by `FStrategyTargetInfo`, rebuilt each frame from whatever the controller last targeted
(`LastSelectionTarget`):

```
FStrategyTargetInfo { FText DisplayName; FText Classification; float DistanceMeters;
                      bool bHasHealth; float HealthFraction; TArray<FTargetAction> Actions; }
FTargetAction        { FName Id; FText Label; FText KeyHint; bool bEnabled; }
```

**The action row is real, not decorative.** It's assembled from what the controller would actually
permit on that target right now — open a container (`O`), talk/trade (`T`), attack (`H`), loot a
downed body — reusing the existing gating helpers (`GetLootableNPC`, `GetInteractableNPC` and
friends) rather than duplicating their rules. Disabled-but-visible beats absent: "Talk" greyed out
because the NPC is hostile teaches the rule; a missing button teaches nothing. The wireframe's
`SCAVENGE` and `TAG FOR SQUAD` are systems that don't exist — they are not built and not faked.

Classification (`STRUCTURE · NEUTRAL · 61m`) comes from what's knowable today: the actor's kind,
its hostility toward the player where a unit has one, and the distance from the nearest controlled
unit. The mock's per-target portrait art doesn't exist; the panel falls back to the same initials
treatment as the squad bar.

### Squad portrait bar (bottom left)

One circular portrait per unit in `GetControlledPlayerUnits()`, in the same deterministic order
`RefreshPlayerPawns()` already establishes so the bar doesn't reshuffle itself. Each shows name,
a health ring from `UHealthComponent` (replicated already), and a selection ring. Left-click
selects; double-click selects **and** focuses the camera — that hard camera cut is exactly what
`player-interface.md` specifies for division switching, arriving early on a single roster.

This needs two new properties on `AStrategyUnit`: `FText DisplayName` and
`TObjectPtr<UTexture2D> PortraitTexture`, both `EditAnywhere` and both authored per Blueprint or
per placed instance. With no texture set the portrait draws the unit's initials on a plain disc,
which is what the wireframe itself does. A later characters pass may well move identity onto its
own component; two properties now is the cheap version that doesn't block that.

### Activity feed (bottom right)

`USmoresActivityLog` (`ULocalPlayerSubsystem`) holds a fixed-capacity ring buffer of
`FActivityEntry { EActivityCategory Category; FText Text; FText Source; double Timestamp;
EActivitySeverity Severity; }` and broadcasts `OnEntryAdded`. The widget renders the last N,
filtered by the selected tab, fades idle entries after 8 seconds, and expands to a scrollable
history on `L`.

Tabs map to categories: **LOG** shows everything, **SQUAD** shows squad-sourced entries (damage
taken, downs, loot, refusals), **COMMS** shows NPC dialogue and trade, **QUESTS** is an empty
stub with a line saying so — `quests-and-objectives.md` has no authored content to feed it, and
`player-interface.md` explicitly rules out a quest tracker.

Producers wired this round, all of which already fire and are currently discarded:

| Event | Where it already happens |
|---|---|
| Damage taken / dealt, downed, death | `UHealthComponent`'s existing delegates |
| Items looted, picked up, moved between holders | `UInventoryComponent` delegates, `AWorldItem` pickup |
| Purchases and sales | `UTraderComponent` / `UWalletComponent` |
| Refusals | the same call that raises the `URefusalWidget` line |

Severity drives colour, matching the mock's red-for-damage treatment. Entries persist until
evicted by the ring buffer — `player-experience.md`'s "notifications persist until dismissed" is
the reason high-speed play is safe, and this is the first half of it.

## Testing

Per `testing.md`'s standing rule, three things here are numbers-and-state-machines and get tests
in the same slice that builds them; everything else is a PIE judgement and stays there.

- **Pace ladder** — stepping clamps at both ends, every `EGamePace` maps to the dilation it
  claims, and `RequestPace` on a non-authority does not mutate local state.
- **Activity log** — the ring buffer evicts oldest-first at capacity, category filtering returns
  the counts it should, and `OnEntryAdded` fires exactly once per post.
- **Target action assembly** — a target with no valid actions yields an empty list rather than a
  list of disabled everything, and each gating helper's refusal produces the disabled-with-reason
  form.

What stays in PIE, permanently: layout at every resolution, whether the feed's 8-second fade is
right, portrait size, whether the pace strip is reachable without looking, and whether any of it
reads at a glance during a fight.

## Implementation Order

Three slices. Every slice follows the same protocol, so it isn't repeated per entry:

1. Read the `game-systems` topics the slice names, this slice's entry, and the source files it
   names.
2. Write the C++ first (CLAUDE.md's "C++ first" rule). Every slice adds new `UCLASS` types, so
   expect a **cold Visual Studio build** — close the editor, build, reopen. Never Live Coding.
   Check the build output for `Compile [x64]` lines; after a system-clock correction UBT can
   report success having compiled nothing.
3. Author the WBPs through `unreal-mcp`'s `UMGToolSet` (`mcp-workflow` skill — read its UMG
   caveats first: batch scripts must be idempotent, `BindWidgetOptional` rows only appear on a
   compiled WBP, and a duplicate `AddWidget` produces `_1`-suffixed names that silently fail to
   bind).
4. Jim PIE-tests the layout and feel; the agent verifies compile, wiring, property state, and
   runs `Automation RunTests Smores`, checking the **count** of tests run, not just the colour.
5. Commit the slice on its own, then move its shipped content into the `game-systems` skill —
   `hud-and-panels.md` for the panels, `input-and-keybinds.md` for the keys — and mark the slice
   `DONE` below.

### Why three, and not one or eight

The honest reasons, per CLAUDE.md's slicing guidance. **Slice 1 is a real boundary**: it ends in a
manual editor pass (eight `IMC_Strategy_Mouse` key mappings MCP can't write) and in the first
layout Jim can actually look at — and if the frame is wrong, it's wrong before two slices of panel
content are built on top of it. **Slices 2 and 3 are a context-budget split and nothing more**:
five widget classes, a replicated component, a subsystem and an interface change is more build
output and more UMG round-tripping than one clean session should carry. They are otherwise
independent of each other and can swap order freely.

What did *not* justify a slice: "the pace strip is a different system from the target panel" (same
pattern, same frame, typing), "portraits need a cold build" (slow, not blocking), and "check the
rail works before adding the feed" (the check is a PIE glance Jim does at the end of Slice 1
anyway).

### Slice 1 — The frame: root layout, nav rail, stub panels, resource strip

The screen's skeleton, and the one manual editor pass.

- `UStrategyUI` becomes the HUD root: a full-screen canvas with anchored regions for all six
  areas, hosting child widgets rather than holding readouts itself.
- `IStrategyHUDCommands` with `RequestPanel` implemented; the other three members stubbed so
  Slices 2 and 3 only fill them in.
- `UNavRailWidget`, `UHUDPanelWidget` bases; `USquadPanelWidget`, `UMapPanelWidget`,
  `UResearchPanelWidget` as stubs, `UHelpPanelWidget` with its static keybind list.
- `UResourceStripWidget` takes over the gold readout from `UStrategyUI` — the `GoldText` binding
  moves, the `AStrategyHUD` push path does not.
- Eight `IA_Strategy_*` actions created and bound in `SetupInputComponent`; **hand Jim the eight
  key mappings as a single checklist.**
- New `game-systems` topic `hud-and-panels.md`; `input-and-keybinds.md` updated with all eight
  keys and the amended `F1`–`F4` reservation.

**Done when:** every rail button and every panel key opens its window; windows drag, resize and
close; gold still reads correctly after the move; the six regions sit where the wireframe puts
them at 1080p and at ultrawide; nothing on the HUD leaks a click through to the world.

### Slice 2 — Time pace and the target panel

- `EGamePace` + `UTimePaceComponent` in `SmoresCore`; `AStrategyGameState` in `smores` hosting it;
  `Server_RequestPace` on the controller; `UTimePaceWidget` reading the component the way
  `AStrategyHUD::GetWallet()` reads the wallet (retry while null — the GameState replicates in
  late on a client).
- `FStrategyTargetInfo` / `FTargetAction`; `GetSelectionTargetInfo()` replacing
  `GetSelectionTargetLabel()` on `IStrategySelectionHost`; `UTargetPanelWidget` with its live
  action row built from the controller's existing gating helpers.
- Tests: the pace ladder and target-action assembly groups above.

**Done when:** `Space` and the buttons visibly change simulation speed in PIE, the readout matches
what `-`/`=` stepped to, and targeting a chest, an NPC and a downed body each produces the right
name, distance, health bar and action row — with the disabled cases actually disabled.

### Slice 3 — Squad portrait bar and the activity feed

- `DisplayName` + `PortraitTexture` on `AStrategyUnit`; `USquadBarWidget` +
  `USquadPortraitWidget` with health ring, selection ring, click-to-select and
  double-click-to-focus; `GetControlledPlayerUnits()` on `IStrategySelectionHost`;
  `RequestSelectUnit` implemented.
- `USmoresActivityLog` + `UActivityFeedWidget` / `UActivityEntryWidget`, tabs, 8-second fade, `L`
  to expand, and the four producer wirings above.
- Tests: the activity-log ring buffer group above.

**Done when:** the bar shows every squad member with a live health ring, clicking one selects it
and double-clicking snaps the camera to it, and a fight produces a readable after-the-fact record
in the feed — including at least one refusal, which should appear in both the refusal line and the
feed.

## Explicitly Out of Scope for This Round

- **The styling pass.** Layout and sizing are in; the mock's cream palette, mono/handwritten
  fonts, rounded outlines and portrait art are a later pass, deliberately, so layout changes don't
  cost restyling twice.
- **Division switcher.** `player-interface.md` wants divisions with their own tabs and camera
  focus. One roster exists; the portrait bar is the thing divisions will later group.
- **Mini-map and time-of-day widget.** `world-map-and-travel.md` owns the mini-map's full spec;
  the `M` panel is a stub until there's a world map to draw. Neither has a region reserved in the
  mock — worth revisiting when the map ships.
- **Real research, quests and comms content.** Stubs only. `tech-and-crafting.md` and
  `quests-and-objectives.md` aren't built, and a fake tech tree would be worse than an empty
  panel.
- **A keybind settings screen.** Every key here is an Enhanced Input action, which is the
  prerequisite; the settings UI and `PlayerMappableKeySettings` are their own piece of work.
- **Notification dismissal semantics.** The feed remembers; nothing yet decides what *demands*
  acknowledgement. `notifications-and-alerts.md` is a placeholder topic and owns that.
- **World-space contextual prompts.** `player-interface.md` wants loot/talk prompts near the
  object in the world rather than on the frame. Different layer, different round.
- **Tooltips.** Nothing here has an explanatory hover. The `RefusalWidget` header already notes
  that a tooltip escaping its panel needs its own top-most layer when it arrives.

## Resolved Design Decisions

Settled with Jim before this document was written — don't reopen them per-slice.

1. **The pace controls are real, not decorative.** They change the simulation from day one. A HUD
   whose most prominent control does nothing teaches the player the HUD is a picture.
2. **No existing keybind moves.** The wireframe's `Q`/`R`/`Tab`/`1`-`2`-`3` all collide with live
   or reserved bindings; free keys were chosen instead and the rail's labels follow.
3. **Gold goes in a top-right resource strip above the target panel** — a region that holds one
   figure now and more later, rather than a number wedged into a list of buttons.
4. **Layout now, styling later.** Plain readable UMG defaults, correct positions and sizes, so the
   layout can be judged in PIE before anything is worth restyling.

## Open Questions Worth Tracking

- **`U` for Research is the weakest key of the eight.** No PC-RPG convention backs it. `R` is the
  natural one and is only unavailable because `R` rotates a dragged item in the inventory context
  — a different context, so it's legal, just against this project's own "prefer not to" rule.
  Worth revisiting if camera height ever leaves `Q`/`E`.
- **Who may change the pace in co-op?** Built as "any player may". The alternatives (host only;
  slowest request wins) are one `if` away and want a real co-op session to judge.
- **Does the target panel survive deselection?** Today's label clears with the target. A panel
  that lingers on the last thing looked at may read better than one that blinks out — a PIE call.
- **Feed capacity and fade timing.** 8 seconds and a fixed ring buffer are the mock's numbers;
  whether either is right is a thing to feel, not to reason about.
- **Portrait bar at eight-plus squad members.** The mock shows nine and stops. Scroll, shrink, or
  wrap is a layout decision nobody has to make until a squad gets that big.
