# Dialog Roadmap — Barks, Conversations and the Content Loader

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices, or
when Jim points at it. The permanent record of how dialog works lives in the `game-systems`
skill's `dialog.md` topic, created by Slice 1 and extended by each slice after it, and this file is
trimmed as each slice ships. Sections marked **SHIPPED** have moved there and keep only a pointer.

> **Status: all three slices are DONE - the roadmap is complete.** Slice 1 built, committed and
> PIE-checked by Jim 2026-09-24; Slice 2 (floating bark text and proximity barks) built and
> PIE-checked by Jim 2026-09-25; Slice 3 (conversations, topics, banter, effects and squad memory,
> in **Yarn**) built on top of the spike's player, which it folded into `SmoresDialog`, and
> PIE-checked by Jim 2026-09-25 ("looks great"). Everything built is documented in `dialog.md`;
> what is left here is the decision record and the open questions. The Yarn plugin itself is
> local-only and never committed while the repo is public (`dialog.md`). The game-data roadmap this
> one depended on closed 2026-09-24.

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

- **A third-party runtime arrived in Slice 3** (the Yarn Spinner plugin). Confining that
  dependency to one module - privately - means nothing else in the game links against it.
- **Dialog reads nearly everything below it** (records, standing, wallets, inventory) and almost
  nothing reads dialog (only `SmoresUI`, to draw the window, and `smores`, to open it). That puts
  it near the top of the stack, like `SmoresAI` in the target map.

Slice 1 created it (registered per `unreal-module-organization.md`'s checklist, and added to its
target module map), with `IDialogHost` as the controller seam. The conversation window gave
`SmoresUI` its edge to it in Slice 3. Effects that need the player controller
(opening trade) go through a narrow interface declared in `SmoresDialog` and implemented by
`AStrategyPlayerController`. That is the `IStrategySelectionHost` pattern, so nothing starts
depending on `smores`.

## The Loader — SHIPPED (Slice 1)

Folders and packages, `mod.json`, discovery and `Requires` ordering, parsing, validation with
package/file/line reports, qualified ids, runtime string tables and translations, hot reload, and
the multiplayer rules (every machine loads the same files; dialog crosses the network as ids;
mismatch detectable, not enforced) are all built and documented in `dialog.md`.

