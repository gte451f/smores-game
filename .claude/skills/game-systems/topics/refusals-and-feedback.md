# Refusals and Player Feedback

## Purpose

The one place the game tells the player **why** it just refused to do something.

Before this existed, every refusal in the game looked identical: nothing happened. A dragged
item snapped back whether it didn't fit, cost more than the player had, or was refused for a
reason only the server knew. A double-clicked chest out of reach did nothing at all, exactly
like a double-clicked chest that wasn't there. The inventory roadmap recorded this three
separate times, one slice apart, because each new system inherited it — the grid in Slice 2,
the purchase in Slice 8, the repack in Slice 9.

The system is deliberately small: a shared vocabulary of refusal *codes*
(`ESmoresRefusalReason`), one place that turns a code into words
(`URefusalWidget::GetRefusalText`), and one line of text at the cursor that says them. It is not a
notification framework, a message queue, or a log — those would all be bigger than the problem.

**The governing principle is that preventing a refusal beats explaining one.** The inventory
grid's red drop preview is the best refusal in the game and never says a word: the cells turn
red before the player releases the mouse, so the gesture never completes and no message is
needed. This system carries what *can't* be shown in advance. Every refusal it announces is one
that got past the prevention layer, and a new one should always be checked against "could the
client have shown this before the player committed?" first.

## Player Surface

- **A short line of text appears at the mouse cursor** when something is refused, and fades
  after about two seconds. It never needs dismissing, never takes focus, and never swallows a
  click — the player can keep working straight through it.
- **The current messages**, and what each one is telling the player to do differently:

| The player sees | It means | What to do about it |
|---|---|---|
| *Too far away* | No pawn is close enough to reach it | Walk a pawn over |
| *No room for that* | It won't fit — no free cells of the right shape, and no matching stack with space | Rearrange the grid, rotate the item, or drop something |
| *Not enough gold* | The price is more than the wallet holds | Sell something first |
| *Can't be worn there* | The item isn't worn in that slot, or isn't wearable at all | Put it in a different slot |
| *They won't deal with you* | The NPC is hostile | Nothing — not while they're trying to kill you |

- **A "denied" sound** plays alongside the line, if one has been assigned in the Blueprint. It
  is optional and unset by default; the text works without it.
- **Repeats collapse.** Clicking a chest you can't reach five times gives one line that stays
  up longer, not five lines and five sounds.
- **Every refusal is also remembered.** The same wording appears in the activity feed's SQUAD
  tab, in amber, so a player who was looking somewhere else can still find out why nothing
  happened. The line answers it *now*; the feed is the record. See `hud-and-panels.md`.
- **The gestures that now speak up**, all of which were previously silent:
  - Double-clicking a container, a body, or a loose world item that no pawn is near
  - Pressing the container key with nothing in reach
  - Double-clicking a hostile NPC
  - Buying something the player can't afford, or trying to trade with someone out of reach
  - A sort button that can't fit everything back into the grid
  - Right-clicking an item into a slot it isn't worn in, or off a paperdoll into a full grid
  - Picking up a world item that won't fit in the pack

## Core Rules

- **A refusal is said once and recorded once.** `AStrategyPlayerController::NotifyRefusal` both
  raises the line and posts to the activity feed, so there is one call site and the two cannot
  disagree about what was refused. The feed's copy is worded by the same
  `URefusalWidget::GetRefusalText`, not by a second phrasing.
  - **The feed needs its own repeat suppression, for a sharper reason than the line does.**
    `URefusalWidget` collapses repeats so leaning on a key doesn't machine-gun the sound; the feed
    collapses them because it is a *fixed-capacity* record, and sixty copies of "Too far away"
    would push everything else out of it. That is the one way the feed can actively lose
    information rather than merely repeat itself.
    `AStrategyPlayerController::FeedRefusalRepeatSeconds` is 2s, matching how long the line stays
    on screen: while the same refusal is still showing, it is still the same refusal.
- **A refusal travels as a code, never as a sentence.** `ESmoresRefusalReason` is what crosses
  the wire and what every rule raises; the words exist only in `URefusalWidget::GetRefusalText`.
  Nothing on the server ever builds one of these strings. That is what keeps one refusal from
  being phrased two ways, keeps the whole set translatable in one pass, and lets the same reason
  drive a different presentation somewhere else later (a red cell, a greyed button, a tooltip)
  without the wording forking.
