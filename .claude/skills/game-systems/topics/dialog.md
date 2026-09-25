# Dialog: the Loader, Facts and Barks

## Purpose

What characters say, and the machinery that picks it. Built by Slice 1 of
`Docs/roadmaps/dialog-roadmap.md`: dialog read from plain text files at startup (the base game
loaded exactly like a mod), a small condition language over a registered list of **facts**, and
**barks** - one-way lines picked by "most specific match wins" and delivered to the activity feed.
Floating bark text is the roadmap's Slice 2, and conversations, topics and banter its Slice 3; both
reuse everything here. Conversations are written in Yarn, and the player they will run on already
exists - see "Conversations: the Yarn player".

The design intent is `game-design`'s `dialogue.md`. This topic is how it is built.

## Player Surface

- **Barks appear in the activity feed**, on the COMMS tab (and LOG), in quotes, with the speaker's
  name as the line's source: *"Coin first. Questions never." - Merchant Ada*. Nothing floats over
  anyone's head yet - decided, and the roadmap's Slice 2 (see Known Gaps).
- **When someone barks** - five moments, all from signals the game already had:

  | Event | When | Speaker | Who they're speaking to |
  |---|---|---|---|
  | `Hurt` | took a hit and is still standing | the one hit | whoever hit them |
  | `Downed` | just went down | the one who went down | nobody |
  | `WitnessedDeath` | an ally died near them | the nearest ally still standing | nobody (the dead one is `Event.Victim`) |
  | `TradeOpened` | a squad member opened their shop | the trader | the squad member at the counter |
  | `NothingToSay` | a squad member tried to talk to someone with no shop | the NPC | that squad member |

- **Who hears it**: every player with at least one squad member within 20 m (`HearingRange`) of
  the speaker. In co-op two players nearby both get the line, each in their own language.
- **A speaker who just spoke stays quiet for 6 seconds** (`SpeakerQuietSeconds`), whatever the line.
  That is what stops a fight turning into a wall of "Ngh." Being *spoken to* ignores it: talking to
  someone always gets an answer if one of their lines is free.
- **Talking to someone changed.** Double-click, `T` and the target panel's Talk all reach the same
  verb. A non-trader used to do nothing at all; now they say a `NothingToSay` line. Out of reach
  now refuses with *Too far away* for a non-trader too, since a greeting is range-gated the same
  as a trade. A hostile NPC still refuses, unchanged.

### Debug execs

All on `AStrategyPlayerController`.

| Command | Does |
|---|---|
| `SmoresReloadDialog` | Re-reads every package from disk and logs the per-package summary. **The writer's loop**: edit a file, type this, hear the change without restarting. This machine only - every machine loads its own copy |
| `SmoresDialogReport` | Every package (id, folder, version, requires, loaded or SKIPPED, counts), barks per event, and every problem. `SmoresDialogReport facts` adds the writers' reference: every fact and who each event carries |
| `SmoresTestBark <Event> [Name]` | Fires an event on a unit and logs **every line considered and why it did or didn't win**, then the winner as this machine shows it, in the current culture. The unit is the one whose name contains `Name` (`SmoresTestBark TradeOpened Ada`), else the clicked NPC, else the first selected squad member. Cooldowns apply; quiet time doesn't. For `WitnessedDeath` the unit is treated as the one who died. Hops to the server |
| `SmoresSetCulture <culture>` | Shows game text in another culture - `SmoresSetCulture fr`, then `en` to return. In the editor it previews game text without touching the editor's own menus, and the preview ends when play stops |

## Writing Dialog

This section is the writers' and modders' reference.

### Where it lives

```
Content/Dialog/core/            the base game - package "core"
    mod.json
    barks/<any name>.csv
    localization/<culture>/<any name>.csv
Mods/<folder>/                  one folder per mod, next to the installed game
    mod.json
    ...the same shape
```

Every `.csv` under `barks/` is read, in file-name order. Anything else in a package folder (a README,
notes) is ignored.

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
- **`Event`** - one of the five above.
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

