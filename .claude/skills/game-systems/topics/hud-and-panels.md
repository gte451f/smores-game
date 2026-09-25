# HUD and Panels

## Purpose

The always-on heads-up display and the floating panels the player opens from it: what each
region of the screen is, who owns it in C++, and the rules that keep a click on the HUD from
also ordering the squad somewhere.

This is the permanent record, and for the HUD it is now the **whole** record: the three-slice plan
that built it shipped in full and the roadmap has been retired, so nothing about the HUD lives
outside this topic and `input-and-keybinds.md`. Every deferred decision the roadmap tracked is in
this topic's Known Gaps with its reasoning; the wireframe it was built against is kept at
`Docs/reference/Squad HUD Wireframes v3-selection.png`.

> **Status: the HUD roadmap has shipped in full.** All six regions are real — the nav rail, the
> resource strip, the time-pace strip, the target panel, the squad portrait bar and the activity
> feed. `UI_Strategy` holds no placeholder boxes any more, and
> `UHUDPlaceholderRegionWidget`/`WBP_HUDPlaceholderRegion` survive only as the tool for the next
> region that arrives before its contents do.

## Player Surface

### The frame

Six regions, positioned to the wireframe at
`Docs/reference/Squad HUD Wireframes v3-selection.png` (committed there because the styling pass
still owes it a visit, and this topic is the only thing that cites it now):

| Region | Where | Today |
|---|---|---|
| Nav rail | Left, top-anchored | **Live.** Five buttons: Squad, Inventory, Map, Research, Help |
| Time pace | Top centre | **Live.** Pause / 1× / 2× / 4× buttons and a readout of the tier actually running |
| Resource strip | Top right | **Live.** Gold balance |
| Target panel | Top right, below the strip | **Live.** Name, what it is, how far away, health, and what you may do to it |
| Squad portraits | Bottom left | **Live.** One portrait per squad member, with health, a selection ring, and the "N selected" count |
| Activity feed | Bottom right | **Live.** Tabbed record of what just happened; nothing fades, expandable |

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

### Squad portrait bar

One portrait per member of your own squad, along the bottom left, in the same order `Tab` cycles
through them. Each shows the unit's name, a health bar, and a ring when it is selected. Under them
sits the "3 selected" count, which is blank rather than "0 selected" when nothing is.

**Click a portrait to select that unit alone; click it twice quickly to also cut the camera to it.**
The first click selects immediately either way — a portrait never waits to see what you do next —
so the camera cut is purely additive to a selection that has already happened. The cut keeps your
camera height and rotation: it moves you over the unit, it doesn't reset your framing.

No unit has a portrait picture today, so every tile draws the unit's initials on a plain disc.
That is the designed fallback, and it is what the wireframe itself shows.

### Activity feed

The bottom-right record of what just happened. Four tabs:

| Tab | Shows |
|---|---|
| LOG | Everything, in the order it happened |
| SQUAD | Your squad's own news — damage taken and dealt, downs, deaths, pickups, refusals |
| COMMS | Talking and trading - including every bark your squad is close enough to hear (`dialog.md`) |
| QUESTS | Nothing, and says so: *"Objectives will appear here once quests exist."* |

Lines are coloured by how they went — red for squad damage, green for a hit landed or a squad
member back on their feet, amber for a refusal. **Nothing fades or times out.** A line stays put
at full brightness until newer news pushes it off the bottom, so the feed always answers "what
just happened" no matter how long the player took to look. `L` expands it: the box grows upward
and shows more lines instead of the last few. That is all the key does, and all it needs to do.

Severity colours the feed only. **The floating damage numbers in the world are a separate system
and are all red**, whoever took the hit (`UDamageNumberWidget`, `SmoresCombat`), so the feed is the
only place a fight is colour-coded by which side it went for.

**A refused action does two things now.** The line at the cursor answers it immediately, and the
feed remembers it for the player who was looking somewhere else. Repeats of the same refusal
inside two seconds count as one event, so leaning on a key can't push everything else out of the
record.

