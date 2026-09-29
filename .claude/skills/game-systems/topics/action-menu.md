# Action Menu and Walking Over to Act

## Purpose

What the squad can do to a thing, and how it gets done. Right-click on something opens a menu of
actions; the target panel shows the same list as buttons; the `T` and `O` keys and a double-click
are shortcuts into it. Whichever route the player uses, the nearest selected squad member walks
over and does it. This topic owns the rules for what each kind of thing offers, who carries an
action out, the walk-over order, the menu, and the hover highlight that says in advance which
right-click the player is about to make.

Design intent lives in `game-design`'s `player-interface.md` ("Right-Click Action Menu", and the
known-vs-hidden numbers rule under Diegetic vs. Non-Diegetic Philosophy). Built by
`Docs/roadmaps/completed/action-menu-roadmap.md`, and PIE-checked by Jim 2026-09-29 - the hover
highlight (rather than hold-to-open) is his confirmed choice.

## Player Surface

- **The hover.** The thing under the cursor - a person, a creature, a body, a squad member, a
  chest, a door, an item on the ground - gets an amber rim before any click. That is the promise:
  a right-click now opens the menu on it. Over empty ground nothing is lit, and a right-click
  moves the squad as it always did. Nothing lights while the cursor is over the HUD or a window,
  or while the camera is turning. The menu's target stays lit while the menu is open.
- **Right-click on a lit thing** targets it (the target panel now describes it too) and opens the
  menu at the cursor: the name, what it is and how far, who would go ("Hana would go"), and one
  row per action. A greyed row says why ("They won't deal with you", "No one selected"). Actions
  whose outcome depends on who attempts them show a chance ("62% chance"), which changes with the
  squad member who would go.
- **Picking a row** closes the menu and sends that squad member over. A click anywhere else closes
  the menu and does nothing else - it doesn't also move or select. The game keeps running while
  the menu is open. Esc does not close it (see Known Gaps).
- **The target panel's buttons, `T`, `O` and the double-click all walk over too.** A double-clicked
  chest across the room sends someone to open it, where it used to refuse *Too far away*.
- **Examine** opens a small window - the name, what kind of thing it is, what the squad can see of
  it, in words. It never walks anywhere and never shows a number about the target.
- **Heal, Kidnap, Pickpocket and Knock out are placeholders.** They're offered under their real
  rules and walked over to; on arrival the feed says *"Is ready to pickpocket Bandit - not built
  yet"*. Jim adds placeholders on purpose, to judge the menu's spacing when it's full.
- **Someone talked to turns to face the squad member** who came to talk or trade.
- **Reach needs a clear line, not just distance.** A squad member can't loot a chest, open a door
  or talk to someone through a wall or a shut door, however close they stand. So a chest behind a
  shut door gives *Can't get there*: the walk stops at the nearest spot outside, which is out of
  reach, until someone opens the door. People in the way don't count as walls.
- **Something going wrong on the way is said.** *Can't get there* when no path reaches it; a feed
  line when the squad member went down, stopped to fight back, or found the target gone. The
  player's own new order (a move, an attack, another action) cancels the walk silently.

## The Rules - What Each Kind of Thing Offers

`FStrategyTargetActions::BuildTargetInfo` implements this table row for row, and
`Source/smores/Tests/TargetInfoTest.cpp` asserts it one test per row. **Change the two together.**

| Target | Entries, in order | When disabled |
|---|---|---|
| Person, standing, neutral | Talk · Trade *(traders only)* · Pickpocket · Knock out · Heal · Attack · Examine | Heal: at full health (no reason) |
| Person, standing, hostile | the same | Talk, Trade: *They won't deal with you*. Pickpocket, Knock out: no reason (awareness brings the real one). Attack: already fighting you (no reason) |
| Person, Downed | Loot · Heal · Kidnap · Examine | — |
| Person, Dead | Loot · Examine | — |
| Creature, standing | Attack · Examine | Attack: already fighting you (no reason) |
| Creature, Downed or Dead | Loot · Examine | — |
| Your own squad member | Heal · Examine | Heal: at full health (no reason) |
| Another player's squad member | Examine | — |
| Container | Loot · Examine | — |
| Door | Open *or* Close · Examine | — |
| Item on the ground | Pick up · Examine | — |

