# Unreal Module Organization

## Purpose

Unlike the other topics in this skill, this one isn't documenting a finished, playable
system — it's a **forward-looking reference** for how `Source/` should eventually split
into multiple Unreal modules as the project grows to cover the full scope described across
the `game-design` skill (factions, economy, characters/squads, base building, tech,
open world, AI/behavior, saves, UI, multiplayer, narrative). The goal is that whoever eventually cuts
the first new module — human or agent — has a plan to extend rather than an ad hoc
decision to invent mid-refactor. **Nothing here is a mandate to split anything today** —
see "When to Actually Split" below.

## Current State

Seven runtime modules: `smores` (`Source/smores/smores.Build.cs`, the primary/game
module), `SmoresCore` (empty proving module, stood up alongside the first real split),
`SmoresCombat` (`HealthComponent`, `CombatComponent`, `DamageNumberActor`/
`DamageNumberWidget`, `AnimNotify_AttackHit`, plus a small `IAttackDamageDealer` interface),
`SmoresItems` (`UItemDefinition`, `FInventoryItem`/`FInventoryEntry`/`UInventoryComponent`,
`UEquipmentComponent`, `AStrategyContainer`, `AStrategyChest`, `AWorldItem`,
`IInventoryHolder`), `SmoresCharacters` (`AStrategyUnit`, `AStrategyPlayerUnit`), `SmoresUI`
(`AStrategyHUD`, `UStrategyUI`, `UStrategyTouchControls`, `UWindowWidget`, `UInventoryWidget`,
`UInventoryCellWidget`, `UInventoryItemWidget`, `UEquipmentWidget`/`UEquipmentSlotWidget`,
`UInventoryDragDropOperation`, plus `IStrategySelectionHost`/`IStrategyCameraCommands`/
`IInventoryMoveHost`), and `SmoresEconomy` (`UWalletComponent`, `IPricingProvider`,
`UTraderComponent`) — see "Migrating Today's Prototype Code" below. One game `Target.cs` and
one Editor `Target.cs`, both referencing all seven modules.

`SmoresEconomy` is the **first module cut for an ownership reason rather than as part of the
original migration**, and it is worth reading as the worked example of "When to Actually
Split" below: it was stood up only once a wallet, a price and a transaction were all arriving
at once and needed a home that depends on `SmoresItems` and nothing else. Standing it up also
*deleted* `IStrategyResourceHost` — the interface existed purely so `SmoresUI` could read a
gold balance living in `smores`, and a component in a module `SmoresUI` already depends on
needs no interface at all.

Measured 2026-09-12, in lines of `.h` + `.cpp`: `smores` 3250, `SmoresUI` 2049,
`SmoresItems` 1222, `SmoresCombat` 878, `SmoresCharacters` 781, `SmoresCore` 11. **Read
that `smores` figure carefully before concluding the primary module is bloating: 2433 of
its 3250 lines are `AStrategyPlayerController` alone** (1832 `.cpp` + 601 `.h`). See "The
junk drawer is a class, not a module" below. `smores/Variant_Strategy/` no longer has `Combat/`,
`Inventory/`, `UI/`, or the unit character classes — `MainMenu/` still has its own `UI/`
(unrelated feature, untouched). There is no `Plugins/` folder for game-specific code yet.
The engine-side plugins already enabled (`ModelContextProtocol`, `AllToolsets`,
`StateTree`) are editor/MCP tooling, unrelated to this topic.

## Concrete Folder Structure

Every module is a **sibling folder directly under `Source/`**, never nested inside
`Source/smores/`. `smores` is the project's primary/game module — the one module carrying
`IMPLEMENT_PRIMARY_GAME_MODULE`, of which a project may have exactly one. It matches the
project name because Epic's template named it that way, not because Unreal requires the two
to agree (see "Module Naming" below). Every other module sits next to it as a peer:

```
Source/
  smores.Target.cs
  smoresEditor.Target.cs
  smores/                         # primary module — exactly one per project, shrinks over time
    smores.Build.cs
    smores.cpp / smores.h
    smoresCharacter.* smoresGameMode.* smoresPlayerController.*   (template base classes)
    MainMenu/
      MainMenuGameMode.* MainMenuHUD.*
      UI/  MainMenuWidget.* OptionsWidget.*
    Variant_Strategy/             # shrinks as pieces below are peeled out
      StrategyGameMode.* StrategyPawn.* StrategyPlayerController.*
      EnvQueryContext_MoveGoal.*
                                   # StrategyPlayerController implements SmoresUI's
                                   # IStrategySelectionHost/IStrategyCameraCommands/
                                   # IInventoryMoveHost interfaces - see below
  SmoresCore/
    SmoresCore.Build.cs
    SmoresCore.cpp / SmoresCore.h
  SmoresItems/
    SmoresItems.Build.cs
    SmoresItems.cpp / SmoresItems.h
    InventoryComponent.*          # moved from smores/Variant_Strategy/Inventory/
    EquipmentComponent.*          # the paperdoll; born here, never lived in smores/
    ItemDefinition.*              # the shared per-item-type data asset
    StrategyChest.* StrategyContainer.*   # moved from smores/Variant_Strategy/
    WorldItem.*                   # a single loose item lying in the world
    InventoryHolder.*             # IInventoryHolder - display name + proximity, implemented
                                   # by StrategyUnit/StrategyContainer/WorldItem. Note the
                                   # direction: unlike the SmoresUI interfaces below, this one
                                   # is declared *down* in the module that owns the concept and
                                   # implemented by types above it - no coupling to resolve, it
                                   # just de-duplicates a rule three holders share
  SmoresCombat/
    SmoresCombat.Build.cs
    SmoresCombat.cpp / SmoresCombat.h
    HealthComponent.*             # moved from smores/Variant_Strategy/Combat/
    AnimNotify_AttackHit.*
    DamageNumberActor.* DamageNumberWidget.*
  SmoresCharacters/
    SmoresCharacters.Build.cs
    SmoresCharacters.cpp / SmoresCharacters.h
    StrategyUnit.* StrategyPlayerUnit.*   # moved from smores/Variant_Strategy/
  SmoresFactions/
  SmoresWorld/
  SmoresAI/                        # individual behavior + per-character job queues,
                                   # much later - see the target map
  SmoresBaseBuilding/
  SmoresTechCrafting/
  SmoresUI/
    SmoresUI.Build.cs
    SmoresUI.cpp / SmoresUI.h
    StrategyHUD.* StrategyUI.* StrategyTouchControls.* WindowWidget.*
    InventoryWidget.* InventoryCellWidget.* InventoryItemWidget.*
    InventoryDragDropOperation.*   # moved from smores/Variant_Strategy/UI/
    StrategySelectionHost.* StrategyCameraCommands.*
    InventoryMoveHost.*            # narrow interfaces, resolve the smores<->SmoresUI coupling
                                   # (AStrategyPlayerController implements all three).
                                   # StrategyResourceHost used to be a fourth; it was deleted
                                   # when gold became a component - see "Framework Classes
                                   # vs. Feature Modules"
  SmoresEconomy/                   # value primitives: WalletComponent, PricingProvider,
                                   # TraderComponent (a UInventoryComponent subclass)
  SmoresMarkets/                   # market simulation, much later - see the target map
  SmoresSaveGame/
  SmoresOnlineSession/
  SmoresEditor/                    # Editor-only, TargetAllowList: Editor
```

Each module folder follows the project's existing flat style (no `Public/`/`Private`
split — headers and `.cpp` files sit side by side, matching how `smores/` is organized
today), with its own `<ModuleName>.Build.cs` and a root `<ModuleName>.cpp`/`.h` pair
carrying the `IMPLEMENT_MODULE` macro.

**`Content/` doesn't need to mirror this 1:1, and reorganizing it is a separate, lower-
priority concern.** Blueprint assets can stay under their current `Content/Variant_Strategy/`
paths even after their parent C++ class moves modules — don't conflate a C++ module split
with a content folder reshuffle; do them independently, and only reorganize Content when
it actually earns its cost.

## Guiding Principles for a Future Split

- **Split along dependency direction, never create a cycle.** Every module in the target
  map below should be able to say who it depends on without that list ever pointing back
  at something above it.
- **Module boundaries aren't the same thing as multiplayer authority boundaries.** Unreal
  compiles the same module for client and server; server-only logic is normally expressed
  with `WITH_SERVER_CODE`/authority checks inside a module, not a separate module per side
  (see `multiplayer-discipline.md`). Don't invent a client/server module split
  that Unreal's own build model doesn't actually need.
- **Structure DLC/mod-facing systems so they *could* become Plugins later without a
  rewrite**, since `multiplayer-and-content.md` commits to DLC as self-contained,
  engine-hook-capable additive layers and mods as best-effort additive content. That means:
  no module below should reach back into content that's supposed to be DLC-shaped, even
  before any DLC actually exists.
- **One module per real domain, not one per class or per design-doc heading.** A design
  topic existing in `game-design` is not by itself a reason to cut a module — only cut one
  where there's an actual compile-isolation, optional-loading, or ownership reason (see
  "When to Actually Split").
- **Per-player and per-pawn state belongs in a component in a feature module, never inline
  on a framework class.** This is the single most important habit for keeping `smores`
  small, and it is *not* a module question at all — see the next section.

## Framework Classes vs. Feature Modules

**The recurring question this section answers:** "I have new per-player (or per-pawn) state
and there's no obvious module for it — am I missing a module?" Almost always, **no**. You're
missing a component.

### Why `smores` legitimately holds the framework classes

`AGameModeBase` / `APlayerController` / `APlayerState` / `APawn` / `AHUD` are Unreal's
composition root: the layer that decides who owns what and wires the pieces together. It
*has* to sit at the top of the dependency graph and see everything below it, and a project
has exactly one such layer. `AStrategyGameMode`/`AStrategyPlayerController`/
`AStrategyPlayerState`/`AStrategyPawn` living in `smores/Variant_Strategy/` is therefore
correct, not a failure to find them a home. Don't invent a module to "rescue" a framework
class from the primary module — that's inverting the graph for no gain.

