# Dialog Roadmap — Barks, Conversations and the Content Loader

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how dialog works will live in the `game-systems`
skill — a new `dialog.md` topic created by Slice 1 and extended by Slice 2 — and this file should
be trimmed as each slice ships.

**Don't start before `game-data-roadmap.md` is complete.** That's Jim's sequencing call, and it
matters for more than scheduling. Dialog reads character records (role, faction, definition id),
faction standing, and the `Tags` that game-data Slice 5 adds to `USmoresDefinition`. All of those
are the game-data layer's to build.

Dialog is a deliberate **area of improvement over Kenshi**, which is the reference game in most
other respects. Barks matter, but so do NPC and recruit backstories, faction dealings, in-squad
banter, and (later) quest conversations. This roadmap builds the machinery for all of those and a
small amount of placeholder content to prove each one. It does not build quests.

## The Shape, in One Paragraph

All dialog is **written ahead of time** by people. Nothing is generated at runtime. It lives in
**plain text files, not `.uasset`s**, which a **loader** reads when the game starts. The base game's
own dialog is loaded exactly like a mod: **the base game is mod #0**, the way RimWorld's Core
folder and Factorio's `base` mod work, so there is only one loading path and modders get
everything our writers get. Two kinds of dialog sit on top of the loader:

- **Barks** are one-way lines picked from a table of rules. They use Valve's "most specific match
  wins" approach from Left 4 Dead (Elan Ruskin's GDC 2012 "AI-driven Dynamic Dialog" talk).
- **Conversations** are authored scripts, written in Ink or Yarn (not yet chosen, see below).
  Rules decide *which* conversation happens, the way Hades does. The script decides *how* it plays
  out.

Both read the same shared pool of **facts** through one small condition language. Anything
written in dialog changes the world through a fixed list of **effects**.

## Where This Sits Against the Game-Data Layer

Dialog follows the three-layer model in `game-systems`' `game-data.md` exactly. It is worth being
explicit, because the loaded files are *not* `.uasset`s and it would be easy to treat them as
something new.

| Dialog piece | Layer | Where it lives |
|---|---|---|
| Loaded barks and conversations | **Definition.** Authored, identical on every machine, never written to at runtime | Held by a `UGameInstanceSubsystem`, loaded from text files |
| What a squad has said, learned or unlocked | **Record.** Saved, per player | A component on `AStrategyPlayerState` |
| A bark on screen, an open conversation | Transient | Neither; rebuilt, never saved |

**Dialog state is squad-scoped.** Jim settled this: a conversation one player's squad has had, or a
flag it set, belongs to that squad, not the world. Each player controls one squad, so "per squad"
means "per `PlayerState`", which is the `UPlayerStandingComponent` precedent. World-level dialog
flags (anything every squad would see change) are deliberately not built. The first real case is a
quest item that only one squad can take, which is the quests roadmap's problem.

## Module Placement

**A new module, `SmoresDialog`**, sitting between `SmoresUI` and the modules it reads:

```
smores → SmoresUI → SmoresDialog → {SmoresCharacters, SmoresEconomy} → {SmoresItems, SmoresCombat} → SmoresCore
```