Plus **`StandingWithSpeaker`** (number): this player's standing with the speaker's faction,
-100 to 100, 0 when the speaker has none.

Each event carries only some of those people, and a condition asking about anyone else is refused:

| Event | Carries |
|---|---|
| `Hurt` | Speaker, Listener, Player (Listener and Player only when a squad member did the hitting) |
| `Downed` | Speaker |
| `WitnessedDeath` | Speaker, Event.Victim |
| `TradeOpened` | Speaker, Listener, Player |
| `NothingToSay` | Speaker, Listener, Player |

`StandingWithSpeaker` needs the Player, so it works in `Hurt`, `TradeOpened` and `NothingToSay`.

### How a line is picked

1. Every line for the event whose conditions all hold, and which this speaker isn't waiting out a
   cooldown on, is a candidate.
2. **The candidate with the most clauses wins.** A three-clause line beats a two-clause one.
3. A tie goes to the line **said least recently, by anyone** - a line never said beats one that was.
4. A tie after that is a weighted pick.
5. If nothing is left, nothing is said.

So writing a more specific line is how to override a general one - and how a mod changes what
players hear without touching the base game's files.

### Translations

`localization/<culture>/<file>.csv`, with columns `Id,Text`. The id is the line's own id in the
same package, written plain. A package translates only its own lines. `fr` covers `fr-CA` too.

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
  Hostility is unchanged (`combat.md`).

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
- **In the editor**, game text normally shows in its source language (`ShouldLoadNativeGameData`),
  and `SmoresSetCulture` uses the engine's *game localization preview* rather than changing the
  editor's language. The preview refuses to start without a native game culture, which the project
  has no localization target to supply - so the text source reports `en` as the game's native
  culture, at low priority so a real target would win. In a game build it is a plain
  `SetCurrentLanguage`.

## C++ Implementation

All in `SmoresDialog` (`Source/SmoresDialog/`), except the controller and GameState glue.

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
| `BarkDirectorComponent.h` | `UBarkDirectorComponent` (server-only, on the GameState) and `UBarkUnitWatcher` - one per unit, the `USquadActivityWatcher` pattern, because three of `UHealthComponent`'s four delegates can't say whose they are |
| `DialogHost.h` | `IDialogHost` - what dialog needs a player's controller to do: `IsSquadMemberWithin`, `DeliverBark`. The `IStrategySelectionHost` pattern; Slice 3's `OpenTrade` effect is the expected next member |

### How a bark travels

1. **An event.** Health: `UBarkUnitWatcher` hears `OnDamaged` / `OnDowned` / `OnDied` on every unit
   (found at the director's `BeginPlay`, on `OnActorSpawned`, and in each level streamed in through
   `LevelAddedToWorld`). Interaction: `InteractWithNPC` calls `Server_RaiseInteractionBark`, which
   re-checks the gates server-side and decides `TradeOpened` or `NothingToSay` itself.
2. **Selection.** `UBarkDirectorComponent::SayIfAny` checks the speaker's quiet time, then
   `SelectBark` over the event's lines. `WitnessedDeath` tries each ally within `WitnessRange`
   (15 m), nearest first, until one has a line. Allies are the same player's squad, or the same
   non-empty faction - two unaffiliated strangers are not allies.
3. **Delivery.** `Deliver` asks every player controller, through `IDialogHost`, whether any of its
   squad is within `HearingRange`, and calls `DeliverBark` on those that are.
   `AStrategyPlayerController` forwards to `Client_NotifyBark(LineId, SpeakerName)`.
4. **Display.** On the client, `USmoresDialogSubsystem::GetLineText` resolves the id through the
   string table, and `PostActivity(Comms, ...)` puts it in the feed.

A line is recorded as said (cooldowns start) whether or not anyone was in earshot - the speaker
said it; nobody happened to hear.

### Registration and packaging

- The module follows `unreal-module-organization.md`'s checklist: `SmoresDialog.Build.cs`, the root
  `SmoresDialog.cpp`/`.h` (with `LogSmoresDialog`), the `.uproject` entry, both `Target.cs` files, and
  `smores.Build.cs`. It depends on `SmoresCore`, `SmoresCombat` and `SmoresCharacters`, plus `Json`
  privately for the manifests.