Note the consequence, since it's the cost side of that placement: **nothing depends on
`smores`**, so anything living there is invisible to every feature module. When a lower
module needs a read from the player controller, it declares a narrow interface and the
controller implements it (the four `I*Host`/`I*Commands` interfaces in `SmoresUI`).

### The real risk: state accumulating *on* those classes

The junk-drawer failure mode isn't framework classes living in `smores`. It's gameplay
state being implemented **inline on them** because no feature module obviously owns it yet.
Each such addition is individually reasonable and collectively fatal.

The project already has the correct pattern and applies it well for pawns — `AStrategyUnit`
implements none of health, combat, or inventory:

```
AStrategyUnit                      <- thin host; ~780 lines for the WHOLE module
  |-- UHealthComponent     (SmoresCombat)
  |-- UCombatComponent     (SmoresCombat)
  |-- UInventoryComponent  (SmoresItems)
  +-- UEquipmentComponent  (SmoresItems)
```

`AStrategyPlayerState` is now the identical shape, and the route it took there is the lesson:
Slice 4 put `Gold` inline on it because one `int32` didn't feel worth a component — a
defensible call for the *first* piece of player state and an indefensible one for the fourth —
and Slice 8 moved it into a `UWalletComponent` the moment `SmoresEconomy` existed to hold one.
The player state now owns no state of its own at all:

```
AStrategyPlayerState               <- thin host; ~40 lines, and meant to stay that way
  +-- UWalletComponent     (SmoresEconomy)
```

### What is queued to land on the player state

Every one of these is per-player, is already committed to in `game-design`, and will want
the same home:

| Incoming state | Design source | Should become |
|---|---|---|
| Gold (**DONE**, Slice 8) | `economy.md` | `UWalletComponent` (`SmoresEconomy`) |
| Squad roster & divisions | `characters-and-squads.md` | `USquadComponent` (`SmoresCharacters`) |
| Faction standing | `factions-and-world-state.md` | `UStandingComponent` (`SmoresFactions`) |
| Research progress | `tech-and-crafting.md` | `UResearchComponent` (`SmoresTechCrafting`) |
| Quest/objective state | `quests-and-objectives.md` | `UObjectiveComponent` |

Added inline, that's an 800-line player state and a genuinely bloated `smores`. Added as
components, the player state stays ~40 lines forever and each domain's state lives in the
module that owns that domain.

### The rule

**When new per-player or per-pawn state arrives and no module obviously owns it, write a
component in the nearest feature module and host it on the framework class — don't add
fields to the framework class, and don't cut a module just to have somewhere to put it.**
A component is cheap, replicates the same way, and can move modules later without touching
its host. `APlayerState` is an `AActor`, so components and `SetIsReplicatedByDefault(true)`
work on it exactly as they do on `AStrategyUnit`.

**Bonus: a component can dissolve an interface — this is no longer hypothetical.**
`IStrategyResourceHost` existed only because `SmoresUI` couldn't see `AStrategyPlayerState` in
`smores`. Slice 8 moved gold to a `UWalletComponent` in `SmoresEconomy`, a module `SmoresUI`
already depends on, and the interface was **deleted**: the HUD reaches the balance with no
interface at all, because `APlayerState` is an engine type visible everywhere:

```cpp
// SmoresUI, no dependency on `smores` and no interface needed
UWalletComponent* Wallet = PC->PlayerState->FindComponentByClass<UWalletComponent>();
```

(`AStrategyHUD` caches that pointer rather than re-scanning every `DrawHUD` frame, and
re-runs the lookup only while it is still null — a player state can replicate in well after
the HUD exists.) Weigh this whenever a new `I*Host` interface is about to be added: if the
thing the UI wants to read is *state* rather than *behavior*, a component is usually the
better answer than a fourth interface.

## Proposed Target Module Map

| Module | Responsibility | Depends on | Design reference |
|---|---|---|---|
| `SmoresCore` | Shared low-level types/utilities with no gameplay-specific logic (common structs, interfaces, math/helpers), plus the **allegiance primitive** — a character's team/faction identity and the "are we enemies?" query | Core, CoreUObject, Engine | `ai-and-behavior.md`'s Stance |
| `SmoresItems` | Item definitions/data tables, stackable-item storage (today's `FInventoryItem`/`UInventoryComponent`) | `SmoresCore` | `inventory.md` (current), `economy.md`'s Goods |
| `SmoresCharacters` | Lineage, attributes/skills, recruitment/wages/morale, injuries, squad/division management | `SmoresCore`, `SmoresItems` | `characters-and-squads.md` |
| `SmoresCombat` | Health/damage, melee resolution, disposition/aggro, incapacitation/capture | `SmoresCore`, `SmoresCharacters` | `combat.md` (both skills) |
| `SmoresFactions` | Faction simulation, standing, territory, assault intelligence, military progression, and faction-level decision-making (relationships, settlement investment, raid/assault decisions) on its own slow clock | `SmoresCore`, `SmoresCharacters` | `factions-and-world-state.md` |
| `SmoresAI` | Individual character behavior: roles/archetypes, perception, drives, goal selection, and the per-character job queue | `SmoresCore`, `SmoresCharacters`, `SmoresCombat`, `SmoresItems`, `SmoresFactions`, `SmoresWorld` | `ai-and-behavior.md`, `orders-and-jobs.md` |
| `SmoresEconomy` **(EXISTS)** | **Value primitives only**: currency/wallet, pricing interface, traders and their stock | `SmoresCore`, `SmoresItems` | `economy.md` |
| `SmoresMarkets` | Market simulation proper: supply/demand, emergent regional pricing, trade routes, caravans | `SmoresCore`, `SmoresItems`, `SmoresEconomy`, `SmoresFactions`, `SmoresWorld` | `economy.md` |
| `SmoresWorld` | Map data, regions/biomes, POIs, fog of war/travel, wildlife, environmental events | `SmoresCore`, `SmoresFactions` (territory overlay) | `open-world.md`, `world-map-and-travel.md` |
| `SmoresBaseBuilding` | Outpost/town-building placement, Building Mode, supply chains/upkeep | `SmoresCore`, `SmoresItems`, `SmoresWorld`, `SmoresFactions` | `base-building.md` |
| `SmoresTechCrafting` | Tech tree, research, crafting | `SmoresCore`, `SmoresItems`, `SmoresCharacters` | `tech-and-crafting.md` |
| `SmoresUI` | HUD/UI framework: squad roster/divisions, mini-map widget, activity/alerts panel, meta menu | reads from most modules above | `player-interface.md`, `game-modes.md` |
| `SmoresSaveGame` | Serialization and version/mod-compatible migration | reads from most modules above | `save-system.md` |
| `SmoresOnlineSession` | Session hosting/join flow; later a thin wrapper over Steamworks/EOS per the "don't call platform SDKs directly" note | `SmoresCore` only | `multiplayer-and-content.md`, `input-and-platforms.md` |
| `SmoresEditor` (Editor-only, `TargetAllowList: Editor`) | Any custom asset editors/validation tooling (e.g. tech-tree or building-placement authoring tools) | varies, never shipped | — |

This table is a target shape, not a literal migration order — several of these modules
correspond to systems that don't exist in any form yet (factions, markets, base building,
tech/crafting are all still design-only per `game-design`).

**Why `SmoresEconomy` is split in two.** An earlier version of this table had one
`SmoresEconomy` covering "markets, emergent pricing, trade routes, caravans" and depending
on `SmoresFactions`. That conflates two layers with very different dependencies, and it bit
in practice: a player's *wallet* is an `int32` that needs nothing but items, yet the table
as written would have dragged a whole faction simulation onto it — which is part of why
Slice 4's gold ended up inline on `AStrategyPlayerState` instead. The low layer (currency,
a price, a transaction) was buildable immediately and is exactly what Slice 8 built; the high
layer (a simulated market that *sets* those prices) genuinely does need factions and world
and is years out. Keep them apart so the cheap half isn't held hostage by the expensive
one, and so the pricing interface has a home that a market sim can later implement from
above.

**Why `SmoresAI` is separate from `SmoresFactions`, and why neither is `SmoresCharacters`.**
The design (`ai-and-behavior.md`, `factions-and-world-state.md`) draws a hard line between
two things that both get called "AI": what one character does in the next few seconds, and
what a faction decides over in-game weeks. They share almost nothing in practice — the
faction layer needs no navigation, no perception, and no animation, and must keep running
for regions no player is near, while individual behavior is the opposite on every count.
Different cadence, different dependencies, two modules.

Neither belongs in `SmoresCharacters`. Behavior is a *consumer* of nearly everything —
health, inventory, faction standing, world/POI data — so putting it on the character
module would eventually force `SmoresCharacters` to depend on `SmoresFactions` and
`SmoresWorld`, dragging the whole simulation onto the class that just needs to walk and
swing. That is the same mistake the `SmoresEconomy`/`SmoresMarkets` note above describes.
`SmoresAI` sits high instead: it depends downward on nearly everything and almost nothing
depends on it (only `SmoresUI`, to show what a character is currently doing), which is a
healthy top-of-stack position rather than a hub.

The character module stays unaware of AI policy: the AI controller class is assigned as an
ordinary Blueprint default (`AIControllerClass` is an engine type), so no upward dependency
is created.

