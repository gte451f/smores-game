# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**smores** is an Unreal Engine 5.8 game project. C++ is split across a primary module (`smores`) plus a growing set of `Smores*`-prefixed feature modules (currently `SmoresCore`, `SmoresCombat`, `SmoresItems`) per the `game-systems` skill's `unreal-module-organization` topic. The intended game is a **squad-based survival RPG in the vein of Kenshi** — the player commands a squad of individuals, not a single hero, but this is an RPG borrowing squad-command concepts, not an RTS or 4X. See the `game-design` skill for the full design intent (vision, pillars, and every major system) and the `game-systems` skill for the current player-facing behavior and implementation.

The current prototype's control scheme is built on the **Strategy** variant of Epic's Top Down template and is currently RTS-style (floating camera, click/drag-box selection, move commands) — that's an implementation detail of the current input/camera layer, not the intended genre. The template's TwinStick variant has been removed; abstract top-down base classes (`smoresCharacter` / `smoresGameMode` / `smoresPlayerController`) remain in `Source/smores/` for potential reuse, but the template's default `Lvl_TopDown` map and its Content/TopDown/ Blueprints have been removed — `LVL_Strategy` and `Lvl_MainMenu` are the only two maps in the project now.

## Starting the editor

UE 5.8 is installed at `C:\Program Files\Epic Games\UE_5.8`. Launch the project with:

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\dev\smores\smores.uproject"
```

Run it detached/backgrounded (it's a long-lived GUI process). Once the editor has finished loading, the `ModelContextProtocol` plugin hosts `unreal-mcp` at `http://127.0.0.1:8000/mcp` — the MCP server only becomes reachable after the editor is fully up, not immediately on process launch.

## Building

UE5.8 projects are built through the Unreal Editor or via UnrealBuildTool. There are no standalone build scripts — use the `.slnx`/`.sln` solution files:

- `smores.slnx` — game module only
- `Automation_smores.slnx` — includes automation/testing targets

To rebuild C++ from the editor: **Tools → Compile**. Live Coding hot-reloads *existing* functions/properties but is unreliable — for new `UCLASS`/`USTRUCT`/`UENUM` types or any structural change, close the editor and do a cold build from Visual Studio.

When executing an already-approved plan, it's safe to close the Unreal Editor gracefully (no need to ask first) as part of a cold-build step — just wait for confirmation it closed before building, and reopen it afterward per **Starting the editor** above.

## Development approach: C++ first

**Prefer writing C++ directly over driving the editor through `unreal-mcp`.** Even with Epic's
newer MCP toolsets, round-tripping gameplay logic through MCP tools is slower and less reliable
than editing `Source/smores/` and doing a build. Put behavior, state, systems, and anything
non-trivial in C++.

Use Blueprints (via `unreal-mcp` or the editor) for what they are actually good at:

- **Wiring**: assigning asset references to the `EditAnywhere` properties on the C++ classes
  (meshes, `UInputAction` / `UInputMappingContext`, widget classes, EQS queries, etc.).
- **Configuration**: tuning exposed `UPROPERTY` values, curves, and data-only assets.
- **Light glue**: small `BlueprintImplementableEvent` hooks (`BP_*`) for cosmetic or
  designer-facing responses.
- **Content-only assets**: materials, Niagara, animation, level layout.

Rule of thumb: if it has meaningful branching, iteration, or lifetime, it belongs in C++.
When a task needs both, write the C++ first, then use MCP/Blueprints only to bind and configure it.

## Multiplayer discipline

Co-op is designed in from day one (self-hosted listen-server or dedicated server, up to 8
players, server-authoritative simulation — see the `game-design` skill's
`multiplayer-and-content.md`), even though multiplayer itself isn't wired up or testable
yet. Unreal's built-in networking (replication, RPCs, server authority) is meant to supply
the large majority of what multiplayer needs; the responsibility on the code side is
discipline now, since retrofitting these habits later is far more expensive than following
them from the start.

When writing gameplay code:

- **No singleton-player assumptions.** Never assume there is exactly one
  `PlayerController`, camera, squad, or HUD in the world. Key state and lookups off the
  owning `PlayerController`/`PlayerState`, not a global/singleton reference — the single
  most expensive habit to retrofit later.
- **Gate shared-state mutation on authority.** Anything that changes world state other than
  the local player's own cosmetic/UI state — health, inventory, faction standing, squad
  membership, item ownership — must check `HasAuthority()` (or run through a
  `Server`-flagged RPC) before mutating it. Never assume client == server.
- **Replicate through the engine's mechanisms, not ad hoc sync.** Shared state goes through
  `UPROPERTY(Replicated)` + `GetLifetimeReplicatedProps` (with `RepNotify` where clients
  react to a change); actions go through RPCs (`Server`/`Client`/`NetMulticast`). Don't
  invent a custom sync path when replication already covers the case.
