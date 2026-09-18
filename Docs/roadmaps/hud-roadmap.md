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

**This roadmap is complete: all three slices have shipped.** What it is still good for is the
Resolved Design Decisions log and the Open Questions — the *why* behind calls that are now code.
For what the HUD does and how, read `game-systems`' `hud-and-panels.md`, which is authoritative.

Everything above the Implementation Order below is therefore a record of the plan as written, kept
because the reasoning is still useful and the wireframe reading is still the clearest statement of
intent. Where the plan and the shipped code differ, each slice's "What shipped differently" notes
say so and the skill is right.

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
| `F1` | Help panel | Conventional. **Done** — the F1–F4 party-slot reservation was dropped outright (F2–F5 would have collided with quicksave) |
| `Space` | Toggle pause | Reserved *for* pause — this is that |
| `-` / `=` | Step the pace ladder slower / faster | Free. Digits stay reserved for control groups |
| `L` | Expand / collapse the activity feed | Free. The mock's `Tab` keeps its cycle-pawn meaning |

All eight are ordinary `UInputAction`s in `IMC_Strategy_Mouse`, per the wiring rule — nothing here
is a widget key handler. **`UInputMappingContext` key mappings must be authored by hand in the
editor** (MCP cannot write them safely), so all eight mappings are created in **one manual pass in
Slice 1**, including the ones Slices 2 and 3 use. That single editor hand-off is the main reason
Slice 1 comes first.

> **Status:** all eight `UInputAction` assets exist, are mapped in `IMC_Strategy_Mouse`, are bound
> in `SetupInputComponent`, and **all eight now do something** — `L` was the last, in Slice 3. The
> live bindings are recorded in `game-systems`' `input-and-keybinds.md`, which is authoritative —
> this table is the plan. The mapped-ahead-of-the-feature pattern (log a line naming the slice,
> treat the key as taken, keep it out of the help panel) is written up there too.

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
filtered by the selected tab, and expands to a longer history on `L`. (Shipped with the fade the
mock called for, which was then removed — see the shipped-differently notes below.)

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

- **Pace ladder** — **DONE (Slice 2, 6 tests).** `Smores.Core.TimePace.*`: stepping clamps at both
  ends, every `EGamePace` maps to the dilation it claims, the ladder holds every tier the enum
  declares, and `SetPace` on a non-authority does not mutate local state.
- **Activity log** — **DONE (Slice 3, 6 tests).** `Smores.Core.ActivityLog.*`: the ring buffer
  evicts oldest-first at capacity, shrinking the capacity evicts immediately rather than on the
  next post, category filtering returns the right counts *and* keeps its order, `OnEntryAdded`
  fires exactly once per post and after eviction has settled, and ids never restart. The
  `ClassWithin` trap that made one of the six fail while the other five passed on the same mistake
  is in `testing.md`.
- **Target action assembly** — **DONE (Slice 2, 7 tests).** `Smores.Strategy.TargetInfo.*`: a
  target with no valid actions yields an empty list rather than a list of disabled everything, and
  each gating helper's refusal produces the disabled-with-reason form. Made possible by
  `BuildTargetInfo` being a static taking the selection as a parameter — see Slice 2's notes.

What stays in PIE, permanently: layout at every resolution, portrait size, whether the pace strip
is reachable without looking, and whether any of it reads at a glance during a fight. (The feed's
fade was one of these; PIE answered it — it went.)

**What PIE actually caught, across the three slices**, which is the argument for that list being
real rather than a hedge: two regions shipped as bare `UBorder`s and leaked right-clicks to the
world (Slice 1); every region shipped white-on-white (Slice 1); and the portrait camera focus
arrived in the right place looking out over the pawn (Slice 3). None of the three is expressible as
a number, and all three were obvious within seconds of looking.

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

### Slice 1 — The frame: root layout, nav rail, stub panels, resource strip — SHIPPED

Built and documented in `game-systems`' `hud-and-panels.md` and `input-and-keybinds.md`; read
those, not this, for how any of it works. `UStrategyUI` is the region host, the nav rail opens
four panel windows, gold lives in `UResourceStripWidget`, and the eight input actions exist and
are bound.

**The eight `IMC_Strategy_Mouse` key mappings are authored** (`P`, `M`, `U`, `F1`, `Space`, `-`,
`=`, `L`) — Jim did them by hand, since MCP cannot write IMC mappings safely (see `mcp-workflow`).
That manual pass was the reason this slice came first, and it is done.

