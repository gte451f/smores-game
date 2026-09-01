# Design Documentation

This folder is the project reference for gameplay systems, player-facing behavior, and implementation notes. These files should describe what the game currently does, where the behavior is implemented, and what future changes must preserve.

The docs are meant for:

- Developers learning or changing a gameplay system.
- AI coding assistants that need stable project context before editing code.
- Future player help files that can be derived from player-facing sections.

## Structure

- `systems/` documents individual gameplay systems and their implementation.
- `player-facing/` documents controls, visible game modes, terms, and player-visible rules.
- `technical/` documents cross-system architecture and implementation conventions.
- `templates/` contains reusable doc templates for new systems.
- `plans/` contains past AI generated plan documents that were eventually implemented by an Agent.  These are for historical reference and need not to be updated by future agents.
- `arch/` contains high level design goals and system descriptions.  These are conceptual in nature and lightly technical.

## Maintenance Rules

- Update relevant design docs when gameplay behavior, controls, scoring, AI, camera behavior, selection rules, or UI-visible rules change.
- Keep docs grounded in current implementation. Put future ideas under `Known Gaps` or `Future Notes`.
- Prefer links to concrete C++ classes, Blueprint assets, maps, and input assets over vague descriptions.
- Do not document binary asset internals unless they are confirmed in the editor or by source-controlled asset metadata.
- When a system has both player-facing behavior and C++ implementation, document both.

## Current System Docs

- [Strategy Camera and Selection](systems/strategy-camera-and-selection.md)
- [Strategy Unit Commands](systems/strategy-unit-commands.md)

## Current Player-Facing Docs

- [Controls](player-facing/controls.md)
- [Game Modes](player-facing/game-modes.md)
- [Terminology](player-facing/terminology.md)

## Current Technical Docs

- [Architecture](technical/architecture.md)
- [Input Model](technical/input-model.md)
- [Save Data](technical/save-data.md)

## High-Level Design (arch/)

Conceptual documents describing complete game systems. Setting-agnostic until a theme is chosen.

- [Game Pillars](arch/game-pillars.md) — vision, player fantasy, design pillars, departures from Kenshi
- [Factions and World State](arch/factions-and-world-state.md) — major/minor factions, relationships, world state simulation, player standing, bounties
- [Economy](arch/economy.md) — supply/demand simulation, trade routes, illicit economy, currency sinks
- [Characters and Squads](arch/characters-and-squads.md) — skills, recruitment, wages, morale, injuries, stealth and illicit skills
- [Combat](arch/combat.md) — automated real-time combat, player control layer, injuries, capture, consequences
- [Base Building](arch/base-building.md) — freeform outpost construction, building types, raids, supply chains, upkeep
- [Tech and Crafting](arch/tech-and-crafting.md) — tech tree structure, research mechanics, crafting facilities and quality
- [Open World](arch/open-world.md) — regions, points of interest, travel, fog of war, world events

### Non-Gameplay Architecture

- [Save System](arch/save-system.md) — world state persistence, save slots, autosave, version and mod compatibility
- [Player Experience](arch/player-experience.md) — tutorial, codex, tooltips, settings, accessibility, localization
- [Content and Release](arch/content-and-release.md) — multiplayer (co-op self-hosted), updates/patches, DLC and expansion design goals