- **Decide data ownership before writing a system.** Before adding new gameplay state,
  decide who authoritatively owns it (usually the server) and who merely holds a
  replicated copy — this determines whether it needs to be `Replicated` at all.
- **Don't add prediction machinery speculatively.** The point-and-click command scheme
  (`Variant_Strategy`) is latency-tolerant by design — no client-side
  prediction/reconciliation unless a specific system proves it's needed.
- **Dedicated server hosting targets Linux**, cross-compiled from the same C++ source as
  the Windows client. Avoid Windows-only APIs/dependencies in gameplay code, and watch
  asset-reference case sensitivity (Linux is case-sensitive, Windows isn't) once a Linux
  cook is attempted.

Session/connect flow, dedicated server packaging, and the Linux cross-compile toolchain
itself aren't built yet — see `multiplayer-and-content.md` for what's scheduled vs.
deferred.

## Architecture

### Module layout

Four runtime modules today: the primary `smores` module, plus `SmoresCore` (empty
proving module), `SmoresCombat` (health/damage, floating damage numbers, the
attack-hit anim notify), and `SmoresItems` (inventory, container/chest actors) — each a
sibling folder directly under `Source/`, per the `game-systems` skill's
`unreal-module-organization` topic, which tracks the target module map and migration
order. Everything else still compiles into `smores`.

```
Source/
  smores/
    smoresCharacter.*          – Abstract base: top-down character with SpringArm + Camera
    smoresGameMode.*           – Abstract base game mode
    smoresPlayerController.*   – Abstract base: point-and-click nav movement via PathFollowingComponent

    Variant_Strategy/          – Squad gameplay, the active variant (RTS-style camera/selection/command controls)
  SmoresCore/                  – Empty proving module; no classes yet
  SmoresCombat/                – HealthComponent, DamageNumberActor/Widget, AnimNotify_AttackHit, IAttackDamageDealer
  SmoresItems/                 – InventoryComponent (FInventoryItem), StrategyContainer, StrategyChest
```

### Class hierarchy pattern

All C++ gameplay classes are `UCLASS(abstract)`. Blueprint subclasses (under `Content/`) provide the actual asset bindings (meshes, input actions, widget classes, etc.). Never instantiate the C++ classes directly.

Blueprint hooks follow the convention `BP_*` (`BP_Damaged`, `BP_UnitSelected`, etc.) and are declared as `BlueprintImplementableEvent`.

### Variant_Strategy

The active gameplay variant. Squad-based by design intent (see the `game-design` skill);
its current control scheme — camera pan/zoom, click/drag-box selection, move commands —
is RTS-style, inherited from Epic's Strategy template. Key classes: `AStrategyPlayerController`
(selection, camera pan/zoom, mouse + touch input), `AStrategyUnit` (abstract AI-driven
character, EQS-refined movement via `AAIController`), `AStrategyPawn` (camera-only pawn),
`AStrategyHUD` (drag-select box), `EnvQueryContext_MoveGoal`. See the `game-systems` skill
for full behavior detail (selection rules, movement/command flow, EQS queries).

### Input system

All input uses **Enhanced Input** (`UInputAction` / `UInputMappingContext`). Input actions are `EditAnywhere` properties on the C++ classes — actual `UInputAction` assets are assigned in the Blueprint subclass. Strategy uses separate `MouseMappingContext` and `TouchMappingContext` (the RTS control scheme supports touch).

### AI

- **Strategy units**: NavMesh + `AAIController::MoveToLocation` with EQS refinement for destination selection.
- NavMesh is configured for dynamic runtime generation (`RuntimeGeneration=Dynamic` in `DefaultEngine.ini`).

### Plugins enabled

| Plugin | Purpose |
|---|---|
| ModelContextProtocol | Hosts the in-editor `unreal-mcp` HTTP server |
| AllToolsets | Registers all Epic AI toolsets (Blueprint, scene, assets, Sequencer, …) with `unreal-mcp` |
| StateTree / GameplayStateTree | No longer used by game code; kept only as an `AllToolsets` dependency |

## Content layout

```
Content/
  MainMenu/           – Title screen (Lvl_MainMenu), the project's GameDefaultMap
  Variant_Strategy/   – Squad gameplay assets, the active variant (RTS-style controls); LVL_Strategy is the editor's EditorStartupMap and GlobalDefaultGameMode
  Characters/         – Shared character assets
  LevelPrototyping/   – Scratch levels
```

## MCP servers

One MCP server is configured (`.mcp.json`): **unreal-mcp**, an HTTP server hosted by the editor at `http://127.0.0.1:8000/mcp` (Epic's `ModelContextProtocol` plugin). Requires the Unreal Editor running.

Reach for these tools for wiring, configuration, and content-only work — not for gameplay logic. See **Development approach: C++ first** above. For the discovery workflow, token-discipline habits, and known tool bugs/caveats, use the `mcp-workflow` skill.
