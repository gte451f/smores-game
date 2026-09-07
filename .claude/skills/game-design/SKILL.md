---
name: game-design
description: Reference and maintain smores' high-level game design intentions — vision, pillars, and the desired end states for every major game system (factions, economy, combat, characters, base building, tech, the open world, saves, player experience, multiplayer/content), with no implementation detail. Use when discussing game design direction, checking whether a proposed feature fits the game's pillars, or documenting/updating design intent. Skip for engine implementation, C++ systems, or MCP/editor work — use the game-systems / player-facing skills and Source/smores/ for that.
---

# High-Level Game Design

This skill is the canonical, standalone home for smores' **non-technical** game design:
the outcomes and player experience the game is designed to produce, stated as desired end
states. It deliberately excludes implementation detail (replication, UPROPERTY, Blueprint
vs. C++, toolset wiring, specific C++ class/method names) — that lives in the
`game-systems` skill and in the C++ source itself.

Read this skill when you need to reason about *what the game should feel like* or
*whether an idea belongs*, not *how to build it*.

This topic covers intendent game designs that may not exist yet.

## Topics

| Topic | Covers |
|---|---|
| [`topics/vision-and-pillars.md`](topics/vision-and-pillars.md) | Core vision, player fantasy, the seven design pillars, what the game deliberately is not, tone/difficulty philosophy, campaign time scale |
| [`topics/factions-and-world-state.md`](topics/factions-and-world-state.md) | What factions and the living world are for, the player's fixed non-faction role, faction military progression as a readable signal, assault-intelligence design intent |
| [`topics/economy.md`](topics/economy.md) | Emergent/marginal pricing, the trade hierarchy, player economic roles, the illicit economy, currency sinks, biome and advancement resources |
| [`topics/combat.md`](topics/combat.md) | Automated real-time combat, why melee is primary and how ranged is kept supporting, injury/capture, retreat |
| [`topics/characters-and-squads.md`](topics/characters-and-squads.md) | Lineage, attributes vs. skills, recruitment/wages/morale, injuries, stealth and illicit operations |
| [`topics/base-building.md`](topics/base-building.md) | The outpost/town-building split, site selection, raids, supply chains, upkeep |
| [`topics/tech-and-crafting.md`](topics/tech-and-crafting.md) | Tech tree structure, research mechanics, crafting quality, faction military tech progression |
| [`topics/open-world.md`](topics/open-world.md) | Static handcrafted map, regions/biomes, points of interest, travel/visibility, wildlife, time scale |
| [`topics/save-system.md`](topics/save-system.md) | What must persist, save slots, version/mod compatibility as player-facing promises |
| [`topics/player-experience.md`](topics/player-experience.md) | Onboarding philosophy, Codex, tooltips, settings, accessibility, localization |
| [`topics/multiplayer-and-content.md`](topics/multiplayer-and-content.md) | Co-op vision and scope, update/patch philosophy toward players, mod/DLC design relationship |

More topics belong here as the game's design surface grows — lore, additional systems, new
end-game objective categories. Add one file per topic under `topics/`, in the same
distilled style, and add a row to the table above.

## Working in this skill

- **Write for end state, not mechanism.** "Factions advance through military tiers at
  different rates so the player can fall behind" belongs here. "Tier is a UPROPERTY on
  `UFactionDefinition`" does not — that belongs in `game-systems` or the C++ source.
- **This skill is the source of truth.** There is no separate design corpus behind it —
  when a design decision changes, edit the topic file directly in the same conversation;
  don't wait to be asked to update docs.
- **Flag contradictions instead of silently resolving them.** If two topic files disagree,
  say so inline (see the "Known inconsistency" callout in `multiplayer-and-content.md` for
  the pattern) and mention it to the user rather than picking a side quietly.
- **New subsystem, new file.** Don't grow one topic file to cover unrelated systems —
  split so each file stays skimmable and independently updatable.
- **Cross-references** between topics use bare filenames in backticks (e.g.
  `` `economy.md` ``) since all topics live in the same `topics/` folder.