**Jim's first PIE pass found two bugs, both fixed** — see items 3 and 3b below. Neither was in the
C++ logic; both were in how the regions were authored in UMG.

**Still to judge in PIE:** whether the six regions sit right at 1080p and at ultrawide, and a
re-check that nothing else on the HUD leaks a click.

#### What shipped differently from the plan above

1. **`RequestPace` was not on `IStrategyHUDCommands` yet.** Stubbing it needed `EGamePace`, and
   creating that enum here would have left a `SmoresCore` type nothing read, so Slice 2 added the
   enum, the component and the interface member together — **done**. The three pace keys and the
   feed key were bound here, each logging the slice that implements it, so the mappings could be
   verified in the one editor pass.
2. **`UHUDRegionWidget` was added**, unplanned: the base every HUD region derives from, carrying
   the click shield and the controller lookup. Slices 2 and 3's four regions inherit it rather
   than re-solving the clickable-HUD trap each time.
3. **The four unbuilt regions are labelled placeholder boxes in `UI_Strategy`**
   (`TimePaceRegion`, `TargetPanelRegion`, `SquadBarRegion`, `ActivityFeedRegion`), so the whole
   frame can be judged now. **Each of Slices 2 and 3 replaces its own placeholders** with the real
   widget and a `BindWidgetOptional` property on `UStrategyUI` — use
   `UMGToolSet.ReplaceWidgetWithTemplate`, which keeps the name, slot and anchors.

   They shipped first as plain `UBorder`s, which **was a bug Jim caught in PIE**: a `UBorder`
   doesn't consume the mouse press, so a right-click over the bottom-right box issued a move order
   to the selected squad. They are now `UHUDPlaceholderRegionWidget` instances. The rule this is an
   instance of is in `hud-and-panels.md`: *anything on the HUD that reads as a panel must be a
   `UHUDRegionWidget`, placeholder or not.* The clickable-HUD trap section above called this out
   and it still got missed — the trap is real.
3b. **Contrast had to be set after all.** Every region shipped invisible: UMG's default `UBorder`
   brush is white and its default `UTextBlock` colour is white, so the gold readout, the region
   labels and the help panel's keybind list were all white-on-white. Backgrounds are now dark
   translucent and text near-white. This does **not** reopen Resolved Decision #4 — it is the
   legibility floor a layout has to clear to be judged at all, and the styling pass still replaces
   it wholesale.
4. **`UHelpPanelWidget` lists only keys that do something.** `Space`, `-`, `=` and `L` are
   deliberately absent from it. **Slices 2 and 3 must add their keys to
   `UHelpPanelWidget::GetDefaultBodyText()` in the same change that makes them work** — a help
   screen naming a dead key is worse than an incomplete one.
5. **`SmoresUI.Build.cs` already has `SmoresCombat`** — done here rather than in Slice 3.
6. **The `F1`–`F4` reservation was dropped, not shrunk to `F2`–`F5`.** `F5` is quicksave, so the
   roadmap's suggestion collided; and party-slot selection now has two better answers (`P` opens
   the roster, and Slice 3's portrait bar selects with one click).
7. **`UI_Strategy` kept its root `Overlay`** with a new full-screen `HUDCanvas` nested inside it,
   rather than having the root replaced — non-destructive to the three surviving bindings
   (`BP_UpdateUnitsCount`, `GetSelectedUnitsCount`, `GetSelectionTargetLabel`). The panel and
   region WBPs use a `Border` root, matching `WBP_Inventory`'s proven chrome.

#### Worth knowing before Slice 2

**Build all seven modules together after adding source files.** An incremental build during this
slice relinked six modules and left `SmoresCharacters.dll` at an earlier vintage; the editor then
crashed on startup with a near-null access violation on the async-loading worker, naming no
project asset, ~10s in at `MAP LOAD`. It looks exactly like the new C++ broke serialization, and
it isn't — `Rebuild.bat` fixed it with no source change. The headless suite passed 73/73 against
those same binaries the whole time, so a green `-nullrhi` run is **not** evidence the editor will
start.

### Slice 2 — Time pace and the target panel — SHIPPED

Built and documented in `game-systems`' `hud-and-panels.md`, `input-and-keybinds.md` and
`testing.md`; read those, not this, for how any of it works. `EGamePace` + `UTimePaceComponent`
live on `AStrategyGameState`, `Space`/`-`/`=` and four buttons drive the ladder,
`GetSelectionTargetInfo()` replaced `GetSelectionTargetLabel()`, and the target panel's action row
is assembled from the controller's own gating helpers.

