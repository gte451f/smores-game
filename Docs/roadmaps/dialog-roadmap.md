# Dialog Roadmap — Barks, Conversations and the Content Loader

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how dialog works lives in the `game-systems`
skill's `dialog.md` topic, created by Slice 1 and extended by each slice after it, and this file is
trimmed as each slice ships. Sections marked **SHIPPED** have moved there and keep only a pointer.

> **Status: Slice 1 is DONE** - built, committed and PIE-checked by Jim 2026-09-24. **Slice 2
> (floating bark text) is next**, and was added after that look; it needs one decision from Jim
> before it starts (a proximity bark - see its entry). **Slice 3 (conversations)** waits on the
> Ink-or-Yarn decision. The game-data roadmap this one depended on closed 2026-09-24.

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

- **A third-party runtime arrives in Slice 3** (the Ink or Yarn plugin). Confining that dependency
  to one module means nothing else in the game links against it.
- **Dialog reads nearly everything below it** (records, standing, wallets, inventory) and almost
  nothing reads dialog (only `SmoresUI`, to draw the window, and `smores`, to open it). That puts
  it near the top of the stack, like `SmoresAI` in the target map.

Slice 1 created it (registered per `unreal-module-organization.md`'s checklist, and added to its
target module map), with `IDialogHost` as the controller seam. Today `SmoresUI` doesn't depend on it;
the conversation window adds that edge. Effects that need the player controller
(opening trade) go through a narrow interface declared in `SmoresDialog` and implemented by
`AStrategyPlayerController`. That is the `IStrategySelectionHost` pattern, so nothing starts
depending on `smores`.

## The Loader — SHIPPED (Slice 1)

Folders and packages, `mod.json`, discovery and `Requires` ordering, parsing, validation with
package/file/line reports, qualified ids, runtime string tables and translations, hot reload, and
the multiplayer rules (every machine loads the same files; dialog crosses the network as ids;
mismatch detectable, not enforced) are all built and documented in `dialog.md`.

What Slice 3 still owes the loader:

- **`conversations/*` in each package**, in the chosen language's compiled format, parsed and
  validated the same way barks are - header metadata in our condition language, broken entries
  skipped with a report.
- **Hot reload ends active conversations first** rather than migrating them.
- **The loader rejects a compiled script older than its source**, in editor builds only (see the
  Ink-or-Yarn criteria below).

## Facts, Conditions and Effects

### Facts and the condition language — SHIPPED (Slice 1)

The fact registry, the context (Speaker, Listener, Player, and the event's payload - `Event.Victim`
today), the "only register a fact something can answer" rule, and the condition language itself
(clauses joined by `;`, no `or`, checked at load time) are built and documented in `dialog.md`. The
grammar already accepts the `Flag(name)` / `Seen(id)` call form; Slice 3 registers those two facts.

**Our condition language owns selection even inside conversation scripts.** Ink and Yarn both have
their own variables and expressions. Those are used for flow *inside* a conversation, reading
facts through the language's "external function" bridge. But whether a conversation or topic is
*available* is always written in our language. That keeps one grammar for writers to learn, one
validator, and one place a modder looks.

### Effects

A fixed list of named actions a line or choice can trigger, each implemented in C++,
**authority-only**, and validated at load time like facts. Slice 1 needs none (barks change
nothing). Slice 3 registers only what its content uses: `SetFlag`, `ChangeStanding`, `TakeMoney`,
`GiveMoney`, `OpenTrade`.

An effect that could fail (`TakeMoney` with too little in the wallet) is checked *before* the
choice is offered. The choice shows disabled with its reason, the same "a disabled action still
shows, with its reason" rule the target panel follows (`hud-and-panels.md`). The effect itself is
all-or-nothing.

## Implementation Order

Three slices. Each follows the game-data roadmap's protocol:

1. Read this entry, `dialog.md` (once Slice 1 creates it), and the source it names.
2. C++ first. Both slices add `UCLASS`/`USTRUCT` types, so expect cold builds.
3. MCP for widget Blueprints and asset wiring (`mcp-workflow`).
4. Headless suite plus new tests, per `testing.md`.
5. Jim looks at the result in PIE.
6. Commit, move shipped content into `game-systems`, and mark the slice `DONE`.

### Why three slices

Per CLAUDE.md's slicing rules:

- **1 → 3: a decision and a human look.** Slice 3 can't start until Jim has picked Ink or Yarn,
  and Slice 1 deliberately doesn't depend on that choice. Barks use our own format, so the loader
  and the whole bark layer get built and judged while the decision is still being made. Slice 1
  also ends in something Jim has to look at: whether barks read right, and how often they fire.
- **2 exists because of that look.** Jim's Slice 1 PIE pass decided barks should also float over
  the speaker. That is its own human look (size, height, duration, a fight's worth of speakers at
  once) and it doesn't wait on Ink or Yarn either, so it can ship while the decision is open. Folding
  it into Slice 3 would hold a ready piece of work hostage to an unmade choice.
- Everything inside Slice 3 (window conversations, topics, banter) is the same machinery in two
  playback modes, and ends in one PIE look. Splitting it would be "it's a different system", which
  CLAUDE.md lists as not a reason.

### Before Slice 1: move the design into `game-design` — DONE

The design decisions now live in `game-design`'s new `dialogue.md`, and `ai-and-behavior.md`'s
"Barks and Dialogue" section points at it (it used to say dialogue was "not a conversation system",
which the conversations layer overrides).

### Slice 1 — The loader, facts, and barks — **DONE**

Shipped into a new `game-systems` topic, `dialog.md` (which also holds the writers' and modders'
reference), with pointers from `hud-and-panels.md` (a new feed producer),
`unreal-module-organization.md` (the new module), `multiplayer-discipline.md`, `factions.md`,
`combat.md`, `inventory.md`, `input-and-keybinds.md`, `game-data.md` and `testing.md`. A new
`SmoresDialog` module holds `USmoresDialogSubsystem` (the loader and library), the fact registry and
condition language, bark selection, `UBarkDirectorComponent` (server-only, on `AStrategyGameState`)
and `IDialogHost` (implemented by `AStrategyPlayerController`, whose `InteractWithNPC` now raises
`TradeOpened` / `NothingToSay`). Content: `Content/Dialog/core` (27 barks, six French translations)
and `Mods/example` (one more-specific bark). Execs: `SmoresReloadDialog`, `SmoresDialogReport`,
`SmoresTestBark`, `SmoresSetCulture`. 23 new tests, 163 green. Verified in PIE by console: both
packages load clean, the example mod's line wins the trader's greeting then falls back through core
as it cools down, `SmoresSetCulture fr` turns the greeting French and `en` turns it back, and a bark
reaches the feed. A packaged Win64 build was checked to contain `Content/Dialog` and to load it from
the pak.

**Jim's PIE pass, 2026-09-24:**

- **The right lines play, and repeat as they should**, for the bandits and the trader.
- **`SmoresSetCulture fr` turns every trader line French and leaves the bandits in English** - as
  built: only the six trader greetings were translated, which was the localization proof's whole
  scope. Translating the rest is content work, not code.
- **No bark fires on proximity.** Expected, and worth knowing why: Slice 1's five events are all
  something happening *to* or *with* the speaker (a hit, a knockdown, a death, a shop opened, being
  talked to). None fires because a squad simply came near, so outside a fight NPCs speak only when
  spoken to. Whether to add one is the decision at the top of Slice 2.
- **Floating text: yes** - barks should also appear over the speaker. And **a back-and-forth
  conversation goes in its own panel**, which is Slice 3's window as already planned. Written into
  `game-design`'s `dialogue.md`; Slice 2 builds the floating text.
- **Frequency isn't judged yet**, since only interaction barks were seen. It carries into Slice 2's
  look, where a fight's worth of floating lines will answer it (the knobs are `SpeakerQuietSeconds`
  6 s, `HearingRange` 20 m and each row's cooldown).

Notes worth carrying into the later slices:

- **Deviations from the sketch above, all deliberate:**
  - **One string table per package, not one per package per culture.** Translations reach the
    game through an `ILocalizedTextSource` registered with the localization manager, which is what
    makes a string-table entry switch language live. Per-culture tables would each be a fixed
    language. `dialog.md`'s localization section has the mechanism, including why every line must
    be supplied on every load.
  - **Ties are "least recently said by anyone", then a weighted pick.** Recency is per line and
    global (a second guard avoids what the first just said); cooldowns are per speaker.
  - **A per-speaker quiet time** (`SpeakerQuietSeconds`, 6 s) sits in front of selection, so a unit
    in a fight doesn't bark on every hit until each line is cooling. Being spoken to ignores it.
  - **Each event declares who it carries**, and a condition naming anyone else is a load error -
    Downed has no listener, WitnessedDeath has a victim. This is what "a typo is an error, not a
    silent never-matches" needed to cover people as well as fact names.
  - **`WitnessedDeath` is raised by the nearest ally still standing**, allies being the same
    player's squad or the same non-empty faction.
  - **`SmoresTestBark` takes an optional name** (`SmoresTestBark TradeOpened Ada`), so a writer can
    try a line without clicking anyone - and so an agent can drive it from the console.
- **Traps hit:**
  - `NewObject<UObject>()` as a stand-in identity trips an abstract-class ensure that fires once per
    session - now in `testing.md`.
  - The editor's culture preview refuses to start without a native game culture, and the project
    has no localization target; the text source reports `en`.
  - `FStringTable::SetSourceString` takes a third (notes) argument in editor builds only.
  - A World Partition cell streaming in is a level being added, not a spawn, so the director
    watches `LevelAddedToWorld` as well as `OnActorSpawned`.
- **Open for later, not Slice 3's problem:** a packaged build ships English culture data only
  (`InternationalizationPreset=English`), so it can't switch to French yet; every package's source
  text is assumed English.

### Slice 2 — Floating bark text

Added after Jim's Slice 1 PIE pass. **Barks only**: anything with back-and-forth is a conversation
and gets Slice 3's window, never floating text (Jim's call, same pass).

**Decision before starting - Jim's: add a proximity bark?** As things stand NPCs only speak when
something happens to them or someone talks to them, so floating text would mostly decorate
interactions the player started. The design names two barks that fire on approach - "a shopkeeper
hawking goods, a guard challenging a loiterer" (`ai-and-behavior.md`) - and floating text is where
they would read best. An **`Approached`** event would be the first:

- raised by the director when one of a player's squad comes within `ApproachRange` (say 8 m) of an
  NPC who is on their feet, having been outside it - on the way in, not every moment they stand
  there - and no more than once per NPC per player per cooldown;
- carrying Speaker (the NPC), Listener (the squad member who came near) and Player, like
  `TradeOpened`;
- checked on a slow timer (straight-line distance between each NPC and each squad member), which is
  the seam real perception replaces later. **The honest limit**: it notices through walls, exactly
  as `HearingRange` already does, until perception exists;
- with core content for it - a generic line plus a trader's hawk and a bandit's challenge, since
  the content sweep wants a generic and a specific line for every event.

**Recommendation: include it.** Without it the slice is right but quiet. If Jim says no, the event
waits for the AI roadmap's perception, and Slice 2 is floating text alone.

**Builds:**

- **`Client_NotifyBark` also carries the speaker** (the actor), so a client knows where to put the
  line. The line still travels as its id. A speaker the client can't resolve (not relevant to it)
  gets the feed line and no bubble.
- **A bark bubble layer on the HUD**: `UBarkBubbleLayerWidget` in `SmoresUI`, a full-screen layer
  hosted by `UStrategyUI` below every window, holding one `UBarkBubbleWidget` per speaker (plus its
  WBP via MCP). Each frame it projects each speaker's head position to the screen through the owning
  player controller and moves the bubble there.
  - **A HUD layer rather than a damage-number-style actor**, for three reasons: it is
    per-local-player by construction (only the players who heard a line see it, each in their own
    language); keeping one bubble per speaker and stopping nearby bubbles overlapping needs every
    bubble in one place; and it needs no widget class wired onto every unit Blueprint.
    `ADamageNumberActor` stays as it is.
  - **It must never take a click** (`HitTestInvisible`). This is a deliberate exception to
    `hud-and-panels.md`'s "anything on the HUD that reads as a panel must be a `UHUDRegionWidget`":
    that rule exists so panels *eat* clicks, and a bubble floats over the world - a click on it must
    reach the unit underneath. Write the exception into that topic in the same change.
- **The rules:**
  - A new line from the same speaker replaces their bubble.
  - A bubble lasts long enough to read - a floor plus a little per character, capped - in **real
    time**, so 8x doesn't flash it past, then fades. It is an *event*, like the damage numbers; the
    feed stays the *record* and keeps every line, unchanged.
  - It follows the speaker as they move, and hides while they are off screen or behind the camera.
    No edge-of-screen arrows: the feed already covers a line said out of sight.
  - Bubbles near each other stack instead of overdrawing.
  - Long lines wrap at a maximum width. The text is the line's string-table `FText`, so it follows
    the culture like the feed does.
  - Styling is the legible floor from `hud-and-panels.md` (dark translucent box, light text) until
    the styling pass.
- **Slice 3's ambient banter should use the same layer**, each line floating over whoever says it.
  Noted there.

**Tests:** whatever is pulled into a plain helper - the lifetime formula, one bubble per speaker, a
replaced bubble's timer restarting, expiry. Placement and look are PIE. If `Approached` is in:
entering fires once, standing inside doesn't fire again, leaving and coming back after the
cooldown does, an incapacitated NPC never does, and one player's approach doesn't use up another's.

**Verification:** Jim in PIE judges the bubble's height and size, how long it stays, what a fight
with several people talking looks like, whether it reads at 4x and 8x, and - if `Approached` is in -
how often people pipe up. The bark-frequency question Slice 1's look left open is answered here.

**Ships into:** `dialog.md`; `hud-and-panels.md` (the new layer, and its click exception).

### Decision needed before Slice 3: Ink or Yarn

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

### Slice 3 — Conversations, topics and banter

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
- **The window**: `UConversationWidget` in `SmoresUI` plus its WBP via MCP - its own panel, never
  floating text, which Jim confirmed in Slice 1's PIE pass. It shows the speaker's
  name and portrait (portrait fallback rules from `hud-and-panels.md`), the current line, and the
  choices, with disabled ones showing their reason. Every line spoken also goes to the COMMS feed,
  which is the transcript the design promises in `player-interface.md`.
- **Ambient playback**, the second mode of the same machinery. An `Ambient` conversation plays
  without a window. Lines go to the feed with a reading delay between them, and float over each
  speaker through Slice 2's bubble layer. Used for **in-squad banter**: a server-side director
  tries an eligible ambient conversation after an engagement ends and at a slow cooldown while the
  squad is idle, casting its participants from that player's squad members standing near each
  other. The validator rejects choices inside an ambient script.
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
- **Barks float over the speaker as well as going to the feed; a back-and-forth conversation gets
  its own panel.** Jim's call after Slice 1's PIE pass, 2026-09-24. The floating text is an event
  that fades; the feed is the record that doesn't.

## Open Questions Worth Tracking

- **Ink or Yarn.** Jim is reading up. See the criteria above. Blocks Slice 3 only.
- **A proximity bark (`Approached`) in Slice 2, or wait for perception?** Jim's call before
  Slice 2 starts; see its entry.
- **How often barks fire.** Slice 1's look only saw interaction barks; Slice 2's look answers it.
- **Text-loaded definitions.** Should mods be able to add character (and item, faction) definitions
  from text? This decides whether "a mod adds an NPC with dialog" is possible at all. It probably
  means the same loader grows a definitions path. That's a bigger modding decision than dialog.
- **Does a single-player conversation pause the game?** Co-op can't, which argues for consistency.
  Kenshi doesn't pause either.
- **Explicit replacement of `core` entries by mods.** Add-only may prove too limiting once modders
  want to fix or rewrite base lines.
- **Hearing range for barks, and whether it should respect awareness** (walls, distance, noise)
  once perception exists (`ai-and-behavior.md`).
- **Who does the talking.** v1 uses whichever selected squad member is in range. Choosing the
  speaker deliberately matters once facts like lineage exist.
