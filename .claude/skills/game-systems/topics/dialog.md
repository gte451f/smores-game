# Dialog: Barks, Conversations and the Loader

## Purpose

What characters say, and the machinery that picks it. Built by the three slices of
`Docs/roadmaps/dialog-roadmap.md`:

- dialog read from plain text files at startup, the base game loaded exactly like a mod;
- a small condition language over a registered list of **facts**;
- **barks** - one-way lines picked by "most specific match wins", delivered to the activity feed
  and floated briefly over the speaker's head, including the `Approached` bark NPCs say when a
  squad comes near;
- **conversations** - authored exchanges with choices, written in **Yarn** (Yarn Spinner), played
  in their own window: an NPC's **greeting**, then the **topics** that apply to them, plus
  Goodbye. Conversations change the world through a fixed list of **effects**, and a squad
  remembers what it has said and heard;
- **banter** - the same machinery with no window: squad members talking among themselves, into
  the feed and floating over each speaker.

The design intent is `game-design`'s `dialogue.md`. This topic is how it is built.

## Player Surface

- **Barks appear in the activity feed**, on the COMMS tab (and LOG), in quotes, with the speaker's
  name as the line's source: *"Coin first. Questions never." - Merchant Ada*. The feed is the
  record, and keeps every line.
- **And they float over the speaker's head** for a few seconds, then fade - see "Bark bubbles"
  below. The bubble is the moment; the feed is the record.
- **When someone barks** - six moments:

  | Event | When | Speaker | Who they're speaking to |
  |---|---|---|---|
  | `Hurt` | took a hit and is still standing | the one hit | whoever hit them |
  | `Downed` | just went down | the one who went down | nobody |
  | `WitnessedDeath` | an ally died near them | the nearest ally still standing | nobody (the dead one is `Event.Victim`) |
  | `TradeOpened` | a squad member opened their shop | the trader | the squad member at the counter |
  | `NothingToSay` | a squad member tried to talk to someone with no shop | the NPC | that squad member |
  | `Approached` | one of a player's squad just came within 8 m | the NPC | the squad member who came near |

- **`Approached` is the one bark that fires on proximity** - a trader hawking, a bandit warning you
  off. It fires on the way in, not for as long as the squad stands there, and the same NPC won't
  greet the same player's squad again until a minute (`ApproachCooldownSeconds`, world time) has
  passed **and** the squad has left and come back. An NPC who is down doesn't greet anyone, and
  squad members never raise it at each other or anyone else. In co-op each player's squad is
  noticed separately. It is checked every half second of world time from straight-line distance,
  so **it notices through walls** - accepted until perception exists.

- **Who hears it**: every player with at least one squad member within 20 m (`HearingRange`) of
  the speaker. In co-op two players nearby both get the line, each in their own language.
- **A speaker who just spoke stays quiet for 6 seconds** (`SpeakerQuietSeconds`), whatever the line.
  That is what stops a fight turning into a wall of "Ngh." Being *spoken to* ignores it: talking to
  someone always gets an answer if one of their lines is free.
- **Talking to someone** - double-click, `T` or the target panel's Talk, all the same verb - goes,
  in order: a hostile NPC refuses; out of reach refuses with *Too far away*; then someone with a
  conversation opens it (see Conversations, below); else a trader opens their shop and says a
  `TradeOpened` line; else they say a `NothingToSay` line. A trader whose conversation a mod
  removed still trades.

### Bark bubbles

Every bark a player hears also floats over the speaker's head, in a dark translucent box with light
text (the HUD's legible floor until the styling pass).

- **Only the players who heard it see it**, each in their own language - the bubble is drawn from
  the same line the feed gets.
- **One bubble per speaker.** A new line from someone already showing one replaces it and restarts
  its clock, keeping its place in a stack.
- **It stays long enough to read, then fades**: 2 seconds, plus 0.06 s per character, capped at
  6 seconds, then a 0.6 s fade. **Real time**, so neither 8x nor the paused tier changes how long a
  line stays up.
- **It follows the speaker** as they move, and hides while they are off screen or behind the
  camera. No edge-of-screen arrows: the feed already has a line said out of sight.
- **Bubbles near each other stack** instead of overdrawing: the one shown first keeps its place and
  later ones move straight up above it. Nothing moves sideways.
- **Long lines wrap** at 260 slate units.
- **It never takes a click.** A click on a bubble reaches the unit underneath it.

A conversation never floats: back-and-forth goes in its window (Jim, after Slice 1's PIE pass).
Banter uses these bubbles, one line over each speaker.

### Conversations

- **Talk to someone who has a conversation and its window opens**: who you're talking to (name,
  and portrait - initials until units have portraits), what has been said, and either
  **Continue** or the choices. It is a floating window like the others: drag it by the title,
  resize it from the corner.
- **Which one**: of the greetings attached to that person whose requirements hold right now, the
  highest priority wins; a tie goes to the one your squad saw least recently. Talk to a Bandit
  who has already taken your toll and you get a different greeting.
- **A choice you can't take right now** either shows greyed out with the reason beside it
  (*Not enough gold*) or isn't offered at all - the writer decides, per choice. The toll shows
  greyed when you're short; the question you already asked goes away.
- **Topics.** When the greeting ends, the window offers every topic that applies to this person
  now, plus **Goodbye**. A topic plays and comes back to the list, rebuilt: asking one topic can
  unlock another, and a told-once story leaves the list after it is told. A topic can belong to
  one character, a role, or a whole faction - which is how a mod gives the base game's people
  something new to say.
- **Ending it**: Goodbye, the window's X, or `T` again. It also **breaks off** - with a line in the
  feed - if the squad member and the NPC end up more than 5 m apart (`BreakOffRange`), either goes
  down, or a fight starts: the NPC turns hostile, either of them attacks, or either is hurt.
  Talking to somebody else ends it too.
- **Everything said goes to the COMMS feed** as well, quoted with the speaker's name - including
  the choice you picked, said by your squad member. Effects that move gold or standing add their
  own line (*Paid 20 gold*, *Raiders standing -10 (now -10)*). The feed is the transcript the
  window doesn't keep once it closes.
- **The world doesn't stop.** A conversation runs in real time, in single-player too (whether it
  should pause is an open design question). In co-op each player's conversation is their own,
  in their own language, and several players may talk to the same NPC at once.
- **Who does the talking**: the squad member nearest the NPC, within reach.
- **Your squad remembers.** The flags a conversation sets and the conversations seen belong to
  your squad - your player - never another player's.

### Banter

- **Now and then, your squad members talk among themselves** - two of them, a few lines each, in
  the feed and floating over whoever is speaking, like barks. No window, no choices.
- **When**: 6 seconds after the last blow of a fight (`AfterFightSeconds`), and about every
  5 minutes of world time while nothing happens (`IdleSeconds`) - never more than once every
  3 minutes (`CooldownSeconds`). Rare on purpose, to feel earned.
- **Who**: squad members on their feet standing within 10 m of each other (`CastRange`). One at a
  conversation window sits it out. A fight starting, or a participant going down, stops it mid-way.
- **Everyone near hears it**, exactly as a bark is heard. The knobs are on
  `UBanterDirectorComponent` (`BP_StrategyGameState`).

### Debug execs

All on `AStrategyPlayerController`.

