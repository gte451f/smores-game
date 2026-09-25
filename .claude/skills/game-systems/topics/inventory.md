# Inventory System

## Purpose

`UInventoryComponent` gives any Actor a 2D cell grid of item storage, server-authoritative
and replicated, with a shared drag-and-drop UI for moving items within one inventory or
between two. The same component and UI back three different holders today: a squad unit's
own carried items, a world container's contents, and a body's loot.

What an item *is* lives in a shared `UItemDefinition` data asset; what a carried copy *is
like* lives in the `FInventoryItem` instance that references it; *where that copy sits* in a
particular holder's grid lives in the `FInventoryEntry` placement that wraps it.

What a copy is *made of* lives in a third kind of asset: a `UItemModifierDefinition`. A copy
carries at most one material and one quality, and they multiply the item's weight and value and
compose its name — so "Masterwork Bronze Spear" is one item definition plus two small modifier
assets rather than a twenty-fourth hand-authored variant.

Items occupy a rectangular footprint of cells rather than one uniform slot, may be rotated 90°
to fit, and merge into stacks capped per holder. Carried weight is tracked and displayed
alongside the grid but still applies no penalty. The player's gold balance lives in a
`UWalletComponent` on their `AStrategyPlayerState`, and as of the trade system it is finally
spent and earned: a trader is a `UTraderComponent` on an NPC, and dragging an item across that
counter is the same drag-and-drop move every other transfer is, with a price attached.

A full grid can also be **tidied and read**: a sort button repacks a holder's contents by weight,
value or quantity on the server, and a category dropdown dims everything that isn't the kind of
thing the player is looking for. The two are deliberately different animals — one is a change to
the world that every player sees, the other is a change to one player's view that never leaves
their machine.

Alongside the carried grid, every unit has a **paperdoll**: `UEquipmentComponent`, a small set
of named worn slots that is deliberately *not* a region of the grid. An item goes in only if
its definition says that's its slot; nothing else gates an equip.

A container's contents come from two places that coexist: items authored one by one on that
chest, and an optional **loot table** rolled on top of them when the level starts. The roll is
seeded from the campaign's world seed plus the chest's own authored key, so the same chest always
holds the same thing - see `game-data.md`'s "Weighted Tables and Seeded Rolls" for the table side.

Items also exist *outside* any grid: an `AWorldItem` is a single item instance lying on the
ground, drawn with its definition's 3D mesh and collected by double-clicking it with a pawn in
range. It is the one holder shape with no grid and no window behind it. This system does
**not** yet handle theft. See `Docs/roadmaps/inventory-roadmap.md` for the target design — this topic only
documents what's actually built.

## Player Surface

- Press the inventory key (`IA_Strategy_Inventory`) with **exactly one** player-controlled
  unit selected to open that unit's inventory window; pressing it again closes the window.
  Selecting more than one player unit and pressing the key does nothing. Cycling to a
  different pawn while a window is open closes the (now stale) window rather than switching
  it to the new pawn.
- Press the container key (`IA_Strategy_ToggleContainer`) to open the nearest world
  container within interaction range of a selected unit; pressing it again closes it. If no
  container is in range, the same key opens a body's loot instead — a Downed NPC or a dead
  one, never an NPC still on its feet, and never a player-controlled unit. The window title
  says which ("Bandit (Downed)" vs. "Bandit (Dead)"), but the two loot identically.
- Double-clicking a container or a body in the world selects and highlights it, and
  opens it immediately if **any** player-controlled pawn (not just the current selection) is
  within interaction range. This rides on the same gesture as the normal select-all
  double-click, not a separate input.
- **A chest can hold rolled contents** - junk, minerals, a sword that may be iron or steel -
  rather than only what a designer placed in it by hand. The same chest holds the same thing every
  time the level starts in the same campaign, so there's nothing to gain by reloading to "re-roll"
  it; two chests using the same table hold different things. A chest can mix both: the two chests
  in `LVL_Strategy` each keep their hand-placed items and roll a table on top.
- Opening a container or loot target also opens the nearest player pawn's own inventory
  window alongside it, so both panels are visible at once for drag-and-drop transfer between
  them.
- Each inventory/container window is a floating panel (optional title bar, drag-to-move,
  resize-from-corner, close button) showing that holder's grid — dragging an item onto
  another cell moves it there, whether that's repacking within one panel or transferring
  between two different ones. Dropping onto a matching stackable item merges the two; a drop
  that doesn't fit, or lands on something that can't stack, is rejected and the item stays
  where it was. There is no swap: an item never displaces another by landing on it.
- Each placed item draws as **one bordered region spanning its whole footprint**, with its
  label centred in it — and turned 90° when the footprint is taller than it is wide, so a
  1×3 sword's name reads down the blade rather than spilling across its neighbours.
- **Picking an item up grabs it where you clicked.** The floating ghost keeps that grip, so a
  sword grabbed by its tip lands with its tip where you dropped it, rather than jumping to put
  its top-left corner under the cursor.
- **Press the rotate key while dragging** (R by default, and player-rebindable like every other
  binding) to turn the held item 90°. The ghost turns with it, as does the drop preview.
  Rotating a square item does nothing — a square turns into itself.
- While a drag hovers a grid, **the cells it would claim light up green or red**: green means
  the drop will be accepted, red that it won't (no room, or a stack it can't merge into). That
  preview is the only thing that tells the player whether the rotate key helped.
- **A drop is literal.** An item that doesn't fit is rejected, never quietly turned sideways to
  make it fit — auto-placement still tries both orientations on its own, but that's a different
  path, and turning an item the player didn't ask to turn works against the deliberate packing
  this design is built around.
- **An item's name says what it's made of and how well it was made.** A copy carrying a material
  or a quality reads as "Bronze Sword" or "Masterwork Bronze Sword" rather than plain "Sword",
  and weighs and prices accordingly — a steel sword is worth over twice an iron one and a
  masterwork steel one over six times. Nothing in the window says "this item has modifiers"; the
  name, the weight and the price *are* the telling.
- **A material also colours the item.** With no item icons authored yet the tint lands on the
  item's label — bronze reads warm orange-brown, steel cool near-white. Quality deliberately
  adds no colour of its own, so a masterwork bronze item still looks bronze.
- **Items of different materials don't stack together.** Two bronze swords are one pile; a
  bronze and a steel one are two, even sorted. That's the same rule as stolen goods not merging
  with honest ones, and for the same reason — one pile can only quote one price.
- Every inventory window shows a **carried-weight readout** above its grid — `Weight: 12.4 /
  30.0` for a pawn, and just `Weight: 8.0` for a chest, which has no capacity of its own. It
  updates live as items move in, out, and between windows. Exceeding the capacity turns the
  readout red and does **nothing else**: the drop still lands, the transfer still completes,
  and the pawn moves exactly as fast as before.
- The HUD carries a **gold readout** ("Gold: 250") next to the selection count. Gold isn't an
  item — it has no weight, no footprint, and never appears in a grid — and it updates live as a
  trade settles, which is the only feedback a purchase gives.
- **Every inventory window has three sort buttons — Weight, Value, Qty — above its grid.**
  Pressing one repacks that holder's contents from the top-left with no gaps, biggest figure
  first: heaviest stack, most valuable stack, or biggest stack. It works on a chest, a corpse and
  a trader's shelf exactly as it does on a pawn's pack.
- **Sorting also merges stacks.** Two half-stacks of the same thing become one, exactly as though
  the player had dragged one onto the other — so a sorted grid never leaves two piles of apples
  sitting side by side. Stolen goods still won't merge with honest ones.
- **Sorting re-picks each item's orientation**, so a sword the player had turned sideways may
  come back upright. That's what a repack is: the player asked for the grid to be tidied, not for
  their arrangement to be preserved.
- **A sort that can't fit everything does nothing at all.** In rare cases packing biggest-first
  can strand an item the old arrangement had room for; rather than dropping it, the whole sort is
  abandoned and the grid is left exactly as it was. Like every other refusal in this system it is
  silent — the grid simply doesn't move.
- **A category dropdown beside the sort buttons dims everything that isn't that kind of item** —
  weapons, armor, consumables, materials, tools, valuables, misc, or "All" for no filter. Dimmed
  items are still fully there: they can be dragged, right-clicked and hovered as usual, and they
  still occupy their cells. The filter is a reading aid for a full grid, not a way to put things
  away.
- **The filter is personal and temporary.** It affects only the player who set it — another
  player looking into the same chest sees their own filter — and it clears whenever the window
  rebinds to a different holder, so a filter set on a chest never comes back to hide half of a
  pawn's pack.
- The **inventory key** also opens that pawn's **Equipment window** beside the pack — a second
  floating panel listing its five worn slots (Main Hand, Off Hand, Head, Body, Feet) and the
  combined weight of what's in them. The two open and close together and are moved and resized
  independently. Only the inventory key brings it up: a pack opened as a transfer partner for a
  container or a corpse comes on its own, and closes any paperdoll already showing — that pack
  may have just rebound to the nearest pawn rather than the selected one, which would leave the
  paperdoll describing somebody else.
- **Right-click a carried item to wear it.** It goes into whichever slot its definition names,
  and whatever was already in that slot comes back to the grid. If the grid has no room for the
  displaced item, nothing happens at all — the swap never half-lands. Right-clicking an item
  that isn't wearable does nothing, and so does right-clicking inside a chest or a body's
  loot panel: only a pawn's own inventory window equips.
- **Dragging a carried item onto a paperdoll slot** does the same thing, and the slot lights up
  green or red on hover exactly like a grid cell does — red for the wrong kind of item, or for
  a swap whose displaced item wouldn't fit back in the grid.
- **Right-click a filled paperdoll slot to take the item off.** It returns to that pawn's own
  pack, never to whatever other window happens to be open. A full pack means it stays worn.
- **Wearing one out of a stack takes one.** Equipping from a stack of five knives leaves four
  in the grid, so one stack can arm several pawns.
- A worn item **leaves the carried grid**, so the pawn's `Weight:` readout drops when something
  is equipped; the paperdoll window reports the worn weight separately. Neither figure does
  anything yet.
- **Clicks no longer fall through an open window** — any mouse button, single or double.
  Previously only a single left-click was consumed, so right-clicking a window also marched the
  squad to whatever was behind it, and a *fast second* click of any button leaked through even
  after the first was caught.
- **Double-click a loose item lying in the world to pick it up.** It goes into the grid of
  whichever player pawn is nearest and close enough, trying both orientations to find room, and
  the item disappears from the ground. No pawn in range means nothing happens at all — there's
  no "walk over and get it" order, and no auto-pickup radius: the player has to have somebody
  standing there already.
- A loose item has to be clicked **more precisely than a chest** — a tighter click radius, so an
  apple lying beside a chest doesn't swallow every double-click meant for the chest. As with a
  chest or a corpse, a double-click that lands on one means *that item*, so it never falls
  through to the select-all-on-screen gesture even when the pickup fails.
- **A pickup that only partly fits takes what fits.** Double-clicking a pile of 20 apples with
  room for 8 leaves 12 on the ground rather than refusing the whole pile or quietly destroying
  the rest. A grid with no room at all leaves the pile untouched.
- Loose items show their **3D mesh**, not their inventory icon. Every item type currently points
  at the same placeholder — a plain black 100-unit sphere, about half a pawn's height — so items
  on the ground are visible and clickable but tell each other apart only by position.
- **Double-click a living NPC to interact with them.** If some player pawn is close enough they
  say something (a greeting bark, or a "nothing to say" one - `dialog.md`), and if they are a
  trader their wares open beside that pawn's pack. Out of reach is a *Too far away* refusal.
  Either way the double-click means *that person* — it never falls through to selecting everyone
  on screen, the same as double-clicking a chest or a body already does. Double-clicking **empty
  ground** still selects all on screen.
- **Press the talk key (`T`) to trade with the NPC you have targeted**, provided a selected unit
  is close enough. Same shape as `H` for attacking, reading the same targeted NPC, and it's the
  accessible alternative to the double-click; pressing it again closes the window.
- **A hostile, unconscious or dead NPC never trades.** A hostile one does nothing at all on a
  double-click; a fallen one opens as loot instead.
- **Buying is dragging an item out of the trader's window into a pawn's pack, and selling is
  dragging one the other way.** There is no buy button and no confirmation step — a trade is the
  same drag every other transfer uses, and the money is settled as the item lands.
- **Hovering an item in either window quotes its price**: "Buy: 135 gold" in the trader's panel,
  "Sell: 45 gold" in the pawn's pack open beside it. A stack quotes the per-unit figure and the
  total together. Anywhere else — a chest, a corpse, a pack with no trader across from it —
  hovering an item says nothing, because nobody is offering anything for it.
- **Not being able to afford something changes nothing at all**, and says "Not enough gold" at
  the cursor. The item stays on the shelf and the balance is untouched. A stack is priced whole,
  so one the player can only partly afford is refused outright rather than sold short — there is
  no way yet to ask for fewer.
