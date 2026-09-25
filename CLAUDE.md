# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

**This file is deliberately short — it holds only what applies to every session.** Anything
with real detail lives in a skill, and this file points at it rather than restating it. Don't
assume this file is the whole picture:

| Skill | Use it for |
|---|---|
| `game-design` | Design intent only: vision, pillars, and the desired end state of every major system. No implementation detail. |
| `game-systems` | What's actually implemented and where it lives in C++: camera/selection, unit commands, inventory, combat, dialog, refusal messaging, keybinds, testing, module organization, multiplayer discipline. |
| `mcp-workflow` | Driving the editor through `unreal-mcp`: discovery workflow, token-discipline habits, known tool bugs. |

## Project Overview

**smores** is an Unreal Engine 5.8 game project. The intended game is a **squad-based
survival RPG in the vein of Kenshi** — the player commands a squad of individuals, not a
single hero. The setting is an open-world sandbox with a living world: as if a 4X game is
playing out at the world level while the player steers their squad. See the `game-design`
skill for the full intent.

The current prototype is built on the **Strategy** variant of Epic's Top Down template, so
its controls are RTS-style (floating camera, click/drag-box selection, move commands) —
that's an implementation detail of today's input/camera layer, not the intended genre.
`LVL_Strategy` and `Lvl_MainMenu` are the only two maps in the project; the abstract
top-down base classes (`smoresCharacter` / `smoresGameMode` / `smoresPlayerController`)
remain in `Source/smores/` but nothing references them.

For the current behavior of any gameplay system — selection rules, command flow, inventory,
combat — read the matching `game-systems` topic before changing it.

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

To rebuild C++ from the editor: **Tools → Compile**. Live Coding hot-reloads *existing*
functions/properties but is unreliable — for new `UCLASS`/`USTRUCT`/`UENUM` types or any
structural change, close the editor and do a cold build from Visual Studio.

When executing an already-approved plan, it's safe to close the Unreal Editor gracefully (no need to ask first) as part of a cold-build step — just wait for confirmation it closed before building, and reopen it afterward per **Starting the editor** above.

## Development approach: C++ first

**Prefer writing C++ directly over driving the editor through `unreal-mcp`.** Even with
Epic's newer MCP toolsets, round-tripping gameplay logic through MCP tools is slower and
less reliable than editing the C++ and doing a build. Put behavior, state, systems, and
anything non-trivial in C++.

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

Co-op is designed in from day one (up to 8 players, server-authoritative simulation), even
though multiplayer itself isn't wired up or testable yet — these habits are far cheaper to
follow now than to retrofit later. **Read `game-systems`' `multiplayer-discipline.md`
before writing or changing gameplay state.** The short version:

- **No singleton-player assumptions** — key state and lookups off the owning
  `PlayerController`/`PlayerState`, never a global or singleton.
- **Gate every shared-state mutation** (health, inventory, ownership, standing, squad
  membership) on `HasAuthority()` or a `Server`-flagged RPC.
- **Replicate through the engine's mechanisms** — `UPROPERTY(Replicated)` +
  `GetLifetimeReplicatedProps` and RPCs, not ad hoc sync.
- **Decide who owns new state before writing it**, and don't add prediction machinery
  speculatively.

## Architecture

### Module layout

Eight runtime modules, each a sibling folder directly under `Source/`: the primary `smores`
module plus `SmoresCore`, `SmoresCombat`, `SmoresItems`, `SmoresCharacters`, `SmoresUI`,
`SmoresEconomy`, and `SmoresDialog`. `smores` holds only the Strategy game framework (game mode,
player controller, player state, camera pawn), the main menu, and the unused template base
classes — every gameplay domain lives in a feature module.

Dependency direction runs `smores` → {`SmoresUI`, `SmoresDialog`} → {`SmoresCharacters`,
`SmoresEconomy`} → {`SmoresItems`, `SmoresCombat`} → `SmoresCore`, and never back. (`SmoresUI`
will depend on `SmoresDialog` once the conversation window exists.) Two consequences worth
knowing before you write anything:

