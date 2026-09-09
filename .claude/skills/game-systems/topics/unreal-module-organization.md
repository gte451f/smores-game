# Unreal Module Organization

## Purpose

Unlike the other topics in this skill, this one isn't documenting a finished, playable
system — it's a **forward-looking reference** for how `Source/` should eventually split
into multiple Unreal modules as the project grows to cover the full scope described across
the `game-design` skill (factions, economy, characters/squads, base building, tech,
open world, saves, UI, multiplayer, narrative). The goal is that whoever eventually cuts
the first new module — human or agent — has a plan to extend rather than an ad hoc
decision to invent mid-refactor. **Nothing here is a mandate to split anything today** —
see "When to Actually Split" below.

## Current State

One runtime module, `smores` (`Source/smores/smores.Build.cs`), one game `Target.cs` and
one Editor `Target.cs`. Organized by feature folder, not by module — see CLAUDE.md's
"Module layout" for the authoritative current tree (`Variant_Strategy/` with `Combat/`,
`Inventory/`, `UI/` subfolders; `MainMenu/` with its own `UI/`). There is no `Plugins/`
folder for game-specific code yet — everything gameplay-related compiles into the one
primary module. The engine-side plugins already enabled (`ModelContextProtocol`,
`AllToolsets`, `StateTree`) are editor/MCP tooling, unrelated to this topic.

## Concrete Folder Structure

Every module is a **sibling folder directly under `Source/`**, never nested inside
`Source/smores/`. `smores` is the project's primary/game module — its name is tied to the
project name (`smores.uproject`) and Unreal requires the primary module to share it, so it
isn't going anywhere without a full project rename (see "Module Naming" below). Every
other module sits next to it as a peer:

```
Source/
  smores.Target.cs
  smoresEditor.Target.cs
  smores/                         # primary module — name fixed, shrinks over time
    smores.Build.cs
    smores.cpp / smores.h
    smoresCharacter.* smoresGameMode.* smoresPlayerController.*   (template base classes)
    MainMenu/
      MainMenuGameMode.* MainMenuHUD.*
      UI/  MainMenuWidget.* OptionsWidget.*
    Variant_Strategy/             # shrinks as pieces below are peeled out
      StrategyGameMode.* StrategyPawn.* StrategyPlayerController.*
      StrategyUnit.* StrategyPlayerUnit.*   (until the Characters/Combat split happens)
      EnvQueryContext_MoveGoal.*
  SmoresCore/
    SmoresCore.Build.cs
    SmoresCore.cpp / SmoresCore.h
  SmoresItems/
    SmoresItems.Build.cs
    SmoresItems.cpp / SmoresItems.h
    InventoryComponent.*          # moved from smores/Variant_Strategy/Inventory/
    StrategyChest.* StrategyContainer.*   # candidates — storage containers
  SmoresCombat/
    SmoresCombat.Build.cs
    SmoresCombat.cpp / SmoresCombat.h
    HealthComponent.*             # moved from smores/Variant_Strategy/Combat/
    AnimNotify_AttackHit.*
    DamageNumberActor.* DamageNumberWidget.*
  SmoresCharacters/                # future: AStrategyUnit/AStrategyPlayerUnit split out of Variant_Strategy
  SmoresFactions/
  SmoresEconomy/
  SmoresWorld/
  SmoresBaseBuilding/
  SmoresTechCrafting/
  SmoresUI/                        # future: StrategyHUD/StrategyUI/inventory widgets — see sequencing note below
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
  (see CLAUDE.md's "Multiplayer discipline"). Don't invent a client/server module split
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

## Proposed Target Module Map

| Module | Responsibility | Depends on | Design reference |
|---|---|---|---|
| `SmoresCore` | Shared low-level types/utilities with no gameplay-specific logic (common structs, interfaces, math/helpers) | Core, CoreUObject, Engine | — |
| `SmoresItems` | Item definitions/data tables, stackable-item storage (today's `FInventoryItem`/`UInventoryComponent`) | `SmoresCore` | `inventory.md` (current), `economy.md`'s Goods |
| `SmoresCharacters` | Lineage, attributes/skills, recruitment/wages/morale, injuries, squad/division management | `SmoresCore`, `SmoresItems` | `characters-and-squads.md` |
| `SmoresCombat` | Health/damage, melee resolution, disposition/aggro, incapacitation/capture | `SmoresCore`, `SmoresCharacters` | `combat.md` (both skills) |
| `SmoresFactions` | Faction simulation, standing, territory, assault intelligence, military progression | `SmoresCore`, `SmoresCharacters` | `factions-and-world-state.md` |
| `SmoresEconomy` | Markets, emergent pricing, trade routes, caravans | `SmoresCore`, `SmoresItems`, `SmoresFactions` | `economy.md` |
| `SmoresWorld` | Map data, regions/biomes, POIs, fog of war/travel, wildlife, environmental events | `SmoresCore`, `SmoresFactions` (territory overlay) | `open-world.md`, `world-map-and-travel.md` |
| `SmoresBaseBuilding` | Outpost/town-building placement, Building Mode, supply chains/upkeep | `SmoresCore`, `SmoresItems`, `SmoresWorld`, `SmoresFactions` | `base-building.md` |
| `SmoresTechCrafting` | Tech tree, research, crafting | `SmoresCore`, `SmoresItems`, `SmoresCharacters` | `tech-and-crafting.md` |
| `SmoresUI` | HUD/UI framework: squad roster/divisions, mini-map widget, activity/alerts panel, meta menu | reads from most modules above | `player-interface.md`, `game-modes.md` |
| `SmoresSaveGame` | Serialization and version/mod-compatible migration | reads from most modules above | `save-system.md` |
| `SmoresOnlineSession` | Session hosting/join flow; later a thin wrapper over Steamworks/EOS per the "don't call platform SDKs directly" note | `SmoresCore` only | `multiplayer-and-content.md`, `input-and-platforms.md` |
| `SmoresEditor` (Editor-only, `TargetAllowList: Editor`) | Any custom asset editors/validation tooling (e.g. tech-tree or building-placement authoring tools) | varies, never shipped | — |

This table is a target shape, not a literal migration order — several of these modules
correspond to systems that don't exist in any form yet (factions, economy, base building,
tech/crafting are all still design-only per `game-design`).

## Module Naming

**Decided: every custom module uses the `Smores` prefix** (`SmoresCore`, `SmoresCombat`,
`SmoresItems`, etc.), matching every module name already used elsewhere in this document.
This guards against collisions with engine modules (`Core` is already taken by the engine
itself) and future third-party plugins, at negligible cost. Not open for re-litigation.

The primary module's name (`smores`, no prefix) is a separate, unrelated fact — it's fixed
to the project name (`smores.uproject`), since Unreal requires the primary module to share
it. Renaming it means renaming the whole project, which is out of scope here.

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
   matches what `smores` already uses).
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

1. **Prove the wiring with an empty module first.** Stand up one new module (e.g.
   `SmoresCore`) with zero classes moved into it — just the `Build.cs` + root
   `.cpp`/`.h` + `.uproject`/`Target.cs` registration from the checklist above — and get a
   clean cold build. This validates the registration steps once, cheaply, before any real
   class carries the risk.
2. **Move the cleanest one-directional dependencies next.** `HealthComponent`,
   `UAnimNotify_AttackHit`, `DamageNumberActor`/`DamageNumberWidget` → `SmoresCombat`; and
   `InventoryComponent` (and probably `StrategyChest`/`StrategyContainer`) → `SmoresItems`.
   These are good first real moves specifically because today's `AStrategyUnit` depends on
   them, not the other way around — moving them doesn't require touching `AStrategyUnit`
   beyond updating its `#include` paths and adding `SmoresCombat`/`SmoresItems` to
   `smores.Build.cs`'s dependencies.
