# Game Data Roadmap — Definitions and Records

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how this data layer works will live in the
`game-systems` skill — in a new `game-data.md` topic created by Slice 1, with additions to
`inventory.md` (Slice 2), `combat.md`/a new `factions.md` (Slice 3) and `inventory.md` again
(Slice 5).

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

## Weighted Tables

A chest opened in the desert should yield different things from one opened in a faction town.
The mechanism is a weighted table whose entries can point at **another table**:

```
DesertChest          60% -> CommonJunk      (table)
                     30% -> DesertMinerals  (table)
                     10% -> Waterskin       (item)

FactionTownChest     60% -> CommonJunk      (table)   <- same asset, authored once
                     30% -> IronclanGoods   (table)
                     10% -> Ledger          (item)
```

plus a per-entry quantity range and a roll count for the whole table ("roll 2–4 times"). An
entry can name a specific item, a sub-table, or a tag ("any common desert mineral").

Three decisions worth stating:

- **Nesting, not context filtering.** A table that filters itself by region and faction at roll
  time is more clever and much harder to debug than composing small tables by hand. `economy.md`
  is explicit that a simpler model producing visible results beats a precise one running
  invisibly. Add context parameters only if authoring actually becomes painful.
- **The roll must be deterministic.** `characters-and-squads.md` requires that a retry under
  identical conditions produces an identical result — only a genuine change (skill, elapsed
  time, a different approach) yields a different outcome. So a container seeds its roll from the
  world seed plus its own stable id, never from a global RNG. Cheap now, expensive later.
- **Swap the payload type and the same structure is a spawn table.** That is deliberately *not*
  in this roadmap (see Out of Scope) — but the weight/roll/nesting logic goes in a shared base
  so the world-activity roadmap extends it rather than rewriting it.

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
| `UItemModifierDefinition`, `ULootTableDefinition`, `UWeightedTableDefinition` | `SmoresItems` | They reference `UItemDefinition`, which lives there |
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
  comment already says "World clock, weather and faction standing will each want a component
  here." It is Unreal's composition root for session-wide replicated state, and it is where the
  faction and character-record components go. `UTimePaceComponent` is the existing precedent for
  the pattern.
- **`AStrategyPlayerState`** — per-player standing goes here as a component, per
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
  loot), and it depends on the item shape from 2 and the record store from 4.

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

### Slice 3 — Factions: definition, record and standing

**Builds:** `UFactionDefinition` in `SmoresCore`; `FFactionRecord`; `UWorldFactionComponent` on
the `GameState`; `UPlayerStandingComponent` on the `PlayerState`. **Storage and queries only.**

- The definition holds authored identity: display name, short name, colour, lineage stance, and
  the faction's **starting** tier. `factions-and-world-state.md` is explicit that faction type is
  a *state*, not an identity — so current tier lives in the record, not the definition. This
  slice is where that distinction first earns its keep on something other than a character.
- `UWorldFactionComponent` (GameState, replicated, authority-gated): the faction records, and the
  faction↔faction standing matrix keyed by an ordered id pair.
- `UPlayerStandingComponent` (PlayerState, replicated to its owner): this player's standing with
  each faction. Per-player, never global — `factions-and-world-state.md` tracks standing
  independently per faction *and* per player, and `multiplayer-discipline.md` requires the
  per-player half to hang off the player state.
- Author three or four faction assets via MCP.
- `SmoresDumpFactions` prints tiers, the matrix and the calling player's standing.

**Explicitly not in this slice:** nothing reads standing to decide hostility. No change to
`AStrategyUnit::Disposition`, no stance derivation, no change to who attacks whom.

**Tests:** standing symmetry and clamping, unknown-faction queries returning a neutral default,
authority gating on every mutation.

**Ships into:** a new `factions.md` topic in `game-systems`.

### Slice 4 — Character definitions and the record store (the soft split)

The heart of the roadmap, and the biggest slice.

**Builds:** `UCharacterDefinition` in `SmoresCharacters`; `FCharacterRecord`;
`UCharacterRecordComponent` on the `GameState`; `AStrategyUnit` rebound to read and write its
record.

```cpp
UCLASS(BlueprintType)
class SMORESCHARACTERS_API UCharacterDefinition : public USmoresDefinition
{
    bool    bUnique;              // exactly one record, ever - Warlord Kess
    FText   Backstory;
    TObjectPtr<UTexture2D> Portrait;   // the squad panel / target panel face, not an item icon
    FName   DefaultFactionId;
    FName   RoleId;               // "shopkeeper", "guard" - an id, not a role system
    FCharacterAttributes  BaseAttributes;        // the seven, from characters-and-squads.md
    TArray<FInventoryItem> DefaultLoadout;
    TArray<FText>          NamePool;             // generated names for non-uniques
    TSubclassOf<AStrategyUnit> ActorClass;
};
```

