---
name: mcp-workflow
description: smores-specific unreal-mcp workflow, token-discipline habits, and known tool bugs/caveats. Use when driving the Unreal Editor via MCP (Blueprint wiring, asset/property reads-writes, plugin toggling) — not for pure C++ work, and not for generic Unreal-MCP usage (see the unreal-engine-skills-for-claude-code:unreal-mcp skill for that). This one captures this project's own tribal knowledge: tool bugs discovered the hard way, discovery workflow, and context-budget habits.
---

# MCP Workflow (smores)

`unreal-mcp` is the only configured MCP server (`.mcp.json`): HTTP, hosted by the editor's
`ModelContextProtocol` plugin at `http://127.0.0.1:8000/mcp`. Requires the editor running.
Reach for it for wiring, configuration, and content-only work — not gameplay logic (see
CLAUDE.md's "Development approach: C++ first").

## Discovery workflow (do this — don't assume)

`unreal-mcp` exposes only three meta-tools; the real tools are discovered on demand, so
**no full tool list is ever in context**:

1. `list_toolsets` — ~50 toolsets registered via the `AllToolsets` plugin. Names come in two
   styles, both used verbatim: `EditorToolset.EditorAppToolset` and
   `editor_toolset.toolsets.blueprint.BlueprintTools`.
2. `describe_toolset(<name>)` — tool names + schemas for one toolset. Some responses are
   large; request only the toolset you need. On a big toolset (BlueprintTools, UMGToolSet)
   the result auto-persists to a file and stays *out* of context — read it back with
   `jq`/grep for the tools you need.
3. `call_tool(tool_name, toolset_name, arguments)` — invoke.

## What's available (high level)

- **Blueprints**: `editor_toolset.toolsets.blueprint.BlueprintTools` — create
  assets/graphs/variables/functions/nodes, wire pins, compile. Prefer `write_graph_dsl`
  (read `get_graph_dsl_docs` first) over node-by-node.
- **Objects/props**: `editor_toolset.toolsets.object.ObjectTools` — read/write properties
  on any object or CDO. Other toolsets depend on it for property access.
- **Also registered**: scene/actor/asset/material tools, UMG, Niagara, Control Rig +
  Sequencer, GAS, GameplayTags, PCG, Physics, and `ProgrammaticToolset` (batch calls via a
  sandboxed Python script). Discover schemas on demand.

## Token discipline (MCP results persist for the whole session)

Every `call_tool` result stays in context until the session ends. On the two tasks that
built these systems, `unreal-mcp` was ~40%+ of total token usage. Keep it small:

- **Run MCP-heavy editor wiring in a `fork` / subagent.** A multi-step asset build is
  ~20-30 calls whose payloads are pure noise once the work is done. Let the fork do the
  wiring and report back "done + any manual steps"; the main context never sees the calls.
- **Do the C++ first, then wire in one pass.** Workflow: close editor → cold build →
  reopen → do all wiring → `/compact`. Don't interleave.
- **Skip `list_properties`.** It dumps the entire inherited property tree with every
  nested sub-schema (a `TextBlock` is ~6 KB). Call `get_properties`/`set_properties` with
  the field names you already know; only fall back to discovery on a failed set.
- **Don't `get_properties` just to learn field names** — `set_properties` returns a bool,
  so set and check.
- **Batch multi-step builds through `ProgrammaticToolset.execute_tool_script`** (one
  round-trip instead of a dozen `AddWidget`/`set_properties`/compile calls).

## Known tool bugs and caveats

- **CDO writes don't set the Blueprint override flag.** `ObjectTools`/`bp_class` CDO
  writes (e.g. `set_cdo_property`) modify the Class Default Object's value directly but
  not UE's "property customized in this Blueprint" marker. Consequences: `bp_inspect` with
  `only_modified: true` (the default) won't show the property even though it has a value;
  the editor's Class Defaults panel may show the old value; the value may not survive an
  editor restart depending on serialization. Prefer `BlueprintTools` (sets the override
  flag + compiles) over raw CDO writes. If you do write a CDO property directly, ask the
  user to open the Blueprint, verify/fix the value in Class Defaults, and save (Ctrl+S) —
  and use `only_modified: false` when inspecting a property you know was set this way.
  Two further consequences discovered the hard way, both worse than "shows a stale value":
  - `ObjectTools.reset_properties` on a level-placed *instance* of that Blueprint doesn't
    fall back to the Blueprint's (unflagged) CDO value — it walks past it to the *native
    C++ class default* instead, and still returns `true`. If the native default is empty
    and the Blueprint default isn't, this silently reintroduces the empty value while
    looking like a successful reset. Don't trust `reset_properties` to recover a value that
    was itself set via a raw CDO write; verify the result with `get_properties` before
    moving on.
  - A genuine Blueprint recompile (a real edit through the editor UI, not another raw MCP
    write) re-instances every placed actor of that class in the open level from the fresh
    CDO, and **discards any per-instance override that was itself set via a raw MCP write**
    (same missing-flag problem, just on the instance instead of the CDO). This is actually
    a reliable way to clean up unflagged per-instance overrides — make one real edit to the
    Blueprint's class defaults (even a no-op re-drag of the same asset) and let the
    resulting compile re-instance everything — but it means a raw per-instance write should
    never be treated as durable; a routine Blueprint compile can wipe it without warning.
- **`UInputMappingContext` key mappings can't be round-tripped.**
  `ObjectTools.get_properties(IMC, ["mappings"])` returns `[]` — the real data is
  `defaultKeyMappings.mappings[]`. Read/write formats disagree: the reader flattens `key`
  to a bare string while the write schema wants `{"keyName": "..."}`, and mappings hold
  refPaths to IMC-owned instanced modifier/trigger subobjects. Rewriting the array via
  `set_properties` risks corrupting working input with no safe partial-append path.
  Creating the `UInputAction` asset itself is fine — duplicate an existing IA of the same
  `ValueType` (`IA_Strategy_CyclePawn` is `Boolean`) — but hand the IMC key-binding step to
  the user. InputAction `UPROPERTY` asset paths use `/Game/Path/IA_Name.IA_Name` (no `_C` —
  that suffix is only for Blueprint-generated classes).
- **No MCP compile/build trigger exists.** Checked exhaustively across all ~50 toolsets
  (Blueprint, Object, Material, Scene, Actor, Asset, Sequencer, Niagara, PCG, GAS,
  Automation Tests, Config Settings, Plugins, Logs, etc.) — none expose compile/build/Live
  Coding. After any C++ change, ask the user to trigger it manually in the editor
  (Ctrl+Alt+F11, or the Compile toolbar button) rather than hunting for an MCP path, then
  resume once they confirm. Verify the compile landed with `get_properties` on an affected
  Blueprint/CDO for the new field name (fails with "could not be read" if the reload
  hasn't happened yet).
- **New `UCLASS`/`USTRUCT`/`UENUM` (or any new `UPROPERTY`) needs a cold build with the
  editor closed** before MCP can reference the new types — Live Coding won't register
  them. See CLAUDE.md's "Building".
- **`PluginToolset.SetPluginEnabled` doesn't persist.** Returns success but writes nothing
  to disk and doesn't change `IsEnabled`'s result, even before restarting (confirmed by
  calling it and immediately re-checking both `IsEnabled` and the `.uproject` file). To
  actually toggle a plugin: close the editor, add/edit an explicit
  `{"Name": ..., "Enabled": false}` entry in `smores.uproject`'s `Plugins` array directly
  (same shape the editor's Plugin Browser writes), then reopen. Check
  `GetPluginDependents`/`GetPluginDependencies` first for non-optional dependents before
  disabling anything.
- **Create/modify operations that touch project assets: get user confirmation first.**