- **Every entry except Examine is disabled with *No one selected* when nobody would carry it out**
  (the rule below). The target's own reason wins over that one: selecting somebody wouldn't make a
  hostile talk, so their Talk still says *They won't deal with you*.
- **Nothing is disabled for distance.** Walking over is the point; the distance figure still shows.
- **Trade is listed on the `UTraderComponent` being there**, and enabled on hostility.
  `GetTraderStock` answers null for a hostile trader, so using it for the listing would make Trade
  vanish instead of greying out.
- **Person or creature** is `UCharacterDefinition::Kind` (`ECharacterKind`, default Person). A unit
  with no definition counts as a person.
- **Key hints:** Loot `O`, Open/Close `O`, Talk `T`, Attack `H`.
- **Vocabulary:** a container's action is **Loot** (the same word as a body's - they open the same
  window). **Open / Close** mean doors only. Action ids live in `StrategyTargetAction`
  (`SmoresUI/StrategyTargetInfo.h`).

### Who acts

`FStrategyTargetActions::ResolveActor` - one function, used by the menu, the panel, the keys and
the double-click, so they can't disagree:

1. **Candidates** are the player's selected units that can act (not Downed or Dead), excluding the
   target itself.
2. **The actor** is the candidate nearest the target in a straight line - the distance the panel
   shows.
3. **Heal alone** may fall back to the target: selected, and nobody else able, they treat themselves.
4. **With nothing selected**, a squad member already within reach acts, so a pawn standing at a
   chest can still open it. Anyone selected but unable means nobody - no one else is quietly sent.
5. **Attack uses every candidate** (`GetAttackers`), exactly as `DoAttackCommand` always has; its row
   says *Everyone selected* when more than one would go.
6. Examine needs nobody.

The actor is worked out afresh every frame, not locked when the menu opens: a locked actor would be
wrong by the time of the click.

### The odds

`FTargetAction::Detail` is the per-actor line. Only Pickpocket, Kidnap and Knock out fill it today,
from `FStrategyTargetActions::GetPlaceholderSuccessChance`: `clamp(0.5 + edge x 0.03, 5%, 95%)`, where
the edge is Agility against the mark's Perception for Pickpocket and Strength against Endurance for
Kidnap and Knock out. **It is a placeholder, owned by the future stealth, capture and takedown
systems.** The shape that
must survive:

- **One function produces the displayed chance and (once there is one) the roll**, so the menu can
  never promise odds the server doesn't use.
- **It reads replicated data only** - `AStrategyUnit::GetAttributes()`, the replicated copy of the
  record's attributes - so it answers the same on a client.
- **It shows a number** because it is built from the squad's own numbers, which the player already
  knows. The target's numbers go into the sum and are never displayed. `FormatSuccessChance` is the
  one place to make it coarser, should working the odds backwards ever matter.
- The detail line also names the actor when it isn't the menu header's one (a self-treating Heal,
  an attack by everyone).

The two squad pawns in `LVL_Strategy` differ on purpose: Pawn 2 uses `DA_Character_SettlerNimble`
(Agility 16), Pawn 1 the plain Settler, so the odds visibly change between them.

## Walking Over to Act

`UActionOrderComponent` (`SmoresCharacters`, a default subobject of every `AStrategyUnit`, NPCs
included) holds at most one order - target, action id, retry count. **Authority only;** nothing
about it replicates, because what the player sees is the unit walking.

**The lifecycle:**

1. **Issue** (`IssueOrder`). Replaces any earlier order silently, and takes the unit over
   (`AStrategyUnit::TakeOverForActionOrder`: drops a pending move query, the walk, any attack).
   Within reach, act at once; otherwise walk.
