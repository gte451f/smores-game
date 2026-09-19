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
- **Clearing a `UInputMappingContext`'s mappings silently does nothing.**
  `ObjectTools.set_properties(IMC, "{\"Mappings\": []}")` returns `true` and writes nothing —
  the duplicate keeps every mapping it inherited. Discovered while trying to make an empty IMC
  by duplicating an existing one; had it gone unnoticed, the copy would have re-fired every
  gameplay action at its own (higher) priority. **Don't build an IMC by duplicate-and-clear** —
  have the user create a blank one in the editor (right-click → Input → Input Mapping Context).
  And never confirm an IMC's contents with `get_properties`, which reports `[]` either way;
  grep the saved `.uasset` binary for `IA_` names instead:
  `python -c "import re,io;print(sorted(set(re.findall(rb'IA_[A-Za-z_]+', io.open('<asset>.uasset','rb').read()))))"`.
- **A material override silently won't stick to a mesh component with no mesh.** Setting
  `OverrideMaterials[0]` on a `UStaticMeshComponent` template whose `StaticMesh` is null returns
  `true` and writes nothing — a component with no mesh reports zero material slots, so
  `UMeshComponent` drops the entry. Same silent-write shape as the IMC bullet above, and engine
  behavior rather than an MCP bug. **Give the component template a default `StaticMesh` first**,
  then set the override; both then survive `compile_blueprint` and the save. Hit while making
  `BP_WorldItem` render black placeholders, where `AWorldItem::RefreshMesh` overwrites the mesh at
  runtime anyway — so the default mesh is pure slot-scaffolding whose value is never used, and
  reads as removable dead weight to anyone tidying up later. Note it wherever it's set.
- **When checking `__ExternalActors__` for stale overrides, grep the referenced *asset* name, not
  the property name.** Property names like `OverrideMaterials` sit in the serialized name table of
  essentially every static-mesh actor in the level whether or not anything overrides them —
  `grep -arl "OverrideMaterials" Content/__ExternalActors__/` returned 114 files against 4 actually
  changed. A per-instance override is only real if the instance serializes the object reference
  (`MI_WorldItem_Black`, `BasicShapes/Sphere`, a `DA_Item_*` name), so grep for that. The
  property-name grep in the stale-override bullets above works for `StartingItems`/`WeightCapacity`
  because those names are *not* carried by unrelated actors — it isn't a general rule, and
  `git status` on `Content/` is the cheaper first check either way.
- **`get_properties` and `set_properties` take different argument shapes.** Read is
  `instance` + `properties` (a list of names). Write is `instance` + **`values`, a JSON
  *string***, and `instance` wants the `{"refPath": "..."}` object form. Passing `properties`
  to a write fails with a schema error naming `values`.
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
- **Two editors open on the project makes every save of a *pre-existing* asset fail, and the
  failure doesn't look like a locking problem.** A second `UnrealEditor.exe` with
  `smores.uproject` open (a stray `.uproject` double-click is the usual cause) holds read locks
  on the `.uasset` files it has loaded, so the MCP-connected editor's `AssetTools.save_assets`
  fails with `MoveFile ... Error Code 32` after an 8-attempt retry loop. The tell is that **new**
  assets save perfectly and **overwrites** don't — which reads as "my write didn't take" rather
  than "something else has the file", especially since the in-memory value reads back correctly
  through `get_properties` the whole time. Check `Get-Process UnrealEditor` before diagnosing a
  save failure any other way; the MCP-connected instance is whichever one the server is bound to,
  not necessarily the newest. Close the duplicate **without saving** (it may hold a stale copy of
  the level), then re-run `save_assets([])` — the first editor's in-memory changes are still
  there, so nothing has to be redone.
- **`AssetTools.save_assets` silently does nothing to an asset that isn't dirty.** Passing explicit
  `asset_paths` for eight clean `DA_Item_*` data assets returned `true` and wrote no bytes - the
  same silent-write family as the IMC-mappings and material-override bullets. There is no force
  flag. To genuinely rewrite an asset (e.g. so it stops serializing a property name that only a
  `CoreRedirect` is resolving), **dirty it first**: `ObjectTools.set_properties` writing a field
  back to its own value marks the package dirty, and `save_assets([])` (save-all-dirty) then
  flushes it. Read the current value first and write exactly that - a value you guessed wrong will
  pass every content sweep. Confirm with a binary grep for the property name, never the return
  value.
- **`ObjectTools.set_properties` on an array property grows it by at most one element per call.**
  Writing a 4-element `TArray<FInventoryItem>` onto a template holding 1 element produced 2
  elements, then 3, then 4 over successive *identical* calls, returning `true` every time. It is
  not a replace, it is a merge capped at `current_length + 1` — so the first call reads exactly
  like one of the silent-write bugs above, and giving up after it is the wrong move. **Read the
  array back and repeat the same write until the length matches.** Seen while putting modifiers
  into `BP_Trader`'s `StartingStock` and `BP_Chest`'s `StartingItems`.
- **`reset_properties` walks past a compiled Blueprint default on an SCS *component* template
  too, not just on a placed actor.** It emptied `BP_Trader`'s `StartingStock` and returned
  `true`. Note this narrows the "a CDO write followed by `compile_blueprint` *is* durable" bullet
  above: the compile does make the value serialize and survive, but it does **not** make
  `reset_properties` fall back to it on a component template the way it does for an actor CDO.
  Recovery is the repeat-write loop in the bullet above, not a reset.
- **A Blueprint-added component's class defaults are not on the actor CDO.** `StartingStock`
  lives on the Trader component, whose template object is
  `/Game/.../BP_Trader.BP_Trader_C:Trader_GEN_VARIABLE` — a `get_properties` against the actor
  CDO fails with "the following properties could not be read", which reads as a stale build
  rather than as looking in the wrong place. The `:<ComponentName>_GEN_VARIABLE` suffix is where
  the write has to go.
- **`SceneTools.save_actor` is broken for World Partition external actors.** It builds a
  `/Game/__ExternalActors__/...` path that doesn't resolve and raises "Asset does not
  exist". Use `AssetTools.save_assets` with an empty list (save-all-dirty) instead — that does
  flush external-actor packages to disk. The argument is named **`asset_paths`**, so the call is
  `{"asset_paths": []}`; `{"assets": []}` fails with a schema error. Note that
  `CompileWidgetBlueprint`'s own tool description tells you to follow up with
  `AssetTools.save_asset` (singular), which does not exist — the tool is `save_assets`.
- **`ProgrammaticToolset.execute_tool_script` does not roll back a partially-run script.** A
  script that throws part-way through leaves everything it already did in place, and the tool
  returns *only* a traceback with no output — which reads exactly like "nothing happened". Hit
  while adding widgets to a WBP: the script added all nine, then threw on the result-formatting
  line at the very end, and a blind re-run would have added a second set named `SortFilterBox_1`,
  `SortWeightButton_1`, … whose `_1` names silently fail to `BindWidget`. **Write batch scripts
  idempotent** — read current state first (`GetWidgets`, `get_properties`) and skip what's
  already there — rather than assuming a failed script is a no-op. Check the real state before
  re-running one.
  - The proximate cause is worth knowing on its own: **the script sandbox's dict is a
    `_StrictDict` that rejects `.get(key, default)`**, raising
    `TypeError: _StrictDict.get() does not support a default value. Use direct key access []
    instead.` Use `x["key"]` with an `if "key" in x` guard; `.get()` with a default will throw at
    whatever point in the script it's reached.
- **`UMGToolSet.GetWidgets` lists *unbound* `BindWidgetOptional` properties as placeholder rows**
  with `widget: "None"` and `bInherited: true`, before any matching widget exists in the tree.
  That's a cheap, positive confirmation that the C++ binding compiled and the editor has picked
  up the new build — and after the widget is added the same row flips to a real refPath, which
  confirms the name matched. Useful in both directions: a name that never appears at all means
  the C++ property isn't there (stale build), while a name stuck on `None` after an edit means
  the widget you added is named something else.
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
- **The `BindWidgetOptional` placeholder row only exists on a *compiled* Widget Blueprint.** On a
  freshly created, never-compiled WBP, `GetWidgets` returns `widgetCount: 0` with no `widget:
  "None"` rows at all — so on a new asset the absence of a name proves nothing, and the
  "placeholder row = my cold build landed" check below applies only to blueprints that have been
  compiled at least once. Add the widget and confirm the row flips to a real refPath instead.
- **`UMGToolSet.CreateWidgetBlueprint` takes a native parent class directly**, including a
  `UCLASS(abstract)` one — `parentClass: {"refPath": "/Script/SmoresUI.RefusalWidget"}` works at
  creation time with no reparent step. Note the object path drops the `U` prefix
  (`RefusalWidget`, not `URefusalWidget`); the `U` form is the C++ name, not the path.
- **`UCanvasPanelSlot` alignment is NOT a top-level property.** `bAutoSize` is, but alignment
  lives at **`LayoutData.alignment`**, alongside `anchors` and `offsets`. The natural-looking
  `set_properties(slot, {"Alignment": {...}})` returns `true` and writes nothing — the same
  silent-write family as the IMC-mappings and material-override bullets above. Correct shape:
  `{"bAutoSize": true, "LayoutData": {"anchors": {...}, "offsets": {...}, "alignment": {"x": 0, "y": 0}}}`.
- **`get_properties`' output shape is directly reusable as `set_properties`' input shape** for
  widget style structs — read the whole struct, change one field, post it back verbatim.
  `FSlateFontInfo`, `FSlateColor`, `FLinearColor`, `FVector2D` and `FAnchorData` all round-trip
  cleanly, with null object refs as the bare string `"None"`. **Prefer read-modify-write of the
  full nested struct** over guessing at a partial one, which is where the silent writes come from.
- **`UMGToolSet.ToggleWidgetAsVariable` returns `null`**, so success is indistinguishable from
  failure — the same problem as `BlueprintTools.compile_blueprint`, and notable because
  `CompileWidgetBlueprint` in that *same* toolset does return a real bool. Confirm the flag
  through `GetWidgets`' `bIsVariable`, never the return value.