| Command | Does |
|---|---|
| `SmoresReloadDialog` | Re-reads every package from disk and logs the per-package summary. **The writer's loop**: edit a file (recompile a `.yarn`), type this, hear the change without restarting. Any running conversation or banter ends first. This machine only - every machine loads its own copy |
| `SmoresDialogReport` | Every package (id, folder, version, requires, loaded or SKIPPED, counts), every conversation (kind, priority, once, who it attaches to, requires), barks per event, and every problem. `SmoresDialogReport facts` adds the writers' reference: every fact, its name as a Yarn function, every effect, the `#reason:` keys, and who each event carries |
| `SmoresTestBark <Event> [Name]` | Fires an event on a unit and logs **every line considered and why it did or didn't win**, then the winner as this machine shows it, in the current culture. The unit is the one whose name contains `Name` (`SmoresTestBark Approached Ada`), else the clicked NPC, else the first selected squad member. Cooldowns apply; quiet time doesn't, and neither do `Approached`'s range and cooldown - the nearest squad member is the listener wherever they are. A line said this way reaches the feed and floats like any other. For `WitnessedDeath` the unit is treated as the one who died. Hops to the server |
| `SmoresSetCulture <culture>` | Shows game text in another culture - `SmoresSetCulture fr`, then `en` to return. In the editor it previews game text without touching the editor's own menus, and the preview ends when play stops |
| `SmoresTestConversation <id> [Name]` | Opens a conversation without walking up: a Greeting or Topic (`SmoresTestConversation example.Shakedown Bandit`) opens its window on the NPC whose name contains `Name` or whose character definition it is (`Bandit` finds a bandit; `LVL_Strategy`'s are placed as "NPC 1" to "NPC 6"), else the clicked NPC, eligible or not, however far away - so it never breaks off for range - with the nearest squad member talking. An Ambient id plays it among the squad now, whatever its `requires:` and however far apart they stand. `SmoresTestConversation banter` has the squad try a banter as its timer would, cooldown aside, and logs why each candidate did or didn't play. With no id (`SmoresTestConversation "" Bandit`), runs Talk's own choice on that NPC and logs why each greeting did or didn't win. Hops to the server |
| `SmoresDialogMemory [set\|clear <flag>]` | Logs this squad's flags and the conversations it has seen (how often, how recently). `SmoresDialogMemory set trader_mentioned_salt_road` sets a flag, to try a conversation that waits on one; `clear` clears it. Hops to the server |

## Writing Dialog

This section is the writers' and modders' reference.

### Where it lives

```
Content/Dialog/core/            the base game - package "core"
    mod.json
    barks/<any name>.csv
    conversations/<name>.yarn   plus the three files ysc writes beside it:
                  <name>.yarnc, <name>-Lines.csv, <name>-Metadata.csv
    localization/<culture>/<any name>.csv
Mods/<folder>/                  one folder per mod, next to the installed game
    mod.json
    ...the same shape
```

Every `.csv` under `barks/` is read, in file-name order, and every `.yarn` under `conversations/`
with the files beside it. Anything else in a package folder (a README, notes) is ignored.

### `mod.json`

```json
{
	"Id": "lanterns",
	"DisplayName": "Lantern-lit roads",
	"Version": "1.0.0",
	"GameVersion": "0.0.0.1",
	"Requires": ["core", "wicks"]
}
```

- **`Id` is required**: lower-case letters, digits and `_`, starting with a letter. It is the prefix
  every line in the package gets. `core` belongs to the base game.
- `Requires` names packages that must load first. `core` always loads first anyway.
- `DisplayName`, `Version` and `GameVersion` are shown in the report. Nothing checks `GameVersion`.

### A bark file

```csv
Id,Event,Conditions,Text,Weight,Cooldown
trade_generic,TradeOpened,,Coin first. Questions never.,1,0
trade_trader_guild,TradeOpened,Speaker.Role == trader; Speaker.Faction == TradersGuild,Guild prices. No haggling.,1,30
hurt_trader,Hurt,Speaker.Role == trader,"I sell knives, I don't catch them.",1,15
```

- **The first row names the columns**, in any order and any case. `Id`, `Event` and `Text` are
  required; `Conditions`, `Weight` and `Cooldown` are optional; any other column (a `Notes` column,
  say) is ignored.
- **`Id`** - letters, digits and `_`, unique within the package. Write it plain (`trade_generic`); the
  loader makes it `core.trade_generic`. Two mods can both have a `greet` and never collide.
- **`Event`** - one of the six above.
- **`Conditions`** - see below. Empty means "always": that's the line's generic fallback.
- **`Text`** - the line, in English. **Text containing a comma needs "double quotes"**; a row with
  more fields than the header is refused rather than silently cut short.
- **`Weight`** - a whole number, 1 or more, default 1. Only ever breaks a full tie (below).
- **`Cooldown`** - seconds before the *same speaker* may say this line again, default 0. Another
  speaker may say it meanwhile: two guards can say the same thing.
- A row whose `Id` starts with `#` is a comment. Blank rows are skipped.

### Conditions

A list of clauses separated by `;`. **Every clause must hold.** There is no "or": write two rows.

```
Speaker.Role == guard; StandingWithSpeaker <= -20; Listener.Faction != Raiders
```

- A clause is a fact, a comparison (`==` `!=` `<` `<=` `>` `>=`) and a value or a second fact.
- Values are numbers (`-20`, `0.35`), ids (`guard`, `Raiders`), "quoted text" (`"Merchant Ada"`),
  or `true` / `false`. Names compare case-insensitively.
- Names and yes/no values only compare with `==` and `!=`.
- `None` means "has none" - `Speaker.Faction == None` is an unaffiliated speaker.
- A fact about someone the moment doesn't have (a Hurt from nobody) never matches, even with `!=`.

### The facts

For each of **`Speaker`**, **`Listener`** and **`Event.Victim`**:

| Fact | Kind | Reads |
|---|---|---|
| `.Definition` | name | the character definition id (`Bandit`, `Trader`, `Settler`), or `None` |
| `.Role` | name | the definition's `RoleId` (`trader`), or `None` |
| `.Faction` | name | the record's faction id (`Raiders`, `TradersGuild`), or `None` - squad members are unaffiliated |
| `.Name` | name | the name the game shows; quote it if it has a space |
| `.LifeState` | name | `Alive`, `Downed` or `Dead` - nothing else is accepted |
| `.Health` | number | health as a fraction, 0 to 1 |

Plus these, about the player whose squad it is:

| Fact | Kind | Reads |
|---|---|---|
| `StandingWithSpeaker` | number | this player's standing with the speaker's faction, -100 to 100, 0 when the speaker has none |
| `Gold` | number | how much gold this player has |
| `Flag(name)` | yes/no | true once this squad's conversations have set that flag (`<<SetFlag name>>`) |
| `Seen(conversation)` | yes/no | true once this squad has had that conversation - its title (`Seen(Shakedown)`, any package) or `package.title` |

Each event carries only some of those people, and a condition asking about anyone else is refused:

| Event | Carries |
|---|---|
| `Hurt` | Speaker, Listener, Player (Listener and Player only when a squad member did the hitting) |
| `Downed` | Speaker |
| `WitnessedDeath` | Speaker, Event.Victim |
| `TradeOpened` | Speaker, Listener, Player |
| `NothingToSay` | Speaker, Listener, Player |
| `Approached` | Speaker, Listener, Player |

`StandingWithSpeaker`, `Gold`, `Flag` and `Seen` need the Player, so they work in `Hurt`,
`TradeOpened`, `NothingToSay` and `Approached` - and in every conversation.

**Give an `Approached` line a cooldown of a minute or so.** The event itself already waits
`ApproachCooldownSeconds` per player; the row's cooldown is per speaker across every player, so it
is what stops one NPC greeting each squad that passes with the same words.

### How a line is picked

1. Every line for the event whose conditions all hold, and which this speaker isn't waiting out a
   cooldown on, is a candidate.
2. **The candidate with the most clauses wins.** A three-clause line beats a two-clause one.
3. A tie goes to the line **said least recently, by anyone** - a line never said beats one that was.
4. A tie after that is a weighted pick.
5. If nothing is left, nothing is said.

So writing a more specific line is how to override a general one - and how a mod changes what
players hear without touching the base game's files.

### A conversation file

A conversation is written in **Yarn** - Yarn Spinner's own language, with its own editor tooling
(its VS Code extension highlights and checks a file as you type). The example mod's bandit
shakedown, trimmed:

```
title: Shakedown
kind: Greeting
attach: Speaker.Definition == Bandit
---
<<declare $asked_about_road = false>>
Bandit: Toll road. Twenty gold, or you walk back the way you came. #line:shakedown_toll
<<jump Choices>>
===

title: Choices
---
-> Pay the toll. <<if gold() >= 20>> #line:shakedown_pay #reason:not_enough_money
    <<TakeMoney 20>>
    <<SetFlag example_paid_toll>>
    Bandit: Pleasure doing business. Road's yours. #line:shakedown_paid
-> Who says it's your road? <<if not $asked_about_road>> #line:shakedown_ask
    <<set $asked_about_road to true>>
    Bandit: The twelve of us in those rocks say so. #line:shakedown_twelve
    <<jump Choices>>
-> Get out of my way. #line:shakedown_refuse
    <<ChangeStanding Raiders -10>>
    Bandit: Wrong answer. #line:shakedown_wrong
===
```

- **Compile it** with `ysc compile <name>.yarn -o . -n <name>` (Yarn Spinner Console **3.2.2**,
  below) and ship the three files it writes beside the `.yarn`. The game reads all four: the
  compiled program holds only ids, the lines file holds the text, and the `.yarn` itself is how a
  problem names the line you wrote. **Recompile after every edit** - in the editor a `.yarnc` older
  than its `.yarn` is refused, and anywhere a `#line:` the lines file doesn't have is.
- **Every line needs a `#line:` id** - `ysc tag <name>.yarn` stamps one on every line without, so
  you never type them. ysc would make one up for an untagged line, but that id changes whenever
  the line moves - orphaning its translations and any voice recording - so the game refuses it.
  Ids are letters, digits and `_`, unique within the package (barks included).
- **A node with a `kind:` header is a conversation**, and its title is its id
  (`example.Shakedown`). A node without our headers - `Choices` above - is an ordinary node a
  conversation jumps into. The headers go under `title:`:

  | Header | On | Means |
  |---|---|---|
  | `kind:` | every conversation | `Greeting` (what an NPC opens with), `Topic` (offered after a greeting), `Ambient` (banter, no window) |
  | `attach:` | Greeting and Topic - required | who it belongs to, as a condition about the **Speaker** only: `Speaker.Definition == Bandit`, `Speaker.Role == trader`, `Speaker.Faction == TradersGuild` |
  | `requires:` | any | what else must be true right now - any condition over Speaker, Listener and Player: `Flag(trader_mentioned_salt_road)`, `StandingWithSpeaker > -20` |
  | `priority:` | any | a whole number, default 0. The highest eligible greeting wins; topics are listed highest first |
  | `once:` | any | `true`: never again for a squad that has seen it |
  | `label:` | Topic - required | the choice that offers it, in plain words (`label: Any chance of a better price?`). Translated under the id `<title>_label` |
  | `participants:` | Ambient - required | 2 to 4 names its lines are said under (`participants: First, Second`) |

  In a conversation the **Speaker** is the NPC and the **Listener** is the squad member talking;
  in banter they are the first and second participants.
- **Who says a line** is the name before its colon. In a window conversation `You:` is the squad
  member talking, any other name is the NPC (the window shows their real name, so `Bandit:` is a
  note to yourself), and no name is narration. In banter every line starts with one of the
  participants.
- **Questions are functions.** Every fact is one, with each `.` written as `_`:
  `speaker_faction()`, `listener_definition()`, `standingwithspeaker()`, `gold()`,
  `flag("name")`, `seen("Shakedown")`. Case doesn't matter, and text compares case-insensitively
  (`speaker_faction() == "raiders"`). Yarn can't call a dotted name - it reads one as an enum.
- **Effects are commands** - the only way a script changes anything:

  | Command | Does |
  |---|---|
  | `<<SetFlag <flag> [true\|false]>>` | sets a flag this squad remembers (`false` clears it); `Flag(name)` reads it back |
  | `<<ChangeStanding <faction> <amount>>>` | moves this player's standing with a faction (`-10`, `5`), kept within -100..100 |
  | `<<TakeMoney <amount>>>` | takes gold, all or nothing - guard its choice with `<<if gold() >= amount>>` |
  | `<<GiveMoney <amount>>>` | gives gold |
  | `<<OpenTrade>>` | opens the NPC's shop - with a `TradeOpened` bark, as Talk on a trader always had - and **ends the conversation**: the trade screen replaces the window, and nothing after it runs. Window conversations only |

  Flag names are shared by every package, so a mod can read a flag the base game sets; prefix
  your own with your mod's id (`example_paid_toll`).
- **A choice that can't be taken** is one with an `<<if>>` that fails. Tag it `#reason:<key>` and
  it shows greyed out with that reason; leave the tag off and it isn't offered. The keys:
  `not_enough_money` (*Not enough gold*). A `#reason:` goes on a choice only.
- **Yarn's own `$variables` last one conversation** - each playthrough gets a fresh set, so one
  squad's `$asked_about_road` is never another's. Anything the squad should remember is a flag.
- **What Yarn can use here**: its operators, `visited()` and `visited_count()` (which count within
  one playthrough), `random()`, `random_range()`, `dice()`, `round()`, `floor()`, `ceil()`,
  `int()`, `min()`, `max()`, and `<<jump>>`, `<<detour>>`, `<<declare>>`, `<<set>>`, `<<if>>`.
  **Not**: `<<wait>>` or any command that isn't an effect, markup (`[b]`), Yarn Spinner's own
  saliency (`when:` headers - our `attach:`/`requires:` choose instead), and node groups.
- **Banter can't offer choices or `<<OpenTrade>>`** - nobody is at a window - and its lines can't
  insert values (`{$x}`) yet.
- **Voice**: every line is one fixed sentence with one permanent id, so a future recording is
  filed under it. Keep lines meant for voice free of inserted `{$values}`.

### Translations

`localization/<culture>/<file>.csv`, with columns `Id,Text`. The id is the line's own id in the
same package, written plain - a bark's id, a conversation line's `#line:` id, or a topic's
`<title>_label`. A conversation line's `Name: ` in front may be kept or left off; either way it
isn't shown as words. A package translates only its own lines. `fr` covers `fr-CA` too.

### When something is wrong

Nothing broken stops the game. **A bad row costs that row**; a bad manifest, a missing requirement
or a `Requires` cycle costs that package (and anything that needs it). Every load logs one line per
package (`Dialog: mod lanterns 1.0.0: loaded, 12 barks, 0 translations, 3 errors, 0 warnings`), then
every problem with its file and line:

```
error: core barks/core.csv:14: in 'Speaker.Rol == guard': 'Speaker.Rol' isn't a fact the game knows - SmoresDialogReport facts lists them - row skipped
```

A faction, definition or role id that no loaded content has is a **warning** - kept, because a mod
may name content from another mod that isn't installed. In the base game's own files it can only be
a typo, so the content sweep fails on it.

**A conversation problem costs that conversation** - or the file, when the file itself can't be
used (missing compiled files, a garbage `.yarnc`, a stale compile). Checked at load time, with the
line you wrote: a line without a `#line:` id; a function that isn't a fact, or called with the wrong
number of values; a command that isn't an effect, with the wrong number of words, or a bad amount;
an unknown `#reason:` key; a bad or missing header; a speaker who isn't a participant, a choice or
`<<OpenTrade>>` in banter. A plain node with a problem costs every conversation that plays it:

```
error: example conversations/shakedown.yarn:31: node 'Choices': runs <<TakeMony>>, which isn't an effect the game has - the effects are SetFlag, ChangeStanding, TakeMoney, GiveMoney, OpenTrade
error: example conversations/shakedown.yarn:18: conversation 'Shakedown' was skipped: it plays node 'Choices', which has the errors above
```

## Core Rules

- **Dialog is a definition, in the game-data sense** (`game-data.md`): authored, identical on every
  machine, never written in play. It is plain text rather than a `.uasset`, which is the whole
  point - a writer or modder needs a text editor, not Unreal. What a bark has *done* this session
  (when each line was said, by whom) is transient, lives in the bark director, and is never saved.
- **The base game is package #0.** `core` is loaded by exactly the code that loads a mod. There is
  one path, so modders get everything our writers get.
- **Order**: `core`, then mods so that everything a mod `Requires` loads first; among mods ready at
  the same time, alphabetical by id, so the order never depends on the file system.
- **Mods only add, in v1.** Nothing can delete or replace a `core` line; a more specific line wins
  instead.
- **Ids are local to a package and qualified by the loader.** Collisions between mods are
  impossible by construction rather than by convention.
- **Only register a fact something can actually answer** (the refusal-reason rule from
  `refusals-and-feedback.md`). The registered list is the writers' reference; a fact nothing can
  answer would be a promise the game doesn't keep. No lineage, no skills, until they exist.
- **Conditions are checked at load time, never discovered wrong in play.** An unknown fact, a type
  mismatch, a subject the event doesn't carry, a misspelt `LifeState`, the same clause twice (which
  would count twice toward specificity) - each is an error with a line number.