Tests: **86 green, up from 73** — six for the pace ladder (`Smores.Core.TimePace.*`) and seven for
target-action assembly (`Smores.Strategy.TargetInfo.*`). The `Blueprint` sweep was run too, since
this slice removed a `BlueprintPure` that a Blueprint consumed.

**Still to judge in PIE:** whether the pace tiers feel right, whether the target panel should
survive deselection, and the pace buttons' label contrast (see note 6 below).

#### What shipped differently from the plan above

1. **`UTargetActionWidget` was added**, unplanned. The action row needs a widget per button, and
   the project's established pattern for a dynamic row is a `TSubclassOf` property plus
   `CreateWidget`/`AddChild` (`UInventoryWidget`'s cells, `UEquipmentWidget`'s slots). `ActionBox`
   holds them and the row is resized rather than rebuilt, since it usually keeps its shape.
2. **`FStrategyTargetInfo` gained an explicit `bHasTarget`.** The plan's struct had no validity
   flag, so "is there a target" would have meant "is the name empty" — and a target's name is
   authored data, so an actor nobody got round to naming would have made the whole panel vanish.
   The bug would have read as the panel being broken rather than as a blank field.
3. **`BuildTargetInfo` is a `public static` taking the selection as a parameter**, not an instance
   method reading `ControlledUnits`. That is what makes the roadmap's own "target action
   assembly" test group possible at all — `testing.md` had already catalogued
   `AStrategyPlayerController` as expensive to test, and this is the worked example of the way
   out. `IsHolderInRangeOfUnits` was extracted from `IsHolderInRangeOfSelection` for the same
   reason, so both paths apply one reach rule.
