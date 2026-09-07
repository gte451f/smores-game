# World Map and Travel

## Purpose

How the player perceives and navigates the open world at a meta level — the map view,
what it shows and annotates, and how that relates to actual travel. This is the
presentation/UI layer on top of `open-world.md`'s "Travel and Visibility" and "Time Scale"
sections, which own the underlying simulation (real-time squad movement, fog of war,
region staleness); this topic shouldn't restate that simulation, only how the player
observes and plans against it.

## No Fast Travel

There is no fast travel of any kind, decided. Opening the map never teleports the squad —
travel is always real-time movement through the open world, affected by terrain,
encumbrance, and athletics skill (see `open-world.md`). The map is a planning and
orientation tool, not a travel mechanic.

## Map View

A dedicated map view shows the **explored, known-playable area** — built from fog-of-war
presence and past visitation (per `open-world.md`'s "Travel and Visibility"), not the full
static world. Unexplored regions aren't shown, or are shown only in vague outline, until
the squad (or a contact/scout) has actually been there.

## Map Annotations

The map view is annotated with, at minimum:

- **World events** — conflicts, trade disruptions, migrations, and other emergent
  simulation events the player has learned about (see `open-world.md`'s "Environmental
  Hazards and World Events").
- **Points of interest** — the authored locations from `open-world.md`'s "Points of
  Interest." Represented with **detail that scales with exploration state**: an
  undiscovered-but-sighted POI shows as a vague, low-detail marker (something is there,
  identity/contents unknown); once actually explored it resolves to full detail (type,
  name, whatever the player has learned). This preserves the "stumble onto it" discovery
  design in `open-world.md` — the map tells the player *something* is out there, never
  what it is before they've earned that.
- **Trade routes** — known roads/routes between settlements and markets.
- **Faction territory** — the player's accumulated read on who controls or contests a
  region (see `factions-and-world-state.md`).
- **Player-placed pins** — manual markers the player drops for their own route planning.
- **Player-constructed bases** — the player's own outpost(s)/base(s) (see
  `base-building.md`).

"Among other things" — this list is a floor, not a ceiling; more annotation layers are
expected as other systems mature.

**No danger/threat shading, decided.** The map never color-codes regions by danger level —
danger is something the player learns through observation and reputation (see
`open-world.md`'s "Regions and Biomes"), not a UI readout. A shaded danger overlay would
hand out exactly the meta-information the game is designed to make the player earn.

## Mini-Map Widget

A mini-map HUD widget is supported, always-on-screen during normal play — distinct from
the full map view above (a deliberate, full-screen check-in) in that it's ambient,
always-visible local awareness. Deliberately minimal, decided:

- **One fixed scale.** No zoom levels on the mini-map — a single zoom, always.
- **No player coordinates.** A precise position readout is more information than the
  intended feel.
- **Main use case is nearby POIs**, at the same graduated detail as the main map (see "Map
  Annotations" above) — this resolves the earlier discovery-vs-markers tension: the
  mini-map can surface that *something* is nearby without revealing what it is until
  explored, so it never becomes a second quest-marker system.
- **Distant pins/pings show direction only.** A player-placed pin or a co-op ping outside
  the mini-map's radius appears as an indicator on the rim, conveying bearing but not
  distance.
- **No danger/threat shading** — same reasoning as the main map, above.
- **No per-unit icons.** Individual squad members, other players, or mobs are not shown —
  too much clutter at this scale. Squad awareness at that level is a gameplay/UI concern
  for `game-systems`, not map design.

## Travel Execution

Actual travel takes place in the open world, not on the map screen. The map informs
*where* to go and *what's known* about the route; the squad still crosses that terrain in
real time, subject to the same hazards, encounters, and pacing as any other movement.
The map view and the 3D world are two views onto the same simulation, never a shortcut
between them.

## Quality-of-Life Navigation (Candidate, Not Committed)

Convenience commands to reduce manual pathing tedium — not fast travel, since they still
execute as normal real-time movement through the world, at normal speed, exposed to normal
hazards and encounters along the way:

- **"Run to [known destination]"** — auto-path the squad to a previously discovered
  location instead of manually walking the route.
- **"Follow the leader"** — subordinate squad members auto-trail a designated lead unit
  rather than requiring individual move orders.

## Open Design Questions Worth Tracking

- Whether this stays a separate topic or folds into `open-world.md` — now that it has its
  own content (map presentation, annotations), the case for staying separate is stronger,
  but not settled.
- The full annotation layer list beyond events/POIs/trade routes/faction territory/pins/
  bases isn't finalized.
- Whether the QoL navigation commands ship at all, and whether they auto-cancel on
  spotting danger or a new encounter, isn't decided.
- How the map reflects `open-world.md`'s region-staleness mechanic ("visited-but-neglected
  regions gradually go stale") — does the map show current world state or the squad's
  last-known information for a region, with the two allowed to diverge? Leaning toward the
  latter (it would reinforce the "knowledge decays without presence" pillar) but not
  decided.