What is deliberately *not* in it: an NPC fight you had no part in. Going down and dying carry no
"who did it", so the feed only reports a non-squad unit's down or death once your own squad has
hurt that unit — see Core Rules. **Barks are the exception, and a deliberate one**: what somebody
*says* is heard by whoever is within 20 m, whether or not they were in the fight, so a bandit's
"Finish it, then." reaches a squad standing nearby. Barks arrive in quotes, with the speaker as the
line's source.

### Bark bubbles

What people say also floats briefly over their heads - a dark translucent box with light text that
follows the speaker, fades after a few seconds, and stacks with its neighbours rather than
overdrawing them. It is a full-screen layer painted **beneath** the six regions, and it never takes
a click. The rules (one per speaker, how long a line stays, stacking) belong to dialog and are in
`dialog.md`'s "Bark bubbles"; the feed still records every line.

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
- **The bark bubble layer is the one deliberate exception: it must never take a click.**
  `UBarkBubbleLayerWidget` is a plain `UUserWidget`, not a region, and it and every bubble on it are
  `HitTestInvisible`. The rule above exists so that *panels* eat clicks; a bubble floats over the
  world, next to the unit that said it, and a click there means that unit - select it, target it,
  right-click to walk to it. A bubble that ate the click would put a dead spot over exactly the
  person the player is looking at. So the test is not "is it drawn on the HUD" but "does it read as
  a panel": anything that does is a region and consumes; anything that floats over the world is
  hit-test invisible and consumes nothing. The layer is painted beneath the regions, so a region
  still shields its own rectangle even with a bubble behind it.
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
    deliberately provisional. Host-only and slowest-request-wins are each one `if` in `SetPace`
    away, and want a real co-op session to judge.
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
- **A portrait's second click is not a Slate double-click, and must not be.** Slate's `SButton`
  turns the second click of a rapid pair into another ordinary press
  (`OnMouseButtonDoubleClick` routes into `OnMouseButtonDown`), so what a `UButton`'s `OnClicked`
  handler sees is two clicks and the gap between them. `USquadPortraitWidget` therefore measures
  that gap itself (`DoubleClickSeconds`, 0.5s to match `IA_Strategy_SelectAllDoubleClick` and
  Windows' own double-click speed) rather than overriding
  `NativeOnMouseButtonDoubleClick`. **Keeping the real `UButton` is the point**: it consumes its
  own press, so a portrait click can never fall through to the world, and the alternative — a
  non-button root with two overrides — would be re-solving the clickable-HUD trap for a third
  time. Three fast clicks read as select, focus, select: the timer resets on a focus rather than
  counting on, so a camera cut can't fire on every click after the first.
- **Focusing the camera on a unit means solving for the root, not moving it to the unit's X and Y.**
  `AStrategyPawn` keeps its camera exactly `DollyDistance` behind its *root* along the camera's own
  look direction, so **the root is the point at the centre of the screen** — and zoom only slides
  the camera along that same ray, so nothing here depends on the zoom level. The root also has to
  stay up at camera height, because of the movement plane constraint in `AStrategyPawn::SetHeight`.
  Put the root at the unit's X and Y and screen centre therefore lands on a point in mid-air
  *above* the unit, with the ground beneath it well below and behind that. **That is how this
  first shipped, and Jim caught it in PIE**: the camera arrives in the right place while looking
  out over the pawn, which reads as the focus having missed rather than as a framing error.
  `FocusCameraOnUnit` instead asks how far along the look direction the unit's height lies and
  puts the root that far back from the unit. Height and rotation are deliberately left alone — the
  player set those, and a focus that reset the framing would cost them it on every portrait click.
- **A `UCanvasPanelSlot`'s size is not reliable in `NativeConstruct`; capture layout numbers at
  first use instead.** `UActivityFeedWidget` needs to remember its authored height so collapsing
  can restore it. Read in `NativeConstruct`, that came back as **zero** — so collapsing restored a
  zero-height box, and the feed toggled between tall and gone, never returning to the size the
  designer laid out. **Jim caught it in PIE**, and the symptom is worth recognising because it
  reads as a maths error in the resize rather than as a bad reading: the expand is correct and
  only the return trip is wrong. Capturing on the first toggle fixes it — by the time a player can
  press the key the layout certainly exists, and `ToggleExpanded` flips `bExpanded` *before*
  touching the slot, so the value read there is still the authored one. Anything else on this HUD
  that wants to remember a laid-out number should do the same, and should refuse to act on a
  non-positive reading rather than applying it.
- **The squad bar owns the selection count, and that is a bug fix, not tidying.** The "N selected"
  readout used to be a bare `UBorder` loose on `HUDCanvas`, which is exactly the shape the
  everything-must-be-a-`UHUDRegionWidget` rule above exists to stop — a right-click on it issued a
  move order to whatever was behind it. Folding it into the bar fixes that by construction, and
  the count belongs beside the portraits anyway, being a fact about the same thing they draw.
- **The roster is rebuilt on a real-time interval, not per frame.** `GetControlledPlayerUnits()`
  is asked every frame and answering honestly means `GetAllActorsOfClass` over the level plus a
  sort, so it runs at most every `RosterRefreshIntervalSeconds` (0.5s). **Real time, not world
  time**: at the paused tier world time runs at 1/10,000 speed, and a roster keyed to world
  seconds would be frozen for as long as the game was. A squad gains or loses a member rarely
  enough that half a second of staleness is invisible; a *destroyed* unit drops out immediately
  regardless, because the list is validity-filtered on every call.
- **Portrait order is the Tab cycle's order, and must stay that way.** Both come from
  `RefreshPlayerPawns`' deterministic sort. A bar that re-sorted itself as units moved or died
  would slide a portrait out from under the player's finger mid-click.
- **The activity feed is per-local-player client-side state, and a `ULocalPlayerSubsystem` is how
  that is enforced rather than merely intended.** Which lines a player has seen is nobody else's
  business, and in co-op two players' feeds legitimately differ. A local player subsystem is keyed
  to a local player *by construction*, so there is no way to write the singleton-player bug into
  it — see `multiplayer-discipline.md`. Nothing in `USmoresActivityLog` is replicated or
  authority-gated, because nothing in it is shared state.
  - **Server-side news reaches it through a client RPC, never by reading server state.**
    `AStrategyPlayerController::Client_NotifyActivity` is the counterpart to
    `Client_NotifyRefusal` and exists for the same reason: a purchase and a pickup are both
    resolved on the server, so the client has no way of knowing what actually moved or what it
    cost. The text is worded server-side because that is where the numbers are; `FText`
    replicates.
  - **`PostActivity` does nothing without a local player**, which is the correct behaviour on a
    dedicated server and for any other player's controller — so callers don't check, exactly as
    they don't for `NotifyRefusal`.
- **The feed stores already-worded text, which is the opposite of a refusal, deliberately.** A
  refusal is one of six fixed sentences and so wants one place to word them; a feed line names an
  item, a quantity, a person and a price, so there is no finite vocabulary to centralise. What
  *does* arrive as an `ESmoresRefusalReason` is still worded by `URefusalWidget::GetRefusalText`
  before it is posted, so the line in the feed and the line at the cursor cannot disagree.
- **Nothing leaves on a timer — only pushed out by newer news.** No fade, no timeout, no
  auto-dismiss: a line is displaced by the next line or it stays. That is what makes
  `player-experience.md`'s "notifications persist until dismissed" true rather than
  approximately true, and it is why the feed can be trusted after a fight instead of only during
  one. The one thing that can genuinely lose information is a flood, which is why a repeated
  refusal counts once per `FeedRefusalRepeatSeconds` (2s, matching how long the refusal line
  stays on screen: while the same refusal is still showing, it is still the same refusal).
- **The feed used to fade lines out after 8 seconds, and that was removed on purpose.** It is
  worth knowing because it will look like an obvious feature to add back. The floating damage
  numbers in the world fade because they are *events* — they say "-12" and nothing is lost when
  they go. The feed is a *record*, and a record that has emptied itself by the time the player
  looks up from the fight is not a record. If the corner ever needs to be quieter when nothing is
  happening, **dim old lines rather than removing them** — keep them readable.
- **`FActivityEntry::Timestamp` is wall-clock, and nothing reads it today.** It is
  `FPlatformTime::Seconds()` rather than world time, and it is kept even though the fade that
  used to consume it is gone, because when it happened is part of what an entry *is* and is the
  first thing any future display of the record will want. Whatever reads it next should stay on
  wall-clock for the original reason: a pause must not freeze it and 8× speed must not run it
  eight times fast.
- **A non-squad unit's down or death only reaches the feed once your squad has hurt it.**
  `OnDowned` and `OnDied` are parameterless, so they cannot say who was responsible. Without a
  gate the only honest options would be reporting every NPC that falls over anywhere in the world
  or reporting none of them. `OnDamaged` *does* carry an instigator, and nothing dies without
  being damaged first, so `USquadActivityWatcher::bHurtByPlayerSquad` remembers that one bit and
  answers the other two events with it.
- **One watcher object per unit, which looks like overkill and is the only thing that works.**
  `UHealthComponent`'s delegates are dynamic, so a handler must be a `UFUNCTION` on a `UObject` —
  no lambdas, no payload binding — and three of the four carry no parameters at all. A single
  handler on the player controller would be told that *somebody* went down with no way to find
  out who. `USquadActivityWatcher` supplies the missing parameter by being the only thing it is
  for.
- **Every per-frame push is guarded at the region that receives it.** `DrawHUD` pushes gold, the
  pace, the selection count, the whole target struct and the squad roster every frame, and calls
  the nav rail's and the feed's refresh — and each region compares what it was handed against what
  it is already showing before touching Slate. **They compare what would be *drawn*, not the raw
  values**, because otherwise a stationary squad invalidates layout sixty times a second over
  sub-millimetre jitter: the target panel rounds distance to the nearest metre and health to the
  nearest percent, and a portrait does the same with health. The squad bar splits the comparison
  in two — it checks the roster (which changes only when a unit joins, dies or streams out) and
  leaves per-unit state to each portrait, because a portrait is the only widget that knows what it
  is currently drawing.
  - **The feed reacts to a delegate but still rebuilds on the frame push, and that is not
    redundant.** The lines change only when something is posted, so the widget subscribes to
    `USmoresActivityLog::OnEntryAdded` — but the handler merely marks itself dirty and returns.
    Several things can post inside one frame (a fight resolving, a trade), and deferring to the
    frame's push is what turns those into one rebuild instead of one each. Anything else on this
    HUD that is told about changes one at a time but draws them together wants the same shape.
  - **A line is compared by id, never by its words.** `FActivityEntry::Id` is monotonic and never
    reused, including past an eviction or a clear. Two lines with identical text are still
    different events, and the same event is never worth redrawing — comparing `FText` would get
    both of those backwards.
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
- **`AStrategyHUD::ShowBarkBubble`** - the player controller's route in for a bark bubble, forwarded
  through `UStrategyUI::ShowBarkBubble` to the layer. The same controller-to-HUD direction as
  `ToggleActivityFeed`, so no interface. `DrawHUD` pushes `RefreshBarkBubbles` every frame.

### The bark bubble layer

- **`UBarkBubbleLayerWidget`** - bound on `UStrategyUI` as `BarkBubbleLayer` (`BindWidgetOptional`,
  like the regions). A full-screen canvas (`BubbleCanvas`) holding a pool of `UBarkBubbleWidget`s
  created from `BubbleWidgetClass`. **Not a `UHUDRegionWidget`** - see Core Rules for why it is the
  exception. Every frame it projects each speaker's head through the owning player's view and
  places their bubble there; the rules it applies (`FBarkBubbleSchedule`, `StackBoxes`) are plain
  helpers in `BarkBubbleSchedule.h`, tested, and described in `dialog.md`.
- **A HUD layer rather than a world-space actor like `ADamageNumberActor`**, for three reasons:
  it is per local player by construction (only the players who heard a line see it, each in their
  own language); one bubble per speaker and stacking need every bubble in one place; and it needs
  no widget component wired onto every unit Blueprint. The damage numbers stay as they are.
- **It follows the per-frame compare rule** - position and opacity are compared before touching
  Slate, and a bubble only re-lays itself out when it gets a new line.
- **`UBarkBubbleWidget`** - one bubble, spawned from `BubbleWidgetClass`: `LineText` and the wrap
  width. Like `UActivityEntryWidget` it is not a region and not a control.

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
- **`USquadBarWidget`** — the portraits and the selection count. Holds them in `PortraitWidgets`
  and resizes that row rather than rebuilding it (same pattern as the target panel's action row and
  `UInventoryWidget`'s cells: a `TSubclassOf` property plus `CreateWidget`/`AddChild`). It compares
  the *roster* and leaves per-unit state to each portrait — see Core Rules.
- **`USquadPortraitWidget`** — one tile, spawned from `PortraitWidgetClass`. Deliberately **not** a
  `UHUDRegionWidget`: it lives inside one, exactly like `UTargetActionWidget`, and its real
  `UButton` consumes its own press. It owns the click/focus timing rule and the initials fallback
  (`GetInitials` takes the first letter of up to two whitespace-separated words, so "Pawn 1" reads
  as "P1" — which is what the wireframe's placeholder discs show). `SelectionRing` is set `Hidden`
  rather than `Collapsed`, so its absence can't change the tile's size and slide the whole bar
  sideways on every selection.
- **`UActivityFeedWidget`** — the tabs and the visible lines. Reads `USmoresActivityLog`,
  subscribes to `OnEntryAdded` to mark itself dirty, and does the rebuild on the HUD's per-frame
  push (see Core Rules for why it is worth the round trip). `NativeDestruct` unbinds:
  the subsystem outlives every widget bound to it, so an unbound delegate here is a dangling
  handler on the next map load rather than a leak that gets collected. `ApplyExpandedHeight`
  resizes its own `UCanvasPanelSlot` on expand, reading the slot's alignment rather than assuming
  it so the box grows *upward* — the feed is bottom-anchored, and a taller box that kept its top
  edge would push its newest lines off the bottom of the viewport. `CollapsedHeight` restores the
  authored layout rather than a number guessed in C++, and is **captured on the first toggle,
  not in `NativeConstruct`** — see Core Rules.
- **`UActivityEntryWidget`** — one line, spawned from `EntryWidgetClass`. Not a
  `UHUDRegionWidget` and not clickable — a feed line is a record, not a control — so it needs
  neither a shield nor a button. Severity colour is the one piece of the wireframe's styling that
  is *not* deferred, because a feed whose lines all read the same is a feed nobody scans after a
  fight, which is the only time it is worth having.
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
- **`USmoresActivityLog`** (`SmoresCore`) — a `ULocalPlayerSubsystem` holding the ring buffer,
  `Post`, `GetEntries()` / `GetEntries(Category)`, `SetCapacity` and `OnEntryAdded`. In
  `SmoresCore` because *everything* has to be able to post to it — combat, items, economy and the
  controller all have something to say, and it is the one module all of them can see. `Get(const
  APlayerController*)` is the front door: it takes a controller rather than a world, so there is no
  "the" player to get wrong, and a remote controller correctly gets nothing back. `OnEntryAdded` is
  a **plain (non-dynamic) multicast delegate**, so a widget binds with `AddUObject` and a test with
  `AddLambda` — giving it an `int32` payload merely to reuse `USmoresTestDelegateListener` would
  have been the tail wagging the dog. Eviction is `RemoveAt(0, N)` rather than circular-index
  bookkeeping: at a capacity in the tens that is one memmove on the frame something is posted,
  against extra arithmetic everywhere the buffer is *read*.
- **`FActivityEntry` / `EActivityCategory` / `EActivitySeverity`** (`ActivityEntry.h`,
  `SmoresCore`) — one feed line. There is deliberately **no `All` category**: the LOG tab shows
  everything by *not filtering*, and a value only the reader ever used would be one more thing
  every producer has to get right. Severity is separate from category because "you took damage"
  and "you dealt damage" are the same category and read completely differently.
- **`USquadActivityWatcher`** (`smores`, `Variant_Strategy/`) — one per watched unit, owned by the
  player controller, turning that unit's health delegates into feed lines. Variant glue wiring
  existing delegates into the local player's feed, which is what the controller's module is for;
  it holds no state anyone else needs, so it is not a component in a feature module.
  `RefreshActivityWatchers` rebuilds the set from the world on the roster's interval, because a
  unit spawned mid-session announces itself to nobody. Whether a unit gets squad wording or
  enemy wording is decided once, at `Watch()` time.
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
- **`GetControlledPlayerUnits()`** — the squad bar's roster, on the real-time interval described
  in Core Rules. It also drives `RefreshActivityWatchers()`, because that is the only cadence that
  notices a unit having joined or left the level.
- **`RequestSelectUnit`** — replaces the selection through the same `DoDeselectAllUnitsCommand` /
  `UnitSelected` path the `Tab` cycle uses, sets `LastSelectionTarget` so the target panel follows,
  and updates `CurrentPlayerPawnIndex` so `Tab` resumes from where the click left off rather than
  from wherever it had got to before the player reached for the mouse. It re-checks that the unit
  is this player's own rather than trusting the widget: the bar is built from
  `GetControlledPlayerUnits()` and so can only offer legal units, but "the UI only ever asks for
  legal things" is not a rule this method should depend on.
- **`FocusCameraOnUnit`** — the camera solve. See Core Rules; the geometry is the whole of it.
- **`PostActivity` / `Client_NotifyActivity`** — the local and server-to-client routes into the
  feed, shaped exactly like `NotifyRefusal` / `Client_NotifyRefusal` and for the same reasons.
- **`Client_NotifyBark`** — the feed's third server-to-client route, and the one that does *not*
  carry worded text: the server sends a bark's **line id**, the speaker (an actor reference) and the
  speaker's name, and the client resolves the words from its own loaded, translated dialog before
  calling `PostActivity` - and, when the speaker resolved on this client, `ShowBarkBubble` on the
  HUD. Dialog crosses the network as ids so each co-op player reads a bark in their own language
  (`dialog.md`).
  `NotifyRefusal` now posts as well as raising the line, with its own repeat suppression.
- **`ToggleActivityFeedKeyPressed`** — goes controller → `AStrategyHUD::ToggleActivityFeed` →
  `UStrategyUI` → the region, **not** through `IStrategyHUDCommands`. `smores` already depends on
  `SmoresUI`, so this direction needs no interface; the interface exists for requests travelling
  the other way, from a widget that cannot see the controller's type.

## Blueprint / Asset Dependencies

All under `Content/Variant_Strategy/UI/`:

| Asset | Parent | Bound names |
|---|---|---|
| `UI_Strategy` | `UStrategyUI` | All six regions: `NavRail`, `ResourceStrip`, `TimePaceRegion`, `TargetPanelRegion`, `SquadBarRegion`, `ActivityFeedRegion`; plus `BarkBubbleLayer`, stretched full-screen and painted beneath the regions. **Its EventGraph is now empty** — see below |
| `WBP_NavRail` | `UNavRailWidget` | `SquadButton`, `InventoryButton`, `MapButton`, `ResearchButton`, `HelpButton` |
| `WBP_ResourceStrip` | `UResourceStripWidget` | `GoldText` |
| `WBP_TimePace` | `UTimePaceWidget` | `PauseButton`, `NormalButton`, `DoubleButton`, `QuadrupleButton`, `PaceText` |
| `WBP_TargetPanel` | `UTargetPanelWidget` | `NameText`, `ClassificationText`, `HealthBar`, `ActionBox`; plus the `ActionWidgetClass` property, which must point at `WBP_TargetAction` or the row silently shows nothing |
| `WBP_TargetAction` | `UTargetActionWidget` | `ActionButton`, `LabelText`, `ReasonText` |
| `WBP_SquadBar` | `USquadBarWidget` | `PortraitBox`, `SelectionCountText`; plus the `PortraitWidgetClass` property, which must point at `WBP_SquadPortrait` or the bar silently shows no portraits |
| `WBP_SquadPortrait` | `USquadPortraitWidget` | `PortraitButton`, `PortraitImage`, `InitialsText`, `NameText`, `HealthBar`, `SelectionRing` |
| `WBP_ActivityFeed` | `UActivityFeedWidget` | `LogTabButton`, `SquadTabButton`, `QuestsTabButton`, `CommsTabButton`, `EntryBox`, `EmptyText`; plus the `EntryWidgetClass` property, which must point at `WBP_ActivityEntry` or the feed silently shows no lines |
| `WBP_ActivityEntry` | `UActivityEntryWidget` | `MessageText`, `SourceText` |
| `WBP_BarkBubbleLayer` | `UBarkBubbleLayerWidget` | `BubbleCanvas`; plus the `BubbleWidgetClass` property, which must point at `WBP_BarkBubble` or barks reach the feed and nothing floats |
| `WBP_BarkBubble` | `UBarkBubbleWidget` | `LineText` |
| `WBP_HUDPlaceholderRegion` | `UHUDPlaceholderRegionWidget` | `LabelText`; no instances left in `UI_Strategy` — kept for the next region that arrives before its contents |
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

**`UI_Strategy`'s EventGraph is empty as of Slice 3, and that is the correct end state.** It held
two chains, both pushing text into the old loose `SelectionCount` widget: `Event Construct →
SetText("0")` and `Event Update Units Count → SetText(ToText(GetSelectedUnitsCount()))`. Both
existed solely to drive a readout `USquadBarWidget::SelectionCountText` now owns, so all eight
nodes were deleted along with the widget. `UStrategyUI::GetSelectedUnitsCount` and
`BP_UpdateUnitsCount` are untouched and still valid C++ — a future WBP may use them again.
**Deleting the widget without the chain would have failed the compile** with "Could not find a
function named X", naming the function but not where it was used; `BlueprintTools.read_graph_dsl`
on the EventGraph is how you find it, and a binary grep of `Content/` for the widget's name is the
cheaper advance check. This is the second slice running in which that trap was live — see the
`mcp-workflow` skill.

`Border_0` (the old count readout) and `Border_361` (a collapsed empty leftover) are both gone from
`UI_Strategy`.

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
An empty box still has to consume clicks; see Core Rules for the bug that proved it. No instances
are left in `UI_Strategy`, so the next region to arrive early is the asset's next use.

### Adding a producer to the activity feed

The feed is the one part of this HUD other systems are expected to write to, so the route in is
deliberately short.

1. **Decide the category and severity.** Category is which tab it belongs under; severity is only
   colour. Both are in `ActivityEntry.h` (`SmoresCore`), so any module can name them.
2. **Word the line where the facts are.** `Post` takes finished `FText`, not a code — see Core
   Rules for why this is the opposite of a refusal. Anything arriving as an
   `ESmoresRefusalReason` goes through `URefusalWidget::GetRefusalText` first.
3. **Pick the route by where the code runs.**
   - Client-side, and you have the player controller → `AStrategyPlayerController::PostActivity`.
   - Server-side → `Client_NotifyActivity`, the same shape as `Client_NotifyRefusal`. Calling
     `PostActivity` on the server means a dedicated server posting into a feed nobody can see.
   - Neither, and you only have a `UObject` → `USmoresActivityLog::Get(SomePlayerController)`.
     There is no world-context overload on purpose; a producer that can't name a player is a
     producer about to assume there is only one.
   - Server-side and it is **dialog** → don't word it at all: raise it through
     `UBarkDirectorComponent`, which sends the line's id (`Client_NotifyBark`) - see `dialog.md`.
4. **If the event is a parameterless delegate, you need something that knows the subject.**
   `USquadActivityWatcher` is the worked example — one instance per unit, because the delegate
   can't tell you which unit fired it.
5. **Don't add a category for a system that doesn't exist.** `Quests` is the exception that
   proves the rule: it exists because the tab is in the wireframe, and it says so on screen
   rather than pretending to be empty-for-now.

## Known Gaps

- **Health events reach the feed only on the machine that ran the damage**, which today is the
  server. So the fight record is correct in a standalone session and on a listen server's own
  screen, and a remote client would see nothing until health events are routed to owning clients.
  A real gap rather than a hidden one: the alternative — replicating a feed nobody can see yet —
  is the speculative machinery `multiplayer-discipline.md` says not to build.
- **The expanded feed does not scroll.** It grows its slot and shows `ExpandedEntryCount` (20)
  lines; the ring buffer holds 64. Reaching the rest means a `UScrollBox` around `EntryBox`, which
  is a WBP change and no C++ change — `EntryBox` is typed as a `UPanelWidget` precisely so that
  swap costs nothing.
- **No unit has a portrait texture**, so every tile draws initials. That is the designed
  fallback, not a fault; `PortraitTexture` on `AStrategyUnit` is authored per Blueprint or per
  placed instance whenever there is art.
- **Identity now comes from the character record** (game-data Slice 4, see `game-data.md`).
  `UnitDisplayName` is the actor's replicated copy of `FCharacterRecord::Name`, so the tile shows
  whatever the record says; a placed unit's authored name still names its record. The portrait is
  the unit's own `PortraitTexture` if set, else `UCharacterDefinition::Portrait` - neither has art
  yet. Biography (`Backstory`) is on the definition and nothing displays it.
- **The portrait bar has no answer for a large squad.** The wireframe shows nine and stops, and
  roughly seven tiles fit the authored slot. Scroll, shrink or wrap is a layout decision nobody
  has to make until a squad gets that big.
- **Nothing decides what *demands* acknowledgement.** The feed remembers; dismissal semantics
  belong to `notifications-and-alerts.md`, still a placeholder topic.
- **`WBP_BarkBubbleLayer`'s `BubbleWidgetClass` is another single point of failure** of the same
  shape: cleared, barks still reach the feed and nothing floats. It warns once, on the first bark.
- **`PortraitWidgetClass` and `EntryWidgetClass` are silent single points of failure**, the same
  shape as `WBP_TargetPanel`'s `ActionWidgetClass` and `BP_StrategyGameMode`'s `GameStateClass`:
  clear one and that region draws its chrome and nothing else. Each logs one warning naming
  itself, which is the only reason it isn't invisible.
- **A portrait click always replaces the selection; `Shift` does nothing to it.** In the world
  `Shift` means "add to / remove from", and the portrait bar ignores it — consistent with the
  `Tab` cycle, which also replaces, but `Shift`-clicking three portraits to build a fire team is
  the obvious thing a player will try. One `IsShiftDown()` in `USquadPortraitWidget`'s click
  handler, routed through the same `IStrategySelectionHost` call the world uses so the two can't
  drift. Wants a squad bigger than the prototype's to judge.
- **`ExpandedHeight` (460px) is a guess.** A record opened to read *after* a fight may want most
  of the screen rather than a slightly taller corner, and the number was picked to fit 20 lines
  rather than because anyone judged it. One property, no code.
- **The feed's 64-entry capacity is the wireframe's number**, and nothing has tested it against a
  long session. `USmoresActivityLog::SetCapacity` shrinks safely at runtime (there is a test for
  it), so this is a tuning question, not a structural one.
- **The target panel collapses the moment its target is deselected.** A panel that lingered on the
  last thing looked at may read better than one that blinks out — it is one line in
  `UTargetPanelWidget::RefreshTargetDisplay` either way, and a PIE call rather than an argument.
- **Nothing restores the time pace across a save.** `UTimePaceComponent::BeginPlay` applies
  whatever tier the component holds, so a restored pace would take effect with no extra code —
  but whether a save should ever restore "paused" belongs to `save-system.md`, which does not
  exist yet.
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
