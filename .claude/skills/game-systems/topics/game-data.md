# Game Data: Definitions, Records and Actors

## Purpose

Where the game keeps its stuff. Not how the stuff behaves — where it lives, how it's authored,
and what will survive a save. This is the storage layer underneath items, characters, factions
and loot, and it deliberately stops at the point where behavior begins. Loot tables are the one
piece that *rolls* anything, and the rolling is seeded so that what it produces is as repeatable
as any other stored fact.

**All three layers now exist.** Factions have records (`FFactionRecord`, see `factions.md`), and
every unit in the level stands in for an `FCharacterRecord` under the *soft split* described below:
records and actors live and die together, and the actor is a working copy kept in sync. The *full*
split - actors spawning near players and despawning away from them while records persist - is the
world-activity roadmap's, not built.

## The Three Layers

| Layer | What it is | Where it lives | Saved? | Built? |
|---|---|---|---|---|
| **Definition** | What a *kind of thing* is — "Sword", "Bandit", "Ironclan". Authored by hand, identical in every campaign, never written to at runtime. | A `.uasset` in the Content Browser | No — it ships with the game | ✅ |
| **Record** | One *particular* thing — this bandit, with this name, this health, this inventory. | A `USTRUCT` in memory, owned by a component on the `GameState` or `PlayerState` | **Yes — records *are* the save file** (no save exists yet) | ✅ factions (`FFactionRecord`) and characters (`FCharacterRecord`) |
| **Actor** | What is physically standing in the level right now. A puppet driven by a record. | `AStrategyUnit` and friends, in the loaded world | No — it is rebuilt from the record | ✅ soft split - every unit is bound to a record; nothing spawns one from a record yet |

The rule the whole model turns on:

> **A definition is never written to. A record is never a copy of the actor — the actor is a
> copy of the record.**

The reason for the inversion is that an actor is expensive — a skeletal mesh, an anim blueprint,
a collision body, an AI controller running pathfinding, a `Tick`. A populated open world holds
far more characters than that can support, and most of them are nowhere near a player. A
character has to be able to die in a battle the player never saw, which means something other
than an actor has to be able to kill them.

## Core Rules

### Every definition derives from `USmoresDefinition`

It carries **four fields** — `DefinitionId`, `DisplayName`, `Description` and `Tags` — because
those are the only four every definition type genuinely shares. `Tags` (an
`FGameplayTagContainer`) arrived in Slice 5 with its first reader, a loot table's "any item with
this tag" entry; it passes the test `Icon` fails below, because an empty tag container is a
legitimate state for *every* type, so nothing ever has to require one. The tag vocabulary lives in
`Config/DefaultGameplayTags.ini`, which holds exactly one tag today (`Item.Mineral`) — add one only
when something reads it.

**Presentation deliberately stays on the subclasses.** It is tempting to hoist `Icon` up too,
but the types disagree about what a picture even is: an item has a small transparent sprite
sized to a grid cell, a character has a framed portrait, a faction has a crest, and a loot table
has none at all. A shared `Icon` would be permanently null on some types and — the practical
cost — **impossible to require**, because the content sweep could never assert "this is set"
without failing the types that legitimately have none. Left on `UItemDefinition`, `Icon` can be
demanded of an item and of nothing else. The same reasoning keeps `WorldMesh`, footprint, stack
size and equip slot where they are.

For types the player never sees, `DisplayName` and `Description` are the designer's own label
and notes.

### Definitions are referenced by id in saved data, by asset pointer in content

- A **record** references its definition by id.
- A **save** stores ids.
- But a **definition referencing another definition** (a recipe naming its output, a character
  naming its loadout, a loot table naming an item or a sub-table) uses an ordinary asset pointer —
  that is normal content linking, it cooks correctly, and the editor shows the reference.

This topic used to list loot-table entries under "by id". Slice 5 built them with asset pointers
instead, per the roadmap's settled decision that the id rule "applies only to data that gets
saved": a table is authored content and nothing about it is saved. What *does* get saved - the
items a roll put in a container - are ordinary `FInventoryItem`s, and inherit that struct's
pointer-not-id gap (see Known Gaps). The mod case the id rule was reaching for is served by tags
instead: a mod's new ore tagged `Item.Mineral` joins every "any mineral" entry without editing a
table.

The id rule exists for one reason: a save written with a mod installed should load without it,
stripping that mod's content rather than refusing to open. That only works if saved data holds
ids that are *allowed* to fail to resolve. It buys nothing for content links, which cook.