```cpp
USTRUCT()
struct FCharacterRecord
{
    FGuid   RecordId;             // this individual, forever
    FName   DefinitionId;         // what kind they are
    FText   Name;                 // authored for a unique, rolled for everyone else
    FName   FactionId;
    FCharacterAttributes  Attributes;   // current; drifts from the definition's base
    float   Health;
    ELifeState  LifeState;        // mirrors UHealthComponent's state machine
    FVector LastKnownLocation;
    TArray<FInventoryEntry>  Carried;
    FEquipmentState          Equipped;
};
```

- **Unique characters.** `bUnique` produces exactly one record at campaign start and never
  respawns. Nothing about "is Kess alive" touches the definition asset — uniqueness is authored,
  condition is recorded. `save-system.md` lists named-NPC alive/dead status and location as
  must-persist, which is this field and this field only.
- **Placed actors register once.** A unit placed in `LVL_Strategy` adopts its existing record if
  there is one and creates it from its definition if there isn't, keyed off an authored
  `PlacedRecordId` on the actor. Without this, reloading a save stands the dead boss back up —
  the classic version of this bug, and the reason the key is authored rather than derived from
  the actor's runtime name.
- **The soft-split boundary, written down.** Records and actors are created and destroyed
  together. Write-back happens whenever the actor's state changes, not only at despawn, because
  there is no despawn. Nothing advances a record that has no actor. Every one of those sentences
  is a thing the world-activity roadmap will change, and they belong in the topic so it is
  obvious what it is changing.
- `SmoresDumpRecord` prints a selected unit's record beside its live component state, which is
  how the sync is verified.

**Hazards.** This touches `AStrategyUnit`, `UHealthComponent`, `UInventoryComponent` and
`UEquipmentComponent` — a cold build and a careful pass over authority gating. Every record
mutation is server-side; `multiplayer-discipline.md` applies in full.

**Tests:** record creation from a definition, unique-definition single-instance, name generation,
write-back round trip (mutate the actor, read the record, and back), authority gating.

**Verification:** Jim PIE-checks that units still select, move, fight, loot and trade exactly as
before — this slice should be invisible in play.

**Ships into:** `game-data.md` (records, the registry, the soft split and its boundary), with a
pointer from `combat.md` and `inventory.md`.

### Slice 5 — Loot tables

**Builds:** `UWeightedTableDefinition` base and `ULootTableDefinition` in `SmoresItems`;
`FGameplayTagContainer Tags` added to `USmoresDefinition` (now that something consumes it);
`AStrategyContainer` rolling its contents instead of listing them.

- Entries name an item, a sub-table or a tag, with a weight, a quantity range and an optional
  modifier pool so a table can yield "a spear, bronze or iron, occasionally well-made." The
  modifier half of that is built as of Slice 2 — a rolled entry applies its pick with
  `FInventoryItem::AddModifier`, which already enforces one per slot, so the table only has to
  choose. Note that a table rolling modifiers is what first makes the stacking change visible in
  bulk: a chest yielding bronze *and* iron spears produces two piles, not one.
- The base class owns weights, roll counts and nesting; `ULootTableDefinition` is the only
  subclass today, and the world-activity roadmap adds the spawn-table sibling. One subclass now
  is deliberate, per the reusable-base default.
- **Deterministic seeding** from the world seed plus the container's stable id, per the
  save-scumming constraint above. This is the piece that is genuinely awkward to add later.
- `AStrategyContainer` keeps `StartingItems` for hand-authored one-offs and gains an optional
  `LootTable` that fills it on `BeginPlay`, authority-only. Both paths coexist; the table is not
  a replacement for authoring a specific chest.
- Author a small table set via MCP — `CommonJunk`, `DesertMinerals`, one faction goods table, and
  two situation tables composing them.
- `SmoresRollTable <id>` prints a sample roll without touching the world.

**Tests:** weight distribution over a fixed seed, nesting, roll counts, tag resolution,
determinism (same seed and id yields the same result), and all-or-nothing behavior when the
target grid can't fit the roll.

**Verification:** Jim opens chests in PIE and judges whether the contents read plausibly.

**Ships into:** `inventory.md` (container contents) and `game-data.md` (the weighted-table base).

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
