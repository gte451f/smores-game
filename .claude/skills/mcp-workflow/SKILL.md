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
  - **Verify a placed instance's inherited value only after save + level reload.** A newly
    added C++ `UPROPERTY` on a *native default subobject* (e.g. `Inventory` on
    `AStrategyUnit`) reads back as the **native C++ default** on every placed instance until
    the owning Blueprint's `.uasset` is actually written to disk — a compile alone isn't
    enough. This looks exactly like "the Blueprint default didn't take", and the obvious fix
    (`reset_properties` on the instance) is the worst possible move: per the bullet above it
    resolves to the native default anyway, *and* it flags that value as an explicit
    per-instance override on every actor it touches, which the next `save_assets` then
    persists to disk. Correct order is CDO write → `compile_blueprint` → `AssetTools.save_assets([])`
    → reload the level → *then* read the instance back. If overrides were already written,
    clean them by setting the intended value explicitly per instance and re-saving, then
    confirm with `grep -arl "<PropertyName>" Content/__ExternalActors__/` returning nothing.
  - **A CDO write followed by `compile_blueprint` *is* durable.** `BlueprintTools` exposes
    no dedicated class-default setter, so the working recipe for a class default is
    `get_default_object` → `ObjectTools.set_properties` → `BlueprintTools.compile_blueprint`.
    The compile promotes the value into a real Blueprint default: it serializes into the
    saved `.uasset` (verifiable by grepping the binary for the referenced asset's name), and
    `reset_properties` on a placed instance then correctly falls back to *it* rather than
    walking past to the native C++ default. The "raw CDO write" hazards above apply to a
    `set_properties` that is never followed by a compile.
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
- **`SceneTools.save_actor` is broken for World Partition external actors.** It builds a
  `/Game/__ExternalActors__/...` path that doesn't resolve and raises "Asset does not
  exist". Use `AssetTools.save_assets([])` (save-all-dirty) instead — that does flush
  external-actor packages to disk.
- **A reshaped `USTRUCT` leaves stale per-instance overrides on placed actors.** After any
  C++ change to a struct used in an `EditAnywhere` array, placed actors that carry an
  override of that array keep it — the removed fields simply don't deserialize, so the
  override survives as garbage (null references, or an *empty* array that silently shadows
  the Blueprint's new defaults). These don't announce themselves: an actor showing nothing
  looks the same as an actor that legitimately holds nothing. Find them by grepping
  `Content/__ExternalActors__/` for the property name, then either re-author the override
  or `ObjectTools.reset_properties` it away — and verify with `get_properties` after.
- **Changing a Blueprint class default writes a stale per-instance override onto every placed
  actor that predates the property.** Not just module moves and relocated properties — a plain
  CDO edit does it. Setting `BP_Chest`'s new `WeightCapacity` 30 → 0 and compiling re-instanced
  both chests placed in `LVL_Strategy`, and re-instancing carried their *old* inherited value
  (30) forward as a genuine override that silently shadowed the new class default. It looks
  exactly like "the class default didn't take". Detect it with
  `grep -arl "<PropertyName>" Content/__ExternalActors__/` after the save; fix it by setting the
  intended value **explicitly per instance** and re-saving, which makes the delta match the CDO
  so nothing serializes — then re-grep to confirm it comes back empty. Verify placed instances
  only after a level reload, not straight after the compile.
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
- **CoreRedirects do not chain.** `A→B` plus `B→C` leaves an asset that stores `A`
  resolving to `B` and stopping there — it loads nothing, and the Blueprint ends up with no
  generated class and no CDO, unrepairable in place (`reparent` needs a class to reparent
  *from*). Always redirect from the name actually serialized in the `.uasset` straight to the
  final name. Check what's serialized rather than guessing — the module recorded in an asset
  can predate a module split by several refactors:
  `python -c "import re,io;print(sorted(set(re.findall(rb'/Script/[A-Za-z0-9_.]+', io.open('<asset>.uasset','rb').read()))))"`.
  This bites hardest on a class renamed *after* it was already moved between modules.
- **`ObjectTools.get_properties` takes `instance` + `properties`**, not `object` /
  `property_names`, and this build has **no `only_modified` argument at all** — the
  `only_modified: false` advice above applies to `bp_inspect`, not to `get_properties`.
- **UMG refPaths need the full `Package.Asset` form** (`/Game/.../WBP_X.WBP_X`); the bare
  package path fails with "is not a valid object path for property 'WidgetBlueprint'".
- **`UMGToolSet.GetWidgetDescription` mis-reports `bInherited`.** It returns `false` for
  widgets that `GetWidgets` correctly reports as `true` (i.e. `BindWidget`-bound), including
  untouched ones. Never use it to judge whether a `BindWidget` binding survived an edit — use
  `GetWidgets`.
- **`BlueprintTools.compile_blueprint` returns `null`**, so success is indistinguishable from
  failure. `UMGToolSet.CompileWidgetBlueprint` returns a real bool and surfaces
  `BindWidget`/type errors — prefer it for widget Blueprints, and confirm in the log
  (`LogBlueprint: Compiling Blueprint ...`).
- **`UMGToolSet.ReplaceWidgetWithTemplate` is the right tool for a panel-class swap.** It
  preserves the widget's name, its existing parent slot object *and* that slot's settings, and
  the C++ `BindWidget` binding — and it reports which properties had no counterpart on the new
  class. Much safer than delete-and-re-add, which breaks the name binding.
- **Create/modify operations that touch project assets: get user confirmation first.**