### An id only has to be unique within its type

`FPrimaryAssetId` is a `{Type, Name}` pair — `ItemDefinition:Sword`. An item and a faction
may both legitimately call themselves `Ironclan`. The content sweep checks uniqueness per type,
not globally.

## C++ Implementation

### `USmoresDefinition` — `Source/SmoresCore/SmoresDefinition.h`

`UCLASS(Abstract, BlueprintType)`, deriving `UPrimaryDataAsset`. Holds the four shared fields
and two virtuals:

- `GetDefinitionType()` — pure virtual. Each concrete type returns its own Asset Manager type
  name. **The string it returns has to match that type's `PrimaryAssetTypesToScan` entry in
  `Config/DefaultGame.ini`**, or nothing will ever resolve one of its assets by id.
- `GetPrimaryAssetId()` — returns `{GetDefinitionType(), DefinitionId}`. It falls back to the
  engine's asset-name behavior when `DefinitionId` is `None`, which covers both an unfilled asset
  and the class-default object of an abstract type (which must never reach the pure virtual).

### `USmoresDefinitionLibrary` — `Source/SmoresCore/SmoresDefinitionLibrary.h`

A `UBlueprintFunctionLibrary` wrapping `UAssetManager`. Three calls:

| Call | Answers |
|---|---|
| `FindDefinition(Type, Id)` | "give me the definition whose id is `Sword`" — null when it resolves to nothing, which is a legitimate outcome, not an error |
| `GetDefinitionIds(Type)` | every id the Asset Manager knows for one type |
| `GetDefinitionTypes()` | every registered type whose base class is a `USmoresDefinition` — i.e. the project's own, and none of the engine's `Map`/`PrimaryAssetLabel`/`GameFeatureData` |

**The load is synchronous, deliberately.** A few dozen small data assets cost nothing to pull in
on demand, and an async handle would force every caller to deal with "not yet". If the definition
count reaches the thousands, or definitions start carrying heavy art, this is the seam that
becomes an async preload — callers ask the same question either way.

`FindDefinition` checks `GetPrimaryAssetObject` first, then falls back to resolving the id to a
`FSoftObjectPath` and `TryLoad`-ing it, rather than `LoadPrimaryAsset` — which would hand back a
streaming handle every caller would have to keep alive.

### Registering a definition type — the step that has no compile error

A type is only reachable by id once it has a line in `Config/DefaultGame.ini` under
`[/Script/Engine.AssetManagerSettings]`:

```ini
+PrimaryAssetTypesToScan=(PrimaryAssetType="ItemDefinition",AssetBaseClass="/Script/SmoresItems.ItemDefinition",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
```

Forgetting it breaks every record, loot table and save that names one of that type's assets, and
breaks **nothing at all at compile time**. `Smores.Content.Definitions.EveryDefinitionResolvesById`
is the test that catches it — it exists for exactly this failure.

### Concrete types today

| Type | Module | `GetDefinitionType()` | Assets |
|---|---|---|---|
| `UItemDefinition` | `SmoresItems` | `ItemDefinition` | eight `DA_Item_*` under `Content/Items/` |
| `UItemModifierDefinition` | `SmoresItems` | `ItemModifierDefinition` | five `DA_Modifier_*` under `Content/Items/Modifiers/` |
| `UFactionDefinition` | `SmoresCore` | `FactionDefinition` | four `DA_Faction_*` under `Content/Factions/` — see `factions.md` |
| `UCharacterDefinition` | `SmoresCharacters` | `CharacterDefinition` | three `DA_Character_*` under `Content/Characters/Definitions/` — see "Character Records" below |
| `ULootTableDefinition` | `SmoresItems` | `LootTableDefinition` | five `DA_Loot_*` under `Content/Items/LootTables/` — see "Weighted Tables and Seeded Rolls" below |

`UWeightedTableDefinition` (`SmoresCore`) is abstract and has no config line of its own: an
abstract base is never an asset, and each concrete table type registers under its own name.

**Not every definition is an asset.** Dialog - barks and conversations - is definition data
in exactly this sense (authored, identical on every machine, never written in play), but it is read
from plain text files at startup rather than authored as `.uasset`s, so that a writer or a modder
needs no Unreal at all. It is therefore not a `USmoresDefinition`, isn't in the Asset Manager, and
has its own loader and content sweep. It still follows the id rule: a bark or a conversation names
a faction or a character definition by id, and an id nothing resolves is a warning. What a squad
has said and heard - its dialog flags and the conversations it has seen - is a **record**, a plain
reflected `FDialogMemoryRecord` on `UDialogMemoryComponent`, for the save system to serialize
unchanged. See `dialog.md`.

