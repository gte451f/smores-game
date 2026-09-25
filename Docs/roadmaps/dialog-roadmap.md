# Dialog Roadmap — Barks, Conversations and the Content Loader

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how dialog works lives in the `game-systems`
skill's `dialog.md` topic, created by Slice 1 and extended by each slice after it, and this file is
trimmed as each slice ships. Sections marked **SHIPPED** have moved there and keep only a pointer.

> **Status: Slices 1 and 2 are DONE** - Slice 1 built, committed and PIE-checked by Jim
> 2026-09-24; Slice 2 (floating bark text and proximity barks) built and PIE-checked by Jim
> 2026-09-25. **Slice 3 (conversations) is next:** Jim chose **Yarn** on 2026-09-25, after a spike
> that played the same scene in Ink and in Yarn from loose files (see "Decided: Yarn"). The spike's
> Yarn player, its tests and the example scene (commits `a466c73` and `c1bffd2`) were merged into
> `main` on 2026-09-25. The Yarn plugin itself is local-only and never committed while the repo is
> public (`dialog.md`). The game-data roadmap this one depended on closed 2026-09-24.

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
- **Conversations** are authored scripts, written in **Yarn** (Yarn Spinner; decided 2026-09-25,
  see "Decided: Yarn").
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

- **A third-party runtime arrives in Slice 3** (the Yarn Spinner plugin, whose player the spike
  module `SmoresDialogSpike` holds until then). Confining that dependency to one module means
  nothing else in the game links against it.
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

- **`conversations/*.yarn` in each package**, each shipped with the three files `ysc` writes
  beside it:
  - `<name>.yarnc`, the program, which holds ids only;
  - `<name>-Lines.csv`, the text;
  - `<name>-Metadata.csv`, each line's other tags.

  The spike's `LoadYarnScript` already reads the first two from loose files (`dialog.md`). A
  broken file or node is skipped with a package/file/line report, like a bark row.
- **Every line must carry an explicit `#line:` id**, or the loader rejects it. `ysc` invents an
  id for an untagged line, but that id changes when the line moves, which would orphan its
  translations and any voice recording. `ysc tag` stamps real ids, so writers never type them.
- **Line text goes into the package's string table**, keyed by the qualified line id exactly like
  a bark's. Translations then use the same `localization/<culture>/*.csv` files and the same text
  source. The program never holds text, so "the client looks the line up by id" is simply Yarn's
  own model.
- **Names are checked at load time.** `ysc` accepts any function or command name without asking,
  so the loader walks each program's instructions and checks:
  - every function called, against the facts Yarn can ask;
  - every command run, against the effects, with its argument count.

  An unknown name is an error for that conversation, not a surprise in the middle of one.
- **Hot reload ends active conversations first** rather than migrating them.
- **The loader rejects a `.yarnc` older than its `.yarn`**, in editor builds only, so a writer
  can't test stale text.

## Facts, Conditions and Effects

### Facts and the condition language — SHIPPED (Slice 1)

The fact registry, the context (Speaker, Listener, Player, and the event's payload - `Event.Victim`
today), the "only register a fact something can answer" rule, and the condition language itself
(clauses joined by `;`, no `or`, checked at load time) are built and documented in `dialog.md`. The
grammar already accepts the `Flag(name)` / `Seen(id)` call form; Slice 3 registers those two facts.

**Our condition language owns selection even inside conversation scripts.** Yarn has its own
variables (`$asked_about_road`) and expressions. Those are used for flow *inside* a conversation,
and read facts through Yarn **functions** (`gold()` in the example). But whether a conversation or
topic is *available* is always written in our language, in its node headers (see Slice 3). That
keeps one grammar for writers to learn, one validator, and one place a modder looks.

- **Yarn's functions come from the fact registry, not by hand.** Check first whether a script may
  call a dotted name such as `Speaker.Faction()`. If not, one bridge function, `fact("Speaker.Faction")`,
  plus a few named shortcuts like `gold()`, covers it.
- **Yarn's `$variables` are scratch, local to one conversation.** The spike gives each conversation
  a fresh store. Anything that must outlive a conversation goes through `<<SetFlag name>>` and is
  read back by `Flag(name)`, so squad memory lives in one place (`UDialogMemoryComponent`), is
  saved in one place, and is readable by our condition language.

### Effects

