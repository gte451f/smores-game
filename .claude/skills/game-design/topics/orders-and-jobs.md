# Orders and Jobs

## Purpose

How the player directs a squad member's *time*, beyond the moment-to-moment "move here,
attack that" of field play. A squad member holds a **standing list of jobs they work
through in priority order** — farm this plot, haul goods to storage, craft these, repair
that wall, practise combat, hold this post — and returns to that list automatically
whenever nothing more urgent is happening.

This is what makes a base productive without the player personally re-tasking every
character every few minutes, and it's what turns a character's time into a resource the
player allocates. It is also the layer that makes the squad more than a combat party: a
character is worth what they produce as well as what they can fight.

Related topics: what a character does with *no* instruction is `ai-and-behavior.md`; where
the work physically happens is `base-building.md`; who is competent at it is
`characters-and-squads.md`.

## The City-Builder Resemblance, and Its Hard Boundary

The interaction deliberately resembles a colony or city-builder's job assignment — the
player queues work, sets its order, and characters get on with it. That resemblance is in
the **interface**, not in the scope, and the distinction is worth stating plainly because
`vision-and-pillars.md` explicitly lists **Not a city builder** among the things this game
is not:

- Every order is issued to a **named individual**, never to a building, a population, or an
  anonymous worker pool. The player commands characters who have skills, wages, injuries,
  and names — the same characters they take into the field.
- There is no population growth, no citizens, no zoning, no settlement-level happiness or
  approval. The workforce is the roster in `characters-and-squads.md`, hired one at a time
  and paid per head.
- Jobs exist to keep the squad supplied, equipped, and skilled. The base still serves the
  squad; the squad does not exist to serve the base.

> **Flagged tension, deliberately not resolved away:** this is the closest any system comes
> to the "Not a city builder" line, and it should be re-read whenever job scope grows. The
> boundary is the individual — the moment the player is allocating an anonymous workforce
> rather than directing named characters, this has drifted past what the pillar allows.

## The Job Queue

- **Each character owns their own queue.** This is decided, not incidental: the alternative
  — a shared base-wide job board that characters pull work from — was considered and
  rejected. A board makes it hard to answer "why is *this* character doing *that*," it
  quietly reassigns people out from under the player's plans, and it pushes the design
  toward the anonymous workforce the section above rules out. The cost is accepted: the
  player is responsible for allocation, and the game will not silently rebalance it.
- **Priority order, worked top-down.** A character does the highest-priority job they can
  actually do right now, and falls through to the next one when the top job is blocked —
  no seed to plant, the workstation is occupied, the materials aren't there.
- **Jobs are either continuing or finite.** "Farm this plot" and "guard this gate" have no
  natural completion and stay on the queue indefinitely. "Haul these crates," "craft five
  of these," and "repair that wall" complete and drop off. The player should be able to
  tell which kind they're queueing at the moment they queue it.
- **A blocked job is visible, never silent.** The most common frustration this system can
  produce is a character standing around for reasons the player can't see. The player must
  be able to find out why someone is idle — and the answer should be specific ("no seed
  stock") rather than a shrug.

## Kinds of Work

The roster of specific jobs depends on the building types in `base-building.md` and the
crafting/research structure in `tech-and-crafting.md`, but the categories are:

- **Production** — operating an extraction, processing, or crafting building.
- **Logistics** — hauling between buildings, stocking storage, loading a caravan. The
  quiet work that makes production chains actually connect (`base-building.md`'s Supply
  Chains and Upkeep).
- **Construction and repair** — building what Building Mode placed, and fixing what raids
  broke.
- **Care** — treating the injured, cooking, tending prisoners. Neglecting this degrades the
  squad over time, per `characters-and-squads.md`.
- **Training** — deliberately practising a skill. See the constraint below.
- **Standing orders** — hold a post, garrison a building, patrol a route. The bridge
  between the job system and base defense.
- **Research** — owned by `tech-and-crafting.md`; it occupies a character's time like any
  other job.

### Training Must Not Be the Efficient Path

`characters-and-squads.md` is firm that skills improve only through use, with no level-up
screen and no shortcuts. A queueable "practise combat" job is in obvious tension with that
unless it is bounded, so: **training is the floor, not the fast lane.** It advances a skill
more slowly than real use of that skill, and its value is that it's available on demand,
safe, and something useful for a character who would otherwise be idle. Real work and real
risk must remain the fastest way to get good at anything, or the game's core loop gets
replaced by a character hitting a training dummy in a safe base — the exact treadmill the
no-shortcuts rule exists to prevent.

## The Three Tiers

Behavior resolves in a strict precedence, and all three tiers exist for a reason:

1. **Reflex** — self-preservation and immediate threat response. Interrupts anything,
   including a direct player order (`ai-and-behavior.md`'s **Reflexes outrank
   deliberation**).
2. **Player direction** — a direct field order ("move here," "attack that") or the job
   queue. Outranks anything the character would have chosen on their own.
3. **Autonomous fallback** — what the character does when the queue is empty and nothing is
   happening, per their role and assignment.

**Interruption is suspension, not cancellation.** A character pulled off a job by a raid
returns to the queue when the fight is over, without the player re-issuing it. A system
that silently drops queued work every time something happens is worse than no system,
because the player can never trust that the base is still running while their attention is
elsewhere.

## Relationship to Divisions and Assignment

`characters-and-squads.md` already establishes two neighbouring concepts, and they are
deliberately independent of this one:

- **Divisions** are an organization and control layer — who moves and gets selected
  together.
- **Assignment** (patrol vs. base vs. labor) is a broad statement of where a character's
  time is meant to go.
- **The job queue** is the specific allocation of that time.

A queue belongs to the character, not to the division or the base: it travels with them
when they're reassigned, and a character carried off to a distant division still holds the
queue they had. Whether a queue can be issued to a whole division at once is open below.

## Who Does the Job Matters

Skill affects the speed, yield, and quality of work, so job assignment is a real decision
rather than a chore. An unskilled character assigned to a plot still farms it — badly, and
slowly, and they get slightly better at farming for having done it. That's the point:
assigning the wrong person is a cost, not a block, and over a campaign the player's
assignment choices are quietly shaping who each character becomes.

## What the Player Needs to See

Owned in detail by `player-interface.md`; this topic only establishes the requirement.
At minimum: what each character is doing right now, the contents and order of their queue,
and — most importantly — why anyone is idle or blocked.

## Open Design Questions Worth Tracking

- Whether jobs can be issued to a division as a unit ("all four of you build this") or only
  to individuals one at a time.
- Whether queues can be copied between characters or saved as reusable templates. Valuable
  as squads grow; also the most likely route back toward feeling like anonymous workforce
  management.
- Whether the player can attach conditions to a job ("farm until there are 100 grain,"
  "keep 20 arrows in stock") or only ordering. Conditions are a large expressiveness gain
  and a large complexity cost.
- Whether hauling is an explicit queued job or happens implicitly as part of production.
  Explicit is more controllable; implicit is far less tedious.
- What a character with an empty queue does — idle at a home anchor, free-roam the base, or
  fall through to their role's default behavior (`ai-and-behavior.md`).
- How queues behave for a character far from base with nothing on their list applicable to
  where they are.
- Whether training has diminishing returns, a hard cap relative to real use, or simply a
  slow rate.
- Whether jobs consume wages differently from field work, or whether a working character
  and an idle one cost exactly the same (`economy.md`'s wage pressure).