- **`Config/DefaultGame.ini` stages `Content/Dialog` as-is**
  (`+DirectoriesToAlwaysStageAsUFS=(Path="Dialog")`). The cooker only cooks `.uasset`s; without
  this line a packaged build has no dialog at all, and says so only as one load error.
  **Verified 2026-09-24**: a `BuildCookRun` Win64 Development package lists
  `smores/Content/Dialog/core/{mod.json, barks/core.csv, localization/fr/barks.csv}` in
  `smores-Windows.pak` (`UnrealPak <pak> -List`), and the packaged game logs
  `Dialog: base game Core 0.1.0: loaded, 27 barks, 6 translations, 0 errors, 0 warnings` - read
  through the pak by the same `IFileManager` calls the editor uses on loose files.
- `.gitattributes` marks `*.csv` as text.
- **`Config/DefaultEditorPerProjectUserSettings.ini` keeps the editor's auto-import out of
  `Content/Dialog`** (a `Dialog/*` exclusion on the `/Game/` `AutoReimportDirectorySettings`, beside
  the engine's own `Localization/*` one). Without it the editor sees new `.csv`/`.json` files in the
  content folder and offers to *import* them - which would make a DataTable asset of every bark
  file. If that prompt ever appears for dialog files, the answer is **Don't Import**.

## Blueprint / Asset Dependencies

None to wire. `UBarkDirectorComponent` is a native default subobject of `AStrategyGameState`, so
`BP_StrategyGameState` has it with no Blueprint change, and the subsystem creates itself.

## Content

| File | Holds |
|---|---|
| `Content/Dialog/core/mod.json` | the base game's manifest, id `core` |
| `Content/Dialog/core/barks/core.csv` | 27 barks for the three existing definitions (Settler, Bandit, Trader): every event has a generic line and at least one more specific one. Placeholder writing in the austere tone of `narrative-and-lore.md` |
| `Content/Dialog/core/localization/fr/barks.csv` | the six trader greetings in placeholder French - the localization proof |
| `Mods/example/` | one bark (three clauses, more specific than anything in core) for a Traders Guild trader greeting a Settler, plus its French. Wins Merchant Ada's greeting whenever it's off its 30 s cooldown, without touching core |

## Testing

27 tests - 23 dialog plus the spike's 4 (`testing.md` has the run commands):

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
- **`Smores.Content.Dialog.CoreLoadsWithZeroProblems`** - the core content sweep: zero errors *and
  zero warnings*, every event with a bark, a generic line and a more specific one, no text mangled
  by a wrong encoding, at least one translation. `Smores.Content.Dialog.ExampleModLoadsCleanly`
  checks the example mod loads and still outranks core.

- `Smores.DialogSpike.*` (4, THROWAWAY with the spike module; Slice 3 ports what they prove) -
  play `Mods/example/conversations/shakedown` from its real files: the whole ask-then-refuse path
  (ids, speakers, text, the used-up question, the command before its line); the pay choice
  unavailable when poor and `TakeMoney` when not; two conversations over one loaded script keeping
  separate places, memory and commands; a missing and a garbage file each refused with a message.

Every dialog test except the two content sweeps and the spike's builds its packages from strings and answers facts
from a map (`Tests/SmoresDialogTestFactory.h`), so a writer retuning a bark can't break one.

## Extension Points

- **A new fact**: register it in `FDialogFactRegistry::MakeBuiltIn` with its type, the subjects it
  reads, and (for a name) its content domain or fixed vocabulary. Only when something can answer it.
- **A new bark event**: a value on `EBarkEvent`, its row in `GetEventSubjects`, and a caller that
  raises it on the server through `UBarkDirectorComponent::RaiseEvent`. Add at least a generic
  line in `core.csv` in the same change - the content sweep requires one per event.
- **New lines, a new translation, a new mod**: content only - see Writing Dialog.
- **Something else the director needs from a controller**: a method on `IDialogHost`.

## Conversations: the Yarn player (built ahead of Slice 3)

Conversations are written in **Yarn** (Yarn Spinner). Jim chose it over Ink on 2026-09-25, after a
spike that played the same scene in both; the reasons are in the roadmap's Resolved Design
Decisions, and the plan for everything built on top is its Slice 3. What exists today is the
**player, proven on one scene, in a throwaway module**. Nothing in the game offers a conversation
yet; `InteractWithNPC` is unchanged.

### What exists

- **`Mods/example/conversations/shakedown.yarn`** - the example conversation, kept for Slice 3 to
  put on a Bandit. It exercises a question the game answers (`gold()`), two actions the game
  carries out (`<<TakeMoney 20>>`, `<<ChangeStanding Raiders -10>>`), Yarn's own memory
  (`$asked_about_road`), a conditional choice, a loop back to the choices, and an explicit
  `#line:` id on every line. Beside it are the three files `ysc` writes from it, all read as plain
  files at runtime:
  - `shakedown.yarnc` - the compiled program, which holds **only line ids, never text**;
  - `shakedown-Lines.csv` - `id,text,file,node,lineNumber`, the text of every line;
  - `shakedown-Metadata.csv` - each line's tags other than `#line:` (empty here).

  The bark loader ignores the `conversations/` folder.
- **`Source/SmoresDialogSpike/`** - THROWAWAY. Slice 3 moves the player into `SmoresDialog`, then
  deletes this module and its three registrations (`smores.uproject`, both `Target.cs` files).
  - `YarnConversation.cpp`:
    - `SmoresDialogSpike::LoadYarnScript` reads the `.yarnc` through the plugin's
      `FYarnProtobufParser` and the lines table through `SmoresDialog::ParseCsv`, the bark
      loader's own CSV reader.
    - `FSpikeYarnConversation` drives the plugin's `FYarnVirtualMachine` directly.
  - `SpikeConversation.h` - the shape the tests and commands see. A line is an id, a speaker and
    text; the choices are id, text and "available". `Advance()` and `Choose()` move it on, and two
    hooks (`GetGold`, `RunCommand`) stand in for the game.
  - `USpikeConversationSubsystem` - the PIE commands `SmoresSpikeTalk [gold]` (default 50) and
    `SmoresSpikeChoose <n>` (from 1). Lines, choices and commands go to the COMMS feed. It is local
    and single-player by design; it exists only so the scene could be played.
  - `Tests/SpikeConversationTest.cpp` - four tests (see Testing).

### How the player is driven - what the spike learned

- **Only the plugin's player is used:**
  - `FYarnVirtualMachine`;
  - `FYarnProtobufParser`, which reads a `.yarnc`;
  - `UYarnInMemoryVariableStorage`, one per conversation, made with `NewObject` as a plain object
    (it is a component, but needs no actor).

  None of the plugin's dialogue runner, presenters, widgets or asset import is used. They assume
  the script runs on the machine that shows it, and ours runs on the server and sends ids.
- **The operators are ours.** Compiled Yarn calls its operators as functions: `gold() >= 20` calls
  `Number.GreaterThanOrEqualTo`, and `not` calls `Bool.Not`. The plugin registers those in
  `UYarnDialogueInstance`, its runner layer, not in the player, so the spike supplies its own
  (`MakeYarnOperators`: the Number, Bool and String families). Not yet supplied: `Enum.*`,
  `visited()`, `visited_count()`, `random()`, `dice()` and the rest of the plugin's built-in
  library.
- **The game's questions are functions** (`gold()`), answered through the VM's
  `CallFunctionHandler`, `FunctionExistsHandler` and `FunctionParamCountHandler`. **The game's
  actions are commands**: `<<TakeMoney 20>>` arrives as an `FYarnCommand` whose `CommandName` and
  `Parameters` are plain strings.
- **`ysc` accepts any function or command name without a declaration.** It inferred `gold()`'s
  type and never asked what `TakeMoney` is. A misspelled name therefore only surfaces when that
  line runs, so the loader has to check the names itself.
- **The VM pauses after every line *and* every command** until `Continue()`. A command that
  finishes at once is continued straight through (`FSpikeYarnConversation::Run`). The pause is what
  lets a `<<wait 2>>` take two seconds.
- **A choice whose condition fails is never hidden.** It arrives with `bIsAvailable` false, and
  this is all-or-nothing: the script can't mark one choice "hide me" and another "grey me out". It
  applies to the used-up question as much as to the toll you can't afford. `SetSelectedOption`
  takes the position in the option set, counting unavailable choices too.
- **Line ids arrive as the whole tag**, `line:shakedown_toll`. The spike strips the `line:` part.
- **Initial values:** a `<<declare>>` value comes from the program whenever the variable store
  doesn't have that variable yet.
- **Custom node headers survive compiling.** Checked with `attach:`, `priority:` and `kind:`
  headers on a node: `ysc` keeps them, and the plugin's reader puts them in `FYarnNode::Headers`.
  Our conversation metadata can therefore live on the node itself.
- **The Lines CSV's `file` column** is an absolute path on the writer's machine; ignore it.
- **Log noise:** the VM logs "Yarn VM: Running node ..." at Log level. It is harmless.

### The Yarn plugin - how we carry it

- **Where:** `Plugins/YarnSpinner/`. This is Yarn Spinner for Unreal **3.2.8-alpha9**, a
  "pre-release", from `github.com/YarnSpinnerTool/YarnSpinner-UnrealEngine` at commit `a2c24dc`
  (2026-09-21).
  - It has two modules: `YarnSpinner` (runtime) and `YarnSpinnerEditor`.
  - It is a project plugin, so it is enabled without a `.uproject` entry.
  - Only `SmoresDialogSpike` depends on it today; `SmoresDialog` will from Slice 3.
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
  - On this machine it is at `Saved/DialogSpike/tools/ysc/ysc.exe`, which is not in git. To
    re-download it: `github.com/YarnSpinnerTool/YarnSpinner-Console/releases`,
    `ysc-win-3.2.2-*.zip`.
  - `ysc compile <file>.yarn -o <dir> -n <name>` writes the three files.
  - `ysc tag <file>.yarn` stamps a `#line:` id on every untagged line, so writers never type ids.
  - Writers write in Yarn Spinner's own tools (such as its VS Code extension). The compiled files
    are what a mod ships.

## Known Gaps

- **Conversations, topics, banter, dialog memory and effects are Slice 3** - see the roadmap. The
  language is decided (Yarn) and its player works (above), but nothing in the game offers a
  conversation yet. A conversation plays in its own panel, never as floating text (Jim, after
  Slice 1's PIE pass).
- **A packaged build can't switch to French yet.** `InternationalizationPreset=English` in
  `DefaultGame.ini` stages English culture data only, so `SmoresSetCulture fr` answers "this build
  doesn't know the culture". The proof is in PIE; a packaged build wants the preset (and
  `CulturesToStage`) widened the day a second language is real. The dialog files themselves stage
  regardless.
- **Every package's source text is assumed English.** A mod written in German has no way to say so.
- **Barks are feed-only today.** Jim's Slice 1 PIE pass decided they should also float briefly over
  the speaker; that is the roadmap's Slice 2, a HUD layer rather than a damage-number-style actor so
  that two people talking at once don't overlap.
- **Nothing barks on approach.** All five events are something happening to or with the speaker,
  so outside a fight NPCs speak only when spoken to - which is how Slice 1's PIE pass read. An
  `Approached` event (distance-based, so it will notice through walls until perception exists) is
  planned in the roadmap's Slice 2 - Jim's call.
- **Only the trader greetings are translated.** The six French lines were the localization proof's
  whole scope, so after `SmoresSetCulture fr` the bandits and everyone else still speak English.
  Translating the rest is content work.
- **The tuning numbers are guesses**: `HearingRange` 20 m, `WitnessRange` 15 m,
  `SpeakerQuietSeconds` 6 s, and every row's cooldown. All on the director or in the file.
- **Hearing ignores walls and awareness.** Straight-line distance only, until perception exists
  (`ai-and-behavior.md`).
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