- **Prevention beats explanation, and the drop preview is the proof.** A refusal the client can
  work out for itself should be shown *before* the gesture completes, not announced after. The
  ordinary drag-and-drop move therefore raises nothing on the normal path — `WouldAcceptDrop`
  already turned the cells red. It announces only when the server refuses a move the client
  predicted would work, which means the grid changed underneath it.
- **Client-side rules are refused client-side.** Reach, hostility and a full grid are all things
  the client already knows, so those paths call `NotifyRefusal` directly rather than asking the
  server and waiting to be told no. `Client_NotifyRefusal` is only for what the client genuinely
  couldn't know: a price it hasn't priced, a range re-check against a pawn that has moved, a
  repack that couldn't fit.
- **A client-side early-out is a refusal and has to say so.** This is the trap the pattern sets,
  and it was caught in PIE rather than by reading the code. `UInventoryItemWidget::TryEquip`
  correctly skipped the RPC for an item that isn't wearable — the client can read the slot off
  the shared definition, so there was no point asking — and then returned in silence. The result
  was the *same rule* giving two different answers depending on the gesture: dragging a cabbage
  onto a slot said "Can't be worn there", right-clicking the same cabbage did nothing at all.
  **Whenever a client short-circuits a request to save a round trip, check that the branch raises
  the reason the server would have sent.** Skipping the RPC is an optimisation; skipping the
  explanation is a bug.
  - Distinguish it from a genuinely *inert* gesture. Right-clicking an item in a chest also
    returns early from that same function and correctly says nothing — right-click has no meaning
    in a window with no equipment target, so there is no refusal to report. "This does nothing
    here" and "this was refused" are different, and the fix was to split one `if` into two.
- **A refusal is never modal.** It doesn't pause anything, doesn't need acknowledging, and is
  `HitTestInvisible` so it can't eat the next click. A refusal that cost the player a click would
  cost more than the refusal did.
- **The refusal line lives on its own top-most layer, not in the HUD.** This was got wrong first
  and caught in playtest. Slate paints viewport widgets by Z-order and then by the order they
  were added; every window in this game goes up at Z-order 0 *after* `UI_Strategy` does, so a
  refusal parented into the HUD renders **behind** the inventory window the player was working
  in — which is exactly where they were looking when it fired. `URefusalWidget` is therefore its
  own widget at Z-order 100.
  - The split isn't just a workaround, it's the right division: a **transient message answering
    what the player just did** must be visible over everything, while the HUD's **persistent
    readouts** (gold, selection count) should stay *coverable* by a dragged window. Raising
    `UI_Strategy`'s Z-order instead would have fixed the message and made the gold readout punch
    through any window dragged over it.
  - Anything else that must escape an open window — a confirmation prompt, a tooltip that has to
    leave its panel — wants this layer or one like it, not a higher Z-order on `UI_Strategy`.
    `RefusalZOrder` is 100 rather than 1 deliberately, so later layers can sit between.
- **"Nothing changed" and "this was refused" are different outcomes.** Several mutators return
  `false` for both — a sort of an already-sorted grid and a sort that couldn't repack both
  changed nothing. Only the second is worth interrupting the player over. That distinction is
  why the `*WithReason` variants exist and why `None` is a legal reason: it means "failed, and
  say nothing".
- **The refusal belongs to the player who asked.** It is raised on that player's own controller
  and shown on that controller's own HUD — in a co-op session nobody else sees it.
- **Only add a reason something actually raises.** A value nothing ever sends is an empty hook
  the player never sees, the same reasoning that kept a dialog stub out of `InteractWithNPC`.

## C++ Implementation

**`ESmoresRefusalReason`** (`Source/SmoresCore/SmoresRefusalReason.h`) — the shared vocabulary:
`None`, `TooFar`, `NoRoom`, `CannotAfford`, `WrongSlot`, `NotInteractable`. A `uint8`-based
`UENUM(BlueprintType)`, so it replicates as an RPC parameter and is readable from Blueprint.

