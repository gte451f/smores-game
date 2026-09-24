# Player Interface

## Purpose

The main play screen's information layer — what's always visible, what only appears when
relevant, and how squad/world state gets surfaced without handing the player conclusions
they haven't earned. Distinct from `game-systems`' HUD/camera *implementation*
(`strategy-camera-and-selection.md`) — this topic is about what the player should be able
to see and why, not the C++/Blueprint behind it.

## Diegetic vs. Non-Diegetic Philosophy

Lean, not decided in full: frame elements (roster, mini-map, pace control, alerts feed)
are ordinary non-diegetic HUD, while anything that would otherwise leak earned-knowledge
information stays diegetic or disappears — mirroring `world-map-and-travel.md`'s
no-danger-shading stance on the map itself. The main screen should never tell the player
something their squad hasn't actually observed.

## Always-On HUD

### Squad Roster and Divisions

A persistent list of current squad members, standing in for the "no single hero" pillar
(`vision-and-pillars.md`) — the player is always managing a roster, not a single health
bar. The roster can be organized into multiple named **divisions** — a scouting party, a
trade caravan, a base garrison, a raiding force — each able to act independently and
simultaneously in different parts of the world, Kenshi-style (see `characters-and-squads.md`'s
Squad Divisions). This is a control/organization layer only; wages, hire fees, and morale
still apply per-character and to the roster as a whole, not per-division.

Because divisions can be scattered across the map rather than always in view together,
the interface needs a **division switcher** distinct from ordinary multi-unit selection:
a list of the player's currently defined divisions (name, member count, at-a-glance
status), with a way to jump the camera/selection to any one of them and cycle through the
rest — the same job Kenshi's squad tabs do. Selecting a division here is a shortcut into
ordinary selection, not a separate control mode; once selected, a division's members
behave like any other selected group. Deeper per-character detail (skills, wages,
injuries) belongs to the character sheet, not this list.

Switching to a division **instantly snaps the camera** to focus on that division's
leader — a hard cut, no travel time, no transition. This is purely a change of the
player's viewpoint, not of anything in the simulation: the division itself doesn't move,
still bound by real-time travel like everything else, so this doesn't conflict with
`world-map-and-travel.md`'s no-fast-travel stance — nothing is being teleported, only
where the camera happens to be looking.

### Mini-Map

Fully specified in `world-map-and-travel.md`'s "Mini-Map Widget" section (fixed scale, no
coordinates, graduated POI detail, no danger shading, no per-unit icons) — this topic only
owns where it sits in the overall screen layout (top-left or top-right), which is an
unresolved layout detail, not a design question.

### Time-of-Day Indicator

A diegetic sun/moon-arc widget reflecting the day/night cycle described in
`vision-and-pillars.md`'s Campaign Time Scale (~40 real-time minutes per in-game day) and
feeding the same read the player gets from lighting in the world itself — this is a
convenience redundancy, not a second source of truth.

### Time-Pace Controls

The on-screen exposure of the discrete speed tiers already defined in
`player-experience.md` (pause, 1/3x, 1/2x, 3/4x, 1x, 2x, 4x, 8x) — in single-player,
available anywhere, at any time, with no mode-specific exemptions, per that topic. This
widget doesn't introduce new pacing behavior, only surfaces it.

**In multiplayer every tier is locked at 1x, pause included** (see
`player-experience.md`). The widget stays visible and honest rather than disappearing — it
reads 1x and offers nothing to press, so the absence is explained where the player would go
looking for it.

No alert ever changes the pace on the player's behalf — see
`notifications-and-alerts.md`.

### Quick-Access Menu Strip

Shortcut icons into inventory, the character/squad menu, and faction relations — a
convenience layer alongside key bindings for the same actions, not a replacement for them.
A currency/resource-at-a-glance readout (cash on hand, at minimum) belongs here too, given
how central wages and hire fees are to squad upkeep (`economy.md`,
`characters-and-squads.md`).

### Activity, Chat, and Alerts Panel

One scrollable feed combining combat log, NPC dialogue/chat transcript, and general world
alerts — the player's after-the-fact record of what happened, in service of
`vision-and-pillars.md`'s "failure should be informative" principle (a wiped patrol should
be explainable in hindsight, and this panel is where that explanation lives). Full
trigger list, severity, and history depth are owned by `notifications-and-alerts.md`; this
topic only owns that they surface here. Persistence-until-dismissed (per
`player-experience.md`'s Accessibility section) applies to this panel specifically — it's
the reason high-speed play doesn't cause missed information.

## Meta Menu

The in-session entry point to Help, Options, and Save/Load — distinct from the title
screen's own entry into those same flows. Options *structure* and Save/Load *semantics*
are owned by `main-menu-and-meta-flow.md` and `save-system.md` respectively; this topic
only owns that they're reachable from a menu during active play.

## Contextual / Selection-Driven UI

- **Selected-character status frame** — appears when one or more squad members are
  selected: portrait, at-a-glance health/injury summary (see `characters-and-squads.md`'s
  Injuries). Deliberately shallow — the full stat sheet lives in the character menu, not
  here.
- **Command feedback** — current order, destination, and engage/disengage state for
  selected characters, reflecting the player-directed layer described in `combat.md`
  (movement orders, target priority, engage/retreat).
- **World-space contextual prompts** — loot, talk, interact prompts appear near the object
  or NPC in the world rather than as a fixed screen-space element. This is the diegetic
  counterpart to the mostly non-diegetic frame elements above.

## Deliberate Exclusions

- **No persistent quest/objective tracker or quest markers.** The Codex is explicitly not
  a quest log (`player-experience.md`), and there's no authored quest content to track
  (`quests-and-objectives.md`) — the main screen shouldn't imply a "what to do next" the
  design doesn't provide.
- **No danger/threat overlays or off-screen enemy-strength readouts.** Same reasoning as
  the map's no-danger-shading rule (`world-map-and-travel.md`) — if any off-screen
  contact indicator exists at all, it should convey presence/direction only, never a
  threat assessment, mirroring the mini-map's distant-pin treatment.
- **No numeric optimization overlays** (DPS meters, hidden-stat readouts). Tooltips
  (`player-experience.md`) explain what a stat *is*, never how to optimize it — the main
  screen holds to the same line.

## Open Design Questions Worth Tracking

- Whether each division has its own designated "leader" for camera-focus purposes, distinct
  from the single roster-wide "squad leader" whose skill feeds morale
  (`characters-and-squads.md`), or whether that's the same designation reused — not decided.
- Exact screen layout (mini-map corner, panel positions, where the division switcher
  lives) is a layout detail, deferred.
- Whether an off-screen threat/engagement indicator exists at all — see Deliberate
  Exclusions.
- How co-op player presence (chat, pings, other players' selections) integrates with the
  Activity/Alerts panel — see `multiplayer-and-content.md`.
- Full diegetic-vs-non-diegetic split isn't finalized — current lean is non-diegetic frame
  elements with diegetic contextual prompts layered into the world, but this needs a real
  UI pass once art direction exists.
- Relationship to `notifications-and-alerts.md`'s full trigger list once that topic is
  designed.
