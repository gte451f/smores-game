# Dialogue

## Purpose

What characters say, to whom, and why. Dialogue is a deliberate **area of improvement over
Kenshi**, which is otherwise the reference game. Kenshi's people say very little, and what they
do say rarely tells the player anything. Here, dialogue should make a role legible without a UI
label, carry real information (a guard who has spotted something is a warning), and give weight
to the people the player deals with: NPC and recruit backstories, faction dealings, banter inside
the squad, and later, quest conversations.

It does this without becoming a narrative game. The world is still the story
(`narrative-and-lore.md`). Dialogue is how its people show it.

## All Dialogue Is Written Ahead of Time

Every line is written by a person before the game ships (or before a mod ships). Nothing is
generated while the game runs. That's settled, and it's what keeps the writing to the austere,
stark tone in `narrative-and-lore.md` and keeps every line translatable.

## Two Kinds of Dialogue

- **Barks** are one-way lines, a sentence or two, said in passing: a trader greeting a customer,
  a bandit going down, someone crying out when hit. A bark **floats briefly over whoever said it**,
  so it can be seen where it was said, and is **recorded in the activity feed** alongside the
  combat log (`player-interface.md`), so it can't be missed. The floating line is an event that
  fades; the feed is the record that doesn't. A bark never changes anything; it only tells the
  player something.

  People also **speak up when a squad comes near** - a shopkeeper calling out to customers, a
  guard challenging someone loitering (`ai-and-behavior.md`) - once as the squad arrives, not over
  and over while it stands there. Until characters can genuinely perceive the world, "near" is
  plain distance and doesn't respect walls; that is an accepted cost of not waiting for perception.
- **Conversations** are authored exchanges with choices, played in **their own panel** - never as
  floating text, which is for single one-way lines only. They're how
  the player deals with a faction's representative, hears a character's history, or (later) takes
  on a quest. A conversation can change the world: standing, money, what a squad has learned.

Both are **chosen by the situation, not scripted to a trigger.** When something happens, the game
looks at who is speaking, who they are speaking to, and what's true right now. The line or
conversation that fits most specifically wins. A general line covers the ordinary case, and a more
specific one takes over when it applies. Nobody has to decide in advance which line plays where.

Barks are not a lesser conversation system. They are a different tool: short, frequent,
unanswerable. Anything the player should be able to reply to is a conversation.

### Topics and banter

- **Topics** let the world add things to talk about without rewriting anyone. When a conversation's
  greeting ends, the player can raise any topic that currently applies to that person. A topic can
  apply to one character, a whole role, or a whole faction. That is how a mod, and later a quest,
  gives existing characters something new to say.
- **Banter** is a conversation the squad has among itself, in the feed, with no window and no
  choices. It happens after a fight or during quiet moments, and it's rare enough to feel earned.

## Dialogue Belongs to the Squad, Not the World

What a squad has heard, said, learned or unlocked belongs to **that squad**. In co-op, one player's
conversation with a faction leader doesn't count as another player's. Each player controls one
squad, so this is the same per-player rule faction standing follows (`factions-and-world-state.md`).

World-level dialogue state (a fact every squad would see change at once) is deliberately not part
of this design. The first real need for it is a quest item only one squad can take, and that
belongs to `quests-and-objectives.md`.

## Co-op: the World Doesn't Stop

Time dilation, pause included, is single-player only (`player-experience.md`), so in co-op a
conversation runs in real time around the people having it. Several players may talk to the same
person at once, and each squad's conversation is its own. Nothing locks a character because
someone else is talking to them.

Each player reads dialogue in their own language, even in a shared session.

## The Base Game Is Written the Way Mods Are

The base game's dialogue is plain text that anyone can open and edit, loaded exactly the way a mod
is loaded. There is one path, so **modders get everything the game's own writers get**. A writer
edits a file and sees the change without restarting the game.

- **Mods add; they don't delete.** A mod can give any existing character, role or faction new barks,
  conversations and topics. A more specific mod line naturally wins over a general base-game line,
  so a mod can change what players hear without touching the base game's files.
- **A broken line is skipped, never fatal.** A mistake in one line costs that line, not the mod, and
  a broken mod never stops the game starting. The author is told exactly what's wrong and where.
- **Mods are data, not code.** A mod can only ask the questions and cause the effects the game
  offers (`multiplayer-and-content.md`).

## Explicitly Out of Scope

- **Runtime AI-generated dialogue.** Dropped, settled.
- **Quests themselves.** Topics and a squad's memory are the hooks a quest system will use.
- **Skill checks and persuasion** until skills exist (`characters-and-squads.md`). No attribute
  checks as a stand-in.
- **Voice, lip-sync and animated portraits.** Text only (`localization.md` notes voice would reopen
  localization).
- **Dialogue that starts fights.** Hostility is derived, never set (`ai-and-behavior.md`'s Stance).

## Open Design Questions Worth Tracking

- **How often barks fire.** Too often and the feed is noise; too rarely and the world is mute.
- **Hearing range, and whether it respects awareness** (walls, distance, noise) once perception
  exists (`ai-and-behavior.md`).
- **Does a single-player conversation pause the game?** Co-op can't, which argues for consistency.
  Kenshi doesn't pause either.
- **Who does the talking** when several squad members are near someone. It matters once a
  character's lineage or history changes how people speak to them.
- **Can a mod replace a base-game line**, not only outrank it? Add-only may prove too limiting once
  modders want to fix or rewrite base lines.
- **Can a mod add a new character**, with dialogue, entirely from text? Today it can give dialogue
  to existing characters only. That's a bigger modding question than dialogue.