2. **Walk** (`AStrategyUnit::MoveToActor`) - a goal-actor move, not `MoveToLocation`'s EQS point,
   so it follows a target that moves and can't end at a random point outside reach. It measures
   from the target's centre to the unit's edge (`ApproachAcceptanceRadius`, 100); the target's own
   size is left out because an actor's bounds can include its reach sphere.
3. **Arrive** (`HandleApproachFinished`). Within the target's own reach
   (`ISmoresInteractable::IsInRangeOf`: inside its interaction sphere *and* `HasClearReach` - no
   world-static geometry, such as a wall or a shut door's leaf, on the line between the two; found
   after the PIE check, when a pawn looted the storeroom chest through its back wall) → act. Otherwise walk again, up to `MaxRetries` (2) times,
   then fail **CannotReach**. A walk that can't even start fails CannotReach at once, and "already
   at goal" (which makes no request, so reports nothing) is treated as an arrival on the spot.
4. **Act** (`Act`). The order ends first, then the host re-checks the entry through the same rules
   (`CanPerformAction` → `FStrategyTargetActions::FindActionFor`). Greyed out now - turned hostile,
   went down - refuses with that entry's own reason. Otherwise the host carries it out.

**Only the order's own walk reaches it.** The path follower reports *every* request's end, aborts
included, and an abort fires synchronously inside whatever call replaced the move. The unit keeps
`ActionMoveRequestId` and forwards only that request's end; anything that starts or stops a walk
clears the id *before* the engine call, so a retry can never hear its own previous walk finish.
`OnEQSFinished` likewise ignores a query that is no longer current, so a late `MoveToLocation`
answer can't hijack a walk that has since started.

**Cancellation:**

| What happened | How the order ends |
|---|---|
| The player's new move order (`Server_MoveUnits` → `MoveToLocation`) | Silently - `MoveToLocation` is the catch-all, cancelling before it stops the unit |
| The player's attack order (`AttackTarget`) | Silently |
| Another action order for the unit | Silently - it replaces this one |
| The unit goes Downed or Dead | Feed: *"Went down before reaching {target}"* |
| The unit swings back at someone who hit it (`OnHealthDamaged`) | Feed: *"Stopped to fight back"* |
| The target stops existing | Feed: *"{target} is no longer there"* |
| No path, after the retries | Refusal: *Can't get there* |
| Arrived and the rules said no | Refusal with the entry's reason; a reason-less one (a greyed Pickpocket) gets *"Couldn't do that to {target} after all"* in the feed |

**`IActionOrderHost`** (`SmoresCharacters`) is the narrow interface the component calls, implemented
by `AStrategyPlayerController` and found through the unit's owning controller. The controller
dispatches by id, server-side:

| Action | On arrival |
|---|---|
| Loot (container or body) | `Client_OpenHolder(Target, Actor)` - the owning client opens the window with **the actor's** pack beside it |
| Talk | The NPC turns to face the actor (`AStrategyUnit::FaceToward`), then `StartTalk(NPC, Actor)` - a conversation if one is eligible, else a trader opens trade, else a NothingToSay bark; the actor is the listener |
| Trade | The NPC turns to face the actor, then `OpenTradeFor(NPC, Actor)` - the shop directly, the actor at the counter |
| Pick up | `PickUpWorldItem(Item, Actor's pack)` |
| Open / Close | `AWorldDoor::SetOpen` |
| Heal, Kidnap, Pickpocket, Knock out | `Client_NotifyActivity`: *"Is ready to pickpocket Bandit - not built yet"*. Nothing else changes |
| Attack | not an order - `Server_AttackCommand`, squad-wide, through combat's own approach |
| Examine | not an order - client-side, immediate |