It lives in `SmoresCore` — until now an empty proving module — because refusing is not an
inventory idea. Items, equipment and trade all raise refusals today and orders and combat will;
all of them need to name a reason in a vocabulary `SmoresUI` can read, and `SmoresCore` is the
one module every one of them can already see. Putting it in `SmoresItems` would have left
`SmoresCombat` unable to use it (that module does not depend on `SmoresItems`), which is the
same constraint `Docs/roadmaps/testing-roadmap.md` records for its own shared test helper.

**`URefusalWidget`** (`Source/SmoresUI/RefusalWidget.*`) — the display. Its own `UUserWidget`,
added to the viewport by `AStrategyHUD::BeginPlay` at `RefusalZOrder` (100), above every window:

- `GetRefusalText(Reason)` — static, `BlueprintPure`. The only place any refusal is worded.
- `ShowRefusal(Reason)` — fills `RefusalText`, makes it `HitTestInvisible`, moves it to the
  cursor, plays `RefusalSound`, fires `BP_RefusalShown`, and starts the clear timer. A `None`
  does nothing.
- `ClearRefusal()` — hides the line and fires `BP_RefusalCleared`. Runs off the timer, and is
  `BlueprintCallable` so a window can dismiss a stale line early.
- `PositionRefusalAtCursor()` — casts the text block's layout slot to `UCanvasPanelSlot` and
  moves it to the mouse. Divides the mouse position by `UWidgetLayoutLibrary::GetViewportScale`,
  because `GetMousePosition` is in viewport pixels while a Canvas slot is in widget-space units —
  skipping that puts the line a long way from the cursor at any DPI scale but 1.0. No-ops
  cleanly when the slot isn't a Canvas slot, when there's no cursor over the viewport, or when
  there's no owning player.
- `NativeDestruct()` — clears the timer, which holds a raw `this`.
- `NativeConstruct()` — forces the widget's own visibility to `SelfHitTestInvisible` rather than
  trusting the WBP. This layer covers the whole screen above every window, so anything
  hit-testable about it blocks the entire game's mouse input — a catastrophic failure whose
  symptom (clicks stop working, everywhere) points nowhere near the cause.
- `RaiseRefusal(OwningPlayer, Reason)` — **static**, and the single place
  HUD → refusal widget → `ShowRefusal` is resolved. Any widget inside `SmoresUI` that decides a
  rule for itself calls this; so does the controller's `NotifyRefusal`, so there is exactly one
  lookup path rather than one per caller. Safe with a null controller, no HUD, or a HUD whose
  widget hasn't been created yet.
- Tunables, all `EditAnywhere` on the widget: `RefusalDisplaySeconds` (2.0),
  `RefusalCursorOffset` (20, 20), `RefusalRepeatSeconds` (0.4), `RefusalSound` (unset).

**`AStrategyHUD`** (`Source/SmoresUI/StrategyHUD.*`) — owns the layer: `RefusalWidgetClass`
(`EditAnywhere`), `RefusalWidget`, `RefusalZOrder` (100), and `GetRefusalWidget()`. Created in
`BeginPlay` and **not** `check()`ed the way `UIWidget` is — a missing class should cost the
messages, not the session, so it warns through `LogSmoresUI` instead.

**`AStrategyPlayerController`** (`Source/smores/Variant_Strategy/StrategyPlayerController.*`) —
the two routes to the line:

- `NotifyRefusal(Reason)` — local and client-side, via the controller's own cached `StrategyHUD`.
  What every client-side rule calls. Does nothing without a HUD, which is correct on a dedicated
  server and is exactly why server code must not call it.
- `Client_NotifyRefusal(Reason)` — `UFUNCTION(Client, Reliable)`, server → owning client, which
  then calls `NotifyRefusal`. What every server-side rule calls.

**The `*WithReason` mutator variants** — the same split as `AddItem`/`AddItemCounted` and
`MoveItem`/`MoveItemCounted`, for the same reason: the bool collapses outcomes the caller has to
tell apart. In each case the plain version is now a one-line forwarder.

| Informative version | Plain forwarder | Raises |
|---|---|---|
| `UInventoryComponent::SortEntriesWithReason` | `SortEntries` | `NoRoom` on an abandoned repack; `None` when already sorted |
| `UEquipmentComponent::EquipWithReason` | `Equip` | `WrongSlot`, or `NoRoom` for the displaced item |
| `UEquipmentComponent::UnequipWithReason` | `Unequip` | `NoRoom` when the grid is full |