A fixed list of named actions a line or choice can trigger, each implemented in C++,
**authority-only**, and validated at load time like facts. In Yarn an effect is a **command**:
`<<TakeMoney 20>>`, `<<ChangeStanding Raiders -10>>`. Its arguments arrive as strings, and the
effect parses them. Slice 1 needs none (barks change nothing). Slice 3 registers only what its
content uses: `SetFlag`, `ChangeStanding`, `TakeMoney`, `GiveMoney`, `OpenTrade`.

An effect that could fail (`TakeMoney` with too little in the wallet) is guarded by a condition on
its choice (`-> Pay the toll. <<if gold() >= 20>>`), so it is checked *before* the choice is
offered. The effect itself stays all-or-nothing, and refuses on its own if a script forgets the
guard.

**Yarn never hides a choice whose condition fails.** It offers every such choice marked
unavailable, alike, so the script alone can't say "grey this one out, hide that one". *Our* window
decides, per choice, from a tag:

- a choice tagged `#reason:<key>` shows greyed with that reason (`-> Pay the toll. <<if gold() >= 20>> #reason:not_enough_money`).
  That is the "a disabled action still shows, with its reason" rule the target panel follows
  (`hud-and-panels.md`);
- a choice without one is left out when unavailable, like the question already asked.

The reason keys are a fixed, load-time-validated list, worded in one place like
`ESmoresRefusalReason`, and they come through the `-Metadata.csv` tags.

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

- **1 → 3: a decision and a human look.** Slice 3 had to wait for the conversation language
  (settled 2026-09-25: Yarn), and Slice 1 deliberately didn't depend on that choice. Barks use our own format, so the loader
  and the whole bark layer get built and judged while the decision is still being made. Slice 1
  also ends in something Jim has to look at: whether barks read right, and how often they fire.
- **2 exists because of that look.** Jim's Slice 1 PIE pass decided barks should also float over
  the speaker. That is its own human look (size, height, duration, a fight's worth of speakers at
  once), and it never depended on the conversation language. Folding it into Slice 3 would have
  held a ready piece of work hostage to that choice.
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
  spoken to. Jim chose to add one: Slice 2's `Approached` event.
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

### Slice 2 — Floating bark text and proximity barks — **DONE**

Added after Jim's Slice 1 PIE pass, and both halves are his calls from it: **barks also float over
the speaker** (anything with back-and-forth is a conversation and gets Slice 3's window, never
floating text), and **NPCs speak up when a squad comes near**. One slice because they are judged
in one look: how often people pipe up is only answerable once you can see them do it.

Built 2026-09-25 and documented in `dialog.md` (Player Surface's event table and "Bark bubbles";
C++ "How `Approached` is raised" and "How a bubble is drawn") and `hud-and-panels.md` (the new
layer, and its click-shield exception in Core Rules). In short:

- `Client_NotifyBark` carries the speaker as an actor reference; a client it doesn't resolve on
  gets the feed line and no bubble.
- `UBarkBubbleLayerWidget` (`SmoresUI`), a full-screen `HitTestInvisible` layer bound on
  `UStrategyUI` as `BarkBubbleLayer`, beneath the regions; `UBarkBubbleWidget` per bubble; WBPs
  `WBP_BarkBubbleLayer` and `WBP_BarkBubble`. Rules in the plain `FBarkBubbleSchedule` /
  `StackBoxes`: one bubble per speaker, 2 s + 0.06 s per character capped at 6 s then a 0.6 s fade,
  real time, stack straight up, wrap at 260.
- `EBarkEvent::Approached` (Speaker, Listener, Player), raised by the director's 0.5 s world-time
  timer through the plain `FApproachTracker`: on the way in only, `ApproachRange` 8 m,
  `ApproachCooldownSeconds` 60 plus a leave-and-return, never by a downed NPC or a squad member,
  per NPC per player. Six `core` lines: two generic, a trader's hawk and a Traders Guild one, a
  bandit's challenge, a hated-by-Raiders warning.
- 9 new tests (5 `Smores.Dialog.Approach.*`, 4 `Smores.UI.BarkBubbles.*`); 176 green.

**Choices made while building, all small and all reversible:**

- **An approach counts once raised**, whether or not the NPC then said anything (quiet time, line
  cooldown). An NPC who happened to be mid-bark doesn't greet that squad until they leave and come
  back after the cooldown.
- **First sight counts as an arrival**: a squad spawning, or an NPC streaming in, already within
  range is greeted on the first check.
- **Downed squad members still count as "the squad is here"**, so a squad that went down together
  isn't greeted as newcomers when it gets up.