**The client's side** is `Server_RequestActionOrder(Actor, Target, ActionId)`. The server re-checks
through `FStrategyTargetActions::ValidateActionOrder`: the actor is this player's own and able, the
action is an order (not Attack or Examine), and the entry is on offer and enabled for that actor.
The server has no selection (`ControlledUnits` is client-only), which is why the rules have two
entry points - *who acts* is a client question, *may this actor do this* is asked on both sides.

## The Menu, the Hover and the Clicks

- **One resolver**, `AStrategyPlayerController::ResolveInteractableUnderCursor`, answers "the thing
  under the cursor" for the hover, the right-click and the double-click, so what is lit is what any
  of them acts on:
  1. the cursor trace (`SelectionTraceChannel`) landing on something hoverable;
  2. else the cursor's ray passing through a hoverable thing's own **mesh bounds** - so a unit is
     found whatever its capsule answers, and a dropped item (no collision at all) is found too.
     Anything further along the ray than what the trace hit is behind it and doesn't count;
  3. else the nearest hoverable within `HoverPickRadius` (40 cm, across the ground) of the point
     under the cursor, in the old double-click ladder's order: item, container, door, unit.

  "Hoverable" (`IsHoverable`) is an item holding something, a container, a door, or any unit. A
  door is aimed at by its **leaf only** (`GetHoverMeshes`), so an open doorway stays ground a
  right-click can walk through. **A single left click keeps its own generous sweep**
  (`DoSelectCommand`, 250 cm) - picking your own moving pawns is a different job.
- **The hover** (`UpdateHover`, every `PlayerTick`) sets `HoverOverlayMaterial` (`M_HoverRim`, an
  additive unlit fresnel rim) as the overlay material on the hovered thing's meshes, and clears it
  when the hover moves. Local and cosmetic, never replicated. It's cleared while
  `AStrategyHUD::IsCursorOverHUD` says the cursor is on a HUD region, a window or the open menu -
  which asks Slate for the widgets under the cursor and looks for one of the kinds that swallow
  clicks - and while the camera is turning.
- **Right-click** (`InteractClick`, on release): the resolver found something → `TargetActor` and
  `AStrategyHUD::OpenActionMenu`. Nothing → `DoMoveUnitsCommand`, as before.
- **`UActionMenuWidget`** (`SmoresUI`, `WBP_ActionMenu`) is its own layer, owned by `AStrategyHUD` at
  **Z-order 50** - above every window at 0, below the refusal line at 100. The whole widget is a
  full-screen click catcher while open: the press and the double-click of any mouse button outside
  the panel close it and do nothing else; the release is left alone (`input-and-keybinds.md`).
  Collapsed while closed, so it eats nothing then. The panel sits at the cursor in a canvas slot and
  flips to the other side of the cursor rather than running off the screen. Rows are
  `UTargetActionWidget` / `WBP_TargetAction`, the same as the panel's.
- **Refresh:** `DrawHUD` pushes `IStrategySelectionHost::GetTargetInfoFor(MenuTarget)` every frame,
  and the menu compares (`FStrategyTargetInfo::DrawsIdenticallyTo`) before touching Slate. The menu
  has its own target rather than reading the panel's, because `Tab` can retarget the panel while
  the menu is open. A target that stops existing closes the menu.
- **A pick** calls `IStrategyHUDCommands::RequestTargetAction(Target, ActionId)` - the target is
  passed explicitly, the panel passing its own and the menu its own. The controller rebuilds the
  row, refuses if the entry has gone grey, and otherwise runs Examine, sends the attack, or issues
  the order.
- **Examine** (`UExamineWidget`, `WBP_Examine`) is a `UWindowWidget`, client-side, kept after it
  closes and rebound on the next look. The body is `ISmoresInteractable::GetExamineText`: a unit's
  definition description, a person's backstory and its condition in words (*Unhurt*, *Hurt*,
  *Badly hurt*, *Down, but still breathing*, *Dead*); a container's or door's `ExamineText`; an item
  definition's description. Empty falls back to *"Nothing remarkable about it."*