Slice 3 added `conversations/*.yarn` to it, with the three files `ysc` writes beside each, and
the load-time checks conversations need - built and documented in `dialog.md` ("A conversation
file", "How a conversation is loaded and checked").

## Facts, Conditions and Effects

### Facts and the condition language — SHIPPED (Slice 1)

The fact registry, the context (Speaker, Listener, Player, and the event's payload - `Event.Victim`
today), the "only register a fact something can answer" rule, and the condition language itself
(clauses joined by `;`, no `or`, checked at load time) are built and documented in `dialog.md`. The
grammar already accepts the `Flag(name)` / `Seen(id)` call form; Slice 3 registers those two facts.

**Our condition language owns selection even inside conversation scripts** - built in Slice 3:
`attach:` / `requires:` headers choose, Yarn's `$variables` and functions run the flow inside, and
every fact is a Yarn function with its dots as underscores (`speaker_faction()`, `gold()`). Yarn
can't call a dotted name, and a single `fact("...")` bridge wouldn't compile once one function had
to answer both numbers and names. `dialog.md` has the details.

### Effects — SHIPPED (Slice 3)

`SetFlag`, `ChangeStanding`, `TakeMoney`, `GiveMoney` and `OpenTrade` - Yarn commands, C++,
authority-only, checked at load time - and the `#reason:` tag that decides a greyed choice from a
hidden one are built and documented in `dialog.md`.

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

### Slice 3 — Conversations, topics and banter — **DONE**

Built 2026-09-25 from the spike's Yarn player, which moved into `SmoresDialog`; the spike module and
its registrations are gone, and its four tests were ported. Everything below is documented in
`dialog.md` (Player Surface's "Conversations" and "Banter", Writing Dialog's "A conversation
file", and the C++ sections), `hud-and-panels.md` (the window) and the other topics it touches.
In short:

- **The format**: each Yarn node with our headers (`kind`, `attach`, `requires`, `priority`, `once`,
  `participants`, and a Topic's `label`) is one conversation; the loader checks every line id,
  function, command and header at load time, with the line.
- **The flow**: Talk picks the highest-priority eligible greeting (ties to the one least recently
  seen); when it ends the window offers every eligible topic plus Goodbye; a topic comes back to
  the list, rebuilt. It breaks off on range (5 m), down, or a fight.
- **The server runs it** (`UConversationComponent` on the controller) and sends ids; each client
  reads its own language. `UDialogMemoryComponent` on the player state holds flags and seen
  conversations, read by the new `Flag()` / `Seen()` facts; `Gold` is a fact too.
- **The window**: `UConversationWidget` / `UConversationChoiceWidget` in `SmoresUI`, with
  `WBP_Conversation` and `WBP_ConversationChoice` wired through MCP.
- **Banter**: `UBanterDirectorComponent` on the GameState plays Ambient conversations among a
  squad after a fight and now and then while idle, through the bark delivery.
- **Content**: a `core` trader greeting with a trade choice, a haggling-flavoured topic that
  unlocks a once-only backstory topic on the Trader, two Settler banters, and in the example mod
  the shakedown on every Bandit (with a follow-up greeting once paid) and a topic on every
  Traders Guild member. The trader greeting and topic labels have placeholder French.
- **Debug execs**: `SmoresTestConversation` (open one by id on a named NPC, play a banter, or run
  Talk's own choice with an explanation) and `SmoresDialogMemory`.
- 17 new tests (13 `Smores.Dialog.Conversation.*`, 3 `Smores.Dialog.Effects.*`,
  `Smores.Dialog.Memory.IsPerPlayer`) and the content sweeps extended; 189 green.

**Choices made while building, all small and all reversible:**

- **Talk is decided on the server now.** The client still refuses hostile and out-of-reach at once;
  what happens next (conversation, trade, bark) comes back from `Server_InteractWithNPC`, so even
  opening trade is a round trip on a remote client.
- **`OpenTrade` ends the conversation** - the trade screen replaces the window - and still raises
  the `TradeOpened` bark, so the trader lines written for a shop opening (the example mod's
  included) keep playing now that Talk on Merchant Ada opens a conversation first.
- **Topics can be raised again**; only `once: true` retires one. A conversation counts as seen the
  moment it starts, so walking off a `once:` one halfway uses it up.
- **"You:" is the squad member's cue**; any other name is the NPC and none is narration. A topic's
  label is plain words in its header, translated under `<title>_label`.
- **Flag names are shared by every package** (a mod can read core's), with a mod's own flags
  prefixed by its id by convention.
- **Text compares case-insensitively in Yarn too** (`String.EqualTo`), as names do in conditions.
- **When no choice is shown** (every one unavailable and untagged), the script ends there.
- **Banter starts at**: 6 s after a fight, every 5 min idle, at most every 3 min, cast within 10 m -
  guesses, on `UBanterDirectorComponent`.
- **No number keys for choices yet** - a new key's IMC mapping is a hand step; the window is
  mouse-only, and `T` is Goodbye.
- `ConversationTypes.h` is `DialogConversationTypes.h`: an engine plugin has the first name, and
  UHT refuses two reflected headers with one name.

Before Jim's look, an agent's PIE smoke test drove it through the console and the window itself:
both packages loaded clean (core 5 conversations, example 3); the shakedown played through the
window - question used up, refusal, `Red Sand Raiders standing -10 (now -10)` in the feed, then
Goodbye; Merchant Ada's greeting opened her shop in place of the window; Talk's selection explained
itself; the range break-off fired for a squad member across the map; a banter cast and played its
four lines; memory reported what was seen. It found a latent bug in every kept window (the close
button bound twice on reopening - fixed in `UWindowWidget`), a transcript that didn't scroll to its
last line once choices appeared (fixed), and that long feed lines don't wrap (a HUD Known Gap).

**Jim's PIE pass, 2026-09-25: "looks great".**

- **The bandit shakedown worked**, and a second bandit knew the squad had paid and played a
  different line - the squad memory and `ShakedownPaid`'s priority, as built.
- **Talking with the trader revealed a moderate conversation tree** - the greeting, then the topics.
- **The window sat centred on screen, which is fine for now.** Its placement stays as built.
- **Squad banter played after a short wait.** No retuning asked for; the knobs stay at their
  starting values (`UBanterDirectorComponent`).

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
- **A conversation's metadata lives in its Yarn node's headers, and our condition language chooses**
  (not Yarn's `when:`). Written into Slice 3 on 2026-09-25 and built that way.
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