Each type's `DefinitionType` is a `static const FPrimaryAssetType` holding the literal type
string, spelled out rather than derived from the class name so it and the config line are visibly
the same string.

`UItemModifierDefinition` is the first type to arrive *after* the base existed, and it cost
exactly the five steps in the extension-point list below and nothing else — the base content
sweeps picked its assets up with no change, which is the shape a faction or character definition
should expect. See `inventory.md` for what a modifier actually does to an item.

It is also the first place the id look-up has a caller that isn't `SmoresDumpDefinitions`:
`SmoresAddItem <Count> <ModifierId>` sends the id to the server and resolves it there through
`USmoresDefinitionLibrary::FindDefinition`, which is the route a record or a loot table will take.

## Character Records and the Soft Split

Built by Slice 4 of `Docs/roadmaps/game-data-roadmap.md`. Every unit in the level now stands in for
an `FCharacterRecord`, and the record - not the unit - is the truth about that character.

### Three pieces

| Piece | What it is | Lives in |
|---|---|---|
| `UCharacterDefinition` | a *kind* of character - "Bandit", "Settler", or one named individual. Unique flag, backstory, portrait, default faction id, role id, the seven base attributes, a default loadout, a name pool, and the actor class that would stand in for one | `SmoresCharacters/CharacterDefinition.h`; assets under `Content/Characters/Definitions/` |
| `FCharacterRecord` | *this* character: `RecordId`, `DefinitionId`, `Name`, `FactionId`, `Attributes` (identity), and `Health`, `LifeState`, `LastKnownLocation`, `Carried`, `Equipped` (condition) | `SmoresCharacters/CharacterRecord.h`, with `FCharacterAttributes` |
| `UCharacterRecordComponent` | every record in the session, plus which live actor stands in for which | `SmoresCharacters/CharacterRecordComponent.h`, a default subobject of `AStrategyGameState` |

`FCharacterAttributes` holds the seven attributes from `characters-and-squads.md` (Strength,
Endurance, Agility, Perception, Intelligence, Willpower, Charisma) as floats defaulting to 10.
**Nothing reads them**, and 10 is a placeholder baseline, not a designed scale. There is
deliberately no skill map - the skill roster is undecided. `LifeState` reuses `EHealthState`
rather than mirroring it in a second enum.

### The soft split - the boundary, written down

Each of these sentences is something the world-activity roadmap will change. They are here so it
is obvious what it is changing.

- **Every record has exactly one actor, and they are made together.** A unit finds or creates its
  record at `BeginPlay`. Nothing spawns a unit from a record and nothing despawns one.
- **Identity flows record → actor; condition flows actor → record.** On creation the unit copies
  the record's name and faction onto itself. From then on its components are the *working copy*
  that play reads every frame, and every change is written back.
- **Write-back happens on every change, not at despawn, because there is no despawn.** Health
  (`OnDamaged` via `OnHealthDamaged`; `OnDowned`/`OnRecovered`/`OnDied`), the carried grid
  (`OnInventoryChanged`), the paperdoll (`OnEquipmentChanged`) all call
  `AStrategyUnit::WriteBackToRecord`, which copies all five condition fields.
  `LastKnownLocation` is written on arrival from a move and alongside every other write-back -
  not every frame - so a walking unit's record lags its actor until it stops.
- **The record outlives its actor.** `EndPlay` writes back one last time and unbinds; it never
  removes the record. Nothing mid-session destroys a unit today, but a record left behind is
  exactly what a dead named character needs.
- **Nothing advances a record that has no actor.** Records only change because an actor wrote
  them.

### Finding or creating the record - `AStrategyUnit::RegisterWithRecordStore`

Authority only; clients see the unit through its replicated components.