**What to build first, and the trigger to actually cut `SmoresAI`.** Per "When to Actually
Split" above, don't create the module speculatively. The piece that unblocks real work is
the **allegiance primitive** in `SmoresCore`: a character's team/faction identity plus an
"are we enemies?" answer. Today `AStrategyUnit::TryEngageNearestPlayerPawn` hardcodes "the
enemy is the nearest player pawn," which contradicts the design in several places at once
(wildlife has no politics; factions fight each other; the player is unaffiliated; co-op
means there is no singular player at all) — `game-systems`' `combat.md` already flags this
as an open extension point. Unreal's built-in `FGenericTeamId` /
`IGenericTeamAgentInterface` is the natural vehicle, and `UAIPerceptionComponent` reads
team attitude for free when filtering what a character notices. Low layer in `SmoresCore`
now (a naive "different id = hostile" answer), real standing in `SmoresFactions` later —
the same two-layer shape as `SmoresEconomy` vs. `SmoresMarkets`.

**The trigger for cutting `SmoresAI` is the first non-combat behavior** — a shopkeeper
idling at a counter, a creature grazing, a squad member working a job queue. At that point
`AStrategyUnit` is carrying behavior policy it has no business owning, which is a real
ownership seam discovered in practice rather than predicted from a design doc. Until then,
disposition and engagement can stay where they are.

