# Factions: Definitions, Records and Standing

## Purpose

Where the game keeps what it knows about factions: who they are, how much territory each holds
right now, how they regard each other, and how each player stands with each of them.

**Storage and queries only.** Nothing in the project reads any of this to decide behavior. No
unit's hostility comes from standing, no patrol checks it, no trader's prices move with it. The
placeholder "aggressive units hunt the nearest player pawn" in `AStrategyUnit` is exactly what
it was before factions existed. Deriving behavior from standing is an AI change
(`ai-and-behavior.md`'s Stance in the `game-design` skill), not a storage one, and is
deliberately a later roadmap's work.

Built by Slice 3 of `Docs/roadmaps/game-data-roadmap.md`. It is the first system on the
definition/record model in `game-data.md` to have a **record** as well as a definition — read
that topic first.

## Player Surface

None yet. Nothing on the HUD shows a faction or a standing. The only surface is two debug execs
on `AStrategyPlayerController`:

| Command | Does |
|---|---|
| `SmoresDumpFactions` | logs every faction's record (current tier beside its authored starting tier, and lineage stance), the faction-to-faction standing matrix, and **this** player's standing with each faction |
| `SmoresAdjustStanding <FactionId> <Delta>` | moves this player's standing with one faction by Delta (default 10), clamped, then dumps. Flags an id the world doesn't know, since the component accepts one on purpose (see below) |

Both hop to the server, like the inventory and gold execs — the records and the player's
standing are server-owned. Faction ids are listed by `SmoresDumpDefinitions`.

## Core Rules

### Three pieces, three owners

| Piece | What it holds | Lives on | Scope |
|---|---|---|---|
| `UFactionDefinition` | who a faction *is* — name, short name, colour, lineage stance, starting tier, starting relations | a `DA_Faction_*` asset | authored, never written at runtime |
| `UWorldFactionComponent` | one `FFactionRecord` per faction (its **current** tier), and the faction↔faction standing matrix | `AStrategyGameState` | session-wide — every player sees the same world |
| `UPlayerStandingComponent` | this player's standing with each faction | `AStrategyPlayerState` | per player — robbing Ironclan doesn't make Ironclan hate your co-op partner |

Picking the wrong home for either half is the expensive mistake `multiplayer-discipline.md`
warns about: faction standing on the GameState would be one global reputation shared by eight
players, and faction tiers on a player state would be eight disagreeing copies of the world.

### Tier is a state, not an identity

`factions-and-world-state.md` (in `game-design`) is explicit that a Minor faction seizing a town
becomes Major. So the definition holds only `StartingTier`, and the current tier is
`FFactionRecord::Tier`. `SetFactionTier` moves the record and never touches the asset — a test
asserts exactly that. Nothing calls it outside tests yet; the world-activity roadmap will.

Lineage stance is the opposite: `game-design` calls it static, authored identity, the same every
playthrough. It lives on the definition only, and nothing reads it yet.

### The standing scale

Whole numbers from **-100 to 100, 0 neutral**, shared by both halves through `SmoresStanding`
(`FactionTypes.h`) so faction↔faction and player↔faction can never disagree about what a
number means.

- **Writes clamp rather than refuse.** A huge penalty on an already-hated faction lands at -100
  instead of being thrown away. `Adjust*` widens to 64 bits before adding, so an extreme delta
  clamps instead of wrapping.
- **Unknown means neutral, never an error.** A query naming a faction the world doesn't know —
  including one a stripped mod took with it — answers 0. That is the same "an id is allowed to
  fail to resolve" promise `game-data.md` makes for saves.
- **A faction regards itself at 100.** Only for a known faction; an unknown id regarding itself
  is still just neutral. Writing a faction's standing with itself is refused.
- **Players start neutral with everyone.** A faction with no entry in a player's list is 0,
  which matches `game-design`'s "an unaffiliated squad starts outside every faction hierarchy".
  There is no per-faction starting player standing; add one to the definition if a faction ever
  needs to be hostile to strangers on sight.

### Faction↔faction standing is symmetric

A pair is stored **once**, keyed by its two ids in lexical order (`FFactionPairStanding::FirstId`
always sorts first). `(Ironclan, Raiders)` and `(Raiders, Ironclan)` are the same cell whichever
way it was written. Lexical rather than `FName`'s own `<`, because that compares name-table
indices, which differ between runs and machines — a saved pair would load under a different key.

A pair absent from the matrix is neutral, so the matrix only holds pairs somebody authored or
changed. `SmoresDumpFactions` says so rather than printing every combination.

### Starting relations are authored on one side

`UFactionDefinition::StartingRelations` names another faction by asset pointer (definition to
definition is content linking; see `game-data.md`) plus a number. Because standing is symmetric,
**author each pair on one faction only.** If both sides author it with different numbers, one is
discarded at campaign start — the later id in lexical order wins, with a warning. The content
sweep rejects that case outright so it never ships.

### Where the records come from

`UWorldFactionComponent::BeginPlay` (server only) asks the Asset Manager for every
`FactionDefinition` and hands them to `InitializeFromDefinitions`. **There is no per-level
faction list to wire** — authoring the asset is the whole of adding a faction.
`InitializeFromDefinitions` is public so tests can seed in-memory definitions, and so a future
"new campaign" flow has a front door that isn't `BeginPlay`. It replaces rather than appends,
seeds in id order (so the result doesn't depend on asset-registry scan order), and skips nulls,
blank ids and duplicate ids with a warning.

## C++ Implementation

All in `SmoresCore` — see "Module placement" below for why.

| File | Holds |
|---|---|
| `Source/SmoresCore/FactionTypes.h` | `EFactionTier`, `ELineageStance`, the `SmoresStanding` scale, and the three plain structs `FFactionRecord`, `FFactionPairStanding`, `FPlayerFactionStanding` |
| `Source/SmoresCore/FactionDefinition.h` | `UFactionDefinition` and `FFactionStartingRelation` |
| `Source/SmoresCore/WorldFactionComponent.h` | `UWorldFactionComponent` |
| `Source/SmoresCore/PlayerStandingComponent.h` | `UPlayerStandingComponent` |

Registered as `FactionDefinition` in `Config/DefaultGame.ini`'s `PrimaryAssetTypesToScan`. The
structs have no out-of-line methods, so they carry no `SMORESCORE_API` — add it the day one grows
a method body in a `.cpp` (see `unreal-module-organization.md`).

### Multiplayer shape

- Every mutator (`InitializeFromDefinitions`, `SetStandingBetween`, `AdjustStandingBetween`,
  `SetFactionTier`, `SetStanding`, `AdjustStanding`) checks the owner's authority, warns and
  returns false off-authority. Every getter works everywhere.
- The world component's two arrays replicate to **every** client. The player component's list
  replicates **owner-only** (`COND_OwnerOnly` — a player state is owned by its controller, so
  that means "this player's machine"). Widening it is one condition, if a squad screen ever wants
  a partner's reputation.
- Both are `TArray`s rather than `TMap`s because Unreal can't replicate a `TMap`. A handful of
  factions makes the linear search free; this is the class that grows an index if counts reach
  the hundreds.
- A client changing standing would route through its own controller, like
  `Server_RequestPace`. Nothing does - the two callers are the debug exec and dialog's
  `<<ChangeStanding>>` effect, which runs on the server in a conversation (`dialog.md`) and posts
  the change to that player's feed. The shakedown's refusal is the one piece of content using it.
- Each component broadcasts one delegate (`OnFactionsChanged`, `OnStandingChanged`) on the
  server from the mutator and on clients from `OnRep`. Only on a real change: re-setting a value
  to what it already was, or a refused write, is silent. Nothing binds them yet; they are there
  for the first UI.

### Module placement

`unreal-module-organization.md`'s target map puts a standing component in a future
`SmoresFactions` module and gives `SmoresCore` "the allegiance primitive". What this built *is*
that primitive plus its storage, and combat, characters and economy all need to read faction
identity, so `SmoresCore` is its home today. `SmoresFactions` is worth cutting when faction
*behavior* arrives (stance derivation, the faction decision clock).

## Blueprint/Asset Dependencies

- Four faction assets under `Content/Factions/`: `DA_Faction_Ironclan` (Major, mono-lineage),
  `DA_Faction_TradersGuild` (Nomadic, cosmopolitan), `DA_Faction_Raiders` (Minor, cosmopolitan)
  and `DA_Faction_Covenant` (id `AshenCovenant`, Minor, guarded). Placeholder names and numbers
  for a setting that isn't chosen yet; the relations give the dump something to show.
- `BP_StrategyGameState` and `BP_StrategyPlayerState` pick the components up as native default
  subobjects — no wiring.

## Testing

- `Source/SmoresCore/Tests/FactionStandingTest.cpp` — `Smores.Core.Factions.*`, six tests:
  symmetry and the lexical key, clamping (including an out-of-range authored relation and an
  extreme delta), unknown-faction neutrality and refusal, seeding from definitions (order,
  duplicates, record-vs-definition tier, re-initialising), broadcast-only-on-change, and authority
  gating on every mutator (ownerless components, per `testing.md`'s one sanctioned exception,
  paired with the authoritative path).
- `Source/SmoresCore/Tests/FactionDefinitionAssetTest.cpp` — the per-type content sweep. Rejects
  an empty `ShortName`, a **fully transparent colour** (draws nothing, reads as a missing banner),
  a relation naming nothing, itself, the same faction twice or a number off the scale, and — the
  rule only a sweep across every faction can see — **a pair authored on both sides with different
  numbers**.

## Extension Points

- **Adding a faction:** author a `DA_Faction_*` asset. Nothing else — the world picks it up at
  `BeginPlay`.
- **Reading standing for behavior** (patrol hostility, trade refusal, prices): read
  `UPlayerStandingComponent::GetStanding` off the relevant player's state, and
  `UWorldFactionComponent::GetStandingBetween` off the GameState. Decide thresholds ("below -50 is
  hostile") in the consuming system, not here — this layer has no opinion about what a number means.
- **Changing standing from gameplay** (a crime, a gift, a completed contract): server-side only,
  through `AdjustStanding`. A client-initiated change routes through a `Server_` RPC on its own
  controller.
- **Moving a faction between tiers:** `SetFactionTier` on the server — the world-activity
  roadmap's job.

## Known Gaps

- **Nothing reads standing for behaviour.** By design for now — see Purpose. The one reader is
  dialog: the `StandingWithSpeaker` fact lets a bark pick a warmer or colder line
  (`dialog.md`). It changes what somebody *says*, never whether they trade or fight.
- **Nothing is saved.** The records and standings are plain reflected structs holding ids, so the
  save system can serialize them unchanged, but no save exists. A new session re-seeds from the
  definitions.
- **No UI.** No faction panel, no standing readout. The delegates are the hook.
- **No crest/banner art.** The definition has a colour only. A crest belongs on
  `UFactionDefinition` (not the base — see `game-data.md` on presentation), and becomes a sweep
  requirement once one exists.
- **Units have a faction, and nothing reads it for behaviour.** Dialog reads it to choose lines
  (`Speaker.Faction`, and whose standing `StandingWithSpeaker` asks about). Since game-data Slice 4 each unit's record holds
  a `FactionId` from its character definition (`DA_Character_Bandit` → `Raiders`,
  `DA_Character_Trader` → `TradersGuild`, settlers none), copied to the actor as a replicated
  `AStrategyUnit::GetFactionId()`. Hostility still comes from the placeholder, not from this -
  see `game-data.md`.
- **Lineage stance is read by nothing** and has three placeholder values
  (`Cosmopolitan` / `Guarded` / `MonoLineage`); the `game-design` text describes a range, not a
  fixed list.
