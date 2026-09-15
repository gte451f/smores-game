# Open World

## Purpose

The open world is the canvas every other system runs on: a large, continuous landmass
with no loading screens between regions, divided by distinct terrain, resources, climate,
faction presence, and danger level. The player can go anywhere from the start, but not
everywhere safely — distance, terrain, and threat are the real barriers, never invisible
walls. The world has its own internal logic independent of the player; learning it is a
genuine form of power (see **Systems Mastery as Power** in `vision-and-pillars.md`).

## Map Design

The world map is **static and handcrafted** — the same geography, settlements, resource
deposits, and points of interest every single playthrough. This is deliberate: it lets
knowledge an experienced player carries from a prior run genuinely transfer (learning the
map is itself a form of mastery); it lets faction pre-dispositions, trade routes, and
authored NPC placement all depend on a world that doesn't change shape; and it lets
mod-added regions extend the map without breaking those assumptions. Procedural generation
is used only for NPC populations and economy values *within* that fixed shape — never for
the world's structure itself.

## Regions and Biomes

Each named region has a dominant terrain type, a resource profile, environmental hazards,
faction presence, and a danger rating the player learns through observation and
reputation, not a UI label — regions blend into each other rather than hard-cutting.
Resources follow biome logic (arid/desert: scarce food and water, possible unique
minerals; forest/jungle: animal materials and timber, dangerous wildlife; plains/
grassland: highest farming yield, primary food-export region; mountain/highland: ore and
stone rich, food-poor, high transport cost; wetland/coastal: fishing and water-adjacent
materials, moderate danger). Understanding the biome map means understanding the economic
map (see `economy.md`) — **danger deliberately scales with resource value**: the richest
deposits sit in the most dangerous places, which is exactly what earns them their price
premium in distant, safer markets.

## Points of Interest

The map is dense with authored, static content — the same location, layout, inhabitants,
and loot every playthrough. There's no quest marker directing the player to most of it;
stumbling onto a POI while traveling is the normal, intended experience, and POI density
itself should read as world logic — settled, patrolled regions have fewer derelict ruins
and stranded survivors than dangerous, contested ones.

Neutral (non-faction) NPCs at POIs support three interaction types, not guaranteed at
every location and not mutually exclusive: **recruit** (a skilled survivor or stranded
mercenary who might join the squad, on the same hire-fee/wage terms as anywhere else, with
some unusual skill profiles unavailable through faction channels), **enslave** (subdue and
sell — mechanically identical to kidnapping anyone else, with standing consequences if
witnessed), and **minor quests** (small, systemic, context-driven tasks — an escort
request, a bounty on nearby enemies — deliberately not an authored quest system).

POI categories: **settlements/towns** (faction trade hubs, recruitment pools, a mix of
permanent named NPCs and repopulating procedural ones); **ruins/abandoned sites** (loot,
salvage, tech-tree research specimens, sometimes squatters to clear first); **traveler
camps/isolated survivors** (the primary sites for recruitment, minor quests, and illicit
activity away from faction witnesses); **resource deposits** (rich enough to found a base
around or contest with factions, subject to the outpost exclusion radius — see
`base-building.md`); **contested/active sites** (a conflict already underway — pick a
side, wait it out, or avoid it); **faction outposts/installations** (attacking has direct
standing consequences; observing gives free intelligence on faction strength); **criminal
and illicit locations** (never on a starting map — found only through criminal standing,
word of mouth, or direct exploration); and **unique landmarks** (one of a kind, sometimes
tied to an end-game objective).

## Travel and Visibility

Squad movement is real-time, affected by terrain, encumbrance, and athletics skill. There
is no traditional fast travel, though known roads and routes move faster than raw terrain.
Fog of war reveals through presence and past visitation, and **visited-but-neglected
regions gradually go stale** — faction control shifts, settlements change — so keeping
accurate knowledge of a region requires an ongoing presence or contact network, not a
one-time visit. Scouting ahead with a high-perception, high-stealth element is a
supported, rewarded tactic for gathering actionable intelligence before committing.

## Time Scale and Day/Night

A full day/night cycle where darkness meaningfully affects stealth detection, patrol
visibility, wildlife behavior, and general travel danger — genuine tactical cover for
illicit activity, not a cosmetic lighting change. A campaign spans **1–5 in-game years** at
a current baseline of 40 real-time minutes per in-game day (subject to further
compression), deliberately abstracting crop cycles, research timelines, and character
skill growth so systems stay meaningful within that window rather than a realistic human
timescale. There's no generational mechanic — the squad that starts the campaign can, in
principle, still be present at its end. On a non-Earth setting, the compression ratio is
defined in real minutes per local day/year, not Earth units.

## Wildlife

Wildlife is a fixed, zone-based danger — unlike factions, it never progresses or gets
stronger over time. A region dangerous at campaign start is exactly as dangerous at
campaign end; there's terrain worth routing around early and returning to later with a
stronger squad, but it never quietly ramps up in the meantime. Wildlife has no standing
and no politics — it's purely a logistics and route-planning problem, reacting to
proximity and hunger, never to player reputation. How that reaction actually plays out —
eat, ignore, stalk, or flee — is owned by `ai-and-behavior.md`, which treats creatures as
the same behavior system as everyone else with drives rather than politics in the
foreground.

## Environmental Hazards and World Events

Harsh terrain, climate extremes, and setting-specific events shape routing and base
placement, but consistently enough to anticipate and plan around — never as a random
punishment. Beyond faction-driven world state (`factions-and-world-state.md`), the world
generates its own events — conflicts, trade disruptions, migration, and (fog-of-war only;
the map is static, nothing spontaneously appears) resource discovery — that emerge from
the underlying simulation rather than a scripted sequence. The player learns about these
by being present, keeping contacts, or reading the economic aftermath — never a push
notification.

## Discovery Is Rewarded Concretely

Finding a resource deposit enables better base placement; locating an illicit market opens
a new economic channel; mapping patrol routes enables safer or more profitable operation
in that region; ruins yield salvage or tech specimens; a hidden settlement unlocks new
recruits. There's no exploration XP — the reward for exploring is the information and
access itself.

## Open Design Questions Worth Tracking

- Exact map scale and region count aren't determined yet — both significantly affect
  travel time, logistics complexity, and how far factions can realistically spread.
- No fast travel is a decided stance, not just a default (see `world-map-and-travel.md`).
  Known roads/routes moving faster than raw terrain is the only speed-up mechanic.
- Setting-specific terrain types and environmental hazards are deferred until setting is
  chosen.
