# Game Data: Definitions, Records and Actors

## Purpose

Where the game keeps its stuff. Not how the stuff behaves — where it lives, how it's authored,
and what will survive a save. This is the storage layer underneath items, characters, factions
and loot, and it deliberately stops at the point where behavior begins.

**Only the first of the three layers exists today.** Records and the record/actor split are
future slices of `Docs/roadmaps/game-data-roadmap.md`; they are described here because the
definition layer was built the shape it is *because* of them, and that shape is hard to
understand otherwise.

## The Three Layers

| Layer | What it is | Where it lives | Saved? | Built? |
|---|---|---|---|---|
| **Definition** | What a *kind of thing* is — "Sword", "Bandit", "Ironclan". Authored by hand, identical in every campaign, never written to at runtime. | A `.uasset` in the Content Browser | No — it ships with the game | ✅ |
| **Record** | One *particular* thing — this bandit, with this name, this health, this inventory. | A `USTRUCT` in memory, owned by a component on the `GameState` or `PlayerState` | **Yes — records *are* the save file** | ❌ not yet |
| **Actor** | What is physically standing in the level right now. A puppet driven by a record. | `AStrategyUnit` and friends, in the loaded world | No — it is rebuilt from the record | partly (actors exist; nothing drives them from a record) |

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

It carries **exactly three fields** — `DefinitionId`, `DisplayName`, `Description` — because
those are the only three every definition type genuinely shares.

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
- A **loot table** entry names an item by id.
- A **save** stores ids.
- But a **definition referencing another definition** (a recipe naming its output, a character
  naming its loadout) uses an ordinary asset pointer — that is normal content linking, it cooks
  correctly, and the editor shows the reference.

The id rule exists for one reason: a save written with a mod installed should load without it,
stripping that mod's content rather than refusing to open. That only works if saved data holds
ids that are *allowed* to fail to resolve. It buys nothing for content links, which cook.

### An id only has to be unique within its type

`FPrimaryAssetId` is a `{Type, Name}` pair — `ItemDefinition:Sword`. An item and a faction
may both legitimately call themselves `Ironclan`. The content sweep checks uniqueness per type,
not globally.

## C++ Implementation

### `USmoresDefinition` — `Source/SmoresCore/SmoresDefinition.h`

`UCLASS(Abstract, BlueprintType)`, deriving `UPrimaryDataAsset`. Holds the three shared fields
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
`Source/SmoresItems/Tests/ItemModifierDefinitionAssetTest.cpp` is the same shape for modifiers. The
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

Unlike the other `Smores*` execs it does **not** hop to the server — the definition registry is
authored content and is identical on every machine.

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

**Renaming or moving a definition property** needs a `CoreRedirect` in `DefaultEngine.ini` or the
authored values are silently lost. Moving a property to a base class *without* renaming needs
none (Unreal serializes by name); doing both at once does. See
`unreal-module-organization.md`'s per-move mechanics.

## Known Gaps

- **There are no records.** Nothing in the project owns a `FCharacterRecord`, and nothing reads
  through one. `AStrategyUnit` and its components are still the truth. Slice 4 of
  `Docs/roadmaps/game-data-roadmap.md` is where that inverts.
- **Nothing calls `FindDefinition` in anger yet.** The lookup is built and tested; its real
  consumers (records, loot tables, recipes, saves) are later slices. The two live callers are
  `SmoresDumpDefinitions` and the `SmoresAddItem` modifier look-up, both debug execs.
- **`FInventoryItem` still holds a definition by `TObjectPtr`, not by id** — and now holds its
  modifiers the same way. That is correct for a carried item under the current design, but the
  roadmap notes the carried item and the record must hold ids once saving is real, and that
  applies to the `Modifiers` array as much as to `Definition`.
- **No `Tags` on the base.** `FGameplayTagContainer` is planned for Slice 5, when loot-table
  entries first give something a reason to consume it. Adding it earlier would be dead data.
- **Synchronous loading.** See above — deliberate, and the seam is `USmoresDefinitionLibrary`.