- **A refused action says why**, in a line at the cursor that fades after a couple of seconds:
  "Too far away" for a chest, body or loose item nobody is near, "No room for that" for a pack
  that can't take it, "Can't be worn there" for the wrong slot. Reaching for something out of
  range used to do nothing at all. The full set of messages, and the rule about when a refusal
  should be *prevented* rather than announced, is in `refusals-and-feedback.md`.
- **A single click on an NPC only targets it now.** It used to also launch a squad attack when
  that NPC was already hostile, which made one click mean two different things depending on the
  target's mood — and would have made double-clicking a hostile trader open their shop and start
  a fight at once. `H` attacks the target.
- Deselecting all units, or having no player pawn selected/in range, closes any open
  inventory window — and the equipment window with it.

## Core Rules

- **Item definitions are shared; item instances are per-copy; placements are per-holder.** A
  `UItemDefinition` (a `USmoresDefinition`, one asset per item type under `Content/Items/`)
  carries everything every copy of that item has in common — the stable `DefinitionId`, display
  name and description it inherits from the shared definition base (see `game-data.md`), plus
  the item-specific `EItemCategory`, 2D `Icon`, 3D `WorldMesh`, `Weight`, `BaseValue`,
  `FootprintWidth`/`FootprintHeight`, `MaxStackSize`, `EEquipSlot`. An `FInventoryItem`
  carries only what varies copy-to-copy: a `Definition` pointer plus `Quantity`, `Condition`,
  `bStolen`, and a `Modifiers` array. An `FInventoryEntry` wraps one `FInventoryItem` with where
  it sits in *this* holder: a stable `EntryId`, an `AnchorCell`, and a `bRotated` flag. All
  display data is read through the accessors on the instance; nothing is duplicated per instance.
- **Material and quality are one mechanism, and it is a multiplier — not an asset per
  combination.** A `UItemModifierDefinition` (in `SmoresItems`, one asset per material or
  quality under `Content/Items/Modifiers/`) holds a `Slot` (`EItemModifierSlot::Material` or
  `::Quality`), a `WeightMultiplier`, a `ValueMultiplier`, a `ConditionMultiplier`, a `Tint`, and
  a `NamePattern`. An `FInventoryItem` holds **at most one modifier per slot**, and the
  multipliers compose by multiplication.

  The alternative — one item definition per combination — is a trap: three materials across
  eight weapon shapes is twenty-four assets, adding a material means authoring eight more, and
  every future recipe becomes combinatorial with them. It is also what makes crafting authorable
  later: material is inherited from a recipe's inputs and quality is chosen at craft time, so
  there is one recipe per shape rather than one per combination.
- **Every derived figure is read off the instance, never off the definition.**
  `GetUnitWeight()`, `GetUnitBaseValue()`, `GetTotalWeight()`, `GetTotalBaseValue()`,
  `GetDisplayName()` and `GetTint()` all multiply or compose through the modifier chain;
  `GetCategory()`, `GetEquipSlot()`, `GetWorldMesh()`, `GetIcon()` and `GetBaseMaxStackSize()`
  pass straight through. **`Item.Definition->Weight` is the figure for a bare, unmodified one of
  these, not what this copy weighs** — which is why the four call sites that used to reach past
  the accessors (the trader's two price calls, the equip-slot lookup and the world mesh) now go
  through them, and why `FInventoryItem` is now exported (`SMORESITEMS_API`) so the bodies can
  live in the `.cpp`.
- **Value rounds per unit, not per stack.** `GetUnitBaseValue()` rounds the multiplied figure
  and `GetTotalBaseValue()` multiplies that by quantity, so a trader's per-unit price and its
  line total can never disagree. A priced item never multiplies down to worthless (floor of 1),
  and an item authored at zero stays worth nothing however good its material.
- **Names compose through `FText::Format`, in slot order, never by concatenation.** Each
  modifier's `NamePattern` takes `{Modifier}` and `{Item}`, and material applies before quality —
  so the result reads "Masterwork Bronze Spear" whichever order the modifiers were added in. The
  pattern lives on the modifier because word order differs by language and a translator has to be
  able to reorder it. A modifier whose `NamePattern` was left blank falls back to English
  `"{Modifier} {Item}"` rather than dropping silently from the name; the content sweep treats an
  empty pattern as an authoring error for exactly that reason.
- **Tints multiply, so White is genuinely neutral.** `GetTint()` is the product of every
  modifier's `Tint`, which is what lets a quality that isn't about colour leave the material's
  colour alone. With no item icons authored yet it lands on the item label in the grid; move it
  to the icon the moment there is one. An unmodified item tints White and draws exactly as before.
- **Modifiers deliberately do not scale stack size.** A masterwork arrow is still an arrow.
- **Storage is a `GridWidth` × `GridHeight` cell grid** (both clamped 1–32, defaulting to
  8×8 and sized per holder type in Blueprint — a pawn's pack is 6×4, a chest 8×6). `Entries`
  holds only the items actually placed, in no particular order; there is no per-cell array and
  no "empty slot" object. A cell is free exactly when no entry's footprint covers it.
- **An item occupies a rectangular footprint**, `FootprintWidth` × `FootprintHeight` from its
  definition, which is the game's stand-in for bulk/volume — deliberately independent of
  `Weight`. `bRotated` swaps those two dimensions; that single 90° turn is the only rotation
  there is. Placement validates against overlap: no two footprints may share a cell, and the
  whole footprint (not just the anchor) must lie inside the grid.
- **`EntryId` is the handle callers use**, not an array index — it survives other entries
  being added or removed, which matters because the UI holds a reference across a
  client→server round trip. Ids come from a server-side `NextEntryId` counter and are unique
  per holder, not globally.
- **Stacking is capped at the definition's `MaxStackSize` × the holder's `StackMultiplier`**
  (`GetEffectiveMaxStackForItem`, never below 1). One number per holder covers "a shelf stacks
  deeper than a backpack" without per-transfer special cases; it's 1.0 everywhere today. Two
  entries merge only if `FInventoryItem::CanStackWith` agrees: same definition, the definition is
  actually stackable (`MaxStackSize > 1`), both carry the same `bStolen` flag — so theft can't be
  laundered by merging — and **both carry the same set of modifiers**. Two bronze spears stack; a
  bronze and a masterwork bronze do not, because a merged stack could only report one weight and
  one price for both. That comparison is per slot rather than per array position, so it is
  order-independent: the array order is an accident of how a copy was built and a player can't
  see it, and a stack that split on it would fragment for no visible reason. `Condition` is
  deliberately *not* compared, since splitting a bulk-material stack per wear value would
  fragment it uselessly.
- **A sort is a change to the world; a filter is a change to one screen — and they are built
  completely differently because of it.** `SortEntries` is an authority-only mutator reached
  through `IInventoryMoveHost::Server_SortInventory`, replicating like any other grid change, so
  every player watching that chest sees it repack. The category filter never leaves the client
  that set it: no RPC, no authority check, nothing replicated, because nothing about the holder
  changed. Deciding which of the two a new inventory feature is, before writing it, is what keeps
  view state off the wire and world state off the client.
- **A repack is all-or-nothing, and that is not paranoia.** It merges, sorts, and re-places into a
  *scratch* array, committing only once every entry has landed. First-fit packing in criterion
  order really can strand an item the previous arrangement had room for — a 1×1 taking the corner
  a 2×2 needed — and a tidy-up button that sometimes eats an item would be far worse than one
  that occasionally declines to run. Items are ordered biggest-footprint-first among equals for
  the same reason: it makes that failure rarer.
- **Sorting merges stacks, and introduces no merging rule of its own.** It pours each later stack
  into the earliest one that will take it, using the same `CanStackWith` and the same
  `GetEffectiveMaxStack` `AddItem` already uses — so the stolen flag still blocks a merge,
  `Condition` is still ignored, and nothing a sort produces could not have been produced by the
  player dragging stacks together by hand.
- **The sort order is fully determined by the contents**, never by the array order replication
  happened to leave: criterion descending, then footprint area descending, then display name, and
  finally entry id. `TArray::Sort` is not stable, so without that last tiebreak two otherwise
  equal entries could swap places on a repack that changed nothing else — which would make
  "sorted" a state the grid never settles into. Keys are compared exactly rather than with a
  tolerance, because a comparator that calls near values equal isn't a strict ordering and `Sort`
  is entitled to misbehave on one that isn't.
- **A filtered-out item is dimmed, never hidden.** Hiding the item widget would expose the empty
  cell layer beneath it, so the player would read occupied cells as free space, try to drop
  something there, and be refused with nothing on screen to explain it. A filter may de-emphasise
  what the grid holds; it may not lie about it. Dimmed items stay fully interactive for the same
  reason.
- **`EItemCategory::None` is the filter's "show everything" row**, which is why nothing should
  author a definition's `Category` as `None` — `Misc` is the catch-all. An item authored as
  `None` would be visible only while no filter is set.
- **Weight and footprint are two independent measures, and are meant to disagree.** Footprint
  is bulk/volume — how much *space* an item claims. Weight is density — `Definition->Weight` ×
  quantity, summed over every placed entry by `GetTotalWeight`, with no relation to how many
  cells those entries cover. A bundle of cloth stresses the grid; an ingot stresses the scale.
- **`WeightCapacity` is per holder and deliberately inert.** It's the denominator of the
  window's readout and nothing else — `IsOverWeightCapacity` is consulted only to colour that
  text. No mutator consults it, so being over capacity never blocks a placement, a transfer or
  a pickup. The movement-speed and stealth-noise penalties this figure eventually feeds belong
  to a later characters/combat pass, which owns those effects. A capacity of 0 means *no limit*
  (`HasWeightLimit`), which is what a static holder like a chest wants — nothing static carries
  anything anywhere.
- **Currency is per-player, not per-pawn, and lives outside the inventory entirely.** The
  balance is a replicated `int32` inside a `UWalletComponent` hosted on `AStrategyPlayerState`,
  not an `FInventoryItem` — it has no weight, no footprint, and occupies no cell. Every
  sub-squad/division under one player draws from the same balance regardless of where in the
  world it is, because divisions are an organization layer rather than a separate economy.
  `AddGold` credits and `TrySpendGold` debits-if-affordable, both authority-only.
- **It is a component rather than a field on the player state, and that is a general rule, not
  a preference about gold.** `APlayerState` is Unreal's composition root for per-player data,
  and the way it becomes an 800-line junk drawer is fields being added inline because no feature
  module obviously owns them yet — which is exactly how gold started. Faction standing has since
  followed (`UPlayerStandingComponent`, see `factions.md`); squad roster and research progress
  are queued for the same class and each want a component of their own. See `unreal-module-organization.md`'s "Framework Classes vs. Feature Modules". The
  bonus the move actually paid: with the balance in `SmoresEconomy`, a module `SmoresUI` already
  depends on, `AStrategyHUD` reads it through
  `PlayerState->FindComponentByClass<UWalletComponent>()` and the `IStrategyResourceHost`
  interface that existed only to reach across that module boundary was deleted outright.
- **A trader is a component on a character, and its presence is the only "is this a trader?"
  flag there is.** `UTraderComponent` carries one NPC's wares and their prices; an NPC without
  one simply isn't a merchant. There is no `bIsTrader` bool that could disagree with the stock,
  no merchant flagged as one with nothing to sell and no stocked NPC who refuses to deal — the
  same single-source-of-truth reasoning `IInventoryHolder` was built on. It is a component
  rather than a shop *building* because `economy.md` wants travelling caravans: a caravan is a
  trader that walks, and as a component an NPC gets trading for free where a building would need
  the whole thing implemented a second time.
- **`UTraderComponent` *is* a `UInventoryComponent`.** A shop shelf is a grid, so the stock
  replicates, opens into the ordinary inventory window, and is dragged in and out of through the
  existing `MoveItem` — no second storage model and no second UI. It defaults to a chest's
  shape: `WeightCapacity` 0 (nothing static carries anything anywhere) and a `StackMultiplier`
  of 2, since a shelf stacks deeper than a backpack.
- **A trader's wares are not the NPC's pockets.** The stock lives on the trader component; that
  same NPC's `AStrategyUnit::Inventory` is its personal belongings and is never for sale. Only
  the wares open when trading; only the belongings open when looting the body.
- **A trade is an ordinary move with gating in front of it, never a fourth resolution inside
  `MoveItem`.** The drag-and-drop UI is completely unaware trading exists: it sends the same
  `Server_MoveInventoryItem` it sends for a chest. What makes it a transaction is recognised
  server-side, where the money is — exactly one end of the move being a `UTraderComponent` means
  gold has to change hands too. Source and destination being the *same* component is excluded
  deliberately: that's somebody repacking one shelf, and nothing is bought by moving an item
  within a single grid.
- **The transaction is all-or-nothing, and the ordering is what makes it so.** A purchase is
  priced against the **whole requested quantity** and refused up front if the balance won't cover
  it. Only then does the item move; the debit afterwards is for the quantity that actually moved,
  which can only be the same or less — so the debit can never fail once the goods have changed
  hands. Pricing the request rather than the result is also what makes "insufficient gold changes
  nothing" literally true: a player who can't cover a full stack is refused rather than quietly
  sold a smaller pile, since there is no way yet for them to ask for one. A sale is the mirror
  and needs no pre-check: the item moves first, and the credit is for what moved.
- **Prices come from an interface, not from the trader's own numbers being read directly.**
  `IPricingProvider` promises a per-unit buy price and a per-unit sell price; `UTraderComponent`
  implements it as the definition's `BaseValue` times a flat `BuyMarkup` or `SellMarkdown`. The
  real market simulation `economy.md` calls for — regional supply and demand, emergent prices,
  trade routes — belongs to a much later `SmoresMarkets` and implements this from above without
  the transaction or the UI changing. Totals are derived (per-unit × quantity) rather than
  virtual, so a bulk discount would be a change to the interface rather than something each
  caller invented separately. Prices take the whole `FInventoryItem` rather than just its
  definition, so a worn item fetching less (`Condition`) or a fence paying less for stolen goods
  (`bStolen`) needs no signature change.
- **A price is a property of the window, not of the item.** The same apple quotes a buy price in
  a trader's panel, a sell price in the pack open beside it, and nothing at all in a chest.
  Whoever opens a window decides which side of the counter it is on; the item widget only asks.
  Identical routing to right-click-to-equip, and for the same reason — resolving it off the
  item's holder instead would have a pawn's pack quoting prices with no trader in sight.
- **Proximity and hostility are re-checked server-side on every transaction.** `MoveItem` has no
  idea how far away the asking pawn was, or what the counterparty currently thinks of it — the
  same reason `Server_PickUpWorldItem` re-checks its own gate. Reach itself needed no new code:
  `AStrategyUnit` already implements `IInventoryHolder`.
- **Worn slots are a paperdoll, not a region of the grid.** `UEquipmentComponent` holds an
  `FEquippedItem` per *occupied* slot — a slot name plus one `FInventoryItem`, with no anchor
  cell and no rotation, because a slot is a named place rather than a rectangle. An absent slot
  is an empty one; there is no "empty slot" object, the same way `Entries` has no empty cells.
  A worn item is always quantity 1.
- **Slot-type matching is the only equip gate there is.** `CanEquipItem` is one comparison:
  the item's `Definition->EquipSlot` against the slot being filled, with `EEquipSlot::None`
  meaning "not wearable at all". No skill, attribute or condition check belongs here — any pawn
  may wear any weapon regardless of training, and the performance consequence of an untrained
  equip is combat/skill resolution's business (`combat.md`'s weapon-class mismatch penalty),
  never an inventory-side restriction.
- **`Equip` and `Unequip` are all-or-nothing.** Both find the displaced item a home with
  `FindFreePlacement` *before* mutating anything, and bail out having changed nothing if there
  isn't one — so running out of grid room can never destroy an item. `Equip` splits one unit off
  the source entry (removing it outright when that empties it) rather than moving the whole
  stack, so a stack of five knives arms five pawns. When the whole entry *is* consumed, the
  displaced item may be placed back onto the cells that entry is vacating; when it isn't, it
  can't.
- **A slot is filled from `EEquipSlot::None` or from a named slot, and both end up in the same
  place.** `None` means "wherever this item belongs", which is what right-click-to-equip sends;
  a drop on a specific paperdoll slot names that slot and still has to match it. There is no
  path that forces an item into a slot it doesn't belong in.
- **Equipped weight is tracked separately and folded into nothing.** A worn item leaves the grid,
  so `UInventoryComponent::GetTotalWeight` no longer counts it and
  `UEquipmentComponent::GetTotalWeight` does. Harmless while weight is inert; the pass that
  gives weight a consequence has to sum both.
- **A world pickup is one item instance, not a holder.** `AWorldItem` carries a single
  `FInventoryItem` and no `UInventoryComponent` at all — deliberately *not* an
  `AStrategyContainer`, which owns a whole grid and opens into a window. The whole interaction is
  "double-click it and it's yours", so there is nothing to open and nothing to drag out of. Its
  mesh is read from the held definition's `WorldMesh` rather than authored on the actor, which is
  what lets one Blueprint subclass serve every item type: contents are set per placed instance
  (or at spawn time), and the mesh follows.
- **`ItemMesh` is the actor's root, and `InteractionRange` hangs off it** — which means the mesh
  can't be scaled without scaling the pickup radius with it (a 0.3 mesh scale silently drops the
  312.5 reach to 94). That's why the placeholder shapes are left at their full 100 units rather
  than shrunk to something daintier. Making world items smaller means giving `AWorldItem` a plain
  `USceneComponent` root with the mesh and sphere as siblings — a C++ change, and one that moves
  the root component out from under already-placed instances, so it isn't free.
- **Pickup is gated by proximity and nothing else**, through the same `InteractionRange` sphere
  and default radius a container uses — the one proximity rule for every transfer
  context. The client picks the nearest in-range pawn, and the server re-checks both that
  proximity and that the named inventory really belongs to an `AStrategyPlayerUnit` before
  touching anything; range is the whole gate, so it can't be left on the requesting machine.
- **A pickup takes what fits and leaves the rest.** `AddItem` is already allowed to place part of
  a stack, so a pickup that read only its bool return would silently destroy the units that
  didn't land. `AddItemCounted` reports the quantity actually taken: the actor is destroyed only
  when the whole stack moved, and otherwise just shrinks. Taking nothing leaves it untouched.
- All mutation (`AddItem`, `AddItemAt`, `RemoveEntry`, `SetEntryQuantity`, `RepositionEntry`,
  `SetGridSize`, `Equip`, `Unequip`, `AWorldItem::SetItem`/`TryPickUp`) is authority-only; called on a non-authority machine, each is a silent no-op.
  `Entries`, `GridWidth`, `GridHeight` and `StackMultiplier` all replicate; `OnRep_Entries`
  re-broadcasts `OnInventoryChanged` on clients (authority already broadcasts it directly from
  the mutators, so it isn't double-fired there). `UEquipmentComponent` is the same shape one
  level down: `EquippedItems` replicates, `OnRep_EquippedItems` re-broadcasts
  `OnEquipmentChanged` on non-authority machines only.
- `AddItem` **merges before it places**: it fills existing stacks of the same type up to the
  effective cap first, then auto-places whatever's left via `FindFreePlacement`, splitting into
  as many entries as the cap requires. It returns `true` only if the *entire* quantity was
  taken — a partial add keeps what fit and warns about the rest. It rejects an item with no
  `Definition`, so a blank `StartingItems` entry places nothing.
- **A set of items goes in whole or not at all, through `AddItemsAllOrNothing`.** Each item merges
  and auto-places exactly as `AddItem` would, but against a scratch copy of the grid committed only
  once every unit has landed, with one broadcast for the lot. It exists for loot: `AddItem` per
  rolled item would keep whatever fit first and silently drop the rest, leaving a chest short by an
  amount that depends on the order the roll came out in. Refusing the whole set turns that into a
  visible failure - a warning, and a chest holding only its authored items. The items are placed
  **biggest footprint first** whatever order they arrive in (stable, so a seeded roll places the
  same way every time), for the repack's reason: first-fit strands a large item far more easily
  than a small one. The bool is honest here in a way `AddItem`'s isn't - there is no partial outcome
  for it to hide.
- **A container's contents are `StartingItems` plus an optional `LootTable`, and the table is not a
  replacement for authoring a chest.** `AStrategyContainer::BeginPlay` (authority only) adds each
  `StartingItems` entry with `AddItem`, as before, then `RollLootTable` rolls the table and adds the
  result all-or-nothing on top. A roll that doesn't fit leaves the chest with its `StartingItems` and
  logs why.
- **The roll is keyed off an authored `PlacedContainerId`, never the actor's name.** Same reasoning
  as `AStrategyUnit::PlacedRecordId`: the runtime name isn't stable across level edits, and a key
  that changed whenever somebody moved a rock would re-roll the chest. Generated in
  `PostActorCreated` when a container is placed in an editor world and regenerated in
  `PostEditImport` when one is pasted or alt-dragged (two chests sharing a key would roll identical
  contents). A container with a table and no key still rolls - from a CRC of its actor name - and
  warns.
- `FindFreePlacement` sweeps the whole grid in the item's natural orientation **before** trying
  the rotated one, so nothing gets turned sideways that didn't need to be; a square footprint
  skips the second pass entirely.
- `SetEntryQuantity` clamps up to the effective stack cap, and a new quantity of 0 or less
  removes the entry outright rather than leaving a zero-count ghost.
- **`MoveItem` never swaps.** It's the static entry point for both repositioning within one
  grid (`SourceInventory == DestInventory`) and transferring between two, and it resolves a
  drop in exactly one of three ways: merge into a stackable entry already covering the
  destination cell; reposition the existing entry (keeping its `EntryId`) when the whole stack
  moves within one grid; or place the item at the destination cell. A drop onto something that
  can't stack, or a footprint that doesn't fit, is **rejected whole** — two differently-shaped
  footprints have no well-defined exchange, so there is deliberately no swap path, and the UI
  simply redraws from unchanged replicated state (the item "snaps back"). All validation runs
  before anything is mutated, so a cross-inventory move can never half-apply; both components'
  authority is checked up front for the same reason.
- `MoveItem`'s `Quantity` parameter takes 0 or less to mean "the whole stack"; a smaller value
  splits it. A split leaves the source entry in place, so the new piece must find room *around*
  it — only a whole-entry move may reuse the cells it currently occupies. The UI always passes
  0 today; partial-stack drags are a later slice.
- This single method is what every current transfer path (reposition, pawn↔container,
  pawn↔loot, pawn↔trader) actually calls.
- **`MoveItem`'s bool return is not enough for a caller that has to do something proportional
  to the move.** A merge only takes what fits under the destination stack's cap and still reports
  success, so "moved" can mean three of the eight that were asked for. `MoveItemCounted` reports
  the quantity that actually changed hands, and `MoveItem` is a one-line forwarder over it — the
  same trap, and the same fix, as `AddItem` vs. `AddItemCounted`. A purchase charges off that
  count, never off the request.
- **"Lootable" and "interactable" are two predicates, each written down exactly once.**
  `IsLootableNPC` is "not a player pawn, and Downed or Dead"; `IsInteractableNPC` is its mirror,
  "not a player pawn, on its feet, and not hostile". The hostility clause lives inside that
  second predicate rather than as a guard at each call site, so dialog inherits the rule when it
  arrives instead of re-deriving it. A double-click finds an NPC *once*, in whatever state it
  happens to be in, and branches on these — rather than sweeping the level twice with two
  different filters that would then have to agree about which NPC was nearer.
- **Every holder answers the same two questions, through `IInventoryHolder`.** A pawn's pack,
  a container and a loose world pickup each implement `GetHolderDisplayName()` (the name shown
  in the window title and the selection label) and `IsInRangeOf(const AActor*)` (may this actor
  transfer items with me). Before the interface each carried its own private copy of both —
  three identical distance tests all named `IsUnitInRange`, one of which took a narrower
  parameter type than the other two, and three differently-named display-name getters. The
  distance test itself is now written once, in `IInventoryHolder::IsActorWithinSphere`, and each
  holder just hands it its own `InteractionRange` sphere, so reach stays per-type (a container's
  and a world item's default to 312.5 units, a unit's to 250) while the rule is single-sourced.
  A unit's was 100 until it proved unreachable in practice: reach is centre-to-centre, the
  capsule radius is 42, so two characters standing in contact are already 84 apart — and
  `MovementAcceptanceRadius` is itself 100, so a pawn ordered to walk to a body parks right on
  the gate. It read as "double-click is unreliable" rather than as a range problem, because a
  sideways step of a few centimetres flipped it without getting visibly closer. Any new holder
  type wants its reach comfortably clear of both numbers, not merely larger than a capsule.
- **The interface deliberately carries no `GetInventory`.** `AWorldItem` holds one
  `FInventoryItem` and no `UInventoryComponent` at all, so a grid accessor would either be
  unimplementable by one of the three implementers or would have to return null from it, leaving
  every caller to remember a check someone eventually won't. Code that needs a grid casts to the
  concrete holder type; only two types have one, so there was no duplication there to collapse.
- **Nor does it carry the player's click radius.** How precisely the player must aim at a thing
  is a property of the input gesture, not of the thing, so `ContainerSelectionRadius` (250) and
  the tighter `WorldItemSelectionRadius` (100) stay on `AStrategyPlayerController` next to the
  other input tuning and are passed into the holder-generic finder.
- **Reach and click precision are separate checks and always have been.** A double-click on a
  chest highlights it from up to `ContainerSelectionRadius` away but only *opens* it if some
  player pawn is inside the chest's own `InteractionRange`. The first is "did the player mean
  this thing", the second is "may they touch it".
- A client-side drag-and-drop widget can't mutate a replicated, authority-only inventory
  directly — the inventory window's drop handler instead calls into `IInventoryMoveHost`
  (implemented by `AStrategyPlayerController`), whose `Server_MoveInventoryItem` RPC is the
  only path that actually calls `UInventoryComponent::MoveItem`.
- **The grid UI is two layers in one `UGridPanel`**: a `UInventoryCellWidget` per cell
  underneath (background and drop-preview highlight only, no item, no mouse handling), and a
  `UInventoryItemWidget` per placed entry above it, spanning its footprint via the grid slot's
  row/column spans. A `UUniformGridPanel` can't host the upper layer at all — `UUniformGridSlot`
  has no span — which is why the panel type is a hard requirement rather than a preference.
- **A window swallows the press of every mouse button that lands on it — including a
  double-click — and lets the release through.** The double-click half is a separate override,
  not a consequence of the press one: Windows sends `WM_xBUTTONDBLCLK` rather than
  `WM_xBUTTONDOWN` for the second click of a rapid pair, which reaches Slate as
  `OnMouseButtonDoubleClick`, an event with its own routing. A widget that overrides only
  `NativeOnMouseButtonDown` therefore catches the first click of a double-click and lets the
  second through to the viewport, where it registers as a press and completes whatever world
  action is bound to that button on release. Every widget in the stack that claims a button
  (`UWindowWidget`, `UInventoryItemWidget`, `UEquipmentSlotWidget`) routes its double-click
  handler straight into its press handler, so a fast second click means exactly what a slow one
  does. The release asymmetry is separate, and also deliberate. Swallowing the press is what stops a right-click on
  an inventory panel from also issuing a move order to the squad. Swallowing the *release* as
  well would be worse than useless: Enhanced Input never saw the press this window ate, so an
  action bound on release (most of them are) has nothing to complete anyway — whereas eating a
  release whose press the viewport *did* see (a drag-select begun on the world and ended over a
  window) would leave that button stuck down in `UPlayerInput` permanently. Slate bubbles up
  from the deepest widget, so the window only ever catches what nothing inside it claimed.
- **A window that closes itself says so.** `UWindowWidget::OnWindowClosed` fires after the
  window has removed itself, and the controller subscribes to every window it spawns. A close
  button can only remove its own widget; it can't take the companions that were opened with it
  (a pawn's paperdoll beside its pack), and it can't drop the input context scoped to a window
  being open. That's what this delegate is for, and it's why both window subclasses call
  `Super::RequestClose_Implementation()` *after* removing themselves.
- **Paperdoll slots handle their own drops, one per slot** — the opposite of the grid's rule
  below, and for the reason that rule exists: grid item widgets overlap grid cell widgets, so a
  per-cell handler can't tell which cell was hit. Equipment slots never overlap, so there's
  nothing to disambiguate.
- **One drop target serves the whole grid**, not one per cell. With item widgets sitting on top
  of cell widgets, per-cell drop handlers get ambiguous about which cell was actually hit; the
  drop bubbles up to the window instead, which recovers the cell from the panel's own geometry.
  That also means the cell size is never hardcoded — it falls out of the panel's arranged size,
  so a resized window still drops where it looks like it will.
- **A drag carries a grab offset**, the footprint cell the pointer came down on, and the drop
  anchor is the hovered cell *minus* that. The floating ghost is positioned by the matching
  fraction (`Offset = -GrabOffset / Footprint` against `EDragPivot::TopLeft`), so ghost and drop
  agree by construction — including after a mid-drag rotate, which transposes both together.
- **The rotate key is an Enhanced Input action on the player controller**, not a widget key
  handler. A `NativeOnKeyDown` on the inventory window would never fire: Slate routes key events
  along the *keyboard focus* path, and the window never takes focus. Enhanced Input works because
  it sits at the **end** of that path — on a click, Slate walks up from the (non-focusable)
  inventory widgets and focuses the game viewport, which holds the keyboard for the whole drag.
- **Its mapping context is scoped to the window being open.** `InventoryMappingContext` is added
  at priority 1 when an inventory or container window opens and removed when the last one
  closes (`UpdateInventoryInputContext`, called from all five open/close paths). That's what lets
  an inventory key reuse a key that means something else in the world, and it keeps every
  inventory binding player-rebindable alongside the gameplay ones — a key routed any other way
  would be invisible to Unreal's player key-mapping system and so unreachable from a settings
  screen.
- **The controller reaches the drag through `UInventoryDragDropOperation::GetActiveDrag()`.**
  Slate owns the in-flight drag; no widget and no controller holds a reference to it. That
  static is the bridge, and it lives in `SmoresUI` so the UMG drag plumbing stays out of the
  controller. Rotation is purely local UI state — the orientation only reaches the server on
  drop, through `Server_MoveInventoryItem`.

## C++ Implementation

- **Primary classes:**
  - `SmoresItems`: `UItemDefinition` (+ `EItemCategory` / `EEquipSlot`),
    `UItemModifierDefinition` (+ the `EItemModifierSlot` enum), `ULootTableDefinition` (+
    `FLootTableEntry`, `FLootModifierPool`, `FLootModifierChoice`, `ELootEntryKind`; its base
    `UWeightedTableDefinition` and the `UWorldSeedComponent` it seeds from are in `SmoresCore` -
    see `game-data.md`), `FInventoryItem`,
    `UInventoryComponent` (+ the `EInventorySortCriterion` enum),
    `FEquippedItem` + `UEquipmentComponent` (the paperdoll),
    `AStrategyContainer` (abstract base for world containers),
    `AStrategyChest` (first concrete container type — no added behavior of its own, exists
    so future chest-specific behavior like locks/keys has a home),
    `AWorldItem` (one loose item lying in the world — a replicated actor with a mesh and an
    interaction sphere, and no inventory component at all)
  - `SmoresUI`: `UWindowWidget` (reusable floating-window chrome), `UInventoryWidget`,
    `UInventoryCellWidget` (+ the `EInventoryCellHighlight` enum), `UInventoryItemWidget`,
    `UEquipmentWidget` (the paperdoll window), `UEquipmentSlotWidget`,
    `UInventoryDragDropOperation`, `IInventoryMoveHost`
  - `SmoresCharacters`: `AStrategyUnit` (owns the `Inventory` and `Equipment` subobjects shared
    by NPCs and player units alike), `AStrategyPlayerUnit`
  - `SmoresEconomy`: `UWalletComponent` (one holder's currency balance), `IPricingProvider`
    (what one item is worth in a given transaction), `UTraderComponent` (a `UInventoryComponent`
    subclass holding one NPC's wares and implementing `IPricingProvider`)
  - `smores` (`Variant_Strategy`): `AStrategyPlayerController`, `AStrategyPlayerState` (which
    now owns no state of its own — it hosts the `UWalletComponent` and nothing else)
- **Important methods:**
  - `FInventoryItem::GetDisplayName` / `GetIcon` / `GetTint` / `GetItemId` / `GetDescription` /
    `GetCategory` / `GetEquipSlot` / `GetWorldMesh` / `GetUnitWeight` / `GetUnitBaseValue` /
    `GetTotalWeight` / `GetTotalBaseValue` / `GetConditionScale` / `GetBaseMaxStackSize` /
    `GetFootprint` / `HasSameDefinitionAs` / `CanStackWith` — the read-through accessors that
    hide both the definition indirection and the modifier chain from callers. **Nothing outside
    this struct should read a field off `Definition` directly.** `GetFootprint(bRotated)` is the
    one place width/height get swapped for rotation
  - `FInventoryItem::AddModifier` / `SetModifier` / `RemoveModifier` / `GetModifier` /
    `HasModifier` / `HasSameModifiersAs` — the one-per-slot rule lives here. `AddModifier`
    **refuses** when the slot is already filled and changes nothing, because quietly replacing
    would lose the material an item was made of without saying so; `SetModifier` is the explicit
    replacement path
  - `UInventoryComponent::GetEffectiveMaxStackForItem` — the stack cap for a carried item, read
    through the instance's accessor. `GetEffectiveMaxStack(const UItemDefinition*)` remains for a
    caller holding a definition and no instance of it (a shop listing, a recipe preview); both
    share the private `ScaleStack` body
  - `FInventoryEntry::GetFootprint` / `CoversCell` / `IsValidEntry` — placement geometry, used
    by every occupancy test in the component and the UI
  - `UInventoryWidget::GetItemLabel` (static) — the one place an item's player-facing label
    is formatted (`"Name"`, or `"Name xN"` for a stack); shared by the summary text block and
    the per-cell widgets
  - `UInventoryComponent::CanPlaceAt` — the single overlap/bounds test. Its `IgnoreEntryId`
    parameter excludes one existing entry from the overlap check, which is what lets an entry
    be re-anchored onto cells it already occupies itself
  - `UInventoryComponent::FindFreePlacement` — auto-placement scan, natural orientation first
  - `UInventoryComponent::GetEntry` / `GetEntryIdAtCell` / `GetEffectiveMaxStack` /
    `GetFreeCellCount` — the read side used by the UI and by `MoveItem`
  - `UInventoryComponent::GetTotalWeight` / `GetWeightCapacity` / `HasWeightLimit` /
    `IsOverWeightCapacity` — the weight read side. Nothing but the UI calls any of them; no
    mutator consults `WeightCapacity`, which is what keeps the figure inert
  - `UWalletComponent::AddGold` / `TrySpendGold` / `CanAfford` / `GetGold` — the currency
    surface. `AddGold`/`TrySpendGold` are authority-only and broadcast `OnGoldChanged`;
    `OnRep_Gold` re-broadcasts it on clients, same split as `UInventoryComponent`'s mutators
    vs. `OnRep_Entries`. `StartingGold` is applied once, on the server, in `BeginPlay`.
    `AStrategyPlayerState::GetWallet` is the accessor; the component is a default subobject, so
    it is never null once the player state exists
  - `UTraderComponent::GetUnitBuyPrice` / `GetUnitSellPrice` (`IPricingProvider`) — the only two
    virtuals a later market simulation has to implement. `BaseValue` × `BuyMarkup` is never
    rounded down to free for an item the designer actually priced (a `FMath::Max(1, ...)` floor);
    the sell price deliberately *may* round to zero, which is the trader declining to pay for
    junk. A definition with no authored `BaseValue` is free on both sides
  - `IPricingProvider::GetBuyPrice` / `GetSellPrice` — non-virtual totals, per-unit × quantity.
    Derived rather than virtual so a bulk discount is a change to the interface rather than
    something each caller invents
  - `UTraderComponent::StartingStock` — the trader's opening wares, Blueprint-authored and
    applied once server-side in the component's own `BeginPlay`. Same shape as
    `AStrategyContainer::StartingItems`, and on the component rather than the actor because the
    component is what gets added per-NPC
  - `UInventoryComponent::AddItem` / `AddItemAt` / `RemoveEntry` / `SetEntryQuantity` /
    `RepositionEntry` / `SetGridSize` — authority-only mutators, all broadcast
    `OnInventoryChanged`
  - `UInventoryComponent::MoveItem` (static) — the single move/merge entry point for both
    repacking and cross-inventory transfer
  - `UEquipmentComponent::Equip` / `Unequip` — the two authority-only mutators, both validating
    (authority on *both* components, slot match, room for the displaced item) before touching
    anything, so neither can half-apply. `Equip` takes one unit off the source entry;
    `SetEntryQuantity` removes it outright if that was the last one
  - `UEquipmentComponent::CanEquipItem` — the whole equip gate, one comparison. The client-side
    drop preview and the server both call it, so the green/red slot can't promise a refusal
  - `UEquipmentComponent::GetSlotForItem` / `GetAllEquipSlots` / `GetSlotDisplayName` (all
    static) — the item's own slot, the fixed five-slot paperdoll roster in display order, and
    the slot's player-facing label read straight off the enum's `UMETA(DisplayName)` so there's
    no second name list to keep in step
  - `UEquipmentComponent::GetEquippedItem` / `IsSlotOccupied` / `GetTotalWeight` /
    `GetOwnerInventory` — the read side. `GetOwnerInventory` is the sibling lookup that decides
    where an unequipped item lands; `GetEquippedItem` is the seam a later combat pass reads the
    equipped weapon through
  - `UInventoryWidget::SetEquipmentTarget` / `GetEquipmentTarget` — what makes right-click mean
    "equip" in a pawn's own window and nothing anywhere else. The controller sets it when it
    opens a pawn window and never for a container or loot window, and `ClearInventory` drops it
    along with the inventory binding so a reused window can't carry the previous pawn's
    paperdoll across
  - `UInventoryItemWidget::TryEquip` — the right-click path: reaches the owning window through
    `GetTypedOuter<UInventoryWidget>()`, checks there's a target and that the item is wearable at
    all, then hops to the server. No target (a chest, a corpse) means silently nothing
  - `UEquipmentSlotWidget::WouldAcceptDrop` — the paperdoll's green/red, deliberately mirroring
    `Equip`'s checks *including* the displaced item's placement test, for the same reason
    `UInventoryWidget::WouldAcceptDrop` mirrors `MoveItem`'s
  - `UEquipmentWidget::RebuildSlots` / `RefreshDisplay` — the slot roster is fixed, so the slot
    widgets are built once per bound pawn and merely refreshed afterwards. That's the opposite of
    `RebuildGrid`, which rebuilds wholesale: a rebuild here would destroy the very widget a drag
    is hovering
  - `AStrategyPlayerController::Server_EquipItem` / `Server_UnequipItem` (`IInventoryMoveHost`) —
    the authoritative callers, doing no validation of their own for the same reason
    `Server_MoveInventoryItem` doesn't
  - `AStrategyPlayerController::OpenEquipmentForPawn` / `CloseEquipment` — the paperdoll window's
    lifecycle, driven entirely from `OpenInventoryForPawn`/`CloseInventory` so the two windows
    can't get out of step. `OpenInventoryForPawn`'s `bOpenEquipment` is what scopes the paperdoll
    to the inventory key: true only from `ToggleInventory`, false from the container and loot
    paths, which close any open paperdoll instead of leaving a stale one
  - `AStrategyPlayerController::HandleWindowClosed` — bound to every window this controller
    spawns (`UWindowWidget::OnWindowClosed`). Closing the pack with its X button takes the
    paperdoll with it; closing the paperdoll leaves the pack, which is what the player asked
    for; either way the scoped input context is re-evaluated
  - `AStrategyPlayerController::SmoresDumpEquipment` / `SmoresEquipItem <EntryIndex>` /
    `SmoresUnequipItem <SlotIndex>` (console execs) — debug-only, all three routed through one
    `Server_DebugEquipment` hop and all three ending in a per-slot dump plus the worn weight.
    `SmoresEquipItem` passes `EEquipSlot::None`, so it exercises the same "wherever it belongs"
    path right-click uses
  - `AStrategyPlayerController::SmoresDumpInventory` / `SmoresAddItem` (console execs) —
    debug-only. `SmoresDumpInventory` logs the selected pawn's **server-side** grid as an
    ASCII occupancy map plus a per-entry list (id, quantity, anchor, footprint, rotation,
    effective stack cap); `SmoresAddItem <Count> [ModifierId]` adds `Count` more of whatever the
    pawn's first entry holds and then dumps, exercising stack-merge, auto-placement and the
    rotation fallback, and logging the composed name, unit weight and unit price of what it
    added. Naming a modifier (`SmoresAddItem 3 Bronze` — `SmoresDumpDefinitions` lists the ids)
    puts it on the copies being added, which is how the modifier model is exercised from a
    running game: the added copies won't merge with the unmodified ones already in the grid, and
    their name, weight and price all read differently. An id that resolves to nothing is reported
    and adds nothing, rather than silently adding a bare item. Both hop to the server via
    `Server_DebugInventory`, since the local replicated copy isn't the authoritative one — and
    the modifier crosses as an **id**, resolved server-side through `USmoresDefinitionLibrary`,
    which is the same look-up records and loot tables will use
  - `UInventoryComponent::AddItemsAllOrNothing` — every item of a set, or none of them (see Core
    Rules). `AddItemCounted`'s body now lives in the protected `AddItemAgainst`, which merges and
    places against any placement array and id counter without broadcasting or logging - which is
    what lets the batch run it against a scratch copy and throw the copy away
  - `AStrategyContainer::RollLootTable` — authority-only, called once from `BeginPlay` after the
    `StartingItems`: builds the stream with `UWeightedTableDefinition::MakeRollStream` from
    `UWorldSeedComponent::GetWorldSeedFor(this)` and `PlacedContainerId`, calls
    `ULootTableDefinition::RollLoot`, and hands the result to `AddItemsAllOrNothing`.
    `GetLootTable` / `GetPlacedContainerId` are the read side
  - `AStrategyPlayerController::SmoresRollTable <TableId> [Seed] [Count]` (console exec) — prints
    sample rolls of a table without touching any container. See `game-data.md`
  - `UInventoryComponent::AddItemCounted` — `AddItem` plus the quantity that actually landed.
    `AddItem` is now a one-line forwarder that discards the count. Any caller still holding the
    source copy (a world pickup) needs the count rather than the bool, since a partial add keeps
    what fit. `MoveItemCounted` is the same fix one level up, and is what a purchase charges off
  - `UInventoryComponent::SortEntries(EInventorySortCriterion)` — authority-only. Merges every
    stack that can merge, orders what's left by the criterion (descending, with the full
    tiebreak chain), and re-places it into a scratch array from the top-left, committing only if
    everything landed. Entry ids survive the repack, so a UI holding one across a round trip
    still resolves. Returns false when nothing changed — no authority, an empty grid, an
    already-sorted grid, or a repack that couldn't fit something
  - `UInventoryComponent::SortEntriesWithReason` — `SortEntries` plus *why* it changed nothing,
    as an `ESmoresRefusalReason`. `SortEntries` is now a one-line forwarder that discards it.
    Same split as `AddItem`/`AddItemCounted` and for the same reason: an abandoned repack and an
    already-sorted grid both return false, and only the first is worth telling the player about.
    `UEquipmentComponent::EquipWithReason` / `UnequipWithReason` are the equivalents for the
    worn slots — see `refusals-and-feedback.md`
  - `UInventoryComponent::CanPlaceAgainst` / `FindFreePlacementAgainst` (protected) — the bodies
    of `CanPlaceAt` / `FindFreePlacement`, taking the placement array to test against instead of
    assuming `Entries`. They exist so the repack can ask "would this fit?" about an arrangement
    that isn't live yet; the public pair are now one-line forwarders passing `Entries`
  - `AWorldItem::TryPickUp` — authority-only; moves as much as will fit into the destination
    grid, destroys the actor when the whole stack moved and shrinks `Item` when only part did.
    Returns false having changed nothing when nothing fit
  - `AWorldItem::SpawnWorldItem` (static) — the authority-only spawn path, deferred so the
    item is in place before `OnConstruction` picks the mesh. The drop debug exec is its only
    caller today; the drag-an-item-onto-the-world gesture will be the second
  - `AWorldItem::SetItem` / `GetItem` / `RefreshMesh` / `OnRep_Item` — the item and its mesh.
    `RefreshMesh` runs from `OnConstruction` as well as `BeginPlay`, so setting `Item` on a placed
    instance updates the editor viewport immediately; `SetItem` refreshes directly because
    `OnRep_Item` only fires on the *other* machines
  - `AWorldItem::IsInRangeOf` / `GetHolderDisplayName` (`IInventoryHolder`) — proximity gate and
    display name, the same two every holder answers
  - `AStrategyPlayerController::FindWorldItemAtLocation` — nearest loose item within
    `WorldItemSelectionRadius` (100, deliberately tighter than `ContainerSelectionRadius`) of the
    double-clicked world location; a thin wrapper over `FindHolderActorAtLocation`
  - `AStrategyPlayerController::Server_PickUpWorldItem` — the authoritative pickup, and the one
    inventory RPC that *does* validate: it re-checks proximity and that the destination is a
    player pawn's own pack before calling `TryPickUp`
  - `AStrategyPlayerController::SmoresDropItem <EntryIndex>` (console exec) — debug-only; spawns
    the pawn's EntryIndex'th grid entry on the ground in front of it via `Server_DebugDropItem`,
    so the pickup path has something to pick up without hand-placing actors. Spawns first and
    removes the entry only on success, so a refused spawn can't destroy the item
  - `AStrategyContainer::IsInRangeOf` / `GetHolderDisplayName` (`IInventoryHolder`) — as above
  - `AStrategyUnit::IsInRangeOf` / `GetHolderDisplayName` (`IInventoryHolder`) — as above. Its
    range check previously took a narrower `const AStrategyUnit*`; the interface widened it to
    `const AActor*`, which is what let one helper serve pawn, container and pickup alike
  - `IInventoryHolder::IsActorWithinSphere` (static) — the single copy of the distance test all
    three implementers forward to, each passing its own `InteractionRange` sphere
  - `AStrategyPlayerController::FindHolderActorAtLocation` — the shared body of every
    `Find*AtLocation`: nearest actor of a given class within a given radius that passes a given
    predicate. Radius is a parameter because click precision is per-gesture; the predicate is a
    parameter because each holder type qualifies on a rule of its own (an NPC must be lootable, a
    world item must actually hold something, a container always qualifies)
  - `AStrategyPlayerController::IsHolderInRangeOfSelection` — the selection-side counterpart:
    is any currently selected unit inside this holder's own reach
  - `AStrategyPlayerController::FindPlayerPawnInRangeOfHolder` — nearest player pawn actually
    close enough to transfer with *any* holder. Unlike `FindClosestPlayerPawn`, range is a filter
    here rather than a tiebreak, since proximity is the gate. Checks every player pawn rather
    than just `ControlledUnits`, because the plain select click that fires alongside a
    double-click may have just cleared the selection
  - `AStrategyPlayerController::IsLootableNPC` (static) — the one place "lootable" is written
    down: not a player pawn, and Downed or Dead
  - `AStrategyPlayerController::ToggleInventory` / `OpenInventoryForPawn` / `CloseInventory`
    — pawn inventory window lifecycle; requires exactly one selected `AStrategyPlayerUnit`
  - `AStrategyPlayerController::ToggleContainer` / `OpenContainer` / `OpenLoot` /
    `CloseContainer` — one shared window (`ContainerWidget`) reused for both world
    containers and body loot; only the window title differs, set at open time — and the loot
    title is the only place Downed and Dead are told apart
  - `AStrategyPlayerController::FindContainerInRange` / `FindLootableNPCInRange` — used by
    the toggle-key path (checks `ControlledUnits`/`SelectedNPC` only). These two share a shape
    but *not* a policy: the container one sweeps every container in the level and prefers
    `SelectedContainer` if it qualifies, while the NPC one never sweeps at all — only
    `SelectedNPC` is ever a candidate, so a key press can't open whichever corpse happened to be
    nearest
  - `AStrategyPlayerController::FindContainerAtLocation` / `FindNPCAtLocation` —
    used by the double-click path (within `ContainerSelectionRadius`), both thin wrappers over
    `FindHolderActorAtLocation` differing only in class and predicate
  - `AStrategyPlayerController::SelectAllDoubleClick` — the one gesture behind five meanings,
    resolved by type in order: loose world item, container, body, living NPC, then
    select-all-on-screen. The world item goes first because it's the smallest thing under the
    cursor and the only one with no selection state to set — finding one either collects it or
    does nothing. Body and living NPC share a single `FindNPCAtLocation` sweep and are told
    apart by `IsLootableNPC`, rather than being two sweeps that would have to agree about which
    NPC was nearer
  - `AStrategyHUD::GetWallet` — resolves the owning player's `UWalletComponent` through its
    `PlayerState` and caches it, re-running the lookup only while the pointer is still null
    (a player state can replicate in well after the HUD exists). `DrawHUD` reads the balance
    off it each frame and pushes the result into `UStrategyUI::SetGold`. No interface is
    involved: `APlayerState` is an engine type visible from every module, which is what let
    `IStrategyResourceHost` be deleted when gold became a component
  - `AStrategyPlayerController::GetWallet` — the same lookup from the controller's side, for the
    transaction and the gold execs
  - `AStrategyPlayerController::SmoresAddGold` / `SmoresSpendGold` (console execs) —
    debug-only, both hopping to the server via `Server_DebugGold` since the balance is
    server-owned. `SmoresSpendGold` past the balance logs `REJECTED` and changes nothing
  - `UStrategyUI::SetGold` / `GetGoldLabel` — `SetGold` early-outs when the balance is
    unchanged (the HUD pushes every frame), so `NativeConstruct` refreshes once on its own to
    cover a widget built after the first push
  - `UInventoryWidget::GetWeightSummary` / `IsOverWeightCapacity` — the window's weight
    readout and the red/normal colour choice; `RefreshDisplay` applies both, so weight tracks
    `OnInventoryChanged` with no separate subscription
  - `UInventoryWidget::SortBy` — dispatches the repack through `IInventoryMoveHost`; the window
    changes nothing itself and redraws only when the repacked entries replicate back
  - `UInventoryWidget::SetCategoryFilter` / `GetCategoryFilter` / `PassesCategoryFilter` /
    `ApplyCategoryFilter` — the whole filter, client-side. `SetCategoryFilter` also syncs the
    dropdown's own selection, since the *window* clears the filter on rebind as well as the
    player changing it. `ApplyCategoryFilter` runs at the end of `RefreshDisplay`, because
    `RebuildGrid` respawns every item widget at full opacity — a filter that didn't re-apply
    there would silently lift itself the first time anything in the grid moved
  - `UInventoryWidget::PopulateCategoryFilterBox` — fills `CategoryFilterBox` from the
    `EItemCategory` enum in `NativeConstruct` and records the row order in `FilterBoxCategories`,
    so the selection maps back by *index* rather than by matching a display string (which
    localisation would break). It clears the box first: the window is re-added to the viewport
    rather than respawned, so this runs on every open
  - `UInventoryItemWidget::SetFilteredOut` / `IsFilteredOut` — the dim itself, via
    `SetRenderOpacity`, which leaves the widget hit-testable and leaves the cell layer covered
  - `AStrategyPlayerController::Server_SortInventory_Implementation` — forwards to
    `SortEntriesWithReason`, relaying a `NoRoom` back through `Client_NotifyRefusal`.
    Deliberately ungated beyond authority, unlike `Server_PickUpWorldItem` and `TryTradeItem`:
    a sort can only rearrange one holder's own contents, so there is nothing for a bad request
    to take
  - `AStrategyPlayerController::SmoresSortInventory <Criterion>` (console exec) — debug-only;
    repacks the selected pawn's grid (0 = weight, 1 = value, 2 = quantity) through the same
    `SortEntries` the buttons call, then dumps the grid
  - `AStrategyPlayerController::LogInventoryGrid` (static) — the ASCII occupancy map plus
    per-entry list, shared by the add/dump exec and the sort exec so both report identically
  - `AStrategyPlayerController::Server_MoveInventoryItem_Implementation` — the sole
    authoritative caller of `UInventoryComponent::MoveItem` from UI drag-drop. Carries entry
    id, destination cell, rotation and quantity; it does no validation of its own, since
    `MoveItem` does all of it — *except* that it first checks whether exactly one end of the
    move is a `UTraderComponent`, which is what silently turns the same drag into a purchase or
    a sale
  - `AStrategyPlayerController::TryTradeItem` — one purchase or sale, applied all-or-nothing.
    Rejects two traders or none; re-checks that the counterparty is an interactable NPC with a
    player pawn in reach; prices a purchase against the whole request before anything moves;
    calls `MoveItemCounted`; then debits or credits for what actually moved. The only place gold
    and items change hands together
  - `AStrategyPlayerController::OpenTrade` — the trade window's lifecycle, reusing the same
    `ContainerWidget` a chest and a corpse use. Sets the trader's stock as its inventory *and*
    its pricing (buy side), then opens the nearest pawn's pack beside it and points that at the
    same pricing (sell side). The pawn window's pricing is set after `OpenInventoryForPawn`,
    which rebinds and therefore clears it
  - `AStrategyPlayerController::InteractWithNPC` — the "interact with this person" verb shared
    by the double-click, the talk key and the target panel: interactable, somebody close enough,
    then a bark (`Server_RaiseInteractionBark`, which picks `TradeOpened` or `NothingToSay` on the
    server) and, for a trader, the trade windows. See `dialog.md`
  - `AStrategyPlayerController::IsInteractableNPC` / `GetTraderStock` — the mirror of
    `IsLootableNPC`, and the trader lookup layered on top of it. Both static, both the single
    place their rule is written down
  - `AStrategyPlayerController::FindNPCAtLocation` — replaces `FindLootableNPCAtLocation`: the
    nearest NPC within `ContainerSelectionRadius` in *whatever* state, so the double-click
    branches on health rather than sweeping twice
  - `AStrategyPlayerController::FindInteractableNPCInRange` — the talk key's counterpart to
    `FindLootableNPCInRange`, and deliberately the same shape: only `SelectedNPC` is ever a
    candidate, so a key press can't open whichever merchant happened to be nearest
  - `AStrategyPlayerController::TalkKeyPressed` — the `T` handler. Toggles the shared container
    window closed if one is already up, exactly as the container key does
  - `UInventoryWidget::SetPricing` / `ClearPricing` / `GetItemPriceTooltip` — which side of a
    trade counter this window sits on, and the one place a price is formatted for the player.
    `ClearInventory` drops the pricing along with the inventory binding, so a pack reused for a
    chest can't still be quoting the last trader; `CloseContainer` clears it explicitly for the
    case where the pack stays open and only the trade ends
  - `UInventoryItemWidget::RefreshPriceTooltip` — asks the owning window (through
    `GetTypedOuter<UInventoryWidget>()`, the same hop `TryEquip` uses) what this item is worth
    and puts the answer on its hover tooltip, clearing it when there's no price
  - `AStrategyPlayerController::SmoresDumpTrader` / `SmoresBuyItem <EntryIndex>` /
    `SmoresSellItem <EntryIndex>` (console execs) — debug-only, all three routed through one
    `Server_DebugTrade` hop and all three ending in a stock dump with per-unit buy/sell prices
    and the player's balance. Click an NPC to target it first, the same as `SmoresKillNPC`. The
    buy/sell ones auto-place into the destination grid (the real path always carries the cell the
    player dropped on) and then run the ordinary `TryTradeItem`, so they exercise the real
    transaction rather than a parallel one
  - `UInventoryWidget::SetInventory` / `ClearInventory` — binds/unbinds an inventory,
    subscribes to `OnInventoryChanged`
  - `UInventoryWidget::RebuildGrid` — builds the grid's two layers into the `UGridPanel`:
    one `UInventoryCellWidget` per cell at `UGridSlot` layer 0, one `UInventoryItemWidget` per
    placed entry at layer 1 with `SetRowSpan`/`SetColumnSpan` from its footprint. Equal
    `SetColumnFill`/`SetRowFill` across the grid is what keeps cells uniform, which
    `ScreenPositionToCell` depends on
  - `UInventoryWidget::ScreenPositionToCell` / `GetDropAnchorCell` — the geometry half of the
    single grid-level drop target: panel-relative pointer position → cell, then minus the
    drag's `GrabOffset` to get the anchor the item's top-left lands on
  - `UInventoryWidget::WouldAcceptDrop` — the preview's yes/no, deliberately mirroring
    `MoveItem`'s resolution (merge into a stackable entry at the anchor, else `CanPlaceAt`)
    so the highlight can't promise a drop the server will reject
  - `UInventoryWidget::UpdateDragPreview` / `ClearDragPreview` — marks the covered cells
    Valid/Invalid and tears the preview down again; subscribed to the drag's `OnRotated` so a
    mid-drag rotate redraws without waiting for the pointer to move
  - `UInventoryItemWidget::SetEntry` / `SetPreviewOrientation` / `NativeOnDragDetected` —
    binds one placed entry, re-draws the floating ghost at a new orientation, and starts the
    drag (recording which footprint cell the pointer grabbed)
  - `UInventoryDragDropOperation::GetActiveDrag` / `ToggleRotation` / `ApplyPreview` — the
    rotate path: the static that finds the in-flight drag through Slate, the transpose it
    applies to `bRotated` + `GrabOffset`, and the decorator repositioning that keeps ghost and
    drop agreeing afterwards
  - `AStrategyPlayerController::RotateDraggedItem` / `UpdateInventoryInputContext` — the
    `IA_Strategy_RotateDraggedItem` handler, and the add/remove of `InventoryMappingContext`
    that scopes every inventory key to a window actually being open
- **Runtime ownership:** `UInventoryComponent` is a default subobject of `AStrategyUnit`
  (every unit, NPC or player-controlled) and of `AStrategyContainer` (every world
  container); `UEquipmentComponent` is a default subobject of `AStrategyUnit` only — a chest
  wears nothing. The two `UInventoryWidget` instances (`InventoryWidget`, `ContainerWidget`)
  and the one `UEquipmentWidget` (`EquipmentWidget`)
  are lazy-created and owned by `AStrategyPlayerController` directly — **not** by
  `AStrategyHUD`, which today only spawns the general `UStrategyUI` widget, pushes the
  selection count / target label / gold balance into it each frame, and draws the
  drag-selection box; it has no inventory role. `AStrategyPlayerState` is spawned per player
  by the game mode (`PlayerStateClass`) and owns one `UWalletComponent` as a default subobject,
  so the gold balance is scoped to one player and never to the world. `UTraderComponent` is
  added per-NPC in Blueprint rather than being a subobject of anything — that's the whole point
  of it: most characters aren't merchants.
- **Data flow (drag-and-drop transfer):** `UInventoryItemWidget::NativeOnDragDetected`
  (source item) → `UInventoryDragDropOperation` payload (source inventory + entry id + a copy
  of the item + rotation + grab offset + cell size) → the target window's
  `UInventoryWidget::NativeOnDrop` (converts the pointer position to a cell, minus the grab
  offset) → `IInventoryMoveHost::Server_MoveInventoryItem` (client → server RPC via the owning
  `AStrategyPlayerController`) → `UInventoryComponent::MoveItem` → `Entries` replicates back
  down → `OnRep_Entries` → `OnInventoryChanged` → `UInventoryWidget` refreshes on every
  observing client. A rejected move mutates nothing, so the refresh redraws the unchanged
  state and the item appears to snap back — the client is never told "no" explicitly, which
  is exactly why the red drop preview exists.
- **Data flow (equip):** `UInventoryItemWidget::NativeOnMouseButtonDown` (right button) →
  `TryEquip` → the owning window's `GetEquipmentTarget` →
  `IInventoryMoveHost::Server_EquipItem` (client → server RPC) → `UEquipmentComponent::Equip` →
  `EquippedItems` *and* `Entries` both replicate back down → `OnRep_EquippedItems` /
  `OnRep_Entries` → `OnEquipmentChanged` / `OnInventoryChanged` → the paperdoll window and the
  inventory window each refresh independently. A drop onto a paperdoll slot is the same path
  from `UEquipmentSlotWidget::NativeOnDrop`, differing only in naming the slot instead of
  sending `EEquipSlot::None`; unequip is the mirror through `Server_UnequipItem`. A rejected
  equip is silent on the wire, exactly like a rejected move.
- **Data flow (purchase/sale):** identical to the transfer flow above for its whole client half
  — the same `UInventoryItemWidget` drag, the same `UInventoryWidget::NativeOnDrop`, the same
  `Server_MoveInventoryItem` RPC — and diverges only once it reaches the server:
  `Server_MoveInventoryItem_Implementation` notices that exactly one of the two components is a
  `UTraderComponent` → `TryTradeItem` (proximity + hostility + affordability) →
  `UInventoryComponent::MoveItemCounted` → `UWalletComponent::TrySpendGold` or `AddGold` for the
  quantity that actually moved → `Entries` and `Gold` both replicate back down → the windows
  refresh from `OnInventoryChanged` and the HUD's next `DrawHUD` reads the new balance. A
  refused purchase mutates nothing on either side, so the item snaps back exactly as an
  ill-fitting drop does, and the player is told nothing (the log says why; the UI doesn't).

## Blueprint / Asset Dependencies

- **`DA_Item_*`** (`Content/Items/`) — `UItemDefinition` assets, one per item type.
  Footprints and stack sizes are authored: `Apple` 1×1 stack 10, `GoldCoin` 1×1 stack 100,
  `HealthPotion` 1×1 stack 5, `PocketKnife` 1×1, `Torch` 1×2, `Rope` 2×2, `TrapKit` 2×2,
  `Sword` 1×3, and three minerals added with the loot tables - `CopperOre`, `RockSalt`, `Sulfur`,
  each 1×1 stack 20, category Material, and the only items carrying a tag (`Item.Mineral`, which
  `DA_Loot_DesertMinerals` picks among). `Icon` is unassigned on all of them — no 2D item art exists yet. `WorldMesh`
  is assigned on all of them, but to the *same* placeholder (`/Engine/BasicShapes/Sphere`) rather
  than to real art: a uniform shape with a predictable centre pivot, which is what lets one ground
  offset serve every item. Because a definition is the only thing a carried item references, **deleting one
  empties every entry holding it**, and **changing a footprint doesn't re-validate already
  placed entries** — an item grown larger can leave overlapping placements until something
  moves them.
- **`WBP_Inventory`** (`Content/Variant_Strategy/UI/`) — `UInventoryWidget` subclass,
  assigned to `AStrategyPlayerController::InventoryWidgetClass`. Shows the selected pawn's
  own inventory.
- **`WBP_ContainerInventory`** — a second `UInventoryWidget` subclass, assigned to
  `ContainerWidgetClass`. Reused for both world containers and body loot; only the
  window title differs at open time.
  Both hold a `UGridPanel` named `SlotContainer` — **not** a `UUniformGridPanel`, which has no
  slot span and so cannot host a footprint-spanning item widget at all. C++ casts and logs a
  warning if the panel is the wrong type or `CellWidgetClass` is unset, since either one
  silently breaks the drop maths. A plain `SlotListText` text block is the fallback visual when
  no grid is wired; it lists each placed entry with its quantity, anchor cell, footprint and
  rotation.
- **`GoldText`** — a `UTextBlock` in `UI_Strategy`, sitting beside `SelectionCount` in the
  same horizontal strip. Optional (`BindWidgetOptional`); C++ fills its text, so no Blueprint
  property binding is involved. `WeightText` in `WBP_Inventory`/`WBP_ContainerInventory` works
  the same way, sitting between the title bar and the grid.
- **`SortWeightButton` / `SortValueButton` / `SortQuantityButton` / `CategoryFilterBox`** — the
  sort-and-filter toolbar strip in both inventory WBPs, between `WeightText` and the grid. All
  four are `BindWidgetOptional`, so a WBP without them still compiles and runs — the feature is
  simply invisible. **C++ binds the click and selection handlers itself in `NativeConstruct` and
  fills the combo box's options itself**, so a designer only places the widgets and names them
  exactly: no Blueprint graph work, no property bindings, and the same "C++ fills it directly"
  shape `GoldText` and `WeightText` use. Leave the combo box's authored `DefaultOptions` empty —
  `PopulateCategoryFilterBox` clears it on every open.
- **`BP_StrategyPlayerState`** — `AStrategyPlayerState` subclass. `StartingGold` is authored on
  its **Wallet component**, not on the actor: the field moved there when gold became a
  component, which silently voided the value previously authored on the Blueprint itself (there
  is no `CoreRedirects` equivalent for a property changing owner). It is assigned to
  `BP_StrategyGameMode`'s `PlayerStateClass`; without that assignment the game mode spawns a
  plain engine `APlayerState`, the HUD shows `Gold: 0` forever, and the gold and trade execs log
  that there is no wallet.
- **A trader NPC Blueprint** — any `AStrategyUnit` subclass with a `UTraderComponent` added.
  Authored on the component: `StartingStock` (the wares), `BuyMarkup` (1.5), `SellMarkdown`
  (0.5), and the grid's own `GridWidth`/`GridHeight`. Nothing flags the actor as a merchant —
  adding the component *is* the flag, so an NPC that should stop trading has its component
  removed rather than a bool cleared. Note that the component defaults to `WeightCapacity` 0 and
  `StackMultiplier` 2 in C++, which is what a shelf wants and what a Blueprint should usually
  leave alone.
- **`IA_Strategy_Talk`** — bound to `TalkAction`; talks to / trades with the targeted NPC.
  Mapped to `T` in `IMC_Strategy_Mouse` (desktop only; not mapped in the touch context).
- **`WBP_InventoryCell`** — `UInventoryCellWidget` subclass, assigned to
  `UInventoryWidget::CellWidgetClass`. One instance per **grid cell**, at grid layer 0. Just a
  `UBorder` named `CellBorder`, whose tint C++ drives from the highlight state; the brush
  itself is a plain filled box. Its root must stay hit-test `Visible` so the grid reads as one
  continuous drop surface.
- **`WBP_InventoryItem`** — `UInventoryItemWidget` subclass, assigned to
  `UInventoryWidget::ItemWidgetClass`. One instance per **placed entry**, at grid layer 1,
  spanning its footprint. Tree is `ItemSizeBox` (`USizeBox`, **no** width/height overrides
  authored — C++ clears them for grid instances and sets them for the drag ghost) → a border →
  `ItemLabel` (`UTextBlock`, centred, `AutoWrapText` off). Nothing in that tree may clip to
  bounds: C++ turns the label 90° with a render transform, which doesn't affect layout, so the
  label has to be free to overflow its box.
- **`IA_Strategy_Inventory`** (`Content/Variant_Strategy/Input/Actions/`) — bound to
  `ToggleInventoryAction`. Desktop-only; not mapped in the touch `InputMappingContext`.
- **`IA_Strategy_ToggleContainer`** — bound to `ToggleContainerAction`. Also desktop-only.
- **`IA_Strategy_RotateDraggedItem`** — bound to `RotateDraggedItemAction`; rotates the item
  being dragged. Mapped in `IMC_Strategy_Inventory` (not the always-on mouse context), which the
  controller adds at priority 1 only while an inventory window is open. Bound on
  `ETriggerEvent::Started` so the item turns on key press rather than release — never
  `Triggered`, which for a held key would spin the item once per frame.
- **`AStrategyContainer` / `AStrategyChest` Blueprint subclasses** — assign `ContainerMesh`'s
  materials (`NormalMaterial`/`SelectedMaterial`), `ContainerDisplayName`, and populate
  `StartingItems`. `StartingItems` is authored **entirely in Blueprint** (and per placed
  instance, as the "Chest 2" actor in `LVL_Strategy` does) — no C++ constructor seeds it,
  since C++ shouldn't hard-code content paths. `BP_Chest` sets its `Inventory` subobject to an
  8×6 grid. `LootTable` is left empty on `BP_Chest` and set **per placed instance**: "Chest 1"
  (hand-placed Gold Coin, Health Potion, Bronze Sword) rolls `DA_Loot_DesertChest` on top, and
  "Chest 2" (Rope, Torch, Trap Kit) rolls `DA_Loot_FactionTownChest`. Both had their `PlacedContainerId` authored by hand, since
  they predate its auto-generation; any chest placed from now on gets one automatically.
- **Unit Blueprints** — a unit's starting pack is its character definition's `DefaultLoadout`,
  not a property on the unit: `BP_PlayerUnit` points at `DA_Character_Settler`, which seeds an
  Apple and a Pocket Knife (this was `AStrategyPlayerUnit::StartingItems` until game-data Slice 4
  removed it). The loadout is placed once, when the unit's record is created, and afterwards the
  record is what a pack is restored from - see `game-data.md`. `BP_PlayerUnit`'s `Inventory`
  subobject is a 6×4 grid with the default 30 `WeightCapacity`. `BP_Chest` sets its capacity to **0** (no limit),
  since a chest doesn't carry anything anywhere.
  `GridWidth`/`GridHeight`/`StackMultiplier`/`WeightCapacity` are the per-holder knobs to
  override on any new holder Blueprint; `Entries` itself is not editable, so starting contents
  always go through `StartingItems` (containers) or `DefaultLoadout` (characters) and `AddItem`'s
  auto-placement. The one path that places entries at stored anchors is
  `UInventoryComponent::RestoreEntries`, which a character record uses to hand a pack back to its
  actor - it keeps ids, anchors and rotation, drops anything empty, duplicated or not fitting, and
  returns the count that landed. `UEquipmentComponent::RestoreEquippedItems` is the paperdoll
  equivalent.
- **`BP_WorldItem`** — the concrete `AWorldItem` subclass, assigned to
  `AStrategyPlayerController::WorldItemClass` (which the drop debug exec spawns, and which logs a
  warning naming the controller if it's unset). One Blueprint serves every item type: `ItemMesh`
  is driven from the held definition, so the only thing worth authoring per placed instance is
  `Item` (its definition and quantity). The mesh is set to `NoCollision` in C++ — a dropped item
  should never shove a pawn around or block a selection trace, and the interaction sphere is what
  actually gates reaching it. `ItemMesh`'s `OverrideMaterials[0]` is set to **`MI_WorldItem_Black`**
  in the Blueprint's class defaults, which is what makes every world item black regardless of which
  mesh its definition supplies: a component-level material override survives the runtime
  `SetStaticMesh` in `RefreshMesh`, so the colour doesn't have to be a per-definition field.
  `ItemMesh`'s `StaticMesh` class default is **also** set (to the same placeholder sphere), and it
  looks redundant because it is overwritten on construction — but **don't remove it**. A static
  mesh component with a null mesh reports zero material slots, so it silently discards any
  `OverrideMaterials` entry written to it: the write returns success and nothing lands. The default
  mesh exists to give the override a slot to stick to, not because its value is ever used.
- **`DA_Modifier_*`** (`Content/Items/Modifiers/`) — `UItemModifierDefinition` assets, five of
  them. Three materials: `Iron` (1.0× / 1.0× / 1.0×, pale grey), `Bronze` (1.1× weight, 0.7×
  value, 0.8× condition, warm orange-brown), `Steel` (0.95× / 2.2× / 1.5×, cool near-white). Two
  qualities: `WellMade` (1.0× / 1.6× / 1.25×) and `Masterwork` (1.0× / 3.0× / 1.6×), both tinting
  pure White so the material's colour comes through. Every one authors
  `NamePattern` = `"{Modifier} {Item}"`.

  **Iron is deliberately the 1.0× baseline.** That is what lets every item asset's authored
  `Weight` and `BaseValue` mean "with no material applied" without any of them being retuned —
  and it is why `DA_Item_IronSword` was rebased to `DA_Item_Sword` (id `Sword`, display name
  "Sword"): "Iron Sword" is now composed from the Sword definition plus the Iron modifier rather
  than baked into one asset. No number changed in that rebase; only the identity did.
- **`MI_WorldItem_Black`** (`Content/Variant_Strategy/Materials/`) — a `M_ContainerColor` instance
  with its `Color` vector parameter set to black, alongside the chests' `MI_Container_Green`
  pair. Placeholder colouring only; real item art would drop the override.
- **`DA_Item_*` weights and values are authored** (Apple 0.2/2g, GoldCoin 0.01/1g,
  HealthPotion 0.5/25g, PocketKnife 0.3/15g, Torch 0.8/5g, Rope 2.0/12g, Sword 3.5/90g,
  TrapKit 4.0/60g, CopperOre 1.5/4g, RockSalt 0.5/3g, Sulfur 0.4/6g) — a definition with a zero `Weight` contributes nothing to the readout, so
  a new item type that forgets to set one looks weightless rather than broken. **These are the
  figures for a bare item**; what a copy actually weighs and is worth depends on its modifiers.

## Extension Points

- **New item types** — add a `DA_Item_*` asset under `Content/Items/`; no code change
  needed. Give it a `DefinitionId`: `UItemDefinition` is registered with the Asset Manager, so
  `USmoresDefinitionLibrary::FindDefinition(UItemDefinition::DefinitionType, Id)` resolves it by
  id and the asset can be renamed or moved without breaking those look-ups. The content sweeps
  in `game-data.md` will fail the build if the id is blank or already taken.
- **New materials and qualities** — add a `DA_Modifier_*` asset under
  `Content/Items/Modifiers/`; no code change needed. Give it a `DefinitionId`, pick its `Slot`,
  and **author its `NamePattern`** — the content sweep treats an empty one as an error, because
  the fallback is hard-coded English word order. Leave a multiplier at 1.0 for "this doesn't
  affect that"; a zero silently erases whatever it multiplies, which is why the sweep rejects it.
  A *new slot* (an enchantment, say) is a code change: add it to `EItemModifierSlot` **at the end
  of the composition order you want**, since the enum's declaration order is the order names
  compose in, and add it to `UItemModifierDefinition::GetAllModifierSlots()`.
- **Rolled contents for a container** — set its `LootTable` (per instance, or on a Blueprint
  subclass for a whole kind of container); the key is already there if it was placed after Slice 5.
  A new table is a `DA_Loot_*` asset - see `game-data.md`. Size the grid for the table's worst case:
  a roll that doesn't fit is refused whole.
- **Anything else that receives a set of items at once** (a quest reward, a crafted batch, a
  caravan's restock) — `AddItemsAllOrNothing` rather than a loop of `AddItem`, for the same reason
  the loot roll uses it.
- **New holder types** — anything with a `UInventoryComponent` gets the grid for free; size
  it with `GridWidth`/`GridHeight` and set `StackMultiplier` above 1.0 for a holder meant to
  stack deeper than a pawn's pack (a storefront shelf, a warehouse chest). Nothing else needs
  a per-holder code path.
- **Theft and sort/filter** — neither exists in code yet, though `UItemDefinition` already
  carries the fields they'll read (`bStolen` on the instance, `Category` on the definition).
  Full target design and rationale live in `Docs/roadmaps/inventory-roadmap.md`.
- **A new kind of trader** — add a `UTraderComponent` to the NPC Blueprint and it works; nothing
  else is per-trader. A caravan is exactly this on a unit that walks.
- **Real market pricing** — implement `IPricingProvider` and have `UTraderComponent` consult it
  instead of its own flat markup. Nothing in `TryTradeItem` or the UI reads a markup directly, so
  the swap is contained to that component.
- **Dialog with a non-trader NPC** — `AStrategyPlayerController::InteractWithNPC` is the seam.
  Dialog Slice 1 filled the silent branch with a `NothingToSay` bark; Slice 2's conversations slot
  in ahead of trade there (the order is in `Docs/roadmaps/dialog-roadmap.md`).
- **A new interaction rule** (a faction refusing to deal, a merchant who only trades at certain
  hours) — extend `IsInteractableNPC`, which is the one place "may the player deal with this
  person" is written down, rather than adding a check per call site.
- **Dropping an item into the world from the UI** — the authority-side half is built and
  verified: `AWorldItem::SpawnWorldItem` plus a `WorldItemClass` on the controller. What's
  missing is only the gesture. Dragging an item out of a window and releasing it over the world
  is the obvious one, and the hook is `UDragDropOperation::DragCancelled` — but that fires for
  *every* unhandled drop, including a drag cancelled with Escape or by a window closing
  mid-drag, so wiring it naively turns a stray click into a dropped item. Design the confirmation
  before the plumbing; `SmoresDropItem` covers testing in the meantime.
- **A fourth holder type** — implement `IInventoryHolder` (`GetHolderDisplayName` +
  `IsInRangeOf`, the latter one line forwarding to `IInventoryHolder::IsActorWithinSphere` with
  the actor's own range sphere) and every existing proximity path accepts it: the double-click
  handler, `FindPlayerPawnInRangeOfHolder`, `IsHolderInRangeOfSelection`. A storefront or a
  traded-with NPC needs no new proximity code at all, only its own gating layered *in front of*
  the transfer. What a new type does still need is a position in `SelectAllDoubleClick`'s
  type-resolution order and a click radius sized to how big a thing it is to aim at — neither is
  inferable, both are deliberate.
- **A new "lootable" rule** — `AStrategyPlayerController::IsLootableNPC` is the single place the
  rule lives ("not a player pawn, and Downed or Dead"). Faction standing, a surrendered NPC, or
  a protected corpse would all extend that one predicate rather than each `Find*` method.
- **New worn slots** — add to `EEquipSlot` *and* to `UEquipmentComponent::GetAllEquipSlots`,
  which is a deliberate hand-written roster rather than an enum iteration: it fixes the
  paperdoll's display order and keeps `None` out of it. Nothing else needs a code change — the
  paperdoll builds one slot widget per entry in that list.
- **Combat reading the equipped weapon** — `UEquipmentComponent::GetEquippedItem(MainHand)` is
  the seam, and it's readable on any machine since `EquippedItems` replicates. That's a
  `SmoresCombat` change, not an inventory one; nothing here should learn what a weapon *does*.
- **Equipped visuals** — `UItemDefinition::WorldMesh` is authored for it and nothing reads it
  yet. Attaching a mesh to a skeletal socket on `OnEquipmentChanged` is purely cosmetic and
  belongs on the character/animation side.
- **Starting equipment** — there's no worn counterpart to `UCharacterDefinition::DefaultLoadout`;
  a pawn starts wearing nothing. If it's wanted, it belongs on the character definition beside the
  loadout and is applied where the loadout is (`AStrategyUnit::RegisterWithRecordStore`, create
  path only) - never on the unit, and never hard-coded in C++.
- **Spending gold** — `UWalletComponent::TrySpendGold` is the seam, and
  `AStrategyPlayerController::TryTradeItem` is the worked example of using it: check
  affordability against the whole request, move the goods, then debit for what actually moved,
  all inside one server-side call so the transaction can't half-apply. A later cost (a wage, a
  bribe, a repair fee) follows the same ordering.
- **Encumbrance effects** — `IsOverWeightCapacity` is the seam, and it's deliberately consulted
  by nothing but the readout's colour. A characters/combat pass that wants a speed or noise
  penalty reads it from there rather than reaching into `GetTotalWeight` itself.
- **New inventory input** — three shapes; pick by what already carries the state:
  - *Modifier + mouse button* (Ctrl+click to split a stack, Shift+click to quick-transfer): read
    `IsControlDown()`/`IsShiftDown()` straight off the click event in the widget's own handler.
    No action asset, no mapping context, no controller involvement — the event already carries
    the modifier state.
  - *A key pressed while a window is open or a drag is in flight*: a new `UInputAction`, mapped
    in `IMC_Strategy_Inventory` (the context scoped to a window being open — **not** the
    always-on mouse context), bound on `AStrategyPlayerController`. If it acts on the drag rather
    than the window, reach it with `UInventoryDragDropOperation::GetActiveDrag()`; if it changes
    shared state, it still goes through a server RPC like every other mutation.
  - *Never* a `NativeOnKeyDown` on an inventory widget (these widgets never hold keyboard focus,
    so it cannot fire) and *never* a Slate input pre-processor (invisible to player rebinding).
  The `UInputAction` asset can be made by duplicating an existing Boolean one; the
  `IMC_Strategy_Inventory` mapping must be authored by hand in the editor. **Check
  `input-and-keybinds.md` before picking a key** — it holds every current binding and the
  reserved list, and is where a new one gets recorded.
- **Save/load** — `FInventoryItem` and `UInventoryComponent`'s state are fully
  `UPROPERTY`-reflected; no struct changes are needed for whatever serialization approach
  the save system eventually adopts.

## Known Gaps

- **Modifier tints land on the item's text label, not on an icon**, because no item icons are
  authored yet. It is the right colour on the wrong surface; move it to the icon the moment
  `UItemDefinition::Icon` is filled in. An unmodified item tints White, so nothing looks
  different from before.
- **`ConditionMultiplier` is authored and read by nothing.** `FInventoryItem::GetConditionScale()`
  computes it correctly and no caller exists — `Condition` itself is still the placeholder it
  always was, and the durability/upkeep pass that consumes both owns that.
- **Loot tables are the only systematic source of modified items.** A table entry's modifier pools
  roll a material and a quality onto what it yields; otherwise a modified item is hand-authored or
  added from the console. Crafting is what will make modifiers a consequence of play rather than of
  a dice roll.
- **A looted chest is full again next session.** The roll is deterministic and containers have no
  record, so each session rebuilds a chest with exactly what it first rolled - the save-scumming
  guarantee - including a chest the player emptied last time. Remembering "already looted" is a
  save-system question; see `game-data.md`.
- **A container's roll happens at `BeginPlay`, on every chest in the level.** Fine for a handful;
  a world of hundreds would want to roll on first open instead. The roll is pure, so moving it is a
  change to *when*, not to *what*.
- **A hand-authored `Modifiers` array can hold two of the same slot.** `AddModifier` refuses it,
  but the array is `EditAnywhere`, so a designer *can* put two materials in a `StartingItems`
  entry by hand. Nothing crashes — the accessors just multiply through everything they find and
  the composed name says both materials — but nothing detects it either. The content sweeps only
  see definition assets, not per-instance authored arrays. Character loadouts moved onto
  `UCharacterDefinition::DefaultLoadout` in game-data Slice 4, so the character sweep *could* now
  check them for a doubled slot; it doesn't yet. Container `StartingItems` remain per-instance.
- No partial-stack drag — the UI always moves the whole stack even though `MoveItem` already
  takes a quantity and supports the split. Splitting needs a player-facing way to say "how
  many", which hasn't been designed.
- The red preview is computed client-side by `WouldAcceptDrop` mirroring `MoveItem`'s rules, so
  the two can drift apart if only one is changed. The preview also says only "not here", never
  "not this" — a drop refused because the destination stack is full looks identical to one
  refused for overlapping another item.
  - A drop refused for a reason the client *couldn't* see is no longer silent:
    `Server_MoveInventoryItem` sends a `NoRoom` back through `Client_NotifyRefusal`, and every
    other refusal in this system now names itself too. See `refusals-and-feedback.md`.
- No re-validation when an item definition's footprint changes under already-placed entries —
  they can end up overlapping until something moves them.
- `SetGridSize` drops entries that no longer fit rather than re-packing them; it's an
  authoring/debug operation, not something gameplay calls. Now that `SortEntries` exists, a
  shrink *could* repack instead of dropping — nothing has needed it yet.
- A sort that can't fit everything **says so now** — `SortEntriesWithReason` reports `NoRoom` and
  `Server_SortInventory` sends it back to the asking client, because "abandoned the repack" and
  "was already sorted" are identical on screen otherwise. An already-sorted grid still says
  nothing, deliberately. See `refusals-and-feedback.md`.
- **Nothing sorts by name or category.** The three criteria are the three *figures* an item
  carries, and alphabetical order was left out because the grid has no icons yet — a name sort
  would be the most useful one the day items are recognisable at a glance, and is a one-line
  addition to `EInventorySortCriterion` plus `GetSortKey`.
- **The sort buttons and the filter box have no keyboard route.** They are widget buttons, not
  keybinds, so a player who prefers the keyboard can't sort at all. Deliberate for now — see
  `input-and-keybinds.md`'s reserved keys before adding one.
- **The filter is forgotten every time the window rebinds**, which is right for a window reused
  across holders but means the player re-picks a category each time they reopen a chest. A
  remembered per-holder filter is a UI-state question nobody has asked for yet.
- **A sort re-picks rotation, so it can undo deliberate packing.** A player who carefully turned
  a sword to fit a corner loses that arrangement to any sort. That's the definition of a repack
  rather than a bug, but it does mean sort is not an undo-able convenience — there is no undo.
- **Equipped weight is reported but never summed with carried weight.** Wearing an item takes it
  out of the grid, so a pawn's `Weight:` readout *drops* when it equips something and the worn
  total is shown in a different window. Nothing reads either figure, so it costs nothing today —
  but the pass that gives weight a gameplay consequence has to add the two, and decide whether
  the readouts merge.
- **Equipment has no ownership check on the wire.** `Server_EquipItem`/`Server_UnequipItem` do no
  more validation than `Server_MoveInventoryItem` does — a client could name any pawn's
  equipment component. Consistent with the existing inventory RPCs rather than a new hole, and
  the fix belongs to all of them at once (see the multiplayer-discipline notes in
  `unreal-module-organization.md`), not to equipment alone.
- No equipped visuals — `WorldMesh` is authored but nothing attaches it to a socket, so an
  equipped item disappears from view entirely rather than showing on the pawn.
- No starting equipment — every pawn starts with empty worn slots; only the character
  definition's `DefaultLoadout` is seeded.
- **Every loose world item looks identical.** All eight definitions point `WorldMesh` at the same
  black placeholder sphere, so an apple and a sword on the ground are indistinguishable until
  picked up. Deliberate — it makes the pickup testable without committing to art — but it is
  placeholder, not a design.
- **A world item can't be reached, only collected.** There's no "go pick that up" order — if no
  pawn is already in range the double-click says "Too far away" rather than walking anyone over.
  Routing the pickup through a move command is a unit-commands change, not an inventory one.
- **Nothing that a body drops is actually dropped.** A killed NPC keeps its inventory on the
  corpse actor, which is exactly the design (`Docs/roadmaps/inventory-roadmap.md`: same actor, same code path,
  no corpse container), but the actor also never despawns — a dead unit stands in the level
  permanently holding its pack. Lifetime for bodies is a combat/characters question, not an
  inventory one.
- No 2D item art — every `DA_Item_*` has a null `Icon`, so the UI shows names only. The item widget draws no icon at all yet, which is why the label's 90° turn for tall
  footprints matters as much as it does.
- **Gold isn't persisted anywhere**, and a player state re-created mid-session re-seeds from
  `StartingGold`. That's a save-system question, not an economy one.
- **A purchase the player can only partly afford is refused outright.** A stack of 20 apples at
  3 gold is priced at 60 and rejected whole at 50 gold, rather than selling 16. Deliberate — it
  is what makes "insufficient gold changes nothing" true — but the *reason* it can't do better
  is that partial-stack drag doesn't exist, so the player has no way to ask for 16. Whichever
  slice builds partial-stack drag should revisit this. The player is at least told *why* now
  ("Not enough gold"), which makes the refusal legible but no less absolute.
- A refused purchase **now says "Not enough gold"** — `TryTradeItem` sends `CannotAfford` back
  through `Client_NotifyRefusal`. This was the case that justified building the refusal system
  at all: unlike a bad *placement* there is no red preview to explain it, because the cells were
  fine and the purse wasn't. See `refusals-and-feedback.md`.
- **A trader's stock never restocks, and the gold they pay out is imaginary.** `StartingStock` is
  placed once at `BeginPlay`; buying a shelf empty leaves it empty for the session, and a trader
  will buy an unlimited amount from the player without ever running short of money, because
  traders have no wallet of their own. Both wait on the market simulation.
- **A dead trader's wares are unreachable.** Looting a body opens that NPC's personal
  `Inventory`; the `UTraderComponent`'s stock is a second grid on the same actor and nothing
  opens it. Killing a merchant therefore destroys their shop rather than looting it. Needs a
  decision about whether a body should expose more than one grid, which is a loot question
  rather than a pricing one.
- **Prices are flat and identical for everyone.** No haggling, no skill or reputation modifier,
  no per-region variation, and both directions come off one `BaseValue` — deliberate, since the
  market simulation that varies them is a much later `SmoresMarkets` implementing
  `IPricingProvider` from above.
- **The hover tooltip is the only place a price appears.** There's no price column in the grid,
  no running total while dragging, and nothing shows the balance next to the trade window (the
  HUD's `Gold:` readout is the only balance on screen).
- **Weight has no gameplay consequence.** Nothing reads `IsOverWeightCapacity` but the
  readout's colour — no speed penalty, no noise penalty, and no pickup is ever refused for
  being too heavy. That's a deliberate deferral, not an oversight, but it does mean an
  over-capacity pawn looks warned-about while nothing is actually happening.
- The gold readout is **polled from `DrawHUD` every frame**, not driven by
  `OnGoldChanged`. That delegate exists and fires correctly on both sides, but nothing
  subscribes to it yet — a consumer that needs to *react* to a balance change (a purchase
  confirmation, an alert) should bind it rather than add a second poll.
- `WeightCapacity` replicates but never changes at runtime, so no refresh is wired to it —
  a future system that varies capacity (a pack upgrade, a strength attribute) needs to
  broadcast `OnInventoryChanged` itself, since only entry changes redraw the window today.
- No stolen-item flag or theft/detection mechanics.