4. **The action row is narrower than the plan implied in two places, both deliberate.** Attack has
   **no range gate** (`DoAttackCommand` sends the squad to close the distance, so "too far to
   attack" isn't a thing this game has), and one of your own squad gets an **empty row** rather
   than disabled buttons — every verb that applies to your own pawn already has a route that isn't
   this panel. The second is the roadmap's own stated test case.
5. **`AStrategyGameState` is `UCLASS(abstract)` with a `BP_StrategyGameState`**, following the
   project's standing class-hierarchy rule even though the class binds no assets. It costs an
   empty Blueprint and one `GameStateClass` entry on `BP_StrategyGameMode`; the alternative
   (concrete C++ set as a constructor default) would not have needed the editor pass, but would
   have been the only gameplay framework class in the project not following the rule. **If that
   property is ever cleared the pace silently does nothing**, which is now recorded in
   `hud-and-panels.md`.
6. **Text inside buttons is near-*black*, not near-white.** `UTimePaceWidget` tints its buttons
   white/gold at runtime, and `UTargetActionWidget` sits on UMG's default light-grey button brush,
   so the Slice 1 legibility floor ("dark panel, light text") inverts inside a button. Worth an
   eyeball in PIE; the styling pass replaces it wholesale either way.
7. **The old selection-target readout was an `Event Tick` chain, not a property binding.**
   Deleting `SelectionTargetBorder`/`SelectionTargetText` left a dead
   `Event Tick → SetText → GetSelectionTargetLabel` chain in `UI_Strategy`'s EventGraph that broke
   the compile. Both this and a second MCP trap found here (a failed `GetWidgets` probe poisoning
   an asset name for the rest of the session) are now in the `mcp-workflow` skill.

#### Worth knowing before Slice 3

**Test-only concrete actors now exist.** `Source/smores/Tests/SmoresStrategyTestActors.h` holds a
spawnable subclass of `AStrategyUnit`, `AStrategyPlayerUnit` and `AStrategyContainer` — the real
ones are all `UCLASS(abstract)` and cannot be spawned at all. Slice 3's portrait bar wants the same
three; add to that file rather than starting a second one.

**Don't drive real AI entry points from a test.** `SetAggressive(true)` fails in both directions:
with no player pawn in the world the unit stands straight back down on its first retarget tick, and
with one it starts a real fight needing a skeletal mesh or a navmesh. `ATestStrategyNPC::
MakeHostileForTest` sets the one piece of state the code under test reads, and says why in a
comment. Recorded in `testing.md`.

### Slice 3 — Squad portrait bar and the activity feed — SHIPPED

Built and documented in `game-systems`' `hud-and-panels.md`, `input-and-keybinds.md`, `testing.md`,
`refusals-and-feedback.md`, `strategy-camera-and-selection.md`, `combat.md` and
`multiplayer-discipline.md`; read those, not this, for how any of it works. `USmoresActivityLog` is
a `ULocalPlayerSubsystem` in `SmoresCore`, the squad bar subsumed the old selection readout,
`GetControlledPlayerUnits()` joined `IStrategySelectionHost`, `RequestSelectUnit` is live, and `L`
expands the feed.

Tests: **92 green, up from 86** — six for the activity log's ring buffer
(`Smores.Core.ActivityLog.*`). The `Blueprint` sweep was run too (442 green), since this slice
deleted widgets and an entire EventGraph from `UI_Strategy`.

**With this, the HUD roadmap is complete.** All six regions of the wireframe are real and
`UI_Strategy` holds no placeholder boxes. What remains below is the resolved-decisions log and the
open questions, which are the only reasons to open this file again.

#### What shipped differently from the plan above

1. **`DisplayName` already existed.** The plan called for `DisplayName` *and* `PortraitTexture` on
   `AStrategyUnit`; the name has been there since the target panel as `UnitDisplayName`, exposed
   through `GetHolderDisplayName()`. Only the texture was added, and the bar reuses the existing
   name — a second name property would eventually disagree with the one the target panel draws.
2. **`USquadActivityWatcher` was added**, unplanned, and it is the slice's one real design
   decision. The plan assumed the feed could subscribe to `UHealthComponent`'s delegates; it can't
   usefully, because three of the four are parameterless, so a handler is told that *somebody* went
   down with no way to find out who. One watcher object per unit supplies the missing parameter.
   It lives in `smores` next to the controller: it is variant glue wiring existing delegates into
   the local player's feed, and holds no state anyone else needs, so CLAUDE.md's
   "component in a feature module" rule doesn't apply to it.
3. **A non-squad unit's down or death is gated on the player's squad having hurt it first**
   (`bHurtByPlayerSquad`). Consequence of note 2: with no instigator on those two delegates, the
   only honest alternatives were reporting every NPC that falls over anywhere in the world, or
   reporting none of them. `OnDamaged` does carry an instigator, and nothing dies without being
   damaged, so remembering one bit answers both events.
4. **`OnEntryAdded` is a plain multicast delegate, not a dynamic one.** An `FActivityEntry`
   payload has no handler on `USmoresTestDelegateListener`, and giving the delegate an `int32`
   payload merely to reuse that listener would have been the tail wagging the dog. A widget binds
   with `AddUObject`, a test with `AddLambda`.
5. **The feed's producers are not the four the plan listed.** `UInventoryComponent`'s
   `OnInventoryChanged` carries no payload, so it cannot say *what* moved and would have produced
   "something changed" lines. The real producers are the controller's own code paths, which know
   the item, the quantity and the price: `Server_PickUpWorldItem`, `TryTradeItem` and
   `NotifyRefusal`. Server-side ones route through a new `Client_NotifyActivity`, the counterpart
   to `Client_NotifyRefusal`.
6. **`NotifyRefusal` needed its own repeat suppression for the feed**, separate from
   `URefusalWidget`'s. The widget collapses repeats so the sound doesn't machine-gun; the feed
   collapses them because it is a fixed-capacity record and a held key would push everything else
   out of it. That is the one way this feed can actively lose information.
7. **The feed resizes its own canvas slot when expanded**, which the plan didn't anticipate. The
   authored slot fits the collapsed six lines, so the history would have drawn outside its own
   background — broken-looking rather than layout-wanting-tuning. It reads the slot's alignment
   rather than assuming it, so the box grows *upward*: the feed is bottom-anchored, and a taller
   box keeping its top edge would push its newest lines off the viewport. **It still does not
   scroll** — see `hud-and-panels.md`'s Known Gaps.
8. **The portrait's focus gesture is timed, not a Slate double-click.** `SButton` routes
   `OnMouseButtonDoubleClick` into its press handler, so a real `UButton` reports two ordinary
   clicks rather than a double-click. Keeping the button is what stops the gesture reaching the
   world, so the widget measures the gap between two `OnClicked` calls itself. The consequence to
   design around: the first click's action has already happened, so the second's must be additive.
9. **`FocusCameraOnUnit` shipped wrong and Jim caught it in PIE**, and the geometry is worth
   knowing. The camera pawn's *root* is what sits at screen centre, and it has to stay at camera
   height because of the movement-plane constraint — so moving it to the unit's X and Y puts
   screen centre on a point in mid-air above the unit, and the camera arrives in the right place
   looking out over the pawn. The fix solves for how far along the look direction the unit's height
   lies. Recorded in `strategy-camera-and-selection.md`, because the mini-map and any future "jump
   there" will hit the identical trap.
10. **The `UI_Strategy` EventGraph trap was live for a second slice running.** The old
    `SelectionCount` readout was driven by two node chains (`Event Construct` and
    `Event Update Units Count`), not a property binding; deleting the widget without them fails the
    compile naming the function but not its use site. All eight nodes were deleted and the graph is
    now empty, which is correct — `GetSelectedUnitsCount` and `BP_UpdateUnitsCount` remain valid
    C++ for a future WBP.
11. **The feed's collapsed height had to be captured at first toggle, not on construct.** Read in
    `NativeConstruct` a `UCanvasPanelSlot`'s size comes back as zero, so collapsing restored a
    zero-height box and the feed toggled between tall and gone. Jim's second PIE pass caught it.
    Recorded in `hud-and-panels.md` as a rule about laid-out numbers generally, not as a note
    about this widget.
12. **A test failure that was an engine rule, and a lesson about green runs.**
    `ULocalPlayerSubsystem` declares `ClassWithin = ULocalPlayer`, and `ULocalPlayer` declares
    `ClassWithin = UEngine`; both are enforced at construction. Building the log over the transient
    package raised a handled ensure — which fires **once per call site per session**, so exactly
    one of the six new tests failed and the other five passed on the identical mistake. Recorded in
    `testing.md`: a green run is not evidence the outer is right.
13. **The fade shipped, then came straight back out.** The mock's "fades after 8s" was built as
    specified and removed after Jim's first PIE pass, on his call. The reasoning belongs in
    `hud-and-panels.md` rather than here, but the short version is worth reading before anyone
    re-adds it from the wireframe: floating damage numbers fade because they are events, and the
    feed is a record. A record that has emptied itself by the time the player looks up from the
    fight is not a record. `FActivityEntry::Timestamp` survives the removal unread, deliberately.

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
- **Does the target panel survive deselection?** The panel now collapses with the target. A panel
  that lingers on the last thing looked at may read better than one that blinks out — a PIE call.
  One line in `UTargetPanelWidget::RefreshTargetDisplay` either way.
- **Should the pace be part of a save?** `UTimePaceComponent::BeginPlay` applies whatever tier the
  component holds, so a restored pace would take effect with no extra code — but whether a save
  should restore "paused" at all is `save-system.md`'s call, not this roadmap's.
- **Is hostility worth replicating now or with the first co-op session?** The target panel reads
  `IsAggressive()` client-side and `Disposition` isn't replicated, so the classification and the
  Talk/Attack enable states would be wrong on a remote client. Display-only, invisible until co-op,
  and a one-line fix — catalogued in `game-systems`' `combat.md`.
- ~~**Feed fade timing.**~~ Answered in PIE: the fade is gone entirely. Lines now leave only when
  newer ones push them off. Reasoning is in `game-systems`' `hud-and-panels.md` so it survives this
  roadmap — the short version is that a record which empties itself is not a record, and a quieter
  corner should dim old lines rather than remove them.
- **Feed capacity.** The 64-entry ring buffer is the mock's number; whether it is right is a thing
  to feel, not to reason about. `EditAnywhere`.
- **Portrait bar at eight-plus squad members.** The mock shows nine and stops; roughly seven tiles
  fit the authored 560x120 slot. Scroll, shrink, or wrap is a layout decision nobody has to make
  until a squad gets that big.
- **Should the expanded feed scroll?** It grows its slot and shows 20 of the 64 entries held.
  Reaching the rest is a `UScrollBox` around `EntryBox` in the WBP and **no C++ change at all** —
  `EntryBox` is typed as a `UPanelWidget` precisely so that swap costs nothing. Whether 20 lines is
  already more history than anyone reads is the actual question.
- **Should a portrait click be additive with `Shift`?** The world's selection modifier means
  "add to / remove from"; a portrait ignores it and always replaces the selection. Consistent with
  the `Tab` cycle, which also replaces — but `Shift`-clicking three portraits to build a fire team
  is the obvious thing a player will try. One `IsShiftDown()` away, and worth a real squad to judge.
- **Is `ExpandedHeight` (460px) the right size, or should the feed fill the screen height?** A
  record you open to read after a fight may want to be much bigger than a slightly taller corner.
  A PIE call, and one property.
