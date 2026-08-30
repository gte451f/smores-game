# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**smores** is an Unreal Engine 5.8 game project with a single C++ module (`smores`). It is an RTS-style game where the player commands AI-driven pawns, built on the **Strategy** variant of Epic's Top Down template. The template's TwinStick variant has been removed; a plain top-down base (`smoresCharacter` / `smoresGameMode` / `smoresPlayerController`) and the default `Lvl_TopDown` map remain.

## Building

UE5.8 projects are built through the Unreal Editor or via UnrealBuildTool. There are no standalone build scripts — use the `.slnx`/`.sln` solution files:

- `smores.slnx` — game module only
- `Automation_smores.slnx` — includes automation/testing targets

To rebuild C++ from the editor: **Tools → Compile** (or Live Coding for hot-reload of *existing* functions/properties). For new `UCLASS`/`USTRUCT`/`UENUM` types or structural changes, close the editor and do a full build from Visual Studio.

> **Live Coding is unreliable for new UCLASS types and sometimes for existing edits** — always do a cold VS build for structural changes.

## Architecture

### Module layout

All C++ lives under `Source/smores/`. There is one runtime module.

```
Source/smores/
  smoresCharacter.*          – Abstract base: top-down character with SpringArm + Camera
  smoresGameMode.*           – Abstract base game mode
  smoresPlayerController.*   – Abstract base: point-and-click nav movement via PathFollowingComponent

  Variant_Strategy/          – RTS gameplay (the active variant)
```

### Class hierarchy pattern

All C++ gameplay classes are `UCLASS(abstract)`. Blueprint subclasses (under `Content/`) provide the actual asset bindings (meshes, input actions, widget classes, etc.). Never instantiate the C++ classes directly.

Blueprint hooks follow the convention `BP_*` (`BP_Damaged`, `BP_UnitSelected`, etc.) and are declared as `BlueprintImplementableEvent`.

### Variant_Strategy

An RTS-style variant. Key types:

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
| StateTree / GameplayStateTree | Enabled but unused by game code (was TwinStick NPC AI); kept as a dependency of the `AllToolsets` StateTree toolset |
| ModelContextProtocol + MCPClientToolset | MCP integration — exposes the in-editor `unreal-mcp` HTTP server |
| AllToolsets | Registers all Epic AI toolsets (Blueprint, scene, assets, Sequencer, …) with `unreal-mcp` |
| ModelingToolsEditorMode | In-editor mesh modeling |
| Terminal | In-editor terminal |

## Content layout

```
Content/
  TopDown/            – Default map (Lvl_TopDown) and BP_TopDownGameMode (set as GlobalDefaultGameMode)
  Variant_Strategy/   – RTS gameplay assets (the active variant)
  Characters/         – Shared character assets
  LevelPrototyping/   – Scratch levels
```

## MCP servers

One MCP server is configured (`.mcp.json`): **unreal-mcp**, an HTTP server hosted by the editor at `http://127.0.0.1:8000/mcp` (Epic's `ModelContextProtocol` plugin). Requires the Unreal Editor running. Replaces the retired `flopperam-unreal`/`unreal-api` servers.

### Discovery workflow (do this — don't assume)

`unreal-mcp` exposes only three meta-tools; the real tools are discovered on demand, so **no full tool list is in context**:

1. `list_toolsets` — ~50 toolsets registered via the `AllToolsets` plugin. Names come in two styles, both used verbatim: `EditorToolset.EditorAppToolset` and `editor_toolset.toolsets.blueprint.BlueprintTools`.
2. `describe_toolset(<name>)` — tool names + schemas for one toolset. Some responses are large; request only the toolset you need.
3. `call_tool(tool_name, toolset_name, arguments)` — invoke.

### What's available (high level)

- **Blueprints**: `editor_toolset.toolsets.blueprint.BlueprintTools` — create assets, graphs, variables, functions, event dispatchers, nodes/pins/wiring, compile. Prefer `write_graph_dsl` (call `get_graph_dsl_docs` first) over node-by-node.
- **Objects/props**: `editor_toolset.toolsets.object.ObjectTools` — read/write properties on any object or CDO, discover classes. UMG/other toolsets depend on it for property access.
- **Scene/assets**: `ActorTools`, `SceneTools` (place/remove actors, levels, camera), `AssetTools`, `MaterialTools`, mesh/texture/data-table toolsets.
- **Also**: UMG, Niagara (5), Control Rig + full Sequencer suite, GAS, GameplayTags, PCG, Physics assets, `SlateInspectorToolset` (editor UI automation), inspect-only BehaviorTree/StateTree/Conversation.
- `editor_toolset.toolsets.programmatic.ProgrammaticToolset` batches multiple tool calls via a sandboxed Python script.

### Caveats

- Prefer `BlueprintTools` (sets override flags + compiles) over raw CDO writes. If writing a CDO property directly, the Blueprint override flag is **not** set automatically — have the user verify and save the Blueprint in the editor afterward.
- Create/modify operations that touch project assets: get user confirmation first.