**Where refusals are raised** (the complete list, since it's short and worth keeping honest):

| Site | Reason | Route |
|---|---|---|
| `UInventoryItemWidget::TryEquip` — item isn't wearable | `WrongSlot` | local (widget) |
| `ToggleContainer` — nothing in reach | `TooFar` | local |
| `SelectAllDoubleClick` — world item / container / body out of reach | `TooFar` | local |
| `InteractWithNPC` — hostile | `NotInteractable` | local |
| `InteractWithNPC` — out of reach | `TooFar` | local |
| `Server_MoveInventoryItem` — `MoveItem` refused | `NoRoom` | RPC |
| `Server_SortInventory` — abandoned repack | `NoRoom` | RPC |
| `Server_EquipItem` / `Server_UnequipItem` | from the component | RPC |
| `Server_PickUpWorldItem` — out of reach / won't fit | `TooFar` / `NoRoom` | RPC |
| `TryTradeItem` — hostile trader | `NotInteractable` | RPC |
| `TryTradeItem` — out of reach | `TooFar` | RPC |
| `TryTradeItem` — insufficient gold | `CannotAfford` | RPC |
| `TryTradeItem` — goods wouldn't fit | `NoRoom` | RPC |

`TryTradeItem`'s range and hostility checks were one combined `if` before this and are now two,
purely so the two can be told apart on screen — "they won't deal with you" and "walk closer" ask
completely different things of the player.

## Blueprint / Asset Dependencies

- **`WBP_RefusalLine`** (`Content/Variant_Strategy/UI/WBP_RefusalLine.uasset`) — a `URefusalWidget`
  subclass whose entire content is a root `UCanvasPanel` and one `UTextBlock` named exactly
  **`RefusalText`**, bound by `BindWidgetOptional`. C++ fills, positions, shows and hides it —
  **no Blueprint graph work**, the same shape `GoldText` established in Slice 4 and the sort
  toolbar extended in Slice 9.
  - **The root must be a Canvas Panel** for the at-the-cursor behaviour. That is the one
    structural requirement: `PositionRefusalAtCursor` casts the text block's slot to
    `UCanvasPanelSlot` and gives up silently if the cast fails. In any other panel the line still
    works, it just stays where it was placed — a fixed-position readout rather than a
    cursor-following one.
  - The binding is **optional**, so a WBP without it compiles and runs. The cost of getting the
    name wrong is therefore silence, not an error — the same failure mode as every other
    `BindWidgetOptional` in this project.
  - Styling is deliberately not the house style: Roboto Bold **20** against `UI_Strategy`'s 24,
    in a warm red (1.0, 0.45, 0.35), with a real drop shadow. `GoldText` sits inside a `Border`
    that gives it a background and has its shadow alpha at 0; `RefusalText` floats over open
    terrain and over windows with nothing behind it, so it needs the shadow to stay legible.
  - **`UI_Strategy` carries nothing for this.** A `RefusalLayer`/`RefusalText` pair was added
    there first and reverted once playtesting showed it rendering behind the inventory window —
    see the layering rule in Core Rules. Don't put it back.
- **`BP_StrategyHUD`** (`Content/Variant_Strategy/Blueprints/BP_StrategyHUD.uasset`) sets
  `RefusalWidgetClass` to `WBP_RefusalLine`. If that assignment is ever lost, the failure is
  silent at compile time and shows up only as refusals never appearing, plus one
  `LogSmoresUI` warning at `BeginPlay` — the same shape as the `PlayerStateClass` hazard recorded
  in `Docs/roadmaps/inventory-roadmap.md`'s Slice 4.
- **`RefusalSound`** is an `EditAnywhere` property on `UStrategyUI` with no default. Assigning a
  `USoundBase` in the WBP's class defaults is the whole wiring step; leaving it unset is
  supported and only costs the sound.

## Extension Points

- **A new reason** is one line in `ESmoresRefusalReason` and one line in `GetRefusalText`.
  `Locked` and `TooHeavy` are the two most likely next ones — neither is listed today because
  locks don't exist and weight has no consequence yet.
- **`BP_RefusalShown(Reason)` / `BP_RefusalCleared()`** are the cosmetic hooks: a shake, a
  flash, a per-reason sound. The reason code is passed in, so a Blueprint can branch on it.
- **A second presentation** — a reason shown as a greyed-out button, a tooltip, or a coloured
  cell rather than a line of text — reads the same enum and calls `GetRefusalText` for its
  wording. That is the whole reason the code and the words are separate.
  - **The first one shipped is the target panel's action row** (`FTargetAction::DisabledReason`,
    rendered by `UTargetActionWidget` — see `hud-and-panels.md`). It is worth knowing because it
    is a different *kind* of use than the line at the cursor: a **standing** reason, displayed
    continuously next to a disabled button for as long as the rule applies, rather than a
    transient answer to something the player just did. Both read the same enum and both get their
    words from `GetRefusalText`, so "Too far away" reads identically whether the player walked
    into the rule or is looking at it in advance — which is the payoff the split was for.
  - **Preventing a refusal still beats explaining one, and a standing reason is the strongest
    form of that.** A greyed button with its reason beside it means the gesture never completes
    *and* the rule is legible before the player tries. The transient line stays the fallback for
    rules that can only be discovered by acting.
  - **`ESmoresRefusalReason::None` is meaningful in this form.** An action can be disabled for a
    reason that isn't a refusal at all — Attack on someone already fighting you — and it carries
    `None`, which renders as no reason text and which `NotifyRefusal` already ignores. Don't add
    an enum value for "you're already doing that"; it isn't a refusal, and a reason nothing raises
    is an empty hook (see the header comment on `ESmoresRefusalReason`).