1. Look up the store (`UCharacterRecordComponent::Get` - the component on the world's GameState).
   None (the main menu, a bare test world) → run as a plain actor, exactly as before records.
2. **Adopt** if a record already exists under the unit's `PlacedRecordId`: bind to it and
   `ApplyRecordToActor` - name, faction, location, then `RestoreEntries`, `RestoreEquippedItems`,
   and finally `RestoreState`, so a unit restored Dead goes inert holding the pack it died with.
   The loadout is *not* handed out again.
3. If that record already has a *different* live actor bound (two placed units sharing a key), log
   an error and fall through to creating a fresh record under a new id.
4. **Create** otherwise: `CreateRecord(CharacterDefinition, PlacedRecordId, UnitDisplayName)`, bind,
   copy name and faction onto the actor, `AddItem` each `DefaultLoadout` entry through the ordinary
   grid rules, then write back.

`ApplyRecordToActor` sets `bApplyingRecord` for its duration so the components' change broadcasts
don't write a half-applied state back into the record it is reading.

### Placed units register once - `PlacedRecordId`

A unit placed in a level carries an authored `FGuid PlacedRecordId` (`EditInstanceOnly`, advanced).
It is how a placed actor finds *its* record on the next load instead of creating a fresh one -
without it, reloading a save stands the dead boss back up. It is authored rather than derived from
the actor's runtime name because that name isn't stable across level edits.

- `AStrategyUnit::PostActorCreated` generates one when a unit is placed in an editor world, and
  `PostEditImport` generates a fresh one when a unit is pasted or alt-dragged (a copy is a different
  person). PIE duplication goes through neither, so a PIE copy keeps its key.
- A unit spawned at runtime has no key and gets a freshly minted record id. A *placed* unit with no
  key (`IsNetStartupActor()`) logs a warning - it works, but a save could never find it again.
- All nine units in `LVL_Strategy` had keys authored by hand, since they predate the auto-generation.

### Names

- A **unique** definition's record is named its `DisplayName`, always.
- Otherwise a placed unit's authored `UnitDisplayName`, when set, names the record - which is why
  "Pawn 1", "NPC 3" and "Merchant Ada" still read the same in play.
- Otherwise the name is rolled from `NamePool`, **seeded from the record id** (`GetTypeHash(RecordId)`
  mod pool size) rather than a random stream, so the same individual always gets the same name - the
  save-scumming rule applied to names. An empty pool or a blank entry falls back to `DisplayName`.

`UnitDisplayName` is now `Replicated`: it is the actor's copy of the record's name, and a client has
to show the name the server chose. `FactionId` is copied the same way (`GetFactionId()`).

### Unique characters

`bUnique` means exactly one record of that definition, **ever** - alive or dead.
`CreateRecord` refuses a second (`HasRecordOfDefinition`) with an error, and a second placed actor of
a unique definition runs without a record rather than becoming a second Kess. Uniqueness is authored
on the definition; condition is in the record; nothing writes "is he dead?" to the asset.

### Faction ids

`CreateRecord` copies `DefaultFactionId` and checks it with `IsFactionKnown`, which asks the
`UWorldFactionComponent` beside it once that has seeded, and otherwise (the two `BeginPlay` in no
guaranteed order) whether the id resolves to a faction definition at all. An unknown id is **kept
with a warning** - a stripped mod's faction must not stop a save loading. The per-type content sweep
is where an authored typo gets caught.

### Multiplayer shape

- **Server-owned and deliberately not replicated.** Every client already sees each character through
  the actor's replicated components, which *are* the copy of the record. Replicating records as well
  would send every character's pack to every player twice, and once the full split lands, records of
  characters nowhere near anyone would be most of the traffic. A client-side view that needs a record
  (a squad roster) wants its own narrower channel. This is the first piece of session-wide state that
  is *not* replicated - see `multiplayer-discipline.md`.
- Every store mutator (`CreateRecord`, `EditRecord`, `BindActor`) refuses off-authority; the unit's
  `RegisterWithRecordStore`, `WriteBackToRecord` and `ApplyRecordToActor` return early on a client,
  where the component delegates still fire through their `OnRep_`s.
- `EditRecord` hands out a pointer into a `TArray`; don't hold it across a `CreateRecord`.

### The debug exec

`SmoresDumpRecord` (on `AStrategyPlayerController`) prints the targeted NPC's record - or the first
selected unit's - beside the live state of its components, field by field, flagging any line that
disagrees with `<-- MISMATCH`. Hops to the server, since only the server holds records. Location
legitimately disagrees while a unit is walking.

### Content

| Asset | Id | Faction | Used by |
|---|---|---|---|
| `DA_Character_Settler` | `Settler` | none | `BP_PlayerUnit` - loadout Apple + Pocket Knife (moved from the old `AStrategyPlayerUnit::StartingItems`, which is gone) |
| `DA_Character_Bandit` | `Bandit` | `Raiders` | `BP_StrategyUnit` - no loadout |
| `DA_Character_Trader` | `Trader` | `TradersGuild` | `BP_Trader` - role `trader`, no loadout; the shelf stock is `UTraderComponent`'s own grid and is **not** part of the record |

All placeholders for a setting that isn't chosen. No unique character is authored yet.

## Weighted Tables and Seeded Rolls

Built by Slice 5 of `Docs/roadmaps/game-data-roadmap.md`. A chest opened in the desert yields
different things from one opened in a faction town, because each names a different **table** -
and the two tables share their common filler by both naming the same smaller one.

### Two classes, split by what they know

| Class | Module | Knows |
|---|---|---|
| `UWeightedTableDefinition` | `SmoresCore` | weights, how many picks a roll makes (`MinRolls`/`MaxRolls`), nesting and where it gives up, and how a roll is seeded. **Nothing about what a table yields** |
| `ULootTableDefinition` | `SmoresItems` | what a landed pick turns into: an item, a sub-table, "any item with this tag", or nothing - plus a quantity range and modifier pools |

The base asks its subclass three questions about its entries - `GetNumEntries`, `GetEntryWeight`,
`GetEntrySubTable` - and hands back each pick that landed on a payload through a callback. That is
the whole seam: the world-activity roadmap's spawn table is a second subclass with characters where
the items are, and none of the rolling logic moves.

The base is in `SmoresCore` rather than beside the loot table (where the roadmap's placement table
put it) because nothing in it is about items; a spawn table should not have to depend on
`SmoresItems` for its base class.

### How a roll works - `UWeightedTableDefinition::RollEntries`

1. Draw a pick count from `[MinRolls, MaxRolls]`, inclusive. A `MaxRolls` below `MinRolls` reads as
   `MinRolls`; zero picks is legitimate ("sometimes nothing at all").
2. For each pick, one draw across the total weight, walked entry by entry. **Weights are integers**,
   so `60 / 30 / 10` means exactly that. An entry of weight zero is never picked; a table with no
   weight at all picks nothing.
3. A pick landing on a **sub-table rolls that table in full** - its own pick count, its own weights.
   `DesertChest` rolling 2-4 times and landing on `CommonJunk` twice is two complete rolls of
   `CommonJunk`.
4. Anything else is handed to the subclass, which turns it into an item.

**Nesting is bounded at `MaxNestingDepth` (8).** A table that names itself, directly or through
another, would otherwise never finish. Hitting the limit abandons the *whole* roll - not just the
branch - logs one error, and returns false; abandoning only the branch would still cost
picks^depth before stopping. The content sweep refuses such a table outright
(`ContainsNestingLoop`, which tells a real loop from a diamond - two branches sharing one table is
fine).

### What a loot entry yields - `ULootTableDefinition`

| `Kind` | Yields |
|---|---|
| `Item` | `[MinQuantity, MaxQuantity]` of one item |
| `Table` | a full roll of another `ULootTableDefinition` |
| `Tag` | one item carrying the tag (`HasTag`, so a child tag counts), chosen **evenly** among the candidates |
| `Nothing` | an empty pick - weight given to "nothing this time" |

- **Tag candidates are sorted by `DefinitionId` before the pick.** The Asset Manager hands items
  back in scan order, which isn't stable; without the sort, "any mineral" could land on a different
  mineral on a different machine from the same seed.
- **Modifier pools.** An `Item` or `Tag` entry carries `ModifierPools`; each pool is rolled once and
  lands on one choice, which may be "none". Landings apply through `FInventoryItem::AddModifier`,
  which refuses a second modifier in a filled slot - so the table only chooses, and a later pool is
  a **fallback** for an earlier one that came up empty ("steel one time in ten, otherwise bronze or
  iron" is `[Steel 1, none 9]` then `[Bronze 1, Iron 1]`). This is what first makes modifier-aware
  stacking visible in bulk: a chest rolling bronze *and* iron knives holds two piles.
- **Rolling is pure.** `RollLoot(Stream, TagCandidates, OutItems, OutSourceTableIds)` turns a stream
  into item instances and touches nothing else. The overload without candidates gathers every
  registered item (`GatherRegisteredItems`). `OutSourceTableIds` names, per item, the innermost table
  that produced it - it is how the debug exec says "from CommonJunk".
- An entry naming nothing usable (an `Item` entry with no item, a tag nothing carries) yields
  nothing and warns. The content sweep is where that is meant to be caught.

### Seeded, never random - `MakeRollStream` and the world seed

`characters-and-squads.md` requires a retry under identical conditions to produce an identical
result, so no roll ever touches a global random number generator. Every roll draws from an
`FRandomStream` its caller made, and a real roll's stream comes from
`UWeightedTableDefinition::MakeRollStream(WorldSeed, StableId)`:

- **`WorldSeed`** is `UWorldSeedComponent` (`SmoresCore`) on `AStrategyGameState` - one number per
  campaign, authored on the GameState Blueprint for now (default 0), with `SetWorldSeed` as the seam
  a new-campaign screen will call. **Server-only and not replicated**: every roll that reads it is
  authority-only, and a client holding it could compute every chest before opening one. A world
  with no seed component (a bare test world, the main menu) rolls from 0 - still deterministic.
- **`StableId`** is the rolling thing's own authored key - for a container, `PlacedContainerId`
  (see `inventory.md`).
- **The mixing is a CRC-32 of five little-endian words** (the seed and the four parts of the GUID),
  spelled out rather than `HashCombine`, because an engine upgrade is free to change `HashCombine`
  and doing so would re-roll every container in every save.
  `Smores.Items.LootTable.RollStreamIsPinned` asserts two known outputs (computed independently with
  Python's `zlib.crc32`), so changing the formula means deleting a test that says not to.

The consequence is the design point: reloading and reopening the same chest in the same campaign
finds the same contents, so there is nothing to save-scum. A different chest, or the same chest in a
different campaign, rolls differently.

### Content

| Asset | Id | Rolls | Entries (weight, quantity) |
|---|---|---|---|
| `DA_Loot_CommonJunk` | `CommonJunk` | 1 | Apple 5 (1-3), Torch 4 (1-2), Rope 3, Pocket Knife 2 (iron 3 / bronze 2), Nothing 2 |
| `DA_Loot_DesertMinerals` | `DesertMinerals` | 1 | any `Item.Mineral` 3 (1-4), Rock Salt 2 (2-5) |
| `DA_Loot_IronclanGoods` | `IronclanGoods` | 1 | Sword 2 (iron 3 / steel 1, then well-made 1 / none 4), Trap Kit 1, Health Potion 2 (1-2), Gold Coin 3 (5-20) |
| `DA_Loot_DesertChest` | `DesertChest` | 2-4 | CommonJunk 60, DesertMinerals 30, Health Potion 10 |
| `DA_Loot_FactionTownChest` | `FactionTownChest` | 2-4 | CommonJunk 60, IronclanGoods 30, Gold Coin 10 (10-30) |

The two situation tables are the roadmap's worked example made real: both name the *same*
`CommonJunk` asset, authored once. Three mineral items arrived with them so `DesertMinerals` has
something to find - `DA_Item_CopperOre`, `DA_Item_RockSalt`, `DA_Item_Sulfur`, all tagged
`Item.Mineral` (see `inventory.md`). All placeholders for a setting that isn't chosen; "Ironclan" is
the faction the name borrows, and nothing reads that link.

## The Content Sweeps

Two layers, matching the class hierarchy. Both run against **real assets under `/Game/`**, which
is the opposite of every other test in the suite — everything else builds its inputs in memory
precisely so a designer retuning an asset can't break it. These assert that the content *tree* is
well formed, which is not a claim about any one asset.

**Base layer** — `Source/SmoresCore/Tests/SmoresDefinitionAssetTest.cpp`, over every
`USmoresDefinition`:

| Test | Catches |
|---|---|
| `EveryAssetIsWellFormed` | an id or display name left blank — the asset is then unreachable by id no matter what it is named |
| `DefinitionIdsAreUniqueWithinType` | two assets sharing an id, so something resolves the wrong one and nothing says so |
| `EveryDefinitionResolvesById` | a missing or mistyped `PrimaryAssetTypesToScan` entry |
| `AtLeastOneTypeIsRegistered` | the config side going empty, which would make the test above pass vacuously |

**Per-type layer** — `Source/SmoresItems/Tests/ItemDefinitionAssetTest.cpp` is the worked example
a future faction or character type should copy, and
`Source/SmoresItems/Tests/ItemModifierDefinitionAssetTest.cpp` is the same shape for modifiers
(and `Source/SmoresCore/Tests/FactionDefinitionAssetTest.cpp` for factions, which adds the one
*cross-asset* rule so far: a starting relation authored on both factions has to agree, and
`Source/SmoresCharacters/Tests/CharacterDefinitionAssetTest.cpp` for characters: a
`DefaultFactionId` must resolve - the one rule the runtime deliberately *doesn't* enforce - a
non-unique character needs a `NamePool`, no pool entry may be blank, no loadout entry may name
nothing, and no attribute may be negative; and
`Source/SmoresItems/Tests/LootTableDefinitionAssetTest.cpp` for loot tables: rolls and quantities
in order, no zero weight on an entry or a modifier choice, every entry naming something, every
`Tag` entry's tag carried by at least one real item, and no nesting loop - nearly all of which fail
*silently* in play, as "this chest seems a bit empty"). The
modifier one is worth reading for *why* a per-type file earns its place: a modifier's numbers are
multipliers, and a multiplier fails differently from a weight or a price. An unfilled field
defaults to 1.0 and is invisible; a field authored to **0** silently erases whatever it
multiplies, and a masterwork spear weighing nothing reads as a bug in the inventory rather than
in the asset. It also rejects an empty `NamePattern`, because the fallback is hard-coded English
word order. Neither rule is one the base sweep could ever know about. It adds footprint bounds, stack-size bounds and
the `Icon` check, and proves the rules themselves against an in-memory definition
(`MalformedDefinitionIsRejected`) rather than by adding a broken asset to `Content/`.

The shared helpers — `ValidateSmoresDefinition()` and `GatherDefinitionAssets()` — live in
`Source/SmoresCore/Tests/SmoresDefinitionRules.h`. `SmoresCore` takes a private `AssetRegistry`
dependency solely for them.

Every sweep asserts it found at least one asset before checking anything: a sweep that finds
nothing is green having looked at nothing, which is `testing.md`'s "check the number, not the
colour" one level down.

**The `Icon` check is currently a warning, not an error**, because no item art has been authored
yet and all eight assets would fail. Promote it to `AddError` the moment the first icon exists —
being able to *require* it of items is the entire reason `Icon` stayed off the base class.

## Player Surface

None. This layer has no player-facing behavior of its own; it is what other systems read.

The one visible surface is a debug exec on `AStrategyPlayerController`:

| Command | Does |
|---|---|
| `SmoresDumpDefinitions` | lists every definition the Asset Manager found, grouped by type, with its id and display name |
| `SmoresDumpRecord` | prints a unit's character record beside its live component state, flagging mismatches - see "The debug exec" above. Hops to the server |
| `SmoresRollTable <TableId> [Seed] [Count]` | rolls a loot table `Count` times from seeds `Seed`, `Seed+1`, ... and prints each roll (every item, with the table it came from) and, for more than one roll, a tally. Touches nothing - no container, no world seed - so it is how to judge a table's spread without opening chests. Detail is printed for up to 10 rolls; beyond that only the tally |

Unlike the other `Smores*` execs, `SmoresDumpDefinitions` and `SmoresRollTable` do **not** hop to
the server — the definition registry and the tables in it are authored content, identical on every
machine.

## Extension Points

**Adding a new definition type**, in order:

1. Subclass `USmoresDefinition` in the module that owns the domain. Override
   `GetDefinitionType()` with a `static const FPrimaryAssetType` on the class.
2. Add its `PrimaryAssetTypesToScan` line to `Config/DefaultGame.ini` (see above). **This is the
   step with no compile error.**
3. Cold build — a new `UCLASS` is never a Live Coding change.
4. Author the assets.
5. Add a per-type test file alongside `ItemDefinitionAssetTest.cpp` if the type has rules beyond
   "has an id, has a name". The base sweeps pick it up with no change.

`SmoresDumpDefinitions` needs no update — it reads whatever types the Asset Manager reports.

**Adding a new kind of weighted table** (the spawn table is the expected first): subclass
`UWeightedTableDefinition` in the module that owns the payload, hold its own entry struct array,
answer `GetNumEntries` / `GetEntryWeight` / `GetEntrySubTable`, and turn each pick handed back by
`RollEntries` into the payload - `ULootTableDefinition::RollLoot` is the worked example, about
forty lines. Then the five steps above. Seed it with `MakeRollStream` from the world seed and the
rolling thing's own authored key; never a fresh `FRandomStream` from the clock.

**Adding a new loot table** is content only: a `DA_Loot_*` asset under `Content/Items/LootTables/`,
then point a container's `LootTable` at it. `SmoresRollTable <Id> 0 100` shows its spread; the
per-type sweep catches the silent mistakes.

**Adding a gameplay tag**: a `+GameplayTagList` line in `Config/DefaultGameplayTags.ini`, then tag
the definitions. Add one only when a table (or something else) reads it.

**Renaming or moving a definition property** needs a `CoreRedirect` in `DefaultEngine.ini` or the
authored values are silently lost. Moving a property to a base class *without* renaming needs
none (Unreal serializes by name); doing both at once does. See
`unreal-module-organization.md`'s per-move mechanics.

## Known Gaps

- **The soft split only.** No record exists without an actor, nothing spawns an actor from a record
  (`UCharacterDefinition::ActorClass` is authored and read by nothing), and nothing advances a record
  on its own. That is the world-activity roadmap's full split.
- **A record can't be created without an actor to place its loadout.** `CreateRecord` fills the
  identity half only; the `DefaultLoadout` is placed by the actor's grid, then written back. The
  full split will need a grid-free packer (or records whose `Carried` starts unplaced).
- **Records are not saved** - no save system exists. They are plain reflected structs holding their
  definition by id, except for the item pointers below.
- **`MaxHealth` is on the actor's component, not the definition or record.** The record holds
  current health only, so a record without an actor can't say what "full health" is.
- **No containment** between records (`ParentRecordId`) - see the roadmap's open questions.
- **Clients can't read records** (deliberate - see "Multiplayer shape"). Today nothing is lost,
  because health, pack, paperdoll, name and faction all reach clients through the unit. What *is*
  invisible off the server: `Attributes`, and (after the full split) any character with no unit.
  `SmoresDumpRecord` also prints to the server's log, not a remote player's. When the first screen
  needs record data on a client, send only what it needs - copy the field onto the unit like
  `FactionId`, or a per-player component carrying just that player's squad records
  (`COND_OwnerOnly`, the `UPlayerStandingComponent` pattern). Don't replicate the whole store.
- **`FindDefinition`'s callers are still few.** The live ones are `SmoresDumpDefinitions`,
  `SmoresRollTable`, the `SmoresAddItem` modifier look-up, `UWorldFactionComponent`'s `BeginPlay`
  (which resolves every faction id to seed the records), `UCharacterRecordComponent::IsFactionKnown`'s
  fallback, and `ULootTableDefinition::GatherRegisteredItems` (the tag candidates for every container
  roll). Loot tables themselves hold items by asset pointer - content linking - so a save loading
  without a mod is still a save-system question, not a table one. Character units still hold their
  definition by asset pointer too; only the *record* holds the id.
- **`FInventoryItem` still holds a definition by `TObjectPtr`, not by id** — and now holds its
  modifiers the same way. That is correct for a carried item under the current design, but the
  roadmap notes the carried item and the record must hold ids once saving is real, and that
  applies to the `Modifiers` array as much as to `Definition`. **`FCharacterRecord::Carried` and
  `Equipped` inherit this** - a character record is id-clean except for the items it holds.
- **Only items are read for tags.** `Tags` is on every definition, but the one reader is a loot
  table's `Tag` entry, which looks only at items. A faction or character tag means nothing yet.
- **The world seed is authored, not chosen, and not saved.** Every campaign today has the seed on
  `BP_StrategyGameState` (0 unless someone changes it). A new-campaign screen should pick one and
  call `SetWorldSeed` *before* anything rolls; the save system must store it, or a reload re-rolls
  every container.
- **A container has no record, so "already looted" is not remembered.** The roll is deterministic,
  so a new session rebuilds each chest with exactly what it first held - which is the save-scumming
  guarantee - but it also means a chest emptied last session is full again. Container contents
  becoming a record (or the container remembering it rolled) is a save-system job; under the soft
  split nothing persists across sessions anyway.
- **Tag entries choose evenly.** "Any mineral" gives every tagged item the same chance. A rare
  mineral that should be rarer needs its own weighted entry beside the tag, which is how
  `DesertMinerals` makes rock salt common. Per-item weights under a tag would be a new field on the
  item, and nothing has needed it.
- **No context filtering, deliberately.** A table can't ask "which region am I in?" - composition is
  the only tool, per the roadmap's settled decision. Add context parameters only if authoring
  genuinely becomes painful.
- **The spawn-table sibling isn't built.** `UWeightedTableDefinition` is ready for it; the
  world-activity roadmap owns it.
- **Synchronous loading.** See above — deliberate, and the seam is `USmoresDefinitionLibrary`.