- **One director per session, on the server.** `UBarkDirectorComponent` sits on
  `AStrategyGameState` and runs only with authority. Every player within hearing reads the *same*
  line, because one person said it - so selection is not per player, and neither is the memory.
- **Dialog crosses the network as ids, never text.** The server sends `core.trade_generic` through
  `Client_NotifyBark`; each client looks it up in its own loaded, translated library. A client
  missing the id (host and client running different dialog) logs it and posts nothing.
- **Dialog reads faction standing and faction identity, and still nothing reads them for
  behaviour.** `StandingWithSpeaker` chooses what a trader says to you, not whether they trade.
  Hostility is unchanged (`combat.md`) - and no effect sets it (a `StartFight` effect waits on the
  AI roadmap; hostility is derived, never set).
- **The server runs every conversation; a client only ever sees ids.** The Yarn player, the
  choice of greeting, the topic list and every effect run on the server, in
  `UConversationComponent` on the player's controller. Each line and choice reaches the owning
  client as its id, and the client looks the words up in its own translated library - the barks'
  rule, so each co-op player reads their own conversation in their own language. The pick comes
  back as a position in the list the client was shown, and the server checks it.
- **Our condition language chooses; Yarn plays.** Which conversation happens, and which topics
  are offered, is decided by `attach:` / `requires:` / `priority:` / `once:` in our language, by
  our code - Yarn Spinner's own saliency (`when:`) is not used. Inside a conversation, flow is
  Yarn's, and it reads facts through functions.
- **Dialog state is squad-scoped.** `UDialogMemoryComponent` on `AStrategyPlayerState` holds this
  player's flags and the conversations they have seen; nothing is world-level. Yarn's own
  `$variables` last one playthrough.
- **Effects are the only way a script changes anything, and every one is authority-only**,
  refusing with nothing changed off-authority (`FDialogEffectRegistry::Run` checks once, for all of
  them). An effect that can fail (`TakeMoney`) is all-or-nothing, and its choice is guarded so it
  shows greyed *before* it could fail - preventing the refusal beats explaining it
  (`refusals-and-feedback.md`). A script that forgets the guard is refused, and the player told why.
- **Each playthrough is its own; the loaded script is shared.** Two squads at the same NPC are two
  players over one loaded program, each with its own place and variables. Nothing locks an NPC.
