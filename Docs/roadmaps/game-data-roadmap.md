# Game Data Roadmap — Definitions and Records

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how this data layer works will live in the
`game-systems` skill — in a new `game-data.md` topic created by Slice 1, with additions to
`inventory.md` (Slice 2), a new `factions.md` (Slice 3, **written**), `game-data.md` again
(Slice 4, **written**) and `inventory.md` plus `game-data.md` again (Slice 5, **written**).

**All five slices have shipped.** What remains here is the status of each, the notes worth carrying
into whichever roadmap comes next (world activity, saves, crafting), the out-of-scope list, and the
Resolved Design Decisions log.

This roadmap answers one question: **where does the game keep its stuff?** Not how the stuff
behaves — where it lives, how it's authored, and what survives a save. It is the storage layer
underneath items, characters, factions and loot, and it deliberately stops at the point where
behavior begins.

Treat every section as "what to build next," not "what exists" — **except** where a heading is
marked SHIPPED, which means that section has moved into the skill and only its summary line
remains here.

## The Core Idea: Definition, Record, Actor — SHIPPED (Slice 1)

The three-layer model (definition / record / actor), the rule that the actor is a copy of the
record rather than the reverse, and why an actor can't be the truth now live in `game-systems`'
`game-data.md`. Read that first — every slice below assumes it.

### The soft split — what this roadmap actually builds

Two versions of the record/actor relationship exist. **This roadmap builds the first one only.**

- **Soft split (this roadmap).** Every character has a record *and* an actor, created together
  at startup, alive together for the whole session. Nothing spawns or despawns. The only change
  from today is **who owns the data**: the record holds it, and the actor's components are a
  working copy kept in sync.
- **Full split (a later roadmap).** Actors spawn when any player comes near and are destroyed
  when every player leaves; records persist regardless; the faction layer advances records that
  have no actor at all.

The soft split is roughly a session's work and buys the habit that is painful to install later —
everything reads through the record. The full split is worth building the moment there is a
faction simulation with something to resolve; until then it produces disappearing NPCs and no
upside. When it lands it is mostly additive, because the records are already authoritative.

## Common Attributes: One Definition Base — SHIPPED (Slice 1)

`USmoresDefinition`, the three fields every definition shares, why presentation stays on the
subclasses, the id-vs-asset-pointer rule, `USmoresDefinitionLibrary` and Asset Manager
registration are all built and documented in `game-systems`' `game-data.md`.

## Materials and Modifiers — SHIPPED (Slice 2)

`UItemModifierDefinition`, the material/quality slot model, `FInventoryItem::Modifiers`, the
accessors that multiply and compose through it, modifier-aware stacking and the five authored
`DA_Modifier_*` assets are all built and documented in `game-systems`' `inventory.md`, with the
new definition type recorded in `game-data.md`.

## Weighted Tables — SHIPPED (Slice 5)

`UWeightedTableDefinition` (weights, roll counts, nesting, seeding), `ULootTableDefinition` (item,
sub-table, tag and nothing entries with quantity ranges and modifier pools), the world seed and
`MakeRollStream`, and the five authored `DA_Loot_*` tables are built and documented in
`game-systems`' `game-data.md` ("Weighted Tables and Seeded Rolls"); what a container does with a
roll is in `inventory.md`. The three decisions this section argued for - nesting over context
filtering, seeded rolls, and a shared base a spawn table can extend - are all in the code and in
Resolved Design Decisions below.

## Explicitly Out of Scope

This roadmap is the storage layer. Each of the following is a natural next thought and is
**deliberately excluded**, with the roadmap that should own it:

- **Simulated world activity** — off-screen battles, faction decisions advancing records with no
  actor, the full record/actor split with spawn and despawn. Its own roadmap, and the reason the
  soft split is enough for now.
- **Spawn tables, patrols, and creature spawning** — same weighted-table base extended with a
  character payload, but spawning is world activity, not storage. World-activity roadmap.
