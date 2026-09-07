# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**smores** is an Unreal Engine 5.8 game project with a single C++ module (`smores`). The intended game is a **squad-based survival RPG in the vein of Kenshi** — the player commands a squad of individuals, not a single hero, but this is an RPG borrowing squad-command concepts, not an RTS or 4X. See the `game-design` skill for the full design intent (vision, pillars, and every major system) and the `player-facing` / `game-systems` skills for the current player-facing behavior and implementation.

The current prototype's control scheme is built on the **Strategy** variant of Epic's Top Down template and is currently RTS-style (floating camera, click/drag-box selection, move commands) — that's an implementation detail of the current input/camera layer, not the intended genre. The template's TwinStick variant has been removed; a plain top-down base (`smoresCharacter` / `smoresGameMode` / `smoresPlayerController`) and the default `Lvl_TopDown` map remain.

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

## Architecture

### Module layout

All C++ lives under `Source/smores/`. There is one runtime module.

```
Source/smores/
  smoresCharacter.*          – Abstract base: top-down character with SpringArm + Camera
  smoresGameMode.*           – Abstract base game mode
  smoresPlayerController.*   – Abstract base: point-and-click nav movement via PathFollowingComponent

  Variant_Strategy/          – Squad gameplay, the active variant (RTS-style camera/selection/command controls)
```

### Class hierarchy pattern

All C++ gameplay classes are `UCLASS(abstract)`. Blueprint subclasses (under `Content/`) provide the actual asset bindings (meshes, input actions, widget classes, etc.). Never instantiate the C++ classes directly.

Blueprint hooks follow the convention `BP_*` (`BP_Damaged`, `BP_UnitSelected`, etc.) and are declared as `BlueprintImplementableEvent`.

### Variant_Strategy

The active gameplay variant. Squad-based by design intent (see the `game-design` skill);
its current control scheme — camera pan/zoom, click/drag-box selection, move commands —
is RTS-style, inherited from Epic's Strategy template. Key types:

| Class | Role |
|---|---|
| `AStrategyPlayerController` | Unit selection (click, drag-box, double-tap), camera pan/zoom, mouse + touch input paths |
| `AStrategyUnit` | Abstract AI-driven character; commanded indirectly by the PC. Uses EQS (`InteractionQuery`, `NoInteractionQuery`) to refine movement destinations |
| `AStrategyPawn` | Camera-only pawn controlled by the Strategy PC |
| `AStrategyHUD` | Renders drag-select box |
| `EnvQueryContext_MoveGoal` | EQS context that exposes the unit's current movement goal |

Selection state lives on `AStrategyPlayerController::ControlledUnits`. Movement commands go through `AStrategyUnit::MoveToLocation`, which runs an EQS query then delegates to the `AAIController`.

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
  TopDown/            – Default map (Lvl_TopDown) and BP_TopDownGameMode (set as GlobalDefaultGameMode)
  Variant_Strategy/   – Squad gameplay assets, the active variant (RTS-style controls)
  Characters/         – Shared character assets
  LevelPrototyping/   – Scratch levels
```

## MCP servers

One MCP server is configured (`.mcp.json`): **unreal-mcp**, an HTTP server hosted by the editor at `http://127.0.0.1:8000/mcp` (Epic's `ModelContextProtocol` plugin). Requires the Unreal Editor running.

Reach for these tools for wiring, configuration, and content-only work — not for gameplay logic. See **Development approach: C++ first** above.

### Discovery workflow (do this — don't assume)

`unreal-mcp` exposes only three meta-tools; the real tools are discovered on demand, so **no full tool list is in context**:

1. `list_toolsets` — ~50 toolsets registered via the `AllToolsets` plugin. Names come in two styles, both used verbatim: `EditorToolset.EditorAppToolset` and `editor_toolset.toolsets.blueprint.BlueprintTools`.
2. `describe_toolset(<name>)` — tool names + schemas for one toolset. Some responses are large; request only the toolset you need.
3. `call_tool(tool_name, toolset_name, arguments)` — invoke.

### What's available (high level)

- **Blueprints**: `editor_toolset.toolsets.blueprint.BlueprintTools` — create assets/graphs/variables/functions/nodes, wire pins, compile. Prefer `write_graph_dsl` (read `get_graph_dsl_docs` first) over node-by-node.
- **Objects/props**: `editor_toolset.toolsets.object.ObjectTools` — read/write properties on any object or CDO. Other toolsets depend on it for property access.
- **Also registered**: scene/actor/asset/material tools, UMG, Niagara, Control Rig + Sequencer, GAS, GameplayTags, PCG, Physics, and `ProgrammaticToolset` (batch calls via a sandboxed Python script). Discover schemas on demand.

### Token discipline (MCP results persist for the whole session)

Every `call_tool` result stays in context until the session ends. On the two tasks
that built these systems, `unreal-mcp` was ~40%+ of total token usage. Keep it small:

- **Run MCP-heavy editor wiring in a `fork` / subagent.** A multi-step asset build is
  ~20–30 calls whose payloads are pure noise once the work is done. Let the fork do the
  wiring and report back "done + any manual steps"; the main context never sees the calls.
- **Do the C++ first, then wire in one pass.** Workflow: close editor → cold build →
  reopen → do all wiring → `/compact`. Don't interleave.
- **Skip `list_properties`.** It dumps the entire inherited property tree with every
  nested sub-schema (a `TextBlock` is ~6 KB). Call `get_properties` / `set_properties`
  with the field names you already know; only fall back to discovery on a failed set.
- **Don't `get_properties` just to learn field names** — `set_properties` returns a bool,
  so set and check.
- **Batch multi-step builds through `ProgrammaticToolset.execute_tool_script`** (one
  round-trip instead of a dozen `AddWidget` / `set_properties` / compile calls).
- `describe_toolset` on a big toolset (BlueprintTools, UMGToolSet) auto-persists to a
  file and stays *out* of context — read it back with `jq`/grep for the tools you need.

### Caveats

- Prefer `BlueprintTools` (sets override flags + compiles) over raw CDO writes. If writing a CDO property directly, the Blueprint override flag is **not** set automatically — have the user verify and save the Blueprint in the editor afterward.
- Create/modify operations that touch project assets: get user confirmation first.
- **`UInputMappingContext` key mappings can't be round-tripped via MCP.** `ObjectTools`
  reads the array back in a different shape than it writes (`key` flattens to a string;
  mappings hold refPaths to IMC-owned instanced modifier/trigger subobjects), and the
  operative array is `defaultKeyMappings.mappings`, not the empty top-level `mappings`.
  Creating the `UInputAction` asset is fine — duplicate an existing IA of the same
  `ValueType` (`IA_Strategy_CyclePawn` is `Boolean`) — but hand the IMC key-binding step
  to the user.
- New `UCLASS`/`USTRUCT`/`UENUM` (or any `UPROPERTY` add) needs a cold build with the
  editor **closed** before the MCP phase can reference the new types — Live Coding won't
  register them. See **Building**.
- **`PluginToolset.SetPluginEnabled` doesn't persist.** It returns success but writes
  nothing to disk and doesn't change `IsEnabled`'s result, even before restarting.
  Confirmed by calling it on a plugin and immediately re-checking `IsEnabled` (still
  true) and the `.uproject` file (unchanged). To actually disable/enable a plugin,
  close the editor and add/edit an explicit `{"Name": ..., "Enabled": false}` entry in
  `smores.uproject`'s `Plugins` array directly (same shape the editor's own Plugin
  Browser writes), then reopen. Use `GetPluginDependents`/`GetPluginDependencies` first
  to check for non-optional dependents before disabling anything.