- **A second top-most layer** (a confirmation prompt, a tooltip that must escape its panel)
  follows `URefusalWidget`'s shape: its own `UUserWidget`, owned and added by `AStrategyHUD` at
  its own Z-order, `SelfHitTestInvisible` unless it genuinely needs the clicks. `RefusalZOrder`
  is 100 so there's room to sit between the windows at 0 and this.
- **Other systems** raise refusals by calling `NotifyRefusal` (client-side rules) or
  `Client_NotifyRefusal` (server-side rules) on the player controller. Nothing about either is
  inventory-specific; unit orders and combat are the obvious next callers.

## Known Gaps

- **The refusal line has no scrollback.** A refusal that fades is gone — there's no log to check
  what just happened, which matters if two things are refused in quick succession and only the
  second is still on screen. A message history is a bigger feature than this one and nobody has
  needed it.
- **`RefusalSound` is unset, so refusals are silent by default.** The hook is wired and the
  suppression window exists to stop it machine-gunning, but there is no audio asset in the
  project to point it at yet. This is the single highest-value thing left to do here: a sound
  lands without being read and works while the player is looking at the pawn rather than the text.
- **Nothing clears a stale line on success.** A refusal sits for its full
  `RefusalDisplaySeconds` even if the player immediately does something that works. Harmless at
  two seconds and `ClearRefusal` is public if it ever stops being.
- **The ordinary drag-and-drop move still explains nothing on the client path**, deliberately —
  the red preview is the explanation. But the preview says only "not here", never "not this":
  an item refused because the destination stack is full shows the same red as one refused for
  overlapping another item.
- **A partially-refused action can't be described.** Every reason here is all-or-nothing, which
  matches every operation that currently raises one. The obvious case that *isn't* — a purchase
  the player can only half afford — is refused outright rather than partially fulfilled, and the
  honest fix for that is partial-stack drag, not a "partial" reason code.
- **`WrongSlot` covers two different problems** — "this item isn't wearable at all" and "it isn't
  worn in *that* slot". They're collapsed because the player's next move is the same for both,
  but a paperdoll that highlighted valid slots during a drag would make the distinction visible
  and worth splitting.
- **No refusal has a keyboard or gamepad route to dismiss it**, and it doesn't need one — it
  fades. Worth remembering if a message ever becomes important enough to need acknowledging,
  because that's the point it stops being this system.
- **Nothing outside inventory, equipment and trade raises a refusal yet.** Unit move orders,
  attack commands and the camera all still fail silently where they fail at all. The mechanism is
  general; the coverage isn't.
