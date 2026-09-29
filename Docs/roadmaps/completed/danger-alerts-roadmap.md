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

> **Status: the one slice is DONE - the roadmap is complete.** Built 2026-09-28 and PIE-checked by
> Jim 2026-09-29. His checkpoint calls: an attack order counts as entering danger (a squad sent in
> on one `H` press must flash together, which changed the build - see below); escalation at half
> health and at going down is right; the 2-second flash is "great". Everything built is documented
> in `game-systems` (`hud-and-panels.md` for the flash and the pace strip, `combat.md` for the
> engagement state, `testing.md` for the 8 new tests), and his answers are written into
> `notifications-and-alerts.md`'s open questions.

## Why this is one slice

Everything below is the same pattern applied in one direction: combat learns when a unit
enters danger, tells anyone listening, and the HUD reacts. The pace lock is bundled in
because it is roughly twenty lines and it is the other half of the same decision — the
reason the flash is the *only* response to danger is that time dilation is single-player.

There is one genuine reason to stop, and it is at the end: **the flash needs Jim's eyes in
PIE.** How brief, how loud, and what exactly counts as "entering danger" are feel questions
that no test answers.

## Slice 1 — The danger flash, and the pace lock — SHIPPED

- **Pace lock** (Task A): `UTimePaceComponent::SetPace` refuses every tier but 1x in a networked
  session, pause included, via the static `IsPaceAllowed(Pace, NetMode)`; the pace strip stays up,
  reads 1x, disables its buttons and explains why in a tooltip. `hud-and-panels.md`.
- **Engagement state** (Task B): on `UCombatComponent` - entered by attacking or by hostile
  attention (`DangerTrigger`, default targeted), ended by a 10s game-time timeout, re-armed by the
  half-health floor and by going down; `bEngaged` replicated, the signal a reliable multicast
  broadcasting `OnDangerSignal`. `combat.md`.
- **The flash** (Task C): `USquadPortraitWidget` subscribes to its own unit and pulses the
  `DangerMarker` badge (red rounded square with a "!", top-right of the disc) for 2 real seconds,
  with `BP_DangerFlash` as the cosmetic hook. `hud-and-panels.md`.
- **The record** (Task D): checked, and deliberately unchanged - the SQUAD tab already answers
  "what happened to Hana". `hud-and-panels.md`'s Known Gaps.
- **The one change the checkpoint made**: Slice 1 first entered an engagement only on hostile
  attention *received*, so of two squad members sent in together only the one the enemy picked
  flashed. Attacking now engages the attacker itself (`JoinEngagement`), before the range check.

## Not in this roadmap

- **The other two flash surfaces.** The design says the division switcher and the map flash
  when the portrait isn't on screen. Divisions don't exist yet, so there is nothing to
  flash and nothing to test. The portrait bar shows the whole squad today, which means the
  portrait-only version is complete as far as current gameplay goes.
- **Unwatched-travel encounters.** `open-world.md`'s fidelity standard needs the *full*
  record/actor split described in `game-data-roadmap.md` — actors spawning and despawning
  around players while records persist — and a world larger than the current map to travel
  across. Nothing here blocks on it and it blocks on plenty.