- **Stacking keeps the bubble shown first in place** and lifts later ones straight up, in the order
  speakers first spoke (a replacement keeps its slot), so a stack doesn't reshuffle every time
  someone speaks. The cost: a newer line from a speaker lower on screen can sit above an older one.

Before Jim's look, an agent's PIE smoke test walked squad members up to NPCs: Merchant Ada gave the
Traders Guild hawk and three bandits their challenge, each within half a second, each in the feed,
and a bubble showed over the speaker. No errors from game code.

**Jim's PIE pass, 2026-09-25: "looks good".**

- **Floating text on approach** for NPCs and the trader.
- **Floating text from squad members during combat** (their `Hurt` and `Downed` lines).
- **Nothing repeats while standing near an NPC for a long time** - the edge-trigger holds.
- No changes asked for: bubble size, height and timing, and every tuning knob, stay at their
  starting values. The knobs, for whenever that changes: `ApproachRange`, `ApproachCooldownSeconds`,
  `SpeakerQuietSeconds`, `HearingRange`, each row's cooldown (director and `core.csv`), and the
  layer's timing, `HeadClearance` and `StackGap` (`WBP_BarkBubbleLayer`).

### Decided: Yarn (2026-09-25)

This was settled by a spike rather than by reading. The same bandit shakedown was written in both
languages, loaded from loose files in `Mods/example/conversations`, and played by four identical
tests and a PIE command.

**Both passed the criterion that couldn't be traded away:** a script loaded from a plain file at
runtime and stepped through with no screen. Both also ran two conversations over one loaded
script. What separated them:

| | Yarn (Yarn Spinner for Unreal's player) | Ink (inkcpp) |
|---|---|---|
| Line ids | Built in; `ysc tag` stamps them | Hand-typed `#id:` tags |
| Text | A separate table keyed by id; the program holds none | Inside the story |
| Voice | One id per fixed line, so a recording is filed under it | Ink assembles sentences as it plays (glue, variations), and those can't be recorded as one clip |
| Builds on 5.8 | 3 build fixes, plus moving its file reader into the runtime half | Unchanged |
| Bugs found | None in playback | One: a hidden choice's tags leaked onto the next choice. Fixed locally |
| A failed choice | Offered, marked unavailable, all-or-nothing | Hidden |
| Licence | YSPL: credit required, and the plugin stays out of this repo while it is public | MIT |

**Jim chose Yarn for its translation and voice handling.** Voice is expected for key characters and
story moments, up to about 20% of dialog. He accepted these costs:

- carrying a patched pre-release plugin. `dialog.md` has the patch list, the licence duties and how
  to update it;
- supplying Yarn's basic operators ourselves;
- Yarn's all-or-nothing unavailable choices, handled by the `#reason:` tag (see Effects).

Ink was the stronger prose language and the simpler dependency. Its half of the spike is kept in
git history (`a466c73`) in case this is ever revisited. Voice acting itself stays out of scope for
this roadmap.

Yarn Spinner 3's own saliency (storylets, the `when:` node header) is **not used**. Our condition
language owns selection, and our metadata lives in our own headers.

### Slice 3 — Conversations, topics and banter

**Starts from the spike, not from scratch.** The Yarn player already works on one scene;
`dialog.md`'s "Conversations: the Yarn player" covers what it does and what it taught. It already:

- loads a compiled script from loose files;
- steps through it, with choices and their availability;
- calls functions and runs commands;
- runs several conversations over one loaded script;
- refuses broken files.

The first step is to **move that player into `SmoresDialog`**. `SmoresDialog` gains the
`YarnSpinner` plugin module as a dependency, plus `SmoresEconomy` for `TakeMoney`. Port the spike's
four tests, then **delete `SmoresDialogSpike`** and its registrations. Keep
`Mods/example/conversations/shakedown.yarn`: it is this slice's first piece of content.

**Builds:**

- **The conversation format.** A `.yarn` file in a package's `conversations/` folder. **Each node
  that carries our headers is one conversation, and those headers are its metadata.** Yarn keeps
  custom node headers through compiling (checked), so no separate file is needed. The headers:
  - `attach:` what it attaches to: a definition id, role, faction or tag (e.g.
    `Speaker.Definition == Bandit`)
  - `requires:` its requirements (our condition language)
  - `priority:`
  - `once:` never again for this squad once seen
  - `kind:` `Greeting`, `Topic` or `Ambient`
  - `participants:` for `Ambient`, the participants it needs

  A node without our headers is an ordinary Yarn node that a conversation jumps into, like the
  shakedown's `Choices`.
- **Complete the operator library.** Add `Enum.*` to the spike's operators, plus whichever
  built-ins the content uses. For `visited()`/`visited_count()`, the plugin's own versions show
  where the VM records node visits in the variable store.
- **Selection, Hades-style.** Of the conversations attached to this NPC whose requirements hold
  and which aren't used up, the highest priority wins. A tie goes to the least recently seen.
- **Topics.** When a greeting conversation ends, the window offers every eligible `Topic`
  attached to this NPC as a choice, plus "Goodbye". Topics are how a mod, and later a quest, adds
  something to *our* NPCs without editing them: attach a topic to `Speaker.Faction == Ironclan`
  and every Ironclan member offers it. The list is built by our code, not inside the script, so topics never
  depend on what the language runtime supports.
- `UDialogMemoryComponent` on `AStrategyPlayerState`: this squad's flags and seen conversations.
  Server-owned, a plain reflected record for the future save system to serialize unchanged. Adds
  the `Flag(name)` and `Seen(id)` facts.
- `UConversationComponent` on `AStrategyPlayerController` (RPCs need a player-owned actor). The
  server runs the Yarn player. A client RPC sends the current line and choices **as ids**, with each
  choice's enabled state and reason. A server RPC sends back the pick. The conversation **ends**
  when either side walks out of range, is downed, or enters combat.
  - Ids are qualified by package like bark ids (`example.shakedown_toll`). Yarn's own form is
    `line:shakedown_toll`, and the spike strips the `line:` part.
  - Each conversation gets its own player and variable store; the loaded program is shared.
- **A debug exec** to open a conversation by id on a named NPC without walking up, like
  `SmoresTestBark`. It replaces the spike's `SmoresSpikeTalk` / `SmoresSpikeChoose`, which go with
  its module.
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
  - **the shakedown, already written**, in `Mods/example/conversations/shakedown.yarn`. Give its
    `Shakedown` node our headers (`attach: Speaker.Definition == Bandit`, `kind: Greeting`), so
    Talk on any Bandit in `LVL_Strategy` opens it instead of today's "Keep walking" bark. Add
    `#reason:not_enough_money` to its pay choice, and point `gold()` at the real wallet.
    - It covers `TakeMoney`, `ChangeStanding` (on `Raiders`, the Bandit's faction), a guarded
      choice, Yarn's own memory and a loop.
    - It sits in the example mod, so it also proves a mod can give a conversation to one of *our*
      characters.
    - Hostile NPCs still refuse to talk first (unchanged). Today's Bandits answer Talk, so they
      qualify.
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
- The spike's four tests, ported: the whole path, the gold-guarded choice, two conversations over
  one script, broken files.
- Load-time checks: a line without an explicit `#line:` id, an unknown function or command, a
  wrong argument count, and an unknown `#reason:` key are each rejected with their file and line.
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
  reopen localization). Yarn was chosen with voice in mind, though. Every line has one permanent
  id and is one fixed sentence, so a future voice roadmap files each recording under its line id.
  Keep voiced lines free of inserted `{$variables}`.
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
  Ink or Yarn was undecided, and avoids running a script engine for every bark.
- **Ids are local to a package, qualified by the loader. Mods only add, in v1.**
- **Dialog crosses the network as ids**, and each client resolves text in its own language.
- **Broken content is skipped with a report, never fatal.** Unknown game-content ids are warnings
  at runtime and errors in our own content sweep.
- **Barks float over the speaker as well as going to the feed; a back-and-forth conversation gets
  its own panel.** Jim's call after Slice 1's PIE pass, 2026-09-24. The floating text is an event
  that fades; the feed is the record that doesn't.
- **NPCs bark on proximity** (`Approached`), from straight-line distance until perception exists.
  Jim's call, 2026-09-24. Noticing through walls is the known, accepted cost of not waiting.
- **Conversations are written in Yarn**, run through the Yarn Spinner for Unreal plugin's player
  only (not its runner, presenters or asset import), from loose files. Jim's call, 2026-09-25,
  after the Ink-vs-Yarn spike, for Yarn's line ids, text table, translation and voice handling.
  See "Decided: Yarn".
- **The Yarn plugin stays out of git while the repo is public**, carried as a local, patched copy
  (`dialog.md` has the patch list). When the repo goes private, it is committed.

## Open Questions Worth Tracking

- **How often barks fire.** Slice 2's look (approach barks, a fight's worth of `Hurt` lines) asked
  for no retuning. Worth another look once fights are bigger and there is more content to say.
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