- **Anything that *reads* faction standing to decide behavior.** Slice 3 stores standing and
  answers "what is X's standing with Y." Nothing derives hostility from it, nothing changes who
  attacks whom, and the placeholder "aggressive units hunt the nearest player pawn" stays
  exactly as it is. Replacing it is an AI change (`ai-and-behavior.md`'s Stance), not a storage
  one.
- **Skills, and any resolution math.** Records carry the seven attributes from
  `characters-and-squads.md` because that list is settled design; they deliberately carry **no
  skill map**, because the same document says the skill roster is explicitly undecided and
  should be assembled per-domain later. Nothing rolls against either.
- **Crafting recipes.** The data shape is sketched above and in the conversation that produced
  this file, but recipes need skills, facilities and a job queue to mean anything. Own roadmap;
  the modifier model in Slice 2 is what keeps it from needing a redesign.
- **Buildings, landmarks and resource deposits.** Same definition/record pattern, blocked on
  `base-building.md`. Own roadmap.
- **NPC dialog and barks.** Needs roles (Slice 4 stores a role id and nothing more) and the
  activity feed, which is built and documented in `game-systems`' `hud-and-panels.md` — its
  "Adding a producer to the activity feed" extension point is the hook. Not this roadmap's slice.
- **The save backend** — file format, atomic writes, migration, slots. `save-system.md` is
  clear that serialization format is an implementation decision. What this roadmap owes the save
  system is that records are plain `UPROPERTY`-reflected structs referencing definitions by id,
  so whatever the save system decides can serialize them unchanged. Same commitment
  `inventory-roadmap.md` already made for items.

## Module Placement

No new module. Every piece lands in an existing one, and the dependency direction
(`smores` → `SmoresUI` → {`SmoresCharacters`, `SmoresEconomy`} → {`SmoresItems`, `SmoresCombat`}
→ `SmoresCore`) is unchanged.

| Piece | Module | Why there |
|---|---|---|
| `USmoresDefinition`, the id look-up helper | `SmoresCore` | Every module authors definitions; the base must sit below all of them |
| `UFactionDefinition`, `UWorldFactionComponent`, `UPlayerStandingComponent` | `SmoresCore` | Combat, characters and economy all need to read faction identity — see the deviation note below |
| `UItemModifierDefinition`, `ULootTableDefinition` | `SmoresItems` | They reference `UItemDefinition`, which lives there |
| `UWeightedTableDefinition`, `UWorldSeedComponent` | `SmoresCore` | *(moved here by Slice 5 - see its notes)* the base knows nothing about items, and the seed is read by every module that rolls |
| `UCharacterDefinition`, `FCharacterRecord`, `UCharacterRecordComponent` | `SmoresCharacters` | The record holds both inventory (`SmoresItems`) and health (`SmoresCombat`) data, and `SmoresCharacters` is the lowest module that can see both |

**Deviation from the target module map, stated rather than silent:**
`unreal-module-organization.md`'s target map puts `UStandingComponent` in a future
`SmoresFactions` module and gives `SmoresCore` "the allegiance primitive — a character's team/
faction identity and the 'are we enemies?' query." What Slice 3 builds *is* that allegiance
primitive plus its storage, so `SmoresCore` is the right home today. `SmoresFactions` becomes
worth cutting when faction *behavior* arrives (stance derivation, the faction decision clock,
settlement investment) — i.e. with the world-activity roadmap. The same skill notes that a
component can move modules later without touching its host, so this is a cheap thing to be
wrong about.

## Relationship to the Current Implementation

What already exists and should be extended rather than reinvented:

- **`UItemDefinition`** (`Source/SmoresItems/ItemDefinition.h`) — reparented onto
  `USmoresDefinition` in Slice 1 and registered with the Asset Manager. Slices 2 and 5 extend
  around it. Its class comment already anticipates crafting and pricing lookups.
- **`FInventoryItem`** (`Source/SmoresItems/InventoryComponent.h`) — the instance half. Slice 2
  added `Modifiers` and made every derived accessor compute through it, so a copy can now differ
  from its definition. Slice 5's loot-table entries hand modifiers to newly rolled items.
- **`AStrategyGameState`** (`Source/smores/Variant_Strategy/StrategyGameState.h`) — its own class
  comment says world clock and weather will each want a component here. It is Unreal's
  composition root for session-wide state; `UTimePaceComponent`, `UWorldFactionComponent` and
  (Slice 4) `UCharacterRecordComponent` sit there.
- **`AStrategyPlayerState`** — per-player standing went here as a component (Slice 3,
  `UPlayerStandingComponent`), per
  `unreal-module-organization.md`'s rule and the `UWalletComponent` precedent. Notably that
  precedent also *deleted* an interface: state the UI wants to read is better as a component
  than as a fourth `I*Host`.
- **The content sweeps** — Slice 1 split them into a base layer
  (`Source/SmoresCore/Tests/SmoresDefinitionAssetTest.cpp`, over every `USmoresDefinition`) and a
  per-type layer (`Source/SmoresItems/Tests/ItemDefinitionAssetTest.cpp`). **A new definition type
  joins the base sweeps by existing**; only rules beyond "has an id, has a name" need a new file.
  Copy the items file as the template.
- **Console `exec` commands** on `AStrategyPlayerController` (`SmoresDumpInventory`,
  `SmoresAddItem`, …) — the established way to inspect server-side state from a running game.
  Each slice below adds one, because most of this work has no player-facing surface of its own.

## Implementation Order

Dependency-ordered slices, **one per clean session**. Every slice follows the same protocol, so
it isn't repeated per entry:

1. Read the new `game-data.md` topic (once Slice 1 creates it), this slice's entry, and the
   source files it names.
2. Write the C++ first (CLAUDE.md's "C++ first" rule). Every slice here adds new
   `UCLASS`/`USTRUCT` types, so expect a **cold Visual Studio build** — close the editor, build,
   reopen. Never Live Coding.
3. Author the content assets via `unreal-mcp` (`mcp-workflow` skill). After any reparent or
   moved property, diff the affected assets' saved properties *and* placed level instances
   against expectations — see `unreal-module-organization.md`'s per-move mechanics for the two
   known silent-failure modes.
4. Run the headless suite (`testing.md`) and add tests per its standing rule — counted returns,
   all-or-nothing operations, state machines, and weight/value arithmetic all appear below.
5. Jim PIE-tests anything player-facing; the agent verifies compile, wiring, tests and property
   state.
6. Commit the slice on its own, then move its shipped content from this file into the
   `game-systems` skill and mark the slice `DONE` below.

### Why five slices

Per CLAUDE.md's slicing rules, each boundary below is justified by a real reason, not by "it's a
different file":

- **1 → 2**: context budget. Slice 2 involves a cold build, re-authoring eight existing item
  assets, diffing placed-actor overrides, and reading build/test logs. Landing it on an
  already-verified foundation keeps one session's worth of log output in one session.
- **2 → 3** and **3 → 4**: Jim has to look. Slice 2 changes item names, weights and prices; Slice
  4 rewires every unit's components. Both need a PIE pass that isn't "read a number."
- **4 → 5**: Slice 5 is content authoring with a visible result (open a chest, see plausible
  loot), and it depends on the item shape from 2 and the record store from 4. (In the event it
  needed neither record nor record store - a container has no record - only the item shape.)

### Slice 1 — The definition base and id look-up — **DONE**

Shipped into `game-systems`' `game-data.md`, with pointers added from `inventory.md` and
`testing.md`. `USmoresDefinition` and `USmoresDefinitionLibrary` live in `SmoresCore`;
`UItemDefinition` derives from the base with `ItemId` renamed to `DefinitionId`; `ItemDefinition`
is registered with the Asset Manager; the content sweep is split into a base layer in `SmoresCore`
and a per-type layer in `SmoresItems`; `SmoresDumpDefinitions` lists what the Asset Manager found.
95 tests green.

Two notes worth carrying into later slices:

- **The `CoreRedirect` for the `ItemId` → `DefinitionId` rename worked**, on all eight assets,
  including the unrenamed move of `DisplayName`/`Description` to the base. The entry is a
  `PropertyRedirects` line in `DefaultEngine.ini` qualified against `UItemDefinition` (the class
  being loaded) rather than the base the property now lives on.
- **The eight `DA_Item_*` assets were re-saved afterwards**, so they now serialize `DefinitionId`
  and the redirect is no longer load-bearing — which matters because CoreRedirects do not chain.
  `AssetTools.save_assets` alone does nothing on a clean asset; the working recipe was
  `ObjectTools.set_properties` writing each id back to its own value (to dirty the package)
  followed by `save_assets([])`. Verify with a binary grep, not the return value.

### Slice 2 — Item modifiers: material and quality — **DONE**

Shipped into `game-systems`' `inventory.md`, with the new definition type and its per-type content
sweep recorded in `game-data.md` and the counts updated in `testing.md`.
`UItemModifierDefinition` (+ `EItemModifierSlot`) lives in `SmoresItems`; `FInventoryItem` gained
`Modifiers` with one-per-slot enforcement, multiplier arithmetic, `FText::Format` name composition
in slot order, a `GetTint()` that multiplies, and modifier-aware order-independent stacking. The
four accessor-bypassing call sites now go through the instance. Five `DA_Modifier_*` assets
authored. 103 tests green.

Four notes worth carrying into later slices:

- **Moving a `USTRUCT`'s method bodies out of its header needs `<MODULE>_API` on the struct** —
  Slices 3 and 4 both add structs (`FFactionRecord`, `FCharacterRecord`) that other modules will
  read. Written up in `unreal-module-organization.md`'s per-move mechanics; read it there rather
  than relying on this line, which goes away with this roadmap.
- **Adding a field to a `USTRUCT` is the benign direction.** The Slice-1 hazard was a *reshaped*
  struct silently voiding placed-instance overrides; a purely additive `Modifiers` array
  deserialized cleanly on all five placed actors that carry item data. Worth knowing the two
  directions differ — a removed or renamed field still needs the full treatment.
- **The per-type content sweep earns its place when a field's failure mode is silent.** A
  modifier's multipliers default to 1.0 (invisible, harmless) but author to 0.0 as "erases
  whatever it multiplies", and an empty `NamePattern` falls back to hard-coded English word order.
  Neither is something the base sweep could know. A new definition type should ask what its
  *zero-ish* values do before deciding it needs no per-type file.
- **`DA_Item_IronSword` was rebased to `DA_Item_Sword`** (Jim's call) so "Iron Sword" composes
  from the definition plus the Iron modifier. Iron is authored as the 1.0x baseline material,
  which is what let every other item asset keep its numbers unchanged — the retune this slice
  called for turned out to be an identity change on one asset, not an arithmetic change on eight.

### Slice 3 — Factions: definition, record and standing — **DONE**

Shipped into a new `game-systems` topic, `factions.md`, with the new definition type recorded in
`game-data.md`, a worked example added to `multiplayer-discipline.md` and the counts updated in
`testing.md`. `UFactionDefinition`, `UWorldFactionComponent` (GameState) and
`UPlayerStandingComponent` (PlayerState) live in `SmoresCore`; four `DA_Faction_*` assets
authored; `SmoresDumpFactions` and `SmoresAdjustStanding` added. Nothing reads standing for
behavior. 111 tests green; Jim PIE-verified both execs.

Notes worth carrying into later slices:

- **The standing scale is -100..100 integers, clamped on every write, 0 neutral**, shared by both
  halves through `SmoresStanding` in `FactionTypes.h`. Slice 4's `FCharacterRecord::FactionId` is
  the first thing that will make a unit *have* a faction; still nothing should read standing to
  decide hostility until the AI roadmap says so.
- **Records seed themselves from the Asset Manager at `BeginPlay`**, with a public
  `InitializeFromDefinitions` for tests. Slice 4's record component can copy that shape — an
  in-memory seeding entry point is what made every faction test independent of `Content/`.
- **Replicated `TArray`s, not `TMap`s** — Unreal can't replicate a map. `FCharacterRecord` storage
  will hit the same wall.
- **Players start neutral with every faction**; there is deliberately no per-faction starting
  player standing on the definition.
- **For Slice 4:** `UCharacterDefinition::DefaultFactionId` and `FCharacterRecord::FactionId`
  should be checked against `UWorldFactionComponent::IsKnownFaction` when a record is created —
  but an unknown id must still be *allowed* (a stripped mod), so warn rather than refuse, the same
  stance `UPlayerStandingComponent` takes. A per-type character sweep can require the id resolve.
- **An array write through `ObjectTools.set_properties` onto an *empty* array landed both elements
  in one call**, unlike the grow-by-one behaviour `mcp-workflow` records for a non-empty one. Still
  read the length back.

### Slice 4 — Character definitions and the record store (the soft split) — **DONE**

Shipped into `game-systems`' `game-data.md` (a new "Character Records and the Soft Split" section:
the three pieces, the boundary, find-or-create, `PlacedRecordId`, names, uniqueness, faction ids,
multiplayer shape, the exec and the content), with pointers from `combat.md`, `inventory.md`,
`factions.md`, `hud-and-panels.md` and `multiplayer-discipline.md`, and counts in `testing.md`.
`UCharacterDefinition`, `FCharacterRecord`/`FCharacterAttributes` and `UCharacterRecordComponent`
(on `AStrategyGameState`) live in `SmoresCharacters`; `AStrategyUnit` finds or creates its record at
`BeginPlay` and writes every change back; `UHealthComponent::RestoreState`,
`UInventoryComponent::RestoreEntries` and `UEquipmentComponent::RestoreEquippedItems` are the
record → actor direction. `AStrategyPlayerUnit::StartingItems` is gone - three `DA_Character_*`
assets carry the loadouts, and all nine placed units have authored `PlacedRecordId`s.
`SmoresDumpRecord` added. 126 tests green; Jim PIE-verified, including `SmoresDumpRecord` on a
selected pawn.

Notes worth carrying into Slice 5:

- **Deviations from the sketch above, all deliberate:** `LifeState` reuses `EHealthState` rather
  than a mirrored `ELifeState`; `Equipped` is `TArray<FEquippedItem>` (there is no
  `FEquipmentState`); `ActorClass` is a `TSoftClassPtr` so loading a definition doesn't load a
  skeletal mesh; and **records are not replicated** - each unit's components already are the
  replicated copy.
- **A placed unit's authored `UnitDisplayName` names its record** (unique characters excepted), so
  the level still reads "Pawn 1" / "NPC 3" / "Merchant Ada". The pool only names unnamed or spawned
  units. That kept this slice invisible in play, which was its verification bar.
- **`CreateRecord` fills identity only; the actor places the loadout and writes back.** A loot
  table's rolled items go into a *container's* grid, not a record, so Slice 5 doesn't hit this -
  but the full split will.
- **The test world's `BeginPlay()` brings up the real `BP_StrategyGameState`**, components and
  all. Now written up in `testing.md`; relevant to any Slice 5 test that wants a GameState.
- **Placed-instance overrides bit again**, exactly as `mcp-workflow` warns: compiling the unit
  Blueprints after setting `CharacterDefinition` froze `None` onto all nine placed units as an
  override. Fixed by writing the intended value per instance and re-saving; the
  `grep -arl CharacterDefinition Content/__ExternalActors__` check is what caught it.
- **`FGuid` round-trips through `ObjectTools.set_properties` as a plain
  `"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"` string.**

### Slice 5 — Loot tables — **DONE**

Shipped into `game-systems`' `game-data.md` (a new "Weighted Tables and Seeded Rolls" section, the
fourth base field, the corrected id-vs-pointer rule) and `inventory.md` (container contents,
`AddItemsAllOrNothing`, the mineral items), with counts in `testing.md`, the new classes in
`unreal-module-organization.md`, a row in `multiplayer-discipline.md`, and the MCP shapes in the
`mcp-workflow` skill. `UWeightedTableDefinition` and `UWorldSeedComponent` (on `AStrategyGameState`)
live in `SmoresCore`; `ULootTableDefinition` in `SmoresItems`; `USmoresDefinition` gained `Tags`;
`AStrategyContainer` gained `LootTable` and an authored `PlacedContainerId` and rolls all-or-nothing
on top of `StartingItems` at `BeginPlay`. Five `DA_Loot_*` tables, three mineral items tagged
`Item.Mineral`, and both placed chests pointed at a situation table. `SmoresRollTable` added.
140 tests green.

Notes worth carrying into later roadmaps:

- **Deviations from the sketch above, all deliberate:**
  - `UWeightedTableDefinition` lives in **`SmoresCore`, not `SmoresItems`** as the placement table
    said. The table's reason ("they reference `UItemDefinition`") is true of the loot table and not
    of the base, which knows nothing about items; a spawn table shouldn't need `SmoresItems`.
  - **Loot entries name items and sub-tables by asset pointer, not by id.** `game-data.md` used to
    list "a loot table entry names an item by id"; that contradicted the Resolved Design Decision
    that the id rule "applies only to data that gets saved". A table is authored content. The mod
    case is served by tags instead - a mod's item tagged `Item.Mineral` joins every "any mineral"
    entry. The topic now says so.
  - **There was no world seed**, so Slice 5 made one: `UWorldSeedComponent`, server-only and not
    replicated (a client knowing it could compute every chest). Authored on the GameState Blueprint
    for now; the new-campaign flow and the save system both need to own it next.
  - **"All-or-nothing when the grid can't fit the roll"** was built as a general
    `UInventoryComponent::AddItemsAllOrNothing` rather than a loot-specific path. It places biggest
    footprint first, so it succeeds in cases where adding one at a time in roll order would strand
    an item.
  - **Three mineral items were added** (Copper Ore, Rock Salt, Sulfur) because `DesertMinerals` had
    nothing to find otherwise. "Waterskin" and "Ledger" from the example above became Health Potion
    and Gold Coin rather than two more new items.
- **The seed mixing is a hand-written CRC-32, pinned by a test** (`RollStreamIsPinned`), because
  `HashCombine` may change with the engine and would silently re-roll every save's containers.
  Anything else seeded from `(world seed, stable id)` - the spawn table, a recipe's quality roll -
  should call `MakeRollStream` rather than inventing its own mix.
- **Tag candidates are sorted by id before a pick.** The Asset Manager's scan order isn't stable;
  anything else that picks from `GetDefinitionIds` must sort first or lose determinism the same way.
- **A container has no record, so "already looted" doesn't survive a new session** - the chest
  rolls the same contents again. That is the save system's problem, and the reason determinism
  mattered: whatever it saves, a reload can't be used to re-roll.
- **An MCP write of a whole nested array onto an empty array landed in one call** (entries with
  modifier pools with choices), and nested struct members read back in lowerCamelCase while writes
  take C++ case - both now in `mcp-workflow`.

## Resolved Design Decisions

Settled during the conversation that produced this file. Don't reopen them without a reason.

- **Three layers, not two.** Definition / record / actor, with the actor as a puppet of the
  record. The inversion — record is the truth, actor is the copy — is the whole point.
- **Soft split now, full split later.** Records and actors live and die together for now. The
  full split waits on a faction simulation that has something to resolve.
- **A unique NPC is a definition marked unique, not a special class.** Authored identity on the
  definition; condition in the record; never state on an asset.
- **Records reference definitions by id; definitions reference each other by asset pointer.** The
  id rule exists to keep `save-system.md`'s mod-stripping promise, and applies only to data that
  gets saved.
- **Material and quality are one mechanism.** Modifiers multiply and compose rather than
  multiplying the asset count.
- **Modifiers come before content.** Reshaping `FInventoryItem` gets more expensive with every
  authored asset and placed instance.
- **Loot tables nest; they don't filter themselves by context.** Simpler to author, far simpler
  to debug, and consistent with `economy.md`'s stance on simulation complexity.
- **Loot rolls are seeded, not random.** Required by the save-scumming design constraint.
- **Records carry the seven attributes but no skills.** The attribute list is settled design; the
  skill roster is explicitly undecided and is to be assembled per-domain later.
- **Factions live in `SmoresCore` for now, not a new `SmoresFactions` module.** What this
  roadmap builds is the allegiance primitive the target map already assigns to `SmoresCore`; the
  module split waits for faction behavior.
- **No new module in this roadmap.** Every piece has an existing owner.

## Open Questions Worth Tracking

Not blockers for any slice here, but whoever builds settlements or the world simulation will hit
them immediately.

- **Containment between records.** A town contains buildings; a building houses a shopkeeper and
  a stock; the shopkeeper carries an inventory. Nothing in `FCharacterRecord` expresses "belongs
  to" — it has a `FactionId` (a reference) and a `LastKnownLocation` (a position), and neither is
  an ownership link. Under the soft split it doesn't need one: placement is authored in the level
  and every actor is loaded. It becomes load-bearing the moment settlements exist, because
  "the city fell, so its garrison died" is a query over containment. Deliberately **not** added
  now — a `ParentRecordId` pointing at nothing is exactly the dead data this roadmap excluded
  skills for — but expect to add it to `FCharacterRecord`, and to whatever a settlement record
  turns out to be.
- **Whether a settlement or building is a record at all.** The three-layer model says yes — an
  authored definition ("Blacksmith Shop"), a record (this one, its condition, its current owner)
  and an actor (the structure when a player is near). That is the `base-building.md` /
  world-content roadmap's call to make, not this one's, but it should be made deliberately rather
  than by default.
- **Record identity for repopulating characters.** `open-world.md` has settlements holding "a mix
  of permanent named NPCs and repopulating procedural ones." Does every anonymous townsperson get
  a permanent `FGuid` record that lives in the save forever, or are they pooled and recycled?
  `save-system.md` already flags a save-size budget as an open question, and this is the biggest
  single input to it.