- **Selection is Hades-style**: of the eligible, the highest priority; a tie to the one least
  recently seen by this squad (never seen beats seen); then load order - so it never depends on
  anything but the files and the squad's memory. A conversation counts as seen the moment it
  starts, so walking off halfway still uses up a `once:` one.

## Localization - how runtime-loaded text gets translated

The least-trodden part of the design, and Slice 1's proof of it: `SmoresSetCulture fr` in PIE, then
talk to Merchant Ada, and her greeting is French.

- **Each loaded package's lines become a runtime string table** - table and text namespace
  `SmoresDialog.<package>`, key = the qualified line id, source string = the file's English. A bark's
  `FText` is `FText::FromStringTable(...)`, and a string-table entry asks the localization manager for
  its display string every time it is drawn.
- **`FDialogLocalizedTextSource` answers that question.** It is an `ILocalizedTextSource` registered
  with `FTextLocalizationManager` on the first load, and whenever the culture changes the manager
  asks every source again - so a bark switches language with no code of ours watching the culture.
- **It supplies every line on every load, translated or not.** The manager's live table is only ever
  added to or overwritten, never cleared, so a culture the source said nothing about would keep
  showing the *previous* culture's words. Switching back from French works only because English
  "translations" (the source text) are supplied too.
- **This deviates from the roadmap's "one string table per package per culture"** - one table per
  package, plus the text source for every culture. Separate per-culture tables would each be a
  fixed language; a string-table entry routed through the manager is what switches live.
- **Conversation lines and topic labels are published the same way**, into the same package
  table, keyed by their qualified ids (`example.shakedown_toll`, `core.TraderPrices_label`). A
  conversation line's speaker cue (`Bandit: `) is taken off before it becomes the source string,
  and off a translation too, so it is never shown as words.
- **In the editor**, game text normally shows in its source language (`ShouldLoadNativeGameData`),
  and `SmoresSetCulture` uses the engine's *game localization preview* rather than changing the
  editor's language. The preview refuses to start without a native game culture, which the project
  has no localization target to supply - so the text source reports `en` as the game's native
  culture, at low priority so a real target would win. In a game build it is a plain
  `SetCurrentLanguage`.

## C++ Implementation

All in `SmoresDialog` (`Source/SmoresDialog/`), except the controller, PlayerState and GameState
glue, and the bubbles and conversation window, which are HUD and live in `SmoresUI` (below).