- **`AddWidget` honours `widgetDisplayName` exactly** when nothing collides — no `_0` suffix.
  That's what makes name-based `BindWidget` wiring reliable, and it's the flip side of the
  `_1`-suffix hazard when re-running a partially-failed batch script.
- **Don't debug a widget from the designer preview.** UMG draws an empty `UTextBlock` as the
  placeholder string "Text Block", and draws `Collapsed` widgets anyway so they stay selectable.
  An authored-empty, authored-collapsed block therefore looks exactly like one left at its
  default. Confirm with `get_properties`, not with your eyes.
- **`CaptureEditorImage` does not return an image.** It returns ~500 KB of base64 that overflows
  the token limit and lands in a `.txt` as plain text. Recoverable and worth it for UMG layout
  work: regex the `"data"` field out, base64-decode to a `.png` in the scratchpad, then `Read`
  that. Two limits — it captures the **whole desktop**, so panel text is unreadable at the
  delivered resolution, and **PIL is not installed**, so it can't be cropped or upscaled. A
  sanity check only.
- **`bIsVariable` is NOT what makes a `BindWidget` resolve — the name is.** `GetWidgets` reports
  `bIsVariable: false` on widgets that are correctly bound to a C++ `BindWidget`/
  `BindWidgetOptional` property (`GoldText` in `UI_Strategy` is the confirmed case, reporting
  `bIsVariable: false` and `bInherited: true` while working perfectly). UE forces the variable on
  for bound properties at compile time regardless of the stored flag. Setting it true is harmless,
  but **if a binding fails, `bIsVariable` is the wrong thing to chase** — check the name's exact
  spelling and case first.
- **`ObjectTools.get_properties` double-encodes its result.** `returnValue` comes back as a JSON
  *string* rather than a nested object, so it needs parsing a second time before any field can be
  read. Same shape for widget properties as for actor ones.
- **`GetWidgets` lists children in designer order, and `<Panel>Slot_N` suffixes are object names,
  not indices.** Three `UOverlay` children came back as slots `_2`, `_3`, `_0` in that order.
  Don't read the suffix as a z-order or a child index — the list order is the answer.
- **`UMGToolSet.GetWidgetDescription` mis-reports `bInherited`.** It returns `false` for
  widgets that `GetWidgets` correctly reports as `true` (i.e. `BindWidget`-bound), including
  untouched ones. Never use it to judge whether a `BindWidget` binding survived an edit — use
  `GetWidgets`.
- **`BlueprintTools.compile_blueprint` returns `null`**, so success is indistinguishable from
  failure. `UMGToolSet.CompileWidgetBlueprint` returns a real bool and surfaces
  `BindWidget`/type errors — prefer it for widget Blueprints, and confirm in the log
  (`LogBlueprint: Compiling Blueprint ...`).
- **A failed existence probe poisons the asset name for the rest of the session.** Calling
  `UMGToolSet.GetWidgets` (or anything else that resolves a soft path) on a WBP that doesn't exist
  yet leaves an empty in-memory `UPackage` behind — the log says
  `LogUObjectGlobals: Failed to find object ...` — and every later `CreateWidgetBlueprint` at that
  exact path then fails with **"The path already exists"**, while `AssetTools.exists` returns
  `false` and `ListWidgetBlueprints` doesn't list it. It reads as a phantom asset, and the obvious
  next move (check whether it exists) actively confirms the wrong answer. **Probe with
  `AssetTools.exists`, never with `GetWidgets` or a soft-path resolve**, which matters precisely
  because an idempotent batch script (see the `execute_tool_script` bullet) *wants* to check first.
  Recovery without restarting the editor: create the asset under a throwaway name and
  `AssetTools.move` it onto the wanted path — that works and leaves no redirector, but verify no
  file remains at the throwaway path afterwards.
- **When C++ removes a `BlueprintPure` a widget consumed, check the WBP's EventGraph, not just its
  widget tree.** Deleting the widgets that displayed the value is not enough if the value was
  pushed by an `Event Tick` → `SetText` → `<the removed function>` chain rather than by a property
  binding — `CompileWidgetBlueprint` then fails with *"Could not find a function named X"* and
  names the function but not where it is used. `BlueprintTools.read_graph_dsl` on the EventGraph is
  the way to find it; `delete_node` on each node in the chain is the fix. Hit while removing
  `UStrategyUI::GetSelectionTargetLabel`, where the readout looked like a property binding and
  wasn't. **The cheap advance check is a binary grep of `Content/` for the function name** before
  touching the C++ — it finds the asset regardless of how the value was wired.
- **`UMGToolSet.ReplaceWidgetWithTemplate` is the right tool for a panel-class swap.** It
  preserves the widget's name, its existing parent slot object *and* that slot's settings, and
  the C++ `BindWidget` binding — and it reports which properties had no counterpart on the new
  class. Much safer than delete-and-re-add, which breaks the name binding.
- **Create/modify operations that touch project assets: get user confirmation first.**