**Known discrepancy**: the table above lists `SmoresCombat` depending on `SmoresCharacters`,
but the actual `AStrategyUnit`/`AStrategyPlayerUnit` move (see "Migrating Today's Prototype
Code") went the other way — `AStrategyUnit` owns a `UCombatComponent` and `UInventoryComponent`
as subobjects, so `SmoresCharacters` depends on `SmoresCombat` and `SmoresItems`, not the
reverse. No cycle exists either way (neither `SmoresCombat` nor `SmoresItems` reference
`AStrategyUnit`), but this table's dependency column needs reconciling with reality —
unresolved, tracked here rather than silently left wrong.

## Module Naming

**Decided: every custom module uses the `Smores` prefix** (`SmoresCore`, `SmoresCombat`,
`SmoresItems`, etc.), matching every module name already used elsewhere in this document.
This guards against collisions with engine modules (`Core` is already taken by the engine
itself) and future third-party plugins, at negligible cost. Not open for re-litigation.

The primary module's name (`smores`, no prefix) is a separate, unrelated fact — it matches
the project name only because Epic's project template names them identically. Unreal does
**not** require the two to agree: the primary module's name just has to be consistent across
`IMPLEMENT_PRIMARY_GAME_MODULE`, the `.uproject`'s `Modules` array, and both `Target.cs`
files. Epic's own Lyra sample proves it — project `LyraStarterGame.uproject`, primary module
`LyraGame`. So renaming `smores` → `SmoresGame` for consistency with the prefix convention is
a real option costing four files in lockstep, not the whole-project rename previously claimed
here. Not worth doing today, but don't decline it on the grounds that it's impossible.

## Modules vs. Plugins: Where DLC and Mods Fit

- Once a first DLC pack is actually being produced, each pack is a strong candidate to ship
  as its own **Game Feature Plugin** — Unreal's idiomatic mechanism for additive,
  self-contained content that can be gracefully absent for a player who joins a session
  without owning it, which is exactly what `multiplayer-and-content.md` requires ("content
  is gracefully hidden or falls back... told clearly... rather than having the join
  silently refused"). Don't build this scaffolding before there's a real DLC pack to hang
  it on.
- Mods are lighter-weight by design (`multiplayer-and-content.md`: best-effort
  compatibility only, not held to save-compatibility promises) — realistic mod support for
  a solo-maintained project means **data/Blueprint/content-only** modding (data tables for
  tech nodes, recipes, building requirements — see `tech-and-crafting.md`'s Modding
  section), not compiled third-party C++. Promising native-code mod plugins would be a
  support and security burden out of proportion to a one-person team; don't scope for it.

## When to Actually Split

Trigger conditions, not a schedule:

- Compile times becoming genuinely painful in one monolithic module.
- A system needs to load/unload independently — the clearest case being DLC content that
  must be absent gracefully for a player without the pack.
- A real ownership seam appears in practice — two areas of code that turn out not to need
  each other's internals, discovered through actual development rather than predicted from
  a design doc's table of contents.

### The junk drawer is a class, not a module

Measure before worrying. As of 2026-09-12, `smores` is 3250 lines — but **2433 of them are
`AStrategyPlayerController`** (1832 `.cpp` + 601 `.h`). Subtract it and the primary module
is ~800 lines of game mode, pawn, player state, EQS context, main menu, and dead template
classes: an ordinary primary module, not a bloated one.

`StrategyPlayerController.cpp` is meanwhile more than twice the size of the next-largest
file in the project and larger than `SmoresCombat` or `SmoresCharacters` *in their
entirety*. It currently owns camera control, unit selection, unit commands, inventory
windows, container/loot windows, touch input, and the debug execs — at least four unrelated
responsibilities. **That, not the module, is the thing actually trending toward
unmaintainable.**

The fix is extraction into components, same as everywhere else in this document, and the
obvious first cut is **camera control** (~400 lines, and the only part with no entanglement
with selection state). `IStrategyCameraCommands` already marks that seam. This is a
refactor to do opportunistically when camera work comes up anyway, not a scheduled project
— but don't let "`smores` is the biggest module" send anyone off cutting a new module when
the real problem is one file inside it.

### Dead template classes

`smoresCharacter`, `smoresGameMode`, and `smoresPlayerController` have **zero references**
— no C++ subclass, nothing in `Content/` (verified 2026-09-12 by grep over both). They
were originally kept "for potential reuse," but the template's `Lvl_TopDown` map and its
Blueprints were already deleted, so nothing remains that could reuse them. Deleting them is
pure subtraction from the primary module whenever someone wants a free win; re-check the
grep first in case a later variant picked them up.

**`Variant_Strategy` is currently doing double duty.** It's simultaneously "the current
RTS-style control-scheme prototype" and the closest thing the codebase has today to a
Characters/Combat module — `AStrategyUnit` already carries health, combat, and inventory
concerns that the target map above splits apart. Expect that class to eventually get pulled
apart into more specific pieces (a character base, a combat component) rather than growing
indefinitely inside `Variant_Strategy`. That refactor is itself a future trigger, not
something to do preemptively.

Splitting a module is a real, mechanical cost (Build.cs dependency wiring, per-module
include paths, the risk of a circular-dependency fight) — pay it when a trigger above
actually shows up, not speculatively.

## Registering a New Module

Creating a module is more than a folder — every new module needs all of the following, or
it silently won't compile in or link:

1. **The module's own `<ModuleName>.Build.cs`**, declaring its `PublicDependencyModuleNames`
   (per the target map's "Depends on" column) and, for anything under
   `Source/<ModuleName>/SubFolder/`, a `PublicIncludePaths` entry — same pattern
   `smores.Build.cs` already uses today.
2. **A root `<ModuleName>.cpp`/`.h` pair** carrying `IMPLEMENT_MODULE(FDefaultModuleImpl,
   <ModuleName>)` (or a custom `IModuleInterface` if the module needs startup/shutdown
   hooks) — every module needs exactly one of these.
3. **An entry in `smores.uproject`'s `"Modules"` array**: `Name`, `Type` (`Runtime`, or
   `Editor` for an editor-only module like `SmoresEditor`), and `LoadingPhase` (`Default`
   matches what `smores` already uses). Note that editing the `.uproject` makes UBT rebuild
   its makefile from scratch, which is what you want for a new module anyway.
4. **An `ExtraModuleNames.Add("<ModuleName>")` line in *both* `smores.Target.cs` and
   `smoresEditor.Target.cs`** — a Runtime module referenced only in the game target won't
   load in editor builds, and vice versa.
5. **A dependency entry in every module that references it** — e.g. once `SmoresCombat`
   exists, `smores`'s own `Build.cs` needs `SmoresCombat` added to
   `PublicDependencyModuleNames` for `AStrategyUnit` to keep calling into
   `UHealthComponent`.

Missing any one of these produces a build error that (mostly) says exactly what's missing
— but step 4 in particular is easy to do for only one of the two `Target.cs` files and get
an editor build that mysteriously can't find the new module.

## Migrating Today's Prototype Code

The existing prototype (`Variant_Strategy/Combat`, `Variant_Strategy/Inventory`,
`AStrategyUnit`, etc.) predates this module map entirely, so migrating it is a real,
sequenced project, not a single mechanical pass. Recommended order, cheapest and lowest-risk
first:

1. **DONE — proved the wiring with an empty module first.** `SmoresCore` was stood up with
   zero classes, just the `Build.cs` + root `.cpp`/`.h` + `.uproject`/`Target.cs`
   registration, validating the registration steps before any real class carried the risk.
2. **DONE — both Combat and Inventory have moved.** `HealthComponent`,
   `UAnimNotify_AttackHit`, `DamageNumberActor`/`DamageNumberWidget` are in `SmoresCombat`;
   `FInventoryItem`/`UInventoryComponent`, `AStrategyContainer`, `AStrategyChest` are in
   `SmoresItems`. Each move turned up one exception to "`AStrategyUnit` depends on them, not
   the other way around" — don't assume any other "obviously one-directional" class actually
   is until checked:
   - `AnimNotify_AttackHit` cast directly to `AStrategyUnit` to call `ApplyAttackDamage()`.
     Resolved with a small `IAttackDamageDealer` interface (declared in `SmoresCombat`); by
     the time `UCombatComponent` existed, that interface ended up implemented there instead
     of on `AStrategyUnit` directly, which is what let step 3 below move the character
     classes without `SmoresCharacters` needing to implement anything from `SmoresCombat`.
   - `AStrategyContainer::IsUnitInRange` took `const AStrategyUnit*` but only ever called
     `GetActorLocation()` on it — no interface needed, just widen the parameter to
     `const AActor*`. Cheaper fix than an interface when the dependency turns out to be
     that shallow; check whether it's actually needed before reaching for the pattern used
     for `AnimNotify_AttackHit`.
   Both `SmoresCombat` and `SmoresItems` got their own log category (`LogSmoresCombat`,
   `LogSmoresItems`) instead of reaching into `smores.h`'s `Logsmores` — expect every future
   module to need its own rather than sharing the primary module's.
3. **DONE — `AStrategyUnit`/`AStrategyPlayerUnit` moved into `SmoresCharacters`.** The
   prerequisite (combat resolution logic extracted out of `AStrategyUnit` into
   `UCombatComponent`, so `AttackTarget`/`PerformAttack`/`ApplyAttackDamage` no longer
   implement swing logic inline) was already done going in. The dependency-direction check
   confirmed no cycle: neither `SmoresCombat` nor `SmoresItems` reference `AStrategyUnit` at
   all, so `SmoresCharacters`'s `Build.cs` just adds `SmoresCombat`/`SmoresItems`/`SmoresCore`
   as dependencies (see the known table discrepancy noted under "Proposed Target Module
   Map"). Placed instances in `LVL_Strategy` were checked post-move via
   `SceneTools.find_actors` — no stale per-instance overrides this time, since no `UPROPERTY`
   moved to a different owning class (the whole actor class relocated modules, not a property
   onto a new component). One new lesson this move surfaced: see the `<Module>_API` export
   macro note added to "Per-move mechanics" below.
4. **DONE — `AStrategyHUD`/`UStrategyUI`/`UStrategyTouchControls`/`UWindowWidget`/
   `UInventoryWidget`/`UInventorySlotWidget`/`UInventoryDragDropOperation` moved into
   `SmoresUI`.** This one genuinely had the two-way tangle predicted above: `smores`'s
   `AStrategyPlayerController` needs `SmoresUI` (it owns/spawns `AStrategyHUD`/
   `UStrategyTouchControls`/`UInventoryWidget`), but three of the moving files also cast
   their owning PlayerController to the concrete `AStrategyPlayerController` and called
   PC-specific methods (`StrategyHUD.cpp`'s selection queries, `StrategyTouchControls.cpp`'s
   camera commands, `InventorySlotWidget.cpp`'s inventory-move RPC). Resolved with three
   narrow interfaces (`IStrategySelectionHost`, `IStrategyCameraCommands`,
   `IInventoryMoveHost`) declared in `SmoresUI` and implemented by `AStrategyPlayerController`
   — since `smores`→`SmoresUI` already had to exist, implementing interfaces declared there
   added no new dependency direction, exactly as anticipated. No method signatures changed;
   `AStrategyPlayerController`'s existing methods already matched the interfaces 1:1 (same
   plain-virtual-override pattern as `IAttackDamageDealer`, RPC macro and all — a
   `UFUNCTION(Server, Reliable)` method can satisfy a plain C++ interface's pure virtual
   with no special handling). Two real complications turned up, both now folded into
   "Per-move mechanics" below: `CoreRedirects` silently failed to resolve 2 of the 7 moved
   classes' Blueprints even across a clean editor restart (needed manual
   `BlueprintTools.set_parent` + recompile instead), and that manual reparent silently reset
   one of those Blueprints' class-default property overrides to null, which only surfaced as
   a PIE crash (`CreateWidget called with a null class`) — not as a compile or load error.

**Per-move mechanics, every time:**

- `git mv` the `.h`/`.cpp` files (preserves history) rather than delete-and-recreate.
- Update `#include` paths in the moved files and anything that referenced them — including
  any qualified include like `#include "Variant_Strategy/StrategyUnit.h"` elsewhere in
  `smores` that assumed the old folder location; it needs to become a bare
  `#include "StrategyUnit.h"` once the header lives in another module's
  `PublicIncludePaths`.
- Add/remove `PublicDependencyModuleNames` and `PublicIncludePaths` entries in both the
  source and destination modules' `Build.cs` files.
- **Add the new module's `<MODULE>_API` export macro to every moved class** (e.g.
  `class SMORESCHARACTERS_API AStrategyUnit : public ACharacter`). A class living in a
  monolithic module doesn't need this — everything links into the same DLL — but once it's
  the sole owner of symbols another module calls (`smores.dll` calling
  `AStrategyUnit::AttackTarget`, say), skipping the macro produces `LNK2019: unresolved
  external symbol` at link time for every method/property another module touches. Cheap to
  get right up front; easy to miss because the compile step alone won't catch it, only the
  link step will.
- **Moving a `UCLASS`/`USTRUCT` changes its native package path** (e.g.
  `/Script/smores.HealthComponent` → `/Script/SmoresCombat.HealthComponent`). Any Blueprint
  or asset that references the old path will show a missing/null parent class until either
  manually reparented or fixed globally with a `CoreRedirects` entry in
  `DefaultEngine.ini`:
  ```ini
  [CoreRedirects]
  +ClassRedirects=(OldName="/Script/smores.HealthComponent",NewName="/Script/SmoresCombat.HealthComponent")
  ```
  Add one per moved class before opening the editor, not after discovering broken
  Blueprints.
- **`CoreRedirects` is not guaranteed to resolve every Blueprint, even with a correctly
  written entry.** The `SmoresUI` move added 7 correctly-formed `ClassRedirects` entries;
  5 of the affected Blueprints resolved cleanly (including on a from-scratch editor
  process), but 2 (`BP_StrategyHUD`, `WBP_StrategyMobileControls`) still failed to load
  their parent class (`LogUObjectGlobals: Failed to find object 'Class /Script/smores.X'`)
  even after a clean editor restart — with no discernible difference in the redirect entry
  itself, and the target native class independently confirmed to exist and be correctly
  registered (`ObjectTools.search_subclasses`). Root cause undetermined; treat this as a
  real per-asset failure mode, not just a config mistake to double-check. **Fallback**:
  `BlueprintTools.set_parent(blueprint, correct_native_class)` followed by
  `BlueprintTools.compile_blueprint` and `AssetTools.save_assets` — this is the doc's
  already-documented "manually reparented" alternative, just confirmed to actually be
  needed in practice now, not merely theoretical.
- **A Blueprint reparented from a broken (null) parent can lose its class-default property
  overrides, not just its parent link — and this fails silently until runtime.** When
  `BP_StrategyHUD`'s parent failed to resolve, its previously-authored `UIWidgetClass`
  class-default override (pointing at `UI_Strategy`) was gone by the time `set_parent`
  fixed the parent link — compiling and saving both succeeded with no warning. The loss
  only surfaced as a PIE crash (`CreateWidget called with a null class`, an `AStrategyHUD`
  BeginPlay `check()` failure) well after the "migration" appeared complete. **After any
  manual reparent, diff the affected Blueprint's `EditAnywhere`/class-default properties
  against what they should be** (`ObjectTools.get_properties` on the Blueprint, which
  resolves to its CDO) before considering the move done — this is a distinct risk from,
  and not covered by, the placed-level-instance per-instance-override risk described next.
- **Moving a `UPROPERTY` off an actor onto a new component has the same content-compatibility
  risk as moving a class, but for properties, and there's no `CoreRedirects`-style fix.** Any
  actor already **placed in a level** (not just Blueprint class defaults) can carry a stale
  per-instance override of that property from before the component existed — e.g. splitting
  `SmoresCombat`'s attack-resolution logic out of `AStrategyUnit` into a `UCombatComponent`
  left every unit pre-placed in `LVL_Strategy` with an empty per-instance override of the
  relocated `AttackMontages`/`DownedMontage`, which silently shadowed a correctly-populated
  Blueprint class default and made combat look broken even after the class default was fixed.
  Fixing only the Blueprint's class defaults is not enough — enumerate placed instances of
  the affected class with `SceneTools.find_actors(actor_type=...)` and check/fix their
  component's properties directly too. See the `mcp-workflow` skill's CDO-override-flag
  caveat for why a raw MCP write to fix this can itself silently fail in a second, compounding
  way.
- **This is a structural change** — new module boundaries, not existing-function edits —
  so it needs the cold Visual Studio build CLAUDE.md already calls for, never Live Coding.
  Close the editor first.
- Commit each module cut as its own commit, separate from any feature work, so a build
  break is easy to bisect to "the move" rather than tangled with unrelated changes.

## Open Design Questions Worth Tracking

- Whether `SmoresUI` should itself split further, given `player-interface.md`'s
  roster/division-switcher complexity, or stays one module — deferred until real UI work
  on those features starts.
- Whether `SmoresOnlineSession` is worth standing up before session/connect flow work
  begins at all (that flow isn't built yet — see `multiplayer-discipline.md`) — listed here as a placeholder
  seam in the dependency graph, not a commitment to start it now.
- The exact boundary between `SmoresFactions` and `SmoresEconomy` will need revisiting once
  both are real code — `economy.md`'s market simulation and `factions-and-world-state.md`'s
  standing/assault systems are tightly coupled by design, and the table above is a starting
  guess, not a settled interface.
- Whether an automation/test module should exist to match `Automation_smores.slnx` isn't
  decided — no such module exists today despite that solution file's name.
- **Breaking up `AStrategyPlayerController`** (2433 lines across four-plus responsibilities)
  — camera control is the natural first extraction. Not scheduled; see "The junk drawer is a
  class, not a module."
- **DONE** — `AStrategyPlayerState::Gold` moved into a `UWalletComponent` in the new
  `SmoresEconomy`, and `IStrategyResourceHost` was deleted, both in Slice 8 of
  `Docs/roadmaps/inventory-roadmap.md`. See "Framework Classes vs. Feature Modules."
- Migration is complete — `SmoresCore`, `SmoresCombat`, `SmoresItems`, `SmoresCharacters` and
  `SmoresUI` (steps 1-4, see "Migrating Today's Prototype Code") are all done, and no further
  *migration* step is planned; the remaining modules in the target map correspond to systems
  that don't exist in any form yet. `SmoresEconomy` was added afterwards and is a different
  kind of event — a **new** module cut for a domain that had just become real, not existing
  code relocating. Expect the rest of the target map to arrive that way too.
