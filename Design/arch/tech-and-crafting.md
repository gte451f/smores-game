# Tech and Crafting

> Conceptual arch doc. See `game-pillars.md` for design philosophy, `base-building.md` for the facility requirements that gate research, and `economy.md` for how crafted goods feed the trade economy.

---

## Purpose

The tech tree defines what the player can build, craft, and field over the course of a campaign. Crafting converts researched knowledge into physical goods — weapons, armor, equipment, prosthetics, and base infrastructure components. Together they form the progression backbone of the late game: the player starts using whatever they can afford or find, and gradually gains the ability to produce better items than the market offers.

---

## Tech Tree Structure

The tech tree is a directed graph of researchable nodes. Unlocking a node makes available new building types, crafting recipes, or capability improvements. Nodes have prerequisites — you cannot unlock advanced metallurgy without basic metallurgy — and some nodes branch, requiring a choice between parallel paths that are mutually difficult (though not necessarily mutually exclusive) to pursue.

### Tree Branches
The exact branches depend on setting, but the conceptual structure includes:

- **Materials and Processing** — unlocks better raw material extraction, refining, and alloy/compound production
- **Weapons and Armor** — unlocks higher-tier combat equipment recipes
- **Base Infrastructure** — unlocks building types: defensive structures, advanced production facilities, research buildings themselves
- **Medicine and Prosthetics** — unlocks higher quality medical treatment, prosthetic limbs, and field medicine equipment
- **Stealth and Illicit** — unlocks tools for illicit operations: better lock-picks, concealment equipment, kidnapping aids
- **Trade and Logistics** — unlocks facilities and items that improve trade efficiency, cargo capacity, or market access

Branches are not siloed — a player pursuing weapon crafting still needs base infrastructure nodes to build the required facilities.

### Multiple Tech Trees
The game may support more than one tech tree if the setting warrants distinct advancement paths (e.g., a conventional crafting tree alongside a separate salvage/adaptation tree, or faction-specific trees with different philosophies). This is deferred until setting is chosen, but the architecture should not assume a single tree.

---

## Research Mechanics

### Research Facilities
Research requires a dedicated building (or buildings, for advanced nodes) to be constructed at a base. Characters must be assigned to research — they cannot research passively. A node in progress pauses if no character is assigned.

### Research Cost
Each node costs time (character-hours of research) and materials (consumed during the research process). For lower-tier nodes, materials are ordinary raw or processed goods. For mid- and high-tier nodes, research consumes **advancement resources** — rare materials sourced from dangerous locations and the specialized trade economy (see `economy.md`). The material cost is the currency and logistics sink; the time cost is the opportunity cost of assigning characters to research rather than labor or field operations.

Advancement resources are the primary bottleneck for reaching high tech tiers. A player who cannot secure a reliable supply of them — through extraction, trade, or raiding — will stall in mid-tier technology regardless of how much time they invest in research. Securing that supply is a strategic objective in its own right.

### Research Speed
Research speed scales with the skill level of assigned characters (a dedicated scholar-type character advances research faster than a combat specialist filling in). Some nodes may require a minimum skill level in a relevant craft to unlock at all — you cannot research advanced smithing without a character who understands basic smithing.

### Knowledge Sources
Not all research is self-derived. Some tech nodes can be unlocked alternatively through:
- **Purchasing schematics** from traders or faction vendors (faster, costs more currency)
- **Reverse engineering** captured or purchased items (requires the item and a research facility)
- **Faction contracts** that reward tech knowledge as payment
- **Raiding** research facilities belonging to factions (stealing the research rather than doing it)

This creates multiple viable paths to the same technology — the player is not locked into a single slow grind.

---

## Crafting

### Crafting Facilities
Each item category requires a specific production building to craft. A blacksmith building produces metal weapons and armor; a medical lab produces medicine and prosthetics; etc. The building quality (base level, and tech-tree upgrades to it) sets the maximum quality of items that can be produced there.

### Crafting Inputs
Every recipe has defined inputs: material types, quantities, and the skill level of the crafting character. A higher-skill crafter produces items at the upper end of the quality range; a lower-skill crafter produces items at the lower end — and may occasionally produce flawed output.

Higher-tier recipes require **advancement resources** as a direct crafting input alongside ordinary materials. A master-tier weapon is not just a function of skill and facility — it requires rare materials the player must source deliberately. This keeps high-end crafted goods genuinely scarce even in the late game; the skill and facility can be built up over time, but the advancement resource input remains a hard logistical requirement.

### Item Quality
Crafted items have a quality rating that affects their stats. Quality is influenced by:
- The crafter's skill level
- The quality of input materials
- The facility level

A master crafter at a high-level facility using premium materials can produce items that significantly outperform anything available on the open market. Reaching this state is a late-game achievement that requires sustained investment in both the skill tree (character skills through use) and the tech tree (facility upgrades through research).

### Crafting for Sale
Crafted goods can be sold into the regional economy. High-quality crafted goods command premium prices, especially in regions with poor supply of that item type. A player who has invested in weapon crafting can become a significant regional supplier — or can corner a market by producing more than regional demand requires and depressing the price for competitors.

---

## Modding Considerations

The tech tree and crafting recipes are explicitly called out in `game-pillars.md` as targets for mod support. This means:

- Tech nodes, recipe definitions, and building requirements must be data-driven (not hardcoded in C++)
- New branches and items should be addable without modifying core code
- Balance values (research costs, recipe ratios, output quality ranges) should be exposed in editable data assets

---

## Faction Military Tech Progression

The tech tree is not exclusive to the player. Factions independently advance through the same military technology tiers over the course of a campaign (see `factions-and-world-state.md` — Faction Military Progression). Each tier in the tree corresponds directly to the equipment and soldier stat/skill envelopes that factions field at that tier.

**Design implication:** The tech tree's tier structure must be defined with faction soldiers in mind, not just player crafting. Each tier should produce a meaningfully distinct soldier profile — better enough that the player will notice when a faction advances, and threatening enough that falling behind the world's military tier is a real strategic problem.

**Advancement resources as a shared bottleneck:** Factions competing for the same advancement resource deposits are competing to progress their military tech. The player is in that competition. Controlling a key deposit denies it to factions whose territory borders it; losing control to a faction accelerates their progression. Resource geography and faction tech race are the same strategic pressure viewed from different angles.

---

## Known Gaps / Future Notes

- **Exact tech tree nodes and branching structure**: Deferred until setting is chosen. The categories above are placeholders.
- **Multiple tech trees**: Whether the game supports faction-specific or parallel alternative trees is undecided.
- **Tech tree visualization**: How the player views and navigates the tech tree in the UI is not yet designed.
- **Research bottlenecks**: Whether there should be hard bottlenecks that force sequential progression (e.g., you must unlock Tier 2 before any Tier 3 node) or whether the tree is more open is an open balance question.
- **Stolen / gifted tech**: The mechanics of acquiring tech through non-research paths (raiding, purchasing, faction rewards) need detail during systems design.
