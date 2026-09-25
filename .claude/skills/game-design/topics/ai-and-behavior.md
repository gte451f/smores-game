# AI and Behavior

## Purpose

What a character does when nobody is telling them what to do. This covers every inhabitant
of the world — faction soldiers, shopkeepers and townsfolk, bandits, wildlife, and the
player's own squad members in the gaps between orders. The intent is one behavior system,
not a separate one per kind of inhabitant: what makes a shopkeeper different from a bandit
different from a deer is **the data describing them**, not different machinery. The test
this design is meant to pass — adding a new kind of inhabitant should be an authoring job,
not a new system.

Boundaries with neighbouring topics: this topic owns *autonomous* behavior. What the
player explicitly commands a squad member to do is `orders-and-jobs.md`. How a fight
resolves once it has started is `combat.md`. How factions decide policy — who to hate,
what a city builds, when to raid — is `factions-and-world-state.md`, and it is a genuinely
different system (see **Individuals Don't Decide Faction Policy** below).

## Every Character Answers the Same Four Questions

1. **Who am I?** — the character's role: shopkeeper, guard, patroller, bandit, herbivore,
   predator, squad member. This defines where they belong, what work they do, what they are
   willing to do, and what they say.
2. **How do I feel about you?** — their stance toward another character: hostile, wary,
   indifferent, friendly. Derived, never authored per-character (see **Stance** below).
3. **What do I actually know about?** — what they have seen and heard, which is much less
   than what is true.
4. **What should I do right now?** — the best available option from the short list their
   role allows, given their stance, awareness, and condition.

The three examples that motivated this topic fall out of the same four questions with no
special-casing:

- A **shopkeeper** has a role anchored to their shop, a small set of permitted activities
  (tend the counter, trade, flee), a friendly-or-indifferent default stance, and a set of
  job-appropriate lines. They never attack anyone because their role has no such activity,
  not because combat was disabled for them.
- A **bandit** has a role that permits patrolling, engaging, looting, and retreating; a
  hostile stance toward several factions; and awareness that notices an enemy. They attack
  an enemy-faction caravan whether or not the player is anywhere nearby — which is the
  **Living World** pillar (`vision-and-pillars.md`) being literally true rather than
  staged.
- A **creature** has a role with no politics at all and behavior driven mainly by its
  condition — hunger, fear, injury. Eat or run is not a creature-specific system; it's the
  same "pick the best available option" step, weighted by drives instead of by orders.

## Stance: Who Counts as an Enemy

**Hostility is always derived, never hardcoded.** Nothing in behavior should ever say "the
enemy is the player." The design depends on this in at least four places:

- `factions-and-world-state.md` establishes that the player is unaffiliated and factions
  fight *each other*. A world where only the player can be attacked isn't a living world,
  it's a shooting gallery pointed at the player.
- `open-world.md` establishes that wildlife "doesn't distinguish friend from foe" — it
  reacts to proximity and hunger, has no standing, and will attack a faction patrol as
  readily as the squad.
- `characters-and-squads.md` makes theft and illicit work meaningful precisely because
  hostility can *change* — a witnessed crime turns an indifferent guard hostile.
- Co-op means there are several players, not one, and no character should be reasoning
  about a singular player at all.

Stance is composed from formal faction standing, local reputation, witnessed acts,
creature drives, and temporary states (currently being attacked, defending territory). A
character's *role* determines what they do about their stance — a hostile shopkeeper
refuses to trade and calls for a guard; a hostile soldier attacks.

> **Current prototype note (contradiction, flagged not resolved):** the existing build's
> only autonomous behavior is "aggressive characters hunt the nearest player pawn." That
> directly contradicts this section and is a placeholder, not a design position. It is the
> first thing that has to go before any of the above becomes expressible.

## Awareness: Characters Act on What They Observed

Characters know what they have seen and heard, not what is true. This is load-bearing for
several systems that already assume it:

- Darkness, weather, and terrain genuinely reduce detection, which is what makes the
  day/night cycle "genuine tactical cover for illicit activity" rather than a lighting
  change (`open-world.md`).
- The difference between a witnessed and an unwitnessed theft (`characters-and-squads.md`)
  only exists if someone can fail to witness it.
- Scouting ahead is rewarded (`open-world.md`) partly because the enemy's awareness is
  limited too — a patrol that hasn't seen the squad hasn't reacted to it.
- A character should never react to something they could not plausibly have perceived.
  When that rule is broken it reads instantly as the game cheating, and it undermines
  **failure should be informative** (`vision-and-pillars.md`) — the player can't learn from
  an outcome they had no way to anticipate.

How far knowledge of an event travels from the characters who witnessed it is deliberately
slow and is an open question shared with `factions-and-world-state.md`.

## Drives: Condition Shapes Behavior

A short set of internal conditions — hunger, fear, fatigue, pain, morale — push a
character toward some activities and away from others. Wildlife is mostly drive-driven;
intelligent characters are mostly role- and stance-driven with drives as a modifier.

The point is that behavior varies without being random: a fed predator ignores the squad
and a starving one attacks a group it can't beat; a badly injured animal runs where a
healthy one would fight; an exhausted, unpaid squad member works badly. That gives the
player something to read and exploit, which is **Systems Mastery as Power**
(`vision-and-pillars.md`) rather than a dice roll.

## Choosing What to Do

A character periodically reconsiders the short list of activities their role permits and
picks the best one for their current stance, awareness, and drives. Two rules govern it:

- **Reflexes outrank deliberation.** Being attacked, being mortally afraid, or standing in
  something lethal interrupts whatever was in progress — including a player-issued job
  (see **The Three Tiers** in `orders-and-jobs.md`). A squad member farming a plot who gets
  shot at does not calmly finish the row.
- **Behavior should be legible.** A player watching a character should be able to tell what
  they're doing and make a reasonable guess as to why. Behavior that is technically
  sophisticated but unreadable is worse than behavior that is simple and obvious, because
  the player can't plan against what they can't interpret.

## The Range of Behavior Is Deliberately Narrow

This is not a simulation of daily life. There are no individual sleep schedules,
relationships between NPCs, personal ambitions, or needs-driven life-sim behavior. The
target is exactly enough behavior for the world to read as inhabited and for threats to
behave believably — a shop that feels staffed, a patrol that feels like it has somewhere to
be, a predator that feels like an animal. Everything past that is scope that competes with
the systems the pillars actually rest on.

This is a deliberate ceiling, not a stage of development to grow out of.

## The Player's Own Characters Use This Too

A squad member between orders runs the same machinery as an NPC, constrained by their
assignment (`characters-and-squads.md`) and their job queue (`orders-and-jobs.md`). The
consequences are intentional:

- An idle squad member has sensible defaults — return to a post, keep working the queue,
  defend themselves — rather than standing inert until told otherwise.
- An NPC's role is, in effect, a permanent job queue they didn't get to choose. A
  shopkeeper tending a counter and a squad member assigned to a forge are running the same
  thing from different sources, which is why base labor doesn't need a second system.
- Auto-defense applies to everyone uniformly. A bystander who gets hit fights back
  regardless of who owns them.

## Individuals Don't Decide Faction Policy

Faction-level reasoning — relationships between factions, what a city invests in, whether
to raid — runs on its own much slower clock and is owned by `factions-and-world-state.md`.
The relationship between the two layers is deliberately one-directional:

**Factions decide policy; individuals decide behavior; information flows downward.** A
faction sets stances and dispatches groups. An individual reads its faction's stance and
acts locally. What travels back up is aggregate outcome — a patrol was wiped, goods went
missing in our territory, a caravan didn't arrive — never individual moment-to-moment
chatter.

The reason to be strict about this: if faction decisions depend on individuals reporting
in, then nothing decides anything in regions where no characters are actively running,
and the world only advances where the player is looking. That is precisely the failure the
**Living World** pillar exists to prevent.

## Distance and Fidelity

Characters far from every player behave at reduced fidelity — the world resolves what
happened to them rather than playing out every step. This is a design commitment, not just
an optimization, because `open-world.md` and `factions-and-world-state.md` both promise
things happening off-screen that the player discovers afterward.

Two constraints on it: the seam should be invisible (a player returning to a region finds
plausible consequences, never a region frozen exactly as they left it or one that
teleported through a week of activity), and it keys off **any** player, since co-op means
up to eight separate points of attention scattered across the map
(`multiplayer-and-content.md`).

## Barks and Dialogue

Characters say things appropriate to their role and situation — a shopkeeper hawking
goods, a guard challenging a loiterer, a bandit announcing an ambush. These surface in the
activity/chat panel (`player-interface.md`) alongside the combat log.

Two things they are: flavor that makes a role legible without a UI label, and genuine
information — a guard saying they've spotted something is the player's warning that
awareness has shifted. Barks are one-way; anything the player can answer is a
conversation, which is a separate layer. Both, and how they're chosen, are owned by
`dialogue.md`. Written to the austere, stark tone in `narrative-and-lore.md`, and
localized like all other strings (`localization.md`).

## Open Design Questions Worth Tracking

- **Whether characters break and flee from overwhelming odds without an explicit order.**
  Already listed as open in `combat.md` — it's the same question, and it needs one answer
  that covers the player's characters and everyone else's.
- Whether intelligent NPCs keep schedules (a shop that closes at night, patrols that rotate
  shifts) or hold their role continuously. Schedules make towns far more convincing and are
  a meaningful cost in both authoring and simulation.
- Whether hostility toward the player persists in a character's memory across time and
  saves, or resets to whatever faction standing implies. A bandit who remembers a specific
  squad is more interesting and much harder to keep coherent.
- Whether creatures have territory and pack behavior, or are simulated as individuals that
  happen to be near each other.
- How far a witness's knowledge spreads and how fast — shared with
  `factions-and-world-state.md`'s open question on information propagation, and it should
  be answered once for both.
- What happens to a character whose role loses its anchor (shop destroyed, faction falls,
  outpost abandoned) — re-role, migrate, or disappear.
- Whether captured or recruited enemies (`combat.md`) carry any behavioral residue of their
  former allegiance.
