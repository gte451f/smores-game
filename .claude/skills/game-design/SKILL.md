---
name: game-design
description: Reference and maintain smores' high-level game design intentions — vision, pillars, and the desired end states for every major game system (factions, economy, combat, characters, base building, tech, the open world, saves, player experience, multiplayer/content, menus, UI, narrative, and more), with no implementation detail. Use when discussing game design direction, checking whether a proposed feature fits the game's pillars, or documenting/updating design intent. Skip for engine implementation, C++ systems, or MCP/editor work — use the game-systems skill and Source/smores/ for that.
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
| [`topics/player-experience.md`](topics/player-experience.md) | Onboarding philosophy, Codex, tooltips, settings, accessibility |
| [`topics/multiplayer-and-content.md`](topics/multiplayer-and-content.md) | Co-op vision and scope, update/patch philosophy toward players, mod/DLC design relationship |
| [`topics/main-menu-and-meta-flow.md`](topics/main-menu-and-meta-flow.md) | *(placeholder)* Title screen, new-game setup, continue/load, options structure, announcements |
| [`topics/difficulty-and-modifiers.md`](topics/difficulty-and-modifiers.md) | *(placeholder)* New-game difficulty modifiers beyond ironman |
| [`topics/tutorial-and-scenario-start.md`](topics/tutorial-and-scenario-start.md) | *(placeholder)* Concrete starting scenario, squad, and first objective |
| [`topics/player-interface.md`](topics/player-interface.md) | *(placeholder)* In-session UI intent — always-on vs. contextual info, squad status, diegetic presentation |
| [`topics/notifications-and-alerts.md`](topics/notifications-and-alerts.md) | *(placeholder)* What triggers a notification, severity, history |
| [`topics/world-map-and-travel.md`](topics/world-map-and-travel.md) | *(placeholder)* Map presentation and travel, may merge into `open-world.md` |
| [`topics/narrative-and-lore.md`](topics/narrative-and-lore.md) | *(placeholder)* Worldbuilding, backstory delivery, in-fiction system justification |
| [`topics/quests-and-objectives.md`](topics/quests-and-objectives.md) | *(placeholder)* What structures player goals, if anything, beyond emergent play |
| [`topics/end-game-and-win-loss.md`](topics/end-game-and-win-loss.md) | *(placeholder)* Whether any fixed win/loss condition exists |
| [`topics/character-death-and-permadeath.md`](topics/character-death-and-permadeath.md) | *(placeholder)* Death consequences outside ironman mode |
| [`topics/audio-design.md`](topics/audio-design.md) | Music rotation, camera-as-listener spatial perspective, player audio controls |
| [`topics/input-and-platforms.md`](topics/input-and-platforms.md) | Windows-primary platform scope, Steam Deck via Proton, sell-widely-first storefront priority (Steam/GOG/Epic, achievements/cloud saves deprioritized), controller/touch input targets |
| [`topics/localization.md`](topics/localization.md) | Day-one string/font/layout localization architecture tied to selling widely, translated-language list grows post-launch |

More topics belong here as the game's design surface grows. Add one file per topic under
`topics/`, in the same distilled style, and add a row to the table above.

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
- **Placeholders are expected.** A topic can exist here with only a `**Status:**
  placeholder` line and a few open questions, long before the corresponding system is
  built. Don't wait for implementation to start before creating the file — the point of
  this skill is to capture intent early.
