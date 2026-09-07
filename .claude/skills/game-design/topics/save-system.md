# Save System

## Purpose

The save system records the full state of a campaign at a moment in time and restores it
faithfully — and because the world is a living simulation (factions shifting, economies
fluctuating, characters accumulating injuries), it must be treated as a first-class
system, not an afterthought. It must also survive game patches and mod changes without
silently corrupting or crashing.

## What Must Persist

**World state** — every faction's type and status, every faction-pair relationship (not
just starting values), settlement ownership and condition, named-NPC alive/dead status
and location, fog of war, active world events, and per-market economy values (supply/
demand, current prices). **The player's squad and bases** — every character's attributes,
skills, injuries, equipment, and wage; squad assignments; each base's buildings, condition,
storage, and production assignments; in-progress research; and standing with every
faction. **In-progress events** — any assault or raid underway (including off-screen
simulation state), caravan positions and routes, and pending scheduled events.

## Save Slots and the Recovery Safety Net

Multiple manual save slots plus a separate autosave (default: every in-game day) that
never overwrites a manual save, backed by a rolling multi-generation backup so a mistake
noticed several in-game days later still has a recovery path. There's no forced
single-save requirement in the base game — an optional ironman mode (one slot, autosave
only) is a difficulty modifier a player opts into (see `multiplayer-and-content.md`), not
a default constraint.

## Version and Mod Compatibility, as Player-Facing Promises

An additive schema change (new fields with defaults) should always load a save normally.
A breaking schema change ships with a migration script that runs automatically and backs
up the original save first; if migration genuinely isn't possible, the player is told
clearly rather than handed a silently broken load. The design implication that matters
here: **breaking changes should be rare and deliberate**, reserved for major version
milestones, never a routine balance patch (see the update categories in
`multiplayer-and-content.md`).

Loading a save without a mod that was active when it was created strips that mod's
referenced content (items, factions, NPCs) rather than refusing to load, and tells the
player plainly what was removed. Loading *with* a new mod not present at save time just
means that mod's content starts fresh from that point forward.

## Reliability

Saves write atomically (a temp file, then a rename) so a mid-write crash can't corrupt the
live save. A checksum failure on load offers the most recent backup instead of silently
loading something potentially broken. Cloud sync is a convenience and backup layer, never
the sole authoritative copy — the local save is.

## Performance, as a Player-Facing Expectation

Saving a large simulated world shouldn't freeze the game (asynchronous where possible),
shouldn't fire in the middle of combat or an event where a mid-sequence save would be
misleading (rate-limited), and should always tell the player a save is in progress and
when it's finished.

## Open Design Questions Worth Tracking

- Exact schema/serialization format is an implementation decision, not a design one.
- A save-size budget and pruning strategy (e.g., trimming price history beyond N days)
  will be needed as world and character history accumulate.
- Full ironman-mode design — squad-wipe handling in particular — isn't worked out yet.
- Console-specific save constraints, if the game ships there, are deferred until platform
  targets are confirmed.
