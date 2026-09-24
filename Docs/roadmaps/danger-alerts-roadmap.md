# Danger Alerts Roadmap

## Purpose

This is a **roadmap**, not a system reference: read it while implementing its slice, or when
Jim points at it. The permanent record of how this works belongs in the `game-systems` skill
— `hud-and-panels.md` for the flash and the pace widget, `combat.md` for the engagement
state — and this file should be trimmed once it ships.

The design is already settled and lives in the `game-design` skill. **Read
`notifications-and-alerts.md` before starting**; it owns the flash-vs-record split, the
engagement-as-the-unit rule, the accessibility requirement, and the decision that an alert
never changes the simulation. `player-experience.md` owns the multiplayer pace lock. Don't
reopen either here.

## Why this is one slice

Everything below is the same pattern applied in one direction: combat learns when a unit
enters danger, tells anyone listening, and the HUD reacts. The pace lock is bundled in
because it is roughly twenty lines and it is the other half of the same decision — the
reason the flash is the *only* response to danger is that time dilation is single-player.

There is one genuine reason to stop, and it is at the end: **the flash needs Jim's eyes in
PIE.** How brief, how loud, and what exactly counts as "entering danger" are feel questions
that no test answers.

## Slice 1 — The danger flash, and the pace lock

### Task A: multiplayer pace lock

`UTimePaceComponent` (`SmoresCore`) is already server-authoritative and lives on the
GameState, so this is a rule, not a rework.

- Reject any pace change other than 1x when the session is not standalone. The component is
  the right place for the refusal — every path (`RequestPace`, the ladder steps, the widget
  buttons) already funnels through it, so one gate covers all of them.
- `UTimePaceWidget` (`SmoresUI`) stays visible and reads 1x rather than hiding, per
  `player-interface.md` — the absence has to be explained where the player looks for it.
- **Pause is inside the lock, not an exemption** — settled, see `player-experience.md`. It
  is a time change like any other.
- Gate on net mode, not player count: anything other than standalone is locked. A host
  alone in a co-op session is still locked, deliberately.

### Task B: engagement state in combat

New state on `UCombatComponent` (`SmoresCombat`) answering "is this unit currently in a
hostile engagement it was not already in."

- Entry from clear is the flash-worthy transition. Everything while already engaged is not.
- Exit is a timeout after no hostile attention — a unit does not leave an engagement by
  winning a single exchange.
- **Escalation re-arms**: crossing a health floor, or going down, is a fresh signal even
  mid-engagement. `UHealthComponent` already owns the numbers; this reads them, it does not
  duplicate them.
- Authority-gated and replicated, per `multiplayer-discipline.md`. The flash is presentation
  and belongs on the client; the *state* is shared truth and belongs to the server.
- Broadcast via a delegate. `SmoresUI` may depend on `SmoresCombat`, never the reverse —
  combat must not know a HUD exists.

### Task C: the flash on the portrait

`USquadPortraitWidget` / `USquadBarWidget` (`SmoresUI`) subscribe and react.

- **Not colour alone.** An icon or shape change alongside the colour, per the accessibility
  section of `notifications-and-alerts.md`, and clearly distinct from the selection ring —
  the two decorate the same tile and must not be confusable.
- Brief and self-limiting. The health bar already underneath it is the ongoing readout; the
  flash marks only the transition.
- A `BP_*` hook for the cosmetic half is the right shape here, so the timing curve and
  styling can be tuned without a build.

### Task D: check the record, don't duplicate it

The activity feed already reports squad damage on its SQUAD tab and **nothing in it fades**
(confirmed in `UActivityFeedWidget`). So the durable half of the design may already exist.

Check before adding: if the existing damage line answers "what happened to Hana," a
separate "entered danger" line is noise. Add one only if the record is genuinely missing
the event.

### Tests

Per `testing.md`'s standing rule — this is a state machine and it has counted outcomes:

- One flash per engagement, not one per hit.
- Escalation inside an engagement re-flashes.
- Leaving and re-entering an engagement flashes again.
- A non-1x pace request is refused when not standalone, and accepted when standalone.

### Checkpoint

Jim runs it in PIE and calls the feel: flash duration, how "entering danger" should be
defined (targeted, hit, or detected), and whether escalation needs more than "went down."
Those answers get written back into `notifications-and-alerts.md`'s open questions.

## Not in this roadmap

- **The other two flash surfaces.** The design says the division switcher and the map flash
  when the portrait isn't on screen. Divisions don't exist yet, so there is nothing to
  flash and nothing to test. The portrait bar shows the whole squad today, which means the
  portrait-only version is complete as far as current gameplay goes.
- **Unwatched-travel encounters.** `open-world.md`'s fidelity standard needs the *full*
  record/actor split described in `game-data-roadmap.md` — actors spawning and despawning
  around players while records persist — and a world larger than the current map to travel
  across. Nothing here blocks on it and it blocks on plenty.