| File | Holds |
|---|---|
| `DialogTypes.h` | `EBarkEvent`, `EDialogSubject` (who a moment carries), `FDialogValue`, `FDialogContext`, `FDialogProblem`, and `SmoresDialog::GetEventSubjects` - the table above |
| `DialogFacts.h` | `FDialogFact`, `FDialogFactRegistry` (a value type; `MakeBuiltIn()` is the game's list), `FDialogKnownIds` (the content ids literals are checked against, gathered from the Asset Manager) |
| `DialogCondition.h` | The condition language: `SmoresDialog::CompileCondition` (tokenize, parse and check in one pass), `FDialogCondition::Evaluate` |
| `DialogLibrary.h` | `FBarkLine`, `FDialogTranslation`, `FDialogPackageInfo`, and `FDialogLibrary` - everything loaded, indexed by id and by event, plus every problem |
| `DialogLoader.h` | `SmoresDialog::ParseCsv`, `GatherPackagesFromDisk` and **`LoadPackages` - the in-memory entry point**: packages built from strings in, a library out, no disk, no string tables, no world |
| `BarkSelection.h` | `FBarkMemory` (when each line was said, by whom) and `SmoresDialog::SelectBark` - pure, reads the memory and never writes it; `FBarkSelection` is the account `SmoresTestBark` prints |
| `DialogText.h` | `FDialogLocalizedTextSource`, `PublishDialogText` (string tables + text source), `GetPublishedLineText`, `SetDialogCulture` / `EndDialogCulturePreview` |
| `SmoresDialogSubsystem.h` | `USmoresDialogSubsystem`, a `UGameInstanceSubsystem`: gathers from disk, loads, publishes the text, logs the report, broadcasts `OnLibraryLoaded`. Survives map changes and exists on every machine |
| `BarkDirectorComponent.h` | `UBarkDirectorComponent` (server-only, on the GameState) and `UBarkUnitWatcher` - one per unit, the `USquadActivityWatcher` pattern, because three of `UHealthComponent`'s four delegates can't say whose they are. Also the approach timer (`CheckApproaches`) and its knobs `ApproachRange` (800 cm), `ApproachCooldownSeconds` (60) and `ApproachCheckSeconds` (0.5) |
| `ApproachTracker.h` | `FApproachTracker` - `Approached`'s edge-trigger as a plain value type: given where every NPC and every squad member stands, which squads just arrived. Per NPC per player; see "How `Approached` is raised" |
| `DialogHost.h` | `IDialogHost` - what dialog needs a player's controller to do: `IsSquadMemberWithin`, `DeliverBark` (line id, speaker, speaker's name), `OpenTradeWith` (the `OpenTrade` effect) and `NotifyDialogRefusal`. The `IStrategySelectionHost` pattern |
| `DialogConversationTypes.h` | `EConversationKind`, `EConversationEndReason`, `FConversationDefinition` (one loaded conversation: its headers, compiled, and a shared pointer to its file's script) and the two structs that cross the network, `FConversationLineView` / `FConversationChoiceView` - ids, never text. Not `ConversationTypes.h`: an engine plugin (CommonConversation) has that name, and UHT refuses two headers of one name |
| `ConversationScript.h` | `FDialogConversationScript` (one compiled `.yarn`: the `FYarnProgram` and what the loader learned about each line - qualified id, speaker cue, text, whether it is a choice, its `#reason:`). **The one header with a Yarn type in it**, included only by `SmoresDialog`'s own `.cpp` files - see Registration |
| `ConversationPlayer.h` | `FDialogConversationPlayer` - one playthrough, driving the plugin's VM (a pimpl, so no Yarn type in the header). Facts and effects come in through `FConversationPlayerSetup`, so a test drives it with stand-ins. Also `FindYarnBuiltIn` (our operators and built-ins), `GetYarnFunctionName` / `FindFactForYarnFunction` (`Speaker.Faction` is `speaker_faction()`), `SplitSpeaker`, `GetSpeakerSlot` |
| `ConversationLoader.cpp` | `LoadDialogConversations`, called by `LoadPackages` for each package after its barks: reads each `.yarn` and its three files, checks everything (below), and adds the conversations and their texts to the library. `DialogLoaderInternal.h` holds the loader helpers it shares with `DialogLoader.cpp` |
| `ConversationSelection.h` | Pure: `IsConversationEligible`, `GetEligibleConversations` (the topic list), `SelectConversation` (the greeting), and `GetConversationInterruption` over an `FConversationWatch` (range, down, fighting) |
| `DialogEffects.h` | `FDialogEffect` / `FDialogEffectRegistry` (a value type; `MakeBuiltIn()` is the five effects), `FDialogEffectContext`, `FDialogEffectOutcome`; `FindChoiceReason` - the `#reason:` keys, each an `ESmoresRefusalReason` |
| `DialogMemoryComponent.h` | `UDialogMemoryComponent` (on `AStrategyPlayerState`) around `FDialogMemoryRecord` - flags, and per conversation how often and how recently it was seen (a counter, not a time, so it survives a save). Server-owned, not replicated, authority-gated |
| `ConversationComponent.h` | `UConversationComponent` (on `AStrategyPlayerController`) - the server side of a window conversation (selection, the Yarn player, the topic list, effects, the break-off timer) and the owning client's `FConversationView` the window draws, with `OnViewChanged` |
| `BanterDirectorComponent.h` | `UBanterDirectorComponent` (on `AStrategyGameState`, server-only) - when each squad banters, casting, and playing it through the bark director's delivery |

In `SmoresUI`:

| File | Holds |
|---|---|
| `BarkBubbleSchedule.h` | `FBarkBubbleTiming` (the lifetime formula), `FBarkBubbleSchedule` (one bubble per speaker, the clock, the fade, expiry) and `SmoresBarkBubbles::StackBoxes` - all plain, all tested |
| `BarkBubbleLayerWidget.h` | `UBarkBubbleLayerWidget` - the full-screen, click-through layer: projects each speaker's head through the owning player's view every frame, stacks, and places a pooled `UBarkBubbleWidget` per bubble |
| `BarkBubbleWidget.h` | `UBarkBubbleWidget` - one bubble: a bound `LineText` and the wrap width (`MaxTextWidth`, 260) |
| `ConversationWidget.h` | `UConversationWidget` - the conversation window, a `UWindowWidget`. Draws the owning player's `FConversationView`, rebuilt the frame after it changes; every click is a request to `UConversationComponent`. Its X is Goodbye |
| `ConversationChoiceWidget.h` | `UConversationChoiceWidget` - one choice row, the `UTargetActionWidget` shape and bound names: a button, the label, and the reason while it is greyed (worded by `URefusalWidget::GetRefusalText`) |

### How a bark travels

1. **An event.** Health: `UBarkUnitWatcher` hears `OnDamaged` / `OnDowned` / `OnDied` on every unit
   (found at the director's `BeginPlay`, on `OnActorSpawned`, and in each level streamed in through
   `LevelAddedToWorld`). Interaction: `InteractWithNPC` calls `Server_RaiseInteractionBark`, which
   re-checks the gates server-side and decides `TradeOpened` or `NothingToSay` itself.
2. **Selection.** `UBarkDirectorComponent::SayIfAny` checks the speaker's quiet time, then
   `SelectBark` over the event's lines. `WitnessedDeath` tries each ally within `WitnessRange`
   (15 m), nearest first, until one has a line. Allies are the same player's squad, or the same
   non-empty faction - two unaffiliated strangers are not allies.
   Approach: the director's own timer (below).
3. **Delivery.** `Deliver` asks every player controller, through `IDialogHost`, whether any of its
   squad is within `HearingRange`, and calls `DeliverBark` on those that are.
   `AStrategyPlayerController` forwards to `Client_NotifyBark(LineId, Speaker, SpeakerName)`. The
   speaker travels as an actor reference, so a client the speaker isn't relevant to receives null.
4. **Display.** On the client, `USmoresDialogSubsystem::GetLineText` resolves the id through the
   string table, and `PostActivity(Comms, ...)` puts it in the feed. If the speaker resolved, the
   same `FText` goes to `AStrategyHUD::ShowBarkBubble` -> `UStrategyUI` -> `UBarkBubbleLayerWidget`,
   the controller-to-HUD direction `ToggleActivityFeed` already uses, so no interface.

A line is recorded as said (cooldowns start) whether or not anyone was in earshot - the speaker
said it; nobody happened to hear.

### How `Approached` is raised

- **`UBarkDirectorComponent::CheckApproaches`** runs on a looping world-time timer
  (`ApproachCheckSeconds`, 0.5 s), server-only. One `TActorIterator<AStrategyUnit>` pass builds the
  tracker's input: every unit as a possible speaker (on their feet or not, squad member or not), and
  each player's squad - every `AStrategyPlayerUnit` grouped by its owning controller's player state,
  **downed members included**, so a squad that went down together hasn't "left" and isn't greeted
  as newcomers when it gets up.
- **`FApproachTracker::Update`** keeps, per (NPC, player), whether that squad was inside
  `ApproachRange` last time and when the NPC was last approached by them. An approach is the squad
  going from outside to inside, to an NPC on their feet, with `ApproachCooldownSeconds` passed
  since the last one; the listener is the nearest member inside. It remembers only pairs that
  differ from "outside, free to fire", and forgets any NPC or player missing from an update.
- **Each approach goes through `RaiseEvent`** with the NPC, the listener and the player -
  quiet time and line cooldowns included. It counts as used the moment it is raised, whether or
  not a line came out.
- **Straight-line distance is the seam.** Real perception (`ai-and-behavior.md`'s Awareness)
  replaces how "inside" is decided; the tracker's rules don't change.

### How a bubble is drawn

- **`UBarkBubbleLayerWidget::ShowBark`** hands the line to `FBarkBubbleSchedule::Show`, which
  replaces the speaker's bubble or appends one. Nothing is drawn yet.
- **`RefreshBubbles`**, pushed every frame from `AStrategyHUD::DrawHUD` like every other region:
  prunes expired bubbles (and any whose speaker is gone); projects each speaker's head - actor
  location plus the collision half-height plus `HeadClearance` (40 cm) - through
  `UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition`; skips anything behind the camera or
  off the layer; builds a box per bubble, centred over the speaker with its bottom edge there;
  runs `StackBoxes` in the order speakers first spoke; then moves each widget and sets its opacity.
- **Widgets are pooled.** The Nth widget draws the Nth visible bubble and the rest are collapsed;
  a widget only re-lays itself out when its bubble's `Serial` changes, i.e. a new line. Position
  and opacity are compared before they touch Slate, per `hud-and-panels.md`'s per-frame rule.
- **The clock is `FPlatformTime::Seconds()`**, wall-clock like the feed's timestamps.
- **The layer and every bubble are `HitTestInvisible`** - the one deliberate exception to the HUD's
  click-shield rule, written up in `hud-and-panels.md`.

### How a conversation runs

1. **Talk.** `InteractWithNPC` runs the client-side gates (hostile, reach), then
   `Server_InteractWithNPC` re-runs them and decides: `UConversationComponent::TryStartGreeting`
   selects over every Greeting (`SelectConversation`, with this player's memory); failing that a
   trader gets `Client_OpenTrade` and a `TradeOpened` bark; anyone else a `NothingToSay` bark.
2. **Start.** `StartConversation` ends any other conversation this player had, records both sides'
   health (a loss later means somebody got hurt), sends `Client_Begin` (both actors and names) and
   starts the watch timer. `RunScript` marks the conversation seen and builds an
   `FDialogConversationPlayer` over the shared script: facts from the dialog subsystem's registry
   with Speaker = NPC, Listener = squad member, Player = this player state; commands to
   `RunEffect`.
3. **Each step.** `PresentScript` sends where the script got to: a line (`Client_ShowLine`, its id
   and speaker slot), or the choices it offers that are **shown** (`Client_ShowChoices` - enabled or
   greyed with a reason; `ShownChoices` maps each back to the player's own position). The client's
   `Server_Continue` / `Server_Choose(index)` move it on; a pick is echoed first
   (`Client_ShowPicked`, said by the squad member), so what it does lands in the feed after it.
4. **Effects.** `RunEffect` runs the command through `FDialogEffectRegistry::Run` with the
   conversation's people and the controller as `IDialogHost`. A feed line goes back through
   `Client_PostOutcome`; a refusal through `NotifyDialogRefusal`. `OpenTrade` goes through
   `IDialogHost::OpenTradeWith` - `Client_OpenTrade` and a `TradeOpened` bark addressed to the
   squad member talking, so the barks written for a shop opening still play - and ends the
   conversation (`OpenedTrade`) once the script stops.
5. **The topic list.** When a script ends, `ShowTopicMenu` sends every eligible Topic
   (`GetEligibleConversations` over this NPC, rebuilt each time) plus Goodbye; a pick runs that
   topic's script, and its end comes back here.
6. **The end.** Goodbye, `Server_Leave`, a new Talk, a reload (`OnLibraryWillReload`), or the
   0.25 s world-time watch (`WatchConversation` -> `GetConversationInterruption`: someone gone or
   more than `BreakOffRange` apart, someone down, the NPC hostile, somebody with an attack target,
   somebody hurt) calls `EndConversation`, which sends `Client_End(reason)`.

**On the client** each RPC updates `FConversationView` - the transcript (every line, resolved into
this machine's language, also posted to the COMMS feed), Continue or the choices, and
`bWaitingForServer` while a request is out - and fires `OnViewChanged`.
`AStrategyPlayerController::HandleConversationViewChanged` adds `WBP_Conversation` to the viewport
while `bOpen` and removes it when not; the window marks itself dirty and rebuilds on its next
tick, so the button just clicked is never destroyed inside its own click. Its X
(`HandleWindowClosed`) and `T` (`TalkKeyPressed`) are `RequestLeave`.

### How a conversation is loaded and checked

`LoadDialogConversations`, for each `conversations/*.yarn` in a package, after its barks:

1. **Its three files, or nothing.** No `.yarnc` or `-Lines.csv` beside it costs the file; so does a
   `.yarnc` the plugin's `FYarnProtobufParser` can't read. In editor builds a `.yarnc` whose
   timestamp is older than the `.yarn`'s costs the file (a packaged build's times are staging
   times). A `.yarnc` with no `.yarn` is a warning, ignored.
2. **The lines table.** Each row's id must appear as `#line:<id>` on the line the table says it came
   from in the `.yarn` - that is how an id ysc made up (`line:57260121`) is caught. The id is
   qualified by the package, checked against every id the package already has (bark ids
   included), and its text split into speaker cue and words. Every `#line:` in the `.yarn` must be
   in the table, or the `.yarn` changed after the compile and the file is refused.
3. **The program, node by node.** Every `CallFunction` must be an operator, a built-in or a fact
   (never one about `Event.Victim`), called with the number of values it takes - read from the
   `PushFloat` the compiler puts before each call. Every `RunCommand` is split the plugin's way
   (`FYarnCommand::ParseCommandText`) and must be an effect with the right number of words, whose
   `Check` accepts the words written out (a `{substitution}` is only checkable when it runs). Jumps
   and detours are followed by name. A problem breaks the node.
4. **The metadata.** A `reason:<key>` tag must be a known key, on a choice; on a choice with no
   `<<if>>` it is a warning, since it can never show.
5. **The conversations**, in the order written: every node with one of our headers. Headers are
   checked (above), `attach:` compiled against the Speaker only, `requires:` against Speaker,
   Listener and Player; everything it reaches must be unbroken; and an Ambient one may not reach a
   choice, `<<OpenTrade>>`, a line said by anyone but a participant, or a line with a
   substitution. A problem costs that conversation, with an error naming the line - so a writer
   fixing one mistake never has to hunt for it.

Problems name the `.yarn` and a line in it: a line's own line for line problems, the header's for
header problems, and for a function or command the first line in its node that mentions it
(`FYarnSourceIndex` - the compiled program carries no line numbers).

### How the player is driven

What the spike learned, and what the player (`ConversationPlayer.cpp`) does with it:

- **Only the plugin's player is used:** `FYarnVirtualMachine`, `FYarnProtobufParser`, and
  `UYarnInMemoryVariableStorage` - one per playthrough, made with `NewObject` as a plain object and
  held by a `TStrongObjectPtr`. None of the plugin's runner, presenters, widgets or asset import:
  they assume the script runs on the machine that shows it. The VM copies the program it is
  given, so each playthrough holds its own copy of the (small) program over the shared script.
- **The operators are ours.** Compiled Yarn calls its operators as functions (`gold() >= 20` calls
  `Number.GreaterThanOrEqualTo`, `not` calls `Bool.Not`); the plugin registers those in its runner
  layer, so `FindYarnBuiltIn` supplies the Number, Bool, String and Enum families plus the
  built-ins listed in Writing Dialog. **One deliberate difference:** `String.EqualTo` ignores case,
  as names do in our condition language.
- **Facts are functions**, answered through `CallFunctionHandler` / `FunctionExistsHandler` /
  `FunctionParamCountHandler` from the fact registry: a number fact answers a Yarn number, a name
  a string (`None` for "has none", empty when unset), a yes/no a bool. Yarn can't call a dotted
  name - `Speaker.Faction()` is read as a member of an enum type called `Speaker` - so each fact is
  reached with its dots as underscores. A single `fact("Speaker.Faction")` bridge was rejected:
  ysc infers one return type per function, and one function answering both numbers and names
  won't compile.
- **ysc accepts any function or command name without a declaration**, and makes up a line id
  (`line:` + 8 hex digits) for an untagged line - hence the load-time checks.
- **The VM pauses after every line and every command** until `Continue()`. Every effect finishes at
  once, so a pause after a command is continued straight through (`Run`). An effect asking to end
  (`OpenTrade`) stops the VM from outside its handler.
- **A choice whose condition fails is never hidden by Yarn** - it arrives with `bIsAvailable`
  false, all alike. The player marks each `bShown` (available, or unavailable with a `#reason:`)
  and the component sends only those; `SetSelectedOption` still takes the position in the whole
  set. When no choice at all is shown, the script ends there rather than wait on a pick nobody can
  make.
- **Line ids arrive as the whole tag** (`line:shakedown_toll`) and are qualified by the package.
- **Initial values:** a `<<declare>>` value comes from the program whenever the variable store
  doesn't have that variable yet. `visited()` reads the variable the VM itself counts a node's
  visits into (`$Yarn.Internal.Visiting.<node>`), for nodes the compiler marked with a tracking
  header.
- **Custom node headers survive compiling** (`FYarnNode::Headers`), so our metadata lives on the
  node. ysc adds its own tags too - a `lastline` tag on the line before a set of choices - which
  the loader ignores.
- **The Lines CSV's `file` column** is an absolute path on the writer's machine; ignored.
- **Log noise:** the VM logs "Yarn VM: Running node ..." and, at the end of a script that jumped,
  "Return with no return address - treating as Stop", both at Log level. Both are harmless.

### How banter plays

- **`UBanterDirectorComponent::CheckSquads`**, on a 2 s world-time timer, server-only, groups every
  `AStrategyPlayerUnit` by its owner's player state and decides, per squad: fighting (a member with
  an attack target, or who lost health since the last look) stops any banter and starts the
  after-fight clock; `AfterFightSeconds` of calm after a fight, or `IdleSeconds` of nothing, tries
  one - if `CooldownSeconds` have passed since the last.
- **`TryBanter`** orders the Ambient conversations as selection does (priority, least recently
  seen, load order) and plays the first it can cast: `FindCast` shuffles the members on their feet
  (not the one at a conversation window), and takes the first pair within `CastRange` for whom
  `requires:` holds with them as Speaker and Listener; further participants only need to be near the
  first.
- **Each line** goes out through `UBarkDirectorComponent::Deliver` - every player with a squad
  member within `HearingRange` gets it in the feed and floating over the speaker - and the next
  follows after the line's reading time (the bubble's formula) plus `LineGapSeconds`, **in real
  time**: the world-time wait is scaled by the current time dilation, so 8x doesn't rush it and
  the paused tier doesn't freeze it. A participant down, gone or fighting ends it.

### Registration and packaging

- The module follows `unreal-module-organization.md`'s checklist: `SmoresDialog.Build.cs`, the root
  `SmoresDialog.cpp`/`.h` (with `LogSmoresDialog`), the `.uproject` entry, both `Target.cs` files, and
  `smores.Build.cs`. It depends on `SmoresCore`, `SmoresCombat` and `SmoresCharacters` publicly, and
  privately on `Json` (the manifests), `SmoresEconomy` (the wallet `gold()` and the money effects
  reach) and **`YarnSpinner`** (the conversation player). `SmoresUI` depends on it for the window.
- **The Yarn dependency is private, and must stay private.** No public header in `SmoresDialog`
  includes a Yarn header: `ConversationScript.h` is the one that does, and only `SmoresDialog`'s
  own `.cpp` files include it; everything public refers to a script as the forward-declared
  `FDialogConversationScript`, and the player hides its VM behind a pimpl. So nothing else in the
  game links against the plugin or needs its include paths.
- **`Config/DefaultGame.ini` stages `Content/Dialog` as-is**
  (`+DirectoriesToAlwaysStageAsUFS=(Path="Dialog")`). The cooker only cooks `.uasset`s; without
  this line a packaged build has no dialog at all, and says so only as one load error.
  The conversation files stage with it - `.yarn`, `.yarnc` and the two CSVs are all just files in
  the folder. Not yet re-checked in a package since Slice 3 (see Known Gaps).
  **Verified 2026-09-24**: a `BuildCookRun` Win64 Development package lists
  `smores/Content/Dialog/core/{mod.json, barks/core.csv, localization/fr/barks.csv}` in
  `smores-Windows.pak` (`UnrealPak <pak> -List`), and the packaged game logs
  `Dialog: base game Core 0.1.0: loaded, 27 barks, 6 translations, 0 errors, 0 warnings` - read
  through the pak by the same `IFileManager` calls the editor uses on loose files.
- `.gitattributes` marks `*.csv` and `*.yarn` as text and `*.yarnc` as binary.
- **`Config/DefaultEditorPerProjectUserSettings.ini` keeps the editor's auto-import out of
  `Content/Dialog`** (a `Dialog/*` exclusion on the `/Game/` `AutoReimportDirectorySettings`, beside
  the engine's own `Localization/*` one). Without it the editor sees new `.csv`/`.json` files in the
  content folder and offers to *import* them - which would make a DataTable asset of every bark
  file. If that prompt ever appears for dialog files, the answer is **Don't Import**.

### The Yarn plugin - how we carry it

- **Where:** `Plugins/YarnSpinner/`. This is Yarn Spinner for Unreal **3.2.8-alpha9**, a
  "pre-release", from `github.com/YarnSpinnerTool/YarnSpinner-UnrealEngine` at commit `a2c24dc`
  (2026-09-21).
  - It has two modules: `YarnSpinner` (runtime) and `YarnSpinnerEditor`.
  - It is a project plugin, so it is enabled without a `.uproject` entry.
  - Only `SmoresDialog` depends on it, privately - see Registration and packaging.
- **It is not in git.** `.gitignore` lists `Plugins/YarnSpinner/` while this repo is public. When
  Jim makes the repo private, delete that line and commit the plugin like any other code.
  - Until then, **a fresh clone won't build**. To put the plugin back: clone the repo above at that
  commit into `Plugins/YarnSpinner/`, then apply the patch list.
- **Our patch list.** Each change is marked in the source with a `smores` comment. Re-apply the
  list after updating the plugin, and re-check it after every engine upgrade:
  1. `YarnProtobufParser.h/.cpp` moved from `YarnSpinnerEditor` into the `YarnSpinner` runtime
     module (`Public/` and `Private/`), with the export macro changed from
     `YARNSPINNEREDITOR_API` to `YARNSPINNER_API`. The plugin only reads compiled programs during
     editor import, so without this a shipped game can't read a `.yarnc`.
  2. `YarnProjectFactory.cpp` now reads `FString CultureCode(CulturePair.Key);`, because 5.8's
     JSON keys no longer convert to `FString` implicitly.
  3. `YarnLocalization.cpp`: `SetSourceString` takes a third argument in editor builds. It is the
     same 5.8 change dialog Slice 1 hit.
  4. `YarnOptionsPresenter.cpp`: a local variable renamed to `CharacterMarkup`, because
     `CharacterAttribute` clashed with a global in `YarnMarkup.cpp` under a unity build.

  Two deprecation warnings remain (`FCoreDelegates::OnPostEngineInit` in
  `YarnSpinnerEditorModule.cpp`). They are warnings, not errors, for now.
- **Licence** (YSPL, `LICENSE.md` in the plugin folder). These apply however the repo is set up:
  - Credit Yarn Spinner visibly in the shipped game, next to the other middleware credits (s.3.1).
    Jim has agreed to.
  - Keep the plugin's copyright notice and licence file.
  - Anyone outside Jim's own company who works on the code, such as a contractor, must agree in
    writing to terms no less strict (s.4).
  - AI tools must not send the plugin's source for training (s.3.3). The Team plan's commercial
    terms cover that.
  - A private repo shared with your own team is explicitly allowed (s.3.4(b)). A *modified* copy
    may also be publishable as a "public fork" (s.3.4(c)), whose own examples are bug fixes and
    engine-compatibility updates. That is our reading, not legal advice. The plugin stays out of
    git until the repo is private, unless Jim decides otherwise.
- **The compiler:** `ysc` **3.2.2**, the version the plugin names, a self-contained Windows `.exe`.
  - On this machine it is at `Saved/DialogSpike/tools/ysc/ysc.exe`, which is not in git (the
    folder is left from the spike; the path is only a convention). To re-download it:
    `github.com/YarnSpinnerTool/YarnSpinner-Console/releases`, `ysc-win-3.2.2-*.zip`.
  - `ysc compile <file>.yarn -o <dir> -n <name>` writes the three files.
  - `ysc tag <file>.yarn` stamps a `#line:` id on every untagged line, so writers never type ids.
  - Writers write in Yarn Spinner's own tools (such as its VS Code extension). The compiled files
    are what a mod ships.

## Blueprint / Asset Dependencies

The directors, the memory and the conversation component need nothing: `UBarkDirectorComponent`
and `UBanterDirectorComponent` are native default subobjects of `AStrategyGameState`,
`UDialogMemoryComponent` of `AStrategyPlayerState` and `UConversationComponent` of
`AStrategyPlayerController`, so their Blueprints have them with no change, and the subsystem
creates itself.

The bubbles, all under `Content/Variant_Strategy/UI/`:

| Asset | Parent | Bound names |
|---|---|---|
| `WBP_BarkBubble` | `UBarkBubbleWidget` | `LineText`, inside a dark translucent `Border` |
| `WBP_BarkBubbleLayer` | `UBarkBubbleLayerWidget` | `BubbleCanvas` (the root canvas); plus the `BubbleWidgetClass` default, which must point at `WBP_BarkBubble` or barks reach the feed and nothing floats (it warns once) |
| `UI_Strategy` | `UStrategyUI` | `BarkBubbleLayer` - a `WBP_BarkBubbleLayer` stretched over the whole screen, painted beneath the six regions |

The conversation window, also under `Content/Variant_Strategy/UI/`:

| Asset | Parent | Bound names |
|---|---|---|
| `WBP_Conversation` | `UConversationWidget` | The window chrome every window has (`TitleBarDragHandle` with `TitleText` and `CloseButton`, `ResizeHandle`), `SpeakerNameText`, `PortraitImage` and `InitialsText` in one overlay, `TranscriptScroll` holding `TranscriptText`, `ContinueButton`, `ChoiceBox`; plus the `ChoiceWidgetClass` default, which must point at `WBP_ConversationChoice` or no choice can show (it warns once) |
| `WBP_ConversationChoice` | `UConversationChoiceWidget` | `ActionButton`, `LabelText`, `ReasonText` - the same names as `WBP_TargetAction` |
| `BP_StrategyPlayerController` | `AStrategyPlayerController` | the `ConversationWidgetClass` default -> `WBP_Conversation`. Empty, a conversation reaches the feed with no window, and the controller warns once |

## Content

| File | Holds |
|---|---|
| `Content/Dialog/core/mod.json` | the base game's manifest, id `core` |
| `Content/Dialog/core/barks/core.csv` | 33 barks for the three existing definitions (Settler, Bandit, Trader): every event has a generic line and at least one more specific one - for `Approached`, two generic, a trader's hawk (plus a Traders Guild one), a bandit's challenge and a hated-by-Raiders warning. Placeholder writing in the austere tone of `narrative-and-lore.md` |
| `Content/Dialog/core/localization/fr/barks.csv` | the six trader greetings in placeholder French - the localization proof |
| `Content/Dialog/core/conversations/trader.yarn` | `TraderGreeting` - any trader (`Speaker.Role == trader`): *Buying or selling?*, with a choice that opens the shop (`<<OpenTrade>>`). `TraderPrices` - a haggling-flavoured topic with no haggling in it (prices stay flat); one answer sets `trader_mentioned_salt_road`. `TraderBackstory` - the Trader definition's backstory, `requires: Flag(trader_mentioned_salt_road)`, `once: true`: standing in for recruit backstories until recruiting exists |
| `Content/Dialog/core/conversations/banter.yarn` | Two Settler banters: `SettlerBanterWater` (anytime) and `SettlerBanterAfterFight` (priority 5, when the first speaker's health is under 70%) |
| `Content/Dialog/core/localization/fr/conversations.csv` | The trader greeting's lines and both topic labels in placeholder French |
| `Mods/example/` | one bark (three clauses, more specific than anything in core) for a Traders Guild trader greeting a Settler, plus its French. Wins Merchant Ada's greeting whenever it's off its 30 s cooldown, without touching core. And two conversation files, proving a mod can give conversations to the base game's characters: |
| `Mods/example/conversations/shakedown.yarn` | `Shakedown` - every Bandit's greeting: the toll (`gold()`, `<<TakeMoney 20>>`, `#reason:not_enough_money`, `<<SetFlag example_paid_toll>>`), a question Yarn's own memory uses up, and a refusal (`<<ChangeStanding Raiders -10>>`). `ShakedownPaid` - priority 10, `requires: Flag(example_paid_toll)`: once a squad has paid, every Bandit waves it through |
| `Mods/example/conversations/guild.yarn` | `GuildTopic` - a topic attached to a whole faction (`Speaker.Faction == TradersGuild`), so Merchant Ada offers it after her greeting |

## Testing

49 tests - 45 dialog and 4 bark-bubble (`testing.md` has the run commands):

- `Smores.Dialog.Condition.*` (5) - parse and evaluate; each kind of mistake rejected with its
  reason; subjects an event lacks; unknown content ids as warnings; errors carrying their line.
- `Smores.Dialog.Barks.*` (4) - most specific wins, least-recently-said breaks ties before weight,
  weight breaks only full ties (fixed seed), cooldowns per speaker.
- `Smores.Dialog.Loader.*` (10) - CSV as spreadsheets write it, `Requires` ordering, a missing
  requirement, a cycle, duplicate ids, the same id in two packages, a broken row skipped while its
  neighbours load, bad manifests, translations, header columns in any order.
- `Smores.Dialog.Text.TranslationsFallBackInCultureOrder` - the text source, asked directly (never
  registered - switching the culture of the editor running the suite would change every other
  test's text).
- `Smores.Dialog.Facts.AnswerFromRealUnits` (in `smores`, which has the concrete unit stand-in) -
  every built-in fact read off real units, a real player state and a real condition.
- `Smores.Dialog.Approach.*` (5) - `FApproachTracker`: entering fires once (with the nearest member
  as listener) and standing inside never again; coming back waits for the cooldown *and* a leave; a
  downed NPC and a squad member never fire; each player and each NPC separately; memory stays
  bounded to who is near whom.
- `Smores.UI.BarkBubbles.*` (4, in `SmoresUI`) - the lifetime formula (floor, per character, cap);
  one bubble per speaker, a replacement restarting its clock and keeping its place; the fade and
  expiry, and a destroyed speaker's bubble dropped; overlapping bubbles stacking straight up while
  bubbles side by side stay put. Placement on screen and the look are PIE.
- **`Smores.Content.Dialog.CoreLoadsWithZeroProblems`** - the core content sweep: zero errors *and
  zero warnings* (a stale compile is an error, so it fails here too), every event with a bark, a
  generic line and a more specific one, a Greeting, a Topic and an Ambient conversation, every
  topic's label with text, no text mangled by a wrong encoding, at least one translation.
  `Smores.Content.Dialog.ExampleModLoadsCleanly` checks the example mod loads, still outranks core,
  and adds its three conversations.
- `Smores.Dialog.Conversation.*` (13):
  - the player - the four tests the spike proved it with, now through the real loader: the whole
    ask-then-refuse path (qualified ids, speaker cues, text, the used-up question left off, the
    command before its line); the pay choice greyed with `CannotAfford` when poor and `TakeMoney`
    when not; two playthroughs over one loaded script keeping separate places, memory and
    commands; a missing and a garbage compiled file each refused with a report while the package's
    barks still load;
  - loading - headers into metadata, texts with the cue off, the `#reason:` on its choice, a
    translation's cue taken off; each load check in `checks.yarn` refused at its own line while
    `Fine` loads; the Ambient rules; a stale compile by timestamp and by a `#line:` the table lacks;
    line ids sharing a package with its barks;
  - selection - priority, `once:`, `requires:` and `attach:`; the tie-break (never seen, then
    longest ago, then load order) and `Seen()` by title or qualified id; the topic list is exactly
    the eligible topics in order; and when a conversation breaks off (range, down, combat).
- `Smores.Dialog.Effects.*` (3) and `Smores.Dialog.Memory.IsPerPlayer` - on real components on a
  real player state: `TakeMoney` all-or-nothing, `GiveMoney`, and `gold()` reading the same
  wallet; every effect refusing off-authority with nothing changed; `ChangeStanding`, `SetFlag`
  and `Flag()`; and two players' memory staying apart, including which of them a
  `requires: Flag(...)` conversation is for.

Every dialog test except the two content sweeps builds its packages from strings and answers facts
from a map (`Tests/SmoresDialogTestFactory.h`), so a writer retuning a bark can't break one. The
conversation tests' compiled scripts are the one thing read from disk - the tests' own copies in
`Source/SmoresDialog/Tests/Conversations/`, never the game's content. **Recompile a fixture after
editing it**, like any `.yarn`. Everything the window and banter do on screen - placement, pacing,
the flow of topics - is PIE.

## Extension Points

- **A new fact**: register it in `FDialogFactRegistry::MakeBuiltIn` with its type, the subjects it
  reads, and (for a name) its content domain or fixed vocabulary. Only when something can answer it.
- **A new bark event**: a value on `EBarkEvent`, its row in `GetEventSubjects`, and a caller that
  raises it on the server through `UBarkDirectorComponent::RaiseEvent`. Add at least a generic
  line in `core.csv` in the same change - the content sweep requires one per event. `Approached`
  is the worked example of an event with no signal of its own: the director makes one on a timer.
- **Real perception for `Approached`**: replace how `CheckApproaches` decides a squad is inside
  (today straight-line distance). `FApproachTracker` takes "who is inside" as input and needn't
  change.
- **Something else that floats**: `UStrategyUI::ShowBarkBubble` takes any actor and any `FText`;
  banter reaches it through the bark director's `Deliver`.
- **New lines, a new translation, a new mod, a new conversation**: content only - see Writing
  Dialog.
- **A new fact is a Yarn function too**, with no extra work: conversations call it with its dots as
  underscores.
- **A new effect**: an `FDialogEffect` in `FDialogEffectRegistry::MakeBuiltIn` - its name, how many
  words it takes, a `Check` for the words at load time, and a `Run` (authority is already checked).
  If it needs the window, set `bNeedsWindow`; if it needs the controller, add to `IDialogHost`. The
  roadmap names the next ones: `Recruit`, `SetWage`, and a `StartFight` once hostility can be set.
- **A new `#reason:` key**: one row in `SmoresDialog::FindChoiceReason` and one in
  `GetChoiceReasonKeys`, naming an `ESmoresRefusalReason` - which may itself be new
  (`refusals-and-feedback.md`).
- **A new conversation kind** would be a value on `EConversationKind` and its rules in
  `ConversationLoader.cpp`'s header checks - but a quest's conversations are expected to be
  Greetings and Topics with quest facts, not a new kind.
- **Something else the director needs from a controller**: a method on `IDialogHost`.

## Known Gaps

- **Conversations and banter are judged only lightly.** Jim's PIE pass (2026-09-25, "looks
  great"): the shakedown played and a second bandit knew the squad had paid; the trader's greeting
  led into its topics; the window, centred, is fine for now; banter played after a short wait. No
  retuning asked for. More content and longer play may change the banter knobs (below) and the
  window's place.
- **Choices can't be picked with number keys.** A player-facing key goes through Enhanced Input
  (`input-and-keybinds.md`), and a new key's mapping is a hand step in the editor - so it waits for
  a pass that decides the keys. The window is mouse-only; `T` is Goodbye.
- **Only the NPC's portrait shows, and no unit has one**, so the window draws initials - the HUD's
  designed fallback. The squad member's name shows on their lines.
- **A conversation doesn't pause single-player.** An open design question (`dialogue.md`); until it
  is answered, the world runs on around the window.
- **Only the talking player sees a conversation.** Another player standing beside the NPC hears
  nothing of it - banter and barks are heard by everyone near; a conversation is private.
- **An effect's feed line is worded on the server**, in the server's language - the trade
  precedent (`Client_NotifyActivity`). The conversation's own lines are ids and translate per
  client.
- **Topic labels live in the node header**, so a label can't have a `#line:` id of its own; its id is
  derived from the node's title (`<title>_label`). Renaming a topic's node orphans its label's
  translations as well as its `Seen()` memory.
- **Yarn markup (`[b]`, `[wave]`) and `<<wait>>` aren't supported** - the text shows as written. The
  VM's own built-ins beyond those listed aren't answered either (`format`, `string`, `number`...);
  a script calling one is refused at load time.
- **Banter lines can't insert values** (`{$x}`), because they go out through the bark path, which
  carries only an id.
- **Banter's knobs are guesses**: `CooldownSeconds` 180, `IdleSeconds` 300, `AfterFightSeconds` 6,
  `CastRange` 10 m, `LineGapSeconds` 0.6 - all world time except the line gap. "Rare enough to feel
  earned" is Jim's call in PIE.
- **A packaged build hasn't been checked since conversations arrived.** The files stage with
  `Content/Dialog` (they are plain files in the folder), and the loader reads them through the same
  file calls; the Slice 1 packaging check hasn't been repeated.
- **The Yarn plugin is local-only while the repo is public** - see "The Yarn plugin - how we carry
  it". A fresh clone won't build without it.

- **How often barks fire is judged only lightly.** Jim's Slice 2 PIE pass (2026-09-25) saw
  approach barks from NPCs and the trader, squad members barking in a fight, and no repeats while
  standing near someone, and asked for no retuning. Bigger fights and more content may change
  that; the knobs are `ApproachRange`, `ApproachCooldownSeconds`, `SpeakerQuietSeconds`,
  `HearingRange`, each row's cooldown, and the layer's timing, `HeadClearance` and `StackGap`.
- **A packaged build can't switch to French yet.** `InternationalizationPreset=English` in
  `DefaultGame.ini` stages English culture data only, so `SmoresSetCulture fr` answers "this build
  doesn't know the culture". The proof is in PIE; a packaged build wants the preset (and
  `CulturesToStage`) widened the day a second language is real. The dialog files themselves stage
  regardless.
- **Every package's source text is assumed English.** A mod written in German has no way to say so.
- **A bubble ignores walls and terrain.** It is drawn over everything in the world, so a speaker
  behind a wall still shows their bubble where the wall is - the HUD-layer choice's trade-off, and
  the same through-walls honesty as hearing.
- **A stacked bubble can end up above someone else's.** Stacking only ever moves up, and the
  bubble shown first keeps its place, so a newer line from a speaker lower on screen can sit above
  an older one. PIE will say whether that reads.
- **First sight counts as an arrival.** A squad that spawns, or an NPC that streams in, already
  within `ApproachRange` gets greeted on the first check, because an unseen pair counts as outside.
- **An approach is used up whether or not a line came out.** An NPC in its quiet time when the
  squad arrives says nothing, and won't greet that squad until they leave and come back after the
  cooldown.
- **Only the trader greetings are translated.** The six French lines were the localization proof's
  whole scope, so after `SmoresSetCulture fr` the bandits and everyone else still speak English.
  Translating the rest is content work.
- **The tuning numbers are guesses**: `HearingRange` 20 m, `WitnessRange` 15 m,
  `SpeakerQuietSeconds` 6 s, `ApproachRange` 8 m, `ApproachCooldownSeconds` 60 s, every row's
  cooldown, and the bubbles' 2 s + 0.06 s per character, capped at 6 s. The director's are on the
  director; the bubbles' are on `WBP_BarkBubbleLayer`.
- **Hearing and approaching ignore walls and awareness.** Straight-line distance only, until
  perception exists (`ai-and-behavior.md`).
- **`WitnessedDeath` only happens through `SmoresKillNPC`**, since nothing in combat kills yet
  (`combat.md`). `SmoresTestBark WitnessedDeath <Name>` exercises it directly.
- **Whether a line already in the feed re-translates when the culture changes hasn't been
  checked.** New lines follow the culture; the proof never needed old ones to.
- **Host/client dialog mismatch is detectable, not enforced.**
  `USmoresDialogSubsystem::GetLoadedPackageVersions` is what a join check would compare; refusing
  the join is the session roadmap's job.
- **A spreadsheet saving CSV with `;` between fields** (Excel in some European locales) produces a
  file the loader can't read - the conditions already use `;`. Save as comma-separated.
- **A packaged build prints the base package as `Core`, not `core`.** Outside the editor an
  `FName` keeps the capitalization it was first created with, and the engine's own `Core` module
  got there first. Every comparison is case-insensitive, so nothing misbehaves - only the report
  and the qualified ids (`Core.trade_generic`) read differently. Keep ids as strings for display
  if it ever matters.
- **`SmoresReloadDialog` reloads this machine only.** In a real co-op session the host and every
  client would each need to reload; nothing forwards it.