## Routes Into the Same Behaviour

| Route | What it asks for |
|---|---|
| Right-click menu row | That row's action on the menu's target |
| Target panel button | That button's action on the panel's target |
| `T` | Talk on the targeted NPC. Still Goodbye during a conversation, and still closes an open trade/container window |
| `O` | Loot on a targeted container or body, Open/Close on a targeted door. With none targeted, today's sweep for a container someone selected is standing at, then a targeted body in reach, then *Too far away* - the one place a key still raises it |
| Double-click | Item → Pick up, container → Loot, door → Open/Close, body → Loot, standing NPC → Talk (a creature just gets targeted). A squad member or empty ground → select all on screen |
| `H` | Unchanged - attack the targeted NPC with everyone selected |

## C++ Implementation

| Piece | Where |
|---|---|
| `ISmoresInteractable` - name, reach, examine text; `IInventoryHolder` derives from it | `SmoresCore/SmoresInteractable.h` |
| `ESmoresRefusalReason::NoOneSelected`, `CannotReach` | `SmoresCore/SmoresRefusalReason.h`, worded in `URefusalWidget::GetRefusalText` |
| `ECharacterKind`, `UCharacterDefinition::Kind` | `SmoresCharacters/CharacterDefinition.h` |
| `AStrategyUnit::IsPerson`, `GetAttributes`, `MoveToActor`, `StopActionApproach`, `TakeOverForActionOrder`, `GetExamineText`; replicated `Attributes` and `Disposition` | `SmoresCharacters/StrategyUnit.*` |
| `UActionOrderComponent`, `IActionOrderHost`, `EActionOrderEnd` | `SmoresCharacters/ActionOrderComponent.*`, `ActionOrderHost.h` |
| `AWorldDoor` | `SmoresItems/WorldDoor.*` - see below |
| `FTargetAction::ActorName`/`Detail`, `FStrategyTargetInfo::Target`/`ActorName`/`DrawsIdenticallyTo`, the action ids | `SmoresUI/StrategyTargetInfo.*` |
| `UActionMenuWidget`, `UExamineWidget`, `UTargetActionWidget::DetailText`, the HUD's menu layer and `IsCursorOverHUD` | `SmoresUI` |
| The rules: `FStrategyTargetActions` (`BuildTargetInfo`, `FindActionFor`, `ResolveActor`, `GetAttackers`, `ValidateActionOrder`, `IsOrderAction`, `IsPlaceholderAction`, the odds, and the gating predicates `IsLootableNPC`/`IsInteractableNPC`/`GetTraderStock`/`IsInRangeOfUnits`) | `smores/Variant_Strategy/StrategyTargetActions.*` - moved out of the controller, which only calls them |
| The routing, hover, dispatch and `IActionOrderHost` | `AStrategyPlayerController` |

**The door.** `AWorldDoor` (`SmoresItems`, abstract; `BP_WorldDoor`) - a root at the middle of the
doorway, an optional frame mesh, a `HingePivot` holding the `LeafMesh`, a 300 cm reach sphere,
`ReplicatedUsing = OnRep_Open bool bOpen`, authority-only `SetOpen`, `OpenYaw` (90), `DoorDisplayName`,
`ExamineText`, and `BP_DoorOpened` / `BP_DoorClosed` hooks for sound and animation. The swing is
instant. The leaf's own collision cuts the navmesh while shut, and the dynamic runtime navmesh
rebuilds as it swings. It lives in `SmoresItems` beside `AStrategyContainer` for now - **flag it to
move** when a world or building module is cut; it isn't an item.

## Blueprint / Asset Dependencies

- `WBP_ActionMenu` (`UActionMenuWidget`): a full-screen `MenuCanvas`, `MenuPanel` (a border, Size To
  Content) holding `NameText`, `ClassificationText`, `ActorText` and the `EntryBox` vertical box.
  Class default `EntryWidgetClass` = `WBP_TargetAction`. Assigned to `BP_StrategyHUD`'s
  `ActionMenuWidgetClass`. Either one unset is warned about once.