This is a real split, not a speculative one (per `unreal-module-organization.md`'s "When to
Actually Split"), for two reasons:

- **A third-party runtime arrives in Slice 2** (the Ink or Yarn plugin). Confining that dependency
  to one module means nothing else in the game links against it.
- **Dialog reads nearly everything below it** (records, standing, wallets, inventory) and almost
  nothing reads dialog (only `SmoresUI`, to draw the window, and `smores`, to open it). That puts
  it near the top of the stack, like `SmoresAI` in the target map.

Add `SmoresDialog` to the target module map in `unreal-module-organization.md` when Slice 1 creates
it. Follow that topic's five-step registration checklist. Effects that need the player controller
(opening trade) go through a narrow interface declared in `SmoresDialog` and implemented by
`AStrategyPlayerController`. That is the `IStrategySelectionHost` pattern, so nothing starts
depending on `smores`.

## The Loader

Runs once at game start, and again on demand (hot reload). It is the same code in the editor, in
a packaged build and on a dedicated server.

### Folders and packages

A **package** is one folder with a manifest. The base game is package `core`. Every mod is another
package.

```
Content/Dialog/core/            the base game, staged into the build as-is
    mod.json
    barks/*.csv
    conversations/*             (Slice 2, in the chosen language)
    localization/<culture>/*.csv
Mods/<ModId>/                   one folder per mod, next to the installed game
    mod.json
    ...the same shape
```

- **Raw text files aren't packaged by default.** Unreal only cooks `.uasset`s, so `Content/Dialog`
  needs adding to the project's list of directories staged as-is
  (`DirectoriesToAlwaysStageAsUFS`). Check a packaged build actually contains it. This is the
  step that fails silently, like the Asset Manager scan line in `game-data.md`.
- **`mod.json`** carries: `Id` (the package's prefix), display name, version, the game version it
  targets, and `Requires` (other package ids). Its exact field list is the slice's call.

### Steps

1. **Discover.** `core` first, then each folder under `Mods/`. A folder with no readable manifest
   is skipped with an error.
2. **Order.** `core`, then mods sorted so everything a mod `Requires` loads before it. A missing
   requirement or a cycle skips the mods involved, with an error naming them.
3. **Parse** each file into in-memory structs.
4. **Validate** everything against the fact and effect lists and against every other loaded
   package. Each problem is reported with package, file and line. **A broken entry is skipped,
   not fatal**, and a broken mod never stops the game starting. The report goes to the log and a
   one-line summary per package ("mod `lanterns`: loaded, 3 errors").
5. **Index** for fast queries: barks by event, conversations by what they attach to.
6. **Register text** as runtime string tables, one per package per culture (below).

### Ids

- **An id is written local to its package and qualified by the loader.** A writer types
  `guard_greet`; the loader stores `lanterns.guard_greet`. A reference into another package is
  written fully qualified. This makes collisions between mods impossible by construction rather
  than by convention.
- **Mods only add in v1.** Nothing can delete or replace a `core` entry. That turns out to be
  enough to change what players see: a mod's more-specific bark wins the match, and a mod's
  higher-priority conversation wins selection, without touching our files. Explicit replacement
  can come later if modders genuinely need it (see Open Questions).
- References to game content (a faction, a character definition, an item) use **definition ids**,
  the same ids records and saves use (`game-data.md`'s id rule). A text file can't hold an asset
  pointer. An id that doesn't resolve is a **validation warning, not an error**, the stance
  `UPlayerStandingComponent` and `CreateRecord` already take for stripped mods. The `core` content
  sweep (see Tests) turns it into an error for our own files.

### Localization

Every player-facing line is `FText` registered through Unreal's runtime string table registry
(`FStringTableRegistry`), keyed by the qualified line id. Source-language text sits in the dialog
files. Translations sit in `localization/<culture>/`, keyed by the same ids, and each mod ships
its own. **This is the least-trodden part of the design.** Unreal's normal translation pipeline
gathers text from assets and code, not from files loaded at runtime. So Slice 1 proves it end to
end: switch culture in PIE and watch a bark change language. It doesn't wait for the full UI.

### Hot reload

`SmoresReloadDialog` reruns the whole load and prints the validation summary. A writer edits a
file, types the command, and sees the change without restarting. Active conversations are ended
first rather than migrated.

### Multiplayer

- **Every machine loads the same files.** The server needs them to decide what happens. Clients
  need them to display it. **Over the network, dialog travels as ids, never text.** A client
  resolves `lanterns.guard_greet` from its own loaded, translated copy, so each player reads their
  own language.
- **Mismatched dialog between host and client is detectable but not enforced here.** The loader
  exposes the list of loaded packages and versions. Refusing a join over a mismatch is the session
  roadmap's job (`SmoresOnlineSession`), because no join flow exists yet.
- **Mods are data, not code.** A mod can only use the facts and effects the game exposes, which is
  the stance `unreal-module-organization.md` already takes on content-only modding.

## Facts, Conditions and Effects

### Facts

A **fact** is a named question the game can answer at the moment of asking, like
`Speaker.Faction`, `Listener.Role` or `StandingWithSpeaker`. They are registered in C++ with a
name, a value type (number, name, or yes/no) and a function that answers it for a context. The
context is:

- **Speaker**: the NPC talking.
- **Listener**: the squad member being spoken to, or the other participant in banter.
- **Player**: the `PlayerState` whose squad this is.
- **Event**: the bark's trigger, where one exists.

**Only register a fact something can actually answer.** This is the refusal-reason rule from
`refusals-and-feedback.md` applied to dialog. No `Speaker.Lineage` until lineage exists, no skill
facts until skills exist. The registered list *is* the writer's and modder's reference, so a dead
entry is a promise the game doesn't keep.

### The condition language

One small language, used everywhere selection happens (bark rules, conversation requirements, topic
availability). It is a list of clauses, all of which must hold:

```
Speaker.Role == guard; StandingWithSpeaker <= -20; Event.Victim.Faction == Speaker.Faction
```

Comparisons, ids, numbers, and `Flag(name)`/`Seen(id)`-style calls. No `or`: write two rules.
That keeps specificity countable (below). Parsed and checked at load time, so a typo in a fact
name is a validation error with a line number, not a silent "never matches".

**Our condition language owns selection even inside conversation scripts.** Ink and Yarn both have
their own variables and expressions. Those are used for flow *inside* a conversation, reading
facts through the language's "external function" bridge. But whether a conversation or topic is
*available* is always written in our language. That keeps one grammar for writers to learn, one
validator, and one place a modder looks.

### Effects

A fixed list of named actions a line or choice can trigger, each implemented in C++,
**authority-only**, and validated at load time like facts. Slice 1 needs none (barks change
nothing). Slice 2 registers only what its content uses: `SetFlag`, `ChangeStanding`, `TakeMoney`,
`GiveMoney`, `OpenTrade`.

An effect that could fail (`TakeMoney` with too little in the wallet) is checked *before* the
choice is offered. The choice shows disabled with its reason, the same "a disabled action still
shows, with its reason" rule the target panel follows (`hud-and-panels.md`). The effect itself is
all-or-nothing.

## Implementation Order

Two slices. Each follows the game-data roadmap's protocol:

1. Read this entry, `dialog.md` (once Slice 1 creates it), and the source it names.
2. C++ first. Both slices add `UCLASS`/`USTRUCT` types, so expect cold builds.
3. MCP for widget Blueprints and asset wiring (`mcp-workflow`).
4. Headless suite plus new tests, per `testing.md`.
5. Jim looks at the result in PIE.
6. Commit, move shipped content into `game-systems`, and mark the slice `DONE`.

### Why two slices

Per CLAUDE.md's slicing rules:

- **1 → 2: a decision and a human look.** Slice 2 can't start until Jim has picked Ink or Yarn,
  and Slice 1 deliberately doesn't depend on that choice. Barks use our own format, so the loader
  and the whole bark layer get built and judged while the decision is still being made. Slice 1
  also ends in something Jim has to look at: whether barks read right, and how often they fire.
- Everything inside Slice 2 (window conversations, topics, banter) is the same machinery in two
  playback modes, and ends in one PIE look. Splitting it would be "it's a different system", which
  CLAUDE.md lists as not a reason.

### Before Slice 1: move the design into `game-design`

The design decisions below (written ahead of time, barks plus rule-selected conversations,
squad-scoped state, base-game-as-mod) are design intent, not implementation. They belong in
`game-design`, as an expansion of `ai-and-behavior.md`'s "Barks and Dialogue" section or a new
dialogue topic, before code is written against them. Check whether this has already happened.

### Slice 1 — The loader, facts, and barks

**Builds:**

- The `SmoresDialog` module, registered per the checklist.
- `USmoresDialogSubsystem` (`UGameInstanceSubsystem`): the loader and the loaded library. It
  survives map changes and exists on every machine. It has an in-memory entry point for tests
  (load a package from strings), the same shape as `UWorldFactionComponent::InitializeFromDefinitions`.
- The fact registry, the condition language (parser, validator, evaluator), and the facts the
  bark events below need that current records can answer: role, definition id, faction, name,
  life state, health fraction, `StandingWithSpeaker`, and the event's own payload.
- The bark format. **CSV**, one row per line: id, event, conditions, text, weight, and a
  per-speaker cooldown. It's writer-friendly (edits in a spreadsheet) and diffs line by line in git.
- **Selection: most clauses matched wins.** A tie goes to the least recently used line, then
  weight. Cooldowns are per speaker, so a guard doesn't repeat himself, but two guards can say the
  same thing.
- `UBarkDirectorComponent` on `AStrategyGameState`, server-only. It receives events, picks a
  line, and delivers it to every player with a squad member within hearing range, through
  `Client_NotifyActivity` on the COMMS tab (`hud-and-panels.md`'s "Adding a producer to the
  activity feed"). Cooldown and recency state is transient and never saved.
- **Events, all from signals that already exist**: `Hurt`, `Downed`, `WitnessedDeath` (allies
  nearby), `TradeOpened`, and `NothingToSay`. `NothingToSay` fills the silent branch in
  `AStrategyPlayerController::InteractWithNPC`, whose comment rightly refuses to fake "they have
  nothing to say" with a refusal. A bark authored for that moment is the honest version. If the
  danger-alerts roadmap has shipped, add `EngagementStarted` from its engagement state; if not,
  don't invent one.
- Execs: `SmoresReloadDialog`, `SmoresDialogReport` (packages, counts, errors), and
  `SmoresTestBark <event>`, which fires an event on the targeted NPC and prints which rule won and
  why. That last one answers "why did that line play?", the question that makes rule systems hard
  to live with.
- Content: a `core` bark set for the three existing definitions (Settler, Bandit, Trader), enough
  that each event has a generic line and at least one more specific one. Plus **one example mod**
  under `Mods/example/` that adds a single more-specific bark, which shows mod loading working
  in PIE.
- Localization proof: one bark translated into a second culture (placeholder text is fine),
  visible after a culture switch.

**Tests** (`testing.md`'s standing rule; this is counted and state-machine code):

- The condition language: parse, evaluate, reject unknown facts and bad syntax with a line number.
- Specificity: the more specific rule wins, ties break as specified, cooldowns hold.
- The loader: `Requires` ordering, a missing requirement, a cycle, duplicate ids within a package,
  the same local id in two packages (legal), a broken row skipped while its neighbors load.
- **The `core` content sweep**: the shipped base-game dialog loads with **zero** errors and at
  least one bark per event. It's the dialog equivalent of `EveryAssetIsWellFormed`: count the
  number, not the colour.

**Verification:** Jim plays in PIE and judges whether barks read right, whether they fire too
often, and whether the feed is the right place for them or they also want a floating line over
the speaker (see Open Questions). Both answers get written back into `game-design`.

**Ships into:** a new `game-systems` topic `dialog.md`; pointers from `hud-and-panels.md` (a new
feed producer) and `unreal-module-organization.md` (the new module).

### Decision needed before Slice 2: Ink or Yarn

Jim's to make. These are the criteria that actually matter for *this* design, in order:

1. **Can the runtime play a script loaded from a loose file at runtime?** This one is
   non-negotiable. Some plugins only play scripts imported into a `.uasset` in the editor, which
   defeats the loader. Check the plugin's runtime entry point, not its import workflow.
2. **Does the plugin build and run on UE 5.8, including a server build with no UI?**
3. **Line ids for translation.** Yarn tags every line with an id and generates string tables out of
   the box. Translating Ink usually means tagging lines by hand or with our own tool.
4. **What the writer's tool exports.** Both languages compile to an intermediate file. Mods should
   ship the compiled file, produced by the free writing tool (Inky for Ink, Yarn Spinner's editor
   for Yarn), so modders never need Unreal. The loader rejects a compiled file older than its
   source in editor builds, so a writer can't test stale text.
5. **Writing feel.** Ink is the stronger language for dense branching prose. Yarn reads more like a
   screenplay.

Yarn Spinner 3's storylets overlap with our selection layer. That's fine and not a reason to pick
Yarn: we keep our condition language for selection either way (see "The condition language").

### Slice 2 — Conversations, topics and banter

**Starts with a spike, and stops if it fails.** Build the chosen plugin on 5.8, load one compiled
script from a loose file, and step through it on a server with no UI. If that doesn't work, stop
and bring the options back to Jim. Don't work around it silently.

**Builds:**

- **The conversation format.** A script in the chosen language, plus header metadata read by our
  loader:
  - what it **attaches to**: a definition id, role, faction or tag
  - its **requirements** (our condition language)
  - **priority**
  - **once** (never again for this squad once seen)
  - **kind**: `Greeting`, `Topic` or `Ambient`
  - for `Ambient`, the **participants** it needs
- **Selection, Hades-style.** Of the conversations attached to this NPC whose requirements hold
  and which aren't used up, the highest priority wins. A tie goes to the least recently seen.
- **Topics.** When a greeting conversation ends, the window offers every eligible `Topic`
  attached to this NPC as a choice, plus "Goodbye". Topics are how a mod, and later a quest, adds
  something to *our* NPCs without editing them: attach a topic to `Faction == ironclan` and every
  Ironclan member offers it. The list is built by our code, not inside the script, so topics never
  depend on what the language runtime supports.
- `UDialogMemoryComponent` on `AStrategyPlayerState`: this squad's flags and seen conversations.
  Server-owned, a plain reflected record for the future save system to serialize unchanged. Adds
  the `Flag(name)` and `Seen(id)` facts.
- `UConversationComponent` on `AStrategyPlayerController` (RPCs need a player-owned actor). The
  server runs the script. A client RPC sends the current line and choices **as ids**, with each
  choice's enabled state and reason. A server RPC sends back the pick. The conversation **ends**
  when either side walks out of range, is downed, or enters combat.
- **The world doesn't stop.** Time dilation, pause included, is single-player only
  (`player-experience.md`), so a co-op conversation runs in real time. Whether *single-player*
  pauses during a conversation is an open design question. Until it's answered, it doesn't pause.
- **Several players may talk to the same NPC at once.** Each squad's conversation is its own
  instance. Nothing locks the NPC.
- **The effects**: `SetFlag`, `ChangeStanding` (through `UPlayerStandingComponent`), `TakeMoney`
  / `GiveMoney` (through the wallet), `OpenTrade` (through the controller interface).
- **`InteractWithNPC`'s new order**: hostile → refusal (unchanged) → an eligible conversation →
  open it → else a trader → open trade directly (so a trader whose conversation a stripped mod
  removed still trades) → else the `NothingToSay` bark. The Talk button, `T` and the double-click
  all reach it already.
- **The window**: `UConversationWidget` in `SmoresUI` plus its WBP via MCP. It shows the speaker's
  name and portrait (portrait fallback rules from `hud-and-panels.md`), the current line, and the
  choices, with disabled ones showing their reason. Every line spoken also goes to the COMMS feed,
  which is the transcript the design promises in `player-interface.md`.
- **Ambient playback**, the second mode of the same machinery. An `Ambient` conversation plays
  without a window. Lines go to the feed with a reading delay between them. Used for **in-squad
  banter**: a server-side director tries an eligible ambient conversation after an engagement ends
  and at a slow cooldown while the squad is idle, casting its participants from that player's
  squad members standing near each other. The validator rejects choices inside an ambient script.
- Content, one example of each shape:
  - a `core` trader greeting with a trade choice and a haggling-flavored topic (no real
    haggling; prices stay flat)
  - a faction guard conversation using `ChangeStanding` and a `TakeMoney` bribe
  - a backstory topic on the Trader definition that unlocks once a flag is set, standing in for
    recruit backstories until recruitment exists
  - one two-Settler banter
  - the example mod gains a topic attached to a whole faction

**Tests:**

- Selection: priority, once-only, requirements, the tie-break.
- The topic list is exactly the eligible topics.
- Effects refuse off-authority, and are all-or-nothing (`TakeMoney` with too little leaves the
  wallet untouched, and the choice showed disabled).
- Memory is per player: two players, one sets a flag, the other doesn't see it.
- A conversation ends on range, down and combat.
- Ambient validation.
- The spike's loose-file load, kept as a test.
- The `core` sweep extended to conversations.

**Verification:** Jim plays each shape in PIE. He judges the window's placement and feel, banter
pacing (too chatty or too rare), and whether the "topics after the greeting" flow reads naturally.

**Ships into:** `dialog.md`; `input-and-keybinds.md` if the window adds any key (choices by number
keys go through Enhanced Input, per the keybind rule); `hud-and-panels.md` for the window.

## Explicitly Out of Scope

- **Runtime AI-generated dialog.** Dropped, settled.
- **Quests.** Topics and flags are the hooks a quest system will use. The quest system itself,
  including world-level flags and "only one squad can take this item", is its own roadmap.
  `quests-and-objectives.md` is still a placeholder.
- **Mods adding brand-new NPCs.** This roadmap lets a mod give dialog to any existing character
  definition, role, faction or tag. But a *new* NPC needs a new character definition, and
  definitions are `.uasset`s that a text-only mod can't make. Loading definitions from text is a
  game-data and modding question, and the most important open question here (below).
- **Skill checks and persuasion.** Wait for skills (`characters-and-squads.md` leaves the roster
  undecided). No attribute checks as a stand-in: that would bake resolution math into content.
- **Recruitment through dialog, and wage renegotiation.** Both are dialog-shaped
  (`characters-and-squads.md`), but neither system exists yet. They get effects (`Recruit`,
  `SetWage`) when they do.
- **A `StartFight` / turn-hostile effect.** Hostility is derived, never set
  (`ai-and-behavior.md`'s Stance). Wait for the AI roadmap.
- **Voice, lip-sync, portraits with expressions.** Text only (`localization.md` notes voice would
  reopen localization).
- **The join-time mismatch check.** The loader exposes the package list; the session roadmap
  enforces it.
- **A player-facing mod manager screen.** Log plus summary for now.

## Resolved Design Decisions

Settled during the conversation that produced this file. Don't reopen them without a reason.

- **All dialog is written ahead of time.** No runtime generation.
- **Dialog is plain text loaded at startup, not `.uasset`s, and the base game is loaded as mod
  #0** through the same loader mods use.
- **Two layers: rule-picked barks, and rule-picked authored conversations.** Selection is dynamic.
  Conversation content is authored.
- **Dialog state is squad-scoped**, on the player's side.
- **One condition language for all selection**, ours, even inside conversation scripts.
- **Barks are our own CSV format, not the conversation language.** That lets Slice 1 proceed while
  Ink or Yarn is undecided, and avoids running a script engine for every bark.
- **Ids are local to a package, qualified by the loader. Mods only add, in v1.**
- **Dialog crosses the network as ids**, and each client resolves text in its own language.
- **Broken content is skipped with a report, never fatal.** Unknown game-content ids are warnings
  at runtime and errors in our own content sweep.

## Open Questions Worth Tracking

- **Ink or Yarn.** Jim is reading up. See the criteria above. Blocks Slice 2 only.
- **Text-loaded definitions.** Should mods be able to add character (and item, faction) definitions
  from text? This decides whether "a mod adds an NPC with dialog" is possible at all. It probably
  means the same loader grows a definitions path. That's a bigger modding decision than dialog.
- **Floating bark text over the speaker, or the feed only?** The design says the feed. Jim's
  Slice 1 look should settle whether that's enough.
- **Does a single-player conversation pause the game?** Co-op can't, which argues for consistency.
  Kenshi doesn't pause either.
- **Explicit replacement of `core` entries by mods.** Add-only may prove too limiting once modders
  want to fix or rewrite base lines.
- **Hearing range for barks, and whether it should respect awareness** (walls, distance, noise)
  once perception exists (`ai-and-behavior.md`).
- **Who does the talking.** v1 uses whichever selected squad member is in range. Choosing the
  speaker deliberately matters once facts like lineage exist.