3. **`AStrategyUnit`/`AStrategyPlayerUnit` into `SmoresCharacters` is the big one, and it's
   not a pure file move.** Per "When to Actually Split" above, these classes currently
   implement combat swing logic inline (`AttackTarget`/`PerformAttack`/`ApplyAttackDamage`)
   rather than delegating to `SmoresCombat`. Pulling the character body out means actually
   extracting that combat resolution logic first — do this only after step 2 has proven the
   module-cut mechanics on lower-stakes code.
4. **`SmoresUI` should come after `SmoresCharacters` exists, not before.** `AStrategyHUD`/
   `UStrategyUI`/the inventory widgets currently reference `AStrategyPlayerController` and
   `AStrategyUnit` directly. Splitting UI out first would just relocate the tangle (`SmoresUI`
   depending back on `smores`, and possibly vice versa) rather than resolve it. `smores`
   (which keeps `AStrategyPlayerController`) depending on `SmoresUI` is fine either way —
   only the reverse would be a cycle.

**Per-move mechanics, every time:**

- `git mv` the `.h`/`.cpp` files (preserves history) rather than delete-and-recreate.
- Update `#include` paths in the moved files and anything that referenced them.
- Add/remove `PublicDependencyModuleNames` and `PublicIncludePaths` entries in both the
  source and destination modules' `Build.cs` files.
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
  begins at all (per CLAUDE.md, that flow "isn't built yet") — listed here as a placeholder
  seam in the dependency graph, not a commitment to start it now.
- The exact boundary between `SmoresFactions` and `SmoresEconomy` will need revisiting once
  both are real code — `economy.md`'s market simulation and `factions-and-world-state.md`'s
  standing/assault systems are tightly coupled by design, and the table above is a starting
  guess, not a settled interface.
- Whether an automation/test module should exist to match `Automation_smores.slnx` isn't
  decided — no such module exists today despite that solution file's name.
- No migration has started yet — the order in "Migrating Today's Prototype Code" is a
  recommendation to follow once work begins, not a record of what's already been done.
  Update that section's status as each step actually happens.