- `WBP_TargetAction` gained `DetailText`, after `ReasonText`.
- `WBP_Examine` (`UExamineWidget`, made from `WBP_HelpPanel`'s chrome): `TitleText`, `CloseButton`,
  `TitleBarDragHandle`, `ResizeHandle`, `ClassificationText`, `BodyText`. Assigned to
  `BP_StrategyPlayerController`'s `ExamineWidgetClass`.
- `M_HoverRim` (`Content/Variant_Strategy/Materials/`) - additive, unlit, fresnel x `RimColor` x
  `Intensity`; flagged for skeletal meshes. `BP_StrategyPlayerController`'s `HoverOverlayMaterial`.
- `BP_WorldDoor` - an engine cube leaf, 120 wide, 220 tall, hinged at one edge.
- `LVL_Strategy`, outliner folder `ActionMenuTest`: a small storeroom (four cube walls with a
  doorway on the south side) at about (550, 2650), `Storeroom Door` in the doorway and
  `Storeroom Chest` inside - so a shut door visibly blocks the walk to the chest.
- `DA_Character_SettlerNimble` - a copy of the Settler with Agility 16, set on Pawn 2.

## Extension Points

- **Replacing a placeholder** (Heal, Kidnap, Pickpocket or Knock out): take its line out of
  `FStrategyTargetActions::IsPlaceholderAction`, give it a branch in
  `AStrategyPlayerController::PerformAction`, and - for the ones with odds - make the roll come
  from `GetPlaceholderSuccessChance`'s successor, the same function the menu displays, seeded per
  the save-scum rule (`UWorldSeedComponent`). The menu itself doesn't change.
- **A new kind of target**: implement `ISmoresInteractable`, add its row to
  `StrategyTargetActions_BuildOffered` and to the table above, and make it hoverable
  (`IsHoverable`, and `GetHoverMeshes` if only part of it should be aimed at).
- **A new action on an existing target**: an id in `StrategyTargetAction`, an entry in
  `StrategyTargetActions_BuildOffered`, and an arrival branch in `PerformAction` - or, if nobody
  walks over to do it, an `IsOrderAction` exclusion and a branch in `RequestTargetAction`.
- **AI orders**: an NPC's `UActionOrderComponent` works the same way, but finds no host today
  (`GetHost` looks for a player's controller). A job queue (`orders-and-jobs.md`) is the obvious
  thing for the component to grow into.

## Known Gaps

- **Esc doesn't close the menu.** Esc is PIE's own stop key, and closing things with it wants one
  "back" stack (menu → window → system menu), designed once. Click-away closes the menu.
- **No cursor change** alongside the hover (a hand over a chest, a sword over an enemy) - the other
  half of the RTS convention.
- **Hover picking uses mesh bounds, which are boxes.** A unit's box is a little generous around its
  arms; tune `HoverPickRadius` and look before tightening anything.
- **Doors:** no locks, and units never open doors on their own - a shut door is a wall to
  pathfinding until someone is ordered to open it. A door's state doesn't survive a save (there is
  no save system). A unit standing in the doorway when it shuts is not handled.
- **A single click keeps its own 250 cm sweep** rather than sharing the hover resolver, so what is
  lit isn't always what a left click picks. Deliberate for now - it exists for picking small moving
  units - and worth revisiting if it ever confuses.
- **The right-click that closes the menu doesn't open a new one** at the new spot; it is swallowed,
  which is the simplest and most predictable answer. A feel call if it grates.
- **The target panel's row is horizontal**, and a standing person now offers up to seven actions.
- **Descriptions are designer notes today.** The character definitions' `Description` fields still
  read as notes to designers, and Examine shows them.
- **A unit with no name** is called *Someone* in a row; the feed shows no source for it.
- Touch has no right mouse button; touch is incidental to the control scheme.