- **Nothing depends on `smores`.** When a lower module needs behavior from
  `AStrategyPlayerController`, it declares a narrow interface in its own module and the
  controller implements it (`IStrategySelectionHost`, `IStrategyCameraCommands`,
  `IInventoryMoveHost` in `SmoresUI`; `IDialogHost` in `SmoresDialog`).
- **New per-player or per-pawn state goes in a component in a feature module, never inline
  on a framework class** — and prefer such a component over a fourth interface when what
  the UI wants is per-player *state* rather than *behavior*.

**Before adding, moving, or splitting a module, read `game-systems`'
`unreal-module-organization.md`.** It holds the per-module class inventory, the target module
map, the five-step registration checklist, and the per-move gotchas (`CoreRedirects`,
`<MODULE>_API` export macros, stale placed-instance overrides) — each of which has already
cost a debugging session at least once.

### Class hierarchy pattern

All C++ gameplay classes are `UCLASS(abstract)`. Blueprint subclasses (under `Content/`) provide the actual asset bindings (meshes, input actions, widget classes, etc.). Never instantiate the C++ classes directly.

Blueprint hooks follow the convention `BP_*` (`BP_Damaged`, `BP_UnitSelected`, etc.) and are declared as `BlueprintImplementableEvent`.

### Input system

All input uses **Enhanced Input** (`UInputAction` / `UInputMappingContext`). Input actions are
`EditAnywhere` properties on the C++ classes — the actual assets are assigned in the
Blueprint subclass.

**Keybinds are player-configurable by design**, which constrains how every binding is built:
a control wired outside Enhanced Input can never be surfaced in a settings screen. **Before
adding any player-facing key or button, read `game-systems`' `input-and-keybinds.md`** — it
holds the current bindings, the keys reserved for systems not built yet, the wiring rule, and
the add-a-binding checklist.

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

`Content/` deliberately does **not** mirror the C++ module layout — a Blueprint can stay at
its current path after its parent class moves modules.

## Testing

There's an automated test suite (in-module automation tests, runnable in-editor or headless).
**Read `game-systems`' `testing.md` when finishing work** — it holds the run commands, the
conventions for writing a test, and the standing rule for when finished work should add one
(counted returns, all-or-nothing operations, state machines, money/weight arithmetic, and
bugs just fixed).

## MCP servers

`unreal-mcp` is the only configured server (`.mcp.json`) — an HTTP server hosted by the editor
at `http://127.0.0.1:8000/mcp`, so it requires the editor running. Reach for it for wiring,
configuration, and content-only work, not gameplay logic (see **Development approach: C++
first**). Use the `mcp-workflow` skill before driving it — it has the discovery workflow,
token-discipline habits, and the known tool bugs/caveats.

## Your Human Partner

His name is Jim and he's new to Unreal and C++ programming but came from a PHP background.  Avoid Jargon heavy reponses until he learns the terms and concepts.


## Roadmaps
Agents layout implementation plans via *-roadmap.md files, which live in `Docs/roadmaps/`.  
These describe related programming work to be done in Unreal and split along boundaries call slices.
When writing roadmaps and organizing work, strive to inject as few sclices as possible.

Roadmaps are **temporary working documents** — read one only when implementing a slice from it
or when Jim references it, not as general background. The permanent record of how a system
works belongs in the `game-systems` skill; when a slice ships, write what it built into the
matching topic there and trim it out of the roadmap.

### Genuine reasons to slice

1. A human has to look at something. PIE feel, layout, whether a number reads right. 
2. You have to do something in a GUI the agent can't drive — hand-wiring an asset, a Blueprint change MCP does badly.
3. Context genuinely runs out. Build errors and log output are expensive. This is a budget limit, not a design principle — and with a 1M-context session it's a much higher ceiling than these roadmaps assume.

### Reasons that look real but aren't

1. "It needs a cold build." The agent can close the editor, build, and reopen — your CLAUDE.md already permits it during an approved plan. Cold builds are slow, not blocking.
2. "It's a different module / different file / different system." If it's the same pattern applied to new code, that's typing, not a decision.
3. "Keeps the commit tidy." One session can make five commits.
4. "We should check it works before building on it." Only if you have to check. If the check is Automation RunTests Smores and reading a number, the agent does that itself and carries on.