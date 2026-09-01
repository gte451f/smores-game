# Game Pillars

## Vision

An open-world, squad-based survival and strategy game where the player starts with nothing and must claw their way to relevance inside a world that neither notices nor accommodates them. Progress is earned through accumulated knowledge of the world's systems — its factions, its economies, its dangers — and through the slow, fragile growth of a squad of individuals who get better only because they survived long enough to learn.

The game is not a pure sandbox. It offers a curated set of optional end-game objectives — advertised win conditions a player can choose to pursue — while the world continues to exist and evolve beyond any single victory. A player who ignores all objectives and plays purely to their own agenda is equally supported. The objectives exist to give players who want a compass a reason to endure the long climb.

---

## Player Fantasy

- **Starting with nothing and making something real.** The opening hours are a fight for basic survival. The late game is a self-sustaining operation with supply chains, trained personnel, and a footprint in the world.
- **Understanding a hostile world and turning it to your advantage.** Faction tensions, trade shortages, and political shifts are readable patterns the knowledgeable player can exploit.
- **Watching individuals become specialists.** The recruit who nearly died on the first patrol becomes your best field commander because they kept getting back up — and their wages reflect it. Squad members are hired and retained for ongoing pay. As their skills grow, so does their asking price. Keeping a capable squad in the field is a continuous economic commitment, not a one-time cost.
- **Building something that persists.** A base is not decoration — it is infrastructure, defense, production, and a statement that you intend to stay.
- **Operating in the shadows.** Stealth, pickpocketing, theft, fencing stolen goods, kidnapping, and the slave trade are fully supported playstyles. A squad can survive — and profit — entirely through illicit means if the player is skilled and careful enough.

---

## Design Pillars

### 1. Earned Progress
Nothing is given. Experience comes from surviving, not from completing quests. Resources require logistics, not just walking to a vendor. A squad member's competence reflects real accumulated risk, not a level-up screen. This applies equally to illicit skills — stealth, pickpocketing, and lockpicking improve through use, not through menu allocation. A veteran squad is also an expensive one — rising wages mean the player must keep generating income to hold the team together.

### 2. Consequence
Actions leave marks. Characters carry injuries that do not vanish at rest. Faction standings shift when you raid a caravan or defend a settlement. Killing a faction leader changes what the faction does next. The world remembers.

Illicit actions are subject to the same rule. Being caught stealing, pickpocketing, or fencing goods damages standing with the relevant faction and may trigger active pursuit. Kidnapping and slaving operations that are witnessed or traced back to the player carry serious political consequences. Running a criminal operation successfully requires managing exposure — not just skill.

### 3. Unscripted Narrative
There is no authored story. The player's narrative emerges from the collision of systems: a shortage creates an opportunity, a betrayal creates an enemy, a lucky recruit becomes irreplaceable. No two campaigns should feel the same.

### 4. Living World
The simulation runs whether or not the player is watching. Factions expand, collapse, and negotiate. Trade routes open and close. Other squads compete for the same resources and territory. The player is one actor in a world full of them.

### 5. Squad Identity
Characters are individuals, not interchangeable units. They carry unique skill histories, injuries, prosthetics, and specializations that build over time. Losing a veteran squad member is a real setback, not a respawn.

### 6. Systems Mastery as Power
The path from weak to strong runs through understanding, not grinding. A player who reads the economy, exploits faction rivalries, times their base expansion to regional stability, and equips their squad for the specific threat they face will outperform a player who simply plays more hours.

### 7. Optional End-Game Objectives
The game ships with a defined set of win conditions that are advertised upfront and trackable in-game. These are not mandatory — the sandbox remains valid without them — but they are designed to be ambitious enough that completing any one of them represents mastery of multiple interlocking systems.

Example objective categories (specifics TBD by setting and systems design):
- **Economic dominance** — control a threshold share of a regional economy, eliminate or absorb key competitors
- **Military/political ascendancy** — install a leader, destroy a ruling faction, or forge a dominant alliance
- **Self-sufficiency** — achieve complete independence from external trade for a sustained period
- **Territorial presence** — physically hold and defend a defined set of high-value locations for a sustained period; the player does not claim political ownership but their squad's presence and reputation shapes who operates there
- **Legacy** — grow a squad to a specific collective power threshold; build a named landmark

Each objective category should have at least one "hard mode" variant requiring the player to meet the condition under active pressure (hostile factions pursuing a counter-objective). Objectives can be combined or stacked for players seeking maximum challenge. Community-contributed objectives should be supportable through the modding system (see below).

---

## Key Departures from Kenshi

| Topic | Kenshi | This Game |
|---|---|---|
| Setting | Post-apocalyptic desert world | Theme TBD — setting is not baked into core systems |
| Tech progression | Research benches unlock blueprints | Structured tech tree(s) with branching paths; factions also advance through the same tiers independently |
| Economy | Present but not a primary focus | Emergent supply/demand per market (no fixed NPC prices); player-exploitable price signals; two-tier caravan trade system |
| Base building | Freeform outpost construction; multiple bases possible | One active outpost at a time; purchasable town buildings inside faction towns as a limited stepping stone |
| Base costs | No ongoing territorial rent | Outposts pay faction rent (lower cash cost, high non-cash burden); town buildings pay higher lease (lower non-cash burden) |
| Combat emphasis | Melee-inclined | Melee primary by explicit design; ranged constrained by friendly fire, slow rate of fire, and shield defense; no artillery, AOE nukes, or sustained fire |
| Character progression | Use-based skill growth | Same; plus distinct slow-changing attributes vs. faster-changing skills; lineage system for setting-agnostic racial/species differences |
| Starting options | Multiple preset starts varying wealth/skills | TBD — at minimum one "destitute" start; others possible |
| End-game goals | Pure sandbox, no defined win conditions | Optional curated win conditions advertised upfront; sandbox play remains valid |
| Squad retention | Recruits join and stay at no ongoing cost | Recruits charge an initial fee and an ongoing wage; wage scales with skill level — a deliberate currency sink |
| Map generation | Procedurally generated world each playthrough | Static, handcrafted map — same geography every playthrough; dense with authored POIs |
| Faction autonomy | Factions are largely reactive | Factions actively raid, assault, and expand; military strength advances through tech tiers over time at varied rates each playthrough |
| Faction military | Static troop quality | Factions advance through 1–4 military tiers with faction-specific troop names; gear separately gated by resource access + tech |
| Player political role | Can build large squads that effectively become factions | Player is always a squad-level actor — no town ownership, taxation, or trade route control |
| Multiplayer | Single-player only | Co-op 2–4 players, self-hosted server; designed in from the start |
| Theft and illicit play | Exploitable for large gains and gear skipping | Constrained by per-session limits, security escalation, stolen goods recognition, fencing requirements, and consistent outcomes on reload |

---

## What This Game Is Not

- Not a city builder. The player is always in the field; the base serves the squad, not the other way around.
- Not a traditional RPG. There is no single hero. Character death is recoverable at the squad level but costly.
- Not an RTS or turn-based tactics game. Combat is real-time and automated — characters fight on their own. The player's combat role is directing movement, setting target priorities, and deciding when to engage or retreat, not micromanaging individual attacks or taking turns.
- Not a 4X or faction management game. The player cannot own towns, set taxes, control trade routes, or declare war as a political entity. Their influence is expressed through reputation, squad capability, and economic activity — not territorial sovereignty. Factions conduct their own expansions, wars, and diplomacy independent of the player.
- Not a power fantasy. The game should feel dangerous at every stage. Late-game strength is relative to late-game threats.

---

## Tone and Difficulty Philosophy

The game is punishing by design. Early failure is expected and instructive. The systems are learnable but not hand-held. There are no difficulty sliders in the core experience — the world behaves consistently and the player must adapt to it.

**Failure should be informative.** When a patrol is wiped out, the player should understand what happened and be able to reason about what they would need to do differently. Failure from invisible systems or poor feedback is a design bug. Failure from underestimating a known danger is correct behavior.

**Recovery should always be possible.** A catastrophic loss — base destroyed, squad gutted — should leave some path forward, even if steep. Unrecoverable states should only exist if the player has truly exhausted every option. Soft-locking the player through opaque systems is a failure mode to avoid.

---

## Modding and Extensibility

Mod support is a first-class feature, not an afterthought. The game should be designed from the start with community contribution in mind.

**Distribution targets:**
- Steam Workshop is the primary channel.
- The architecture should not foreclose other ecosystems (e.g., Nexus Mods, in-game mod browser).

**What mods should be able to add or change:**
- Factions (new major/minor factions, relationships, starting territories)
- Items, equipment tiers, and crafting recipes
- Tech tree nodes and branches
- Regions, biomes, and points of interest
- End-game objectives
- Character types and skill definitions
- Economy goods and trade route configurations

**Design implication:** Core systems must be data-driven where possible. Hardcoded faction IDs, item tables, or tech trees that live only in C++ are a modding anti-pattern. Blueprint subclassing and data assets are preferred over hard-coded logic for anything a modder would reasonably want to change.

**Out of scope for modding (initial):** Engine-level changes, multiplayer modifications, and changes to core save/load serialization format.

---

## Time Scale

A campaign spans **1–5 in-game years**. One in-game day equals **40 real-time minutes** (current baseline; may be reduced further during development). The game has a full day/night cycle that affects stealth, patrol behavior, and travel risk.

If the setting is on a non-Earth world, "day" and "year" refer to that planet's equivalents — the compression ratio (real minutes per in-game planetary cycle) is what is fixed, not the Earth unit labels. See `open-world.md` for the full time scale table and the setting-dependent calendar note.

Time is deliberately compressed. Crop cycles, research timelines, and skill growth all occur faster than realistic. Players are expected to accept this abstraction — the alternative is a game that takes years of real time to play. Systems must be tuned to feel meaningful within the 1–5 year window, not on a human historical timescale. There are no generational mechanics; characters do not age out of the campaign.

---

## Core Systems Reference

Brief summaries of each arch document. Read the relevant doc for full design detail; read this section to understand what each system does and how it connects to the others.

### Factions and World State (`factions-and-world-state.md`)
Three faction types: **Major** (control towns), **Minor** (control outposts), **Nomadic** (no fixed location). Factions conduct raids and full assaults against each other independently. Faction military strength advances through 1–4 tiers over time — each tier defines a stat/skill envelope and a gear profile (gated separately by resource access + tech). The player can fall behind the world's military progression. Faction troop tiers use faction-specific terminology ("Veteran Commando", "Elite Shock Trooper") visible on soldiers. A layered intelligence system (scouting, rumors, criminal contacts, NPC relationships) gives the player advance warning of assaults. The player never becomes a faction.

### Economy (`economy.md`)
No fixed NPC prices. Supply and demand per market produce emergent pricing — goods are cheap where produced, expensive where scarce. A two-tier caravan system (outpost caravans to nearby towns; town caravans to friendly/allied factions) physically animates the world economy. Advancement resources are rare high-value materials that gate mid/high tech tiers and are a shared bottleneck between the player and factions. Players participate as traders, producers, raiders, or criminal operators. Fencing stolen goods requires criminal faction standing or travel to distant factions.

### Characters and Squads (`characters-and-squads.md`)
Seven attributes (Strength, Endurance, Agility, Perception, Intelligence, Willpower, Charisma) change slowly through sustained activity. Skills change faster through use — combat, stealth, crafting, trade, and illicit skills all improve by doing. Skill is the primary outcome determinant (~70%); attributes are modifiers (~30%). Every character belongs to a **lineage** (setting-agnostic term for race/species) that applies attribute modifiers, a skill ceiling adjustment, and innate traits. Squad members charge a hire fee and ongoing wages that scale with skill — a continuous currency sink. Theft is constrained by session limits, security escalation, stolen goods recognition, and consistent outcomes on reload.

### Combat (`combat.md`)
Real-time automated combat — characters fight on their own. Player directs movement, target priority, engagement, and retreat. **Melee is primary by design**: ranged is constrained by friendly fire into melee, slow rate of fire, and shield defenses. No artillery, AOE nukes, or sustained fire. Characters are knocked unconscious before death, creating rescue decisions. Capture and prisoner mechanics support ranged consequences. Faction troop tier directly affects combat outcomes through the stat/skill envelope system.

### Base Building (`base-building.md`)
Two forms of player infrastructure: one active **outpost** (freeform construction, one at a time) and **town buildings** (purchased space inside faction towns, limited capability). Outposts pay lower faction rent but carry high non-cash costs (construction, garrison, repair, upkeep). Town buildings pay higher lease but the faction handles defense. Outposts must maintain minimum distance from all authored points of interest. Faction relationship determines rent level and raid/assault threat. Town buildings are lost if the hosting faction loses the town.

### Tech and Crafting (`tech-and-crafting.md`)
Directed graph tech tree with branching paths. Research requires dedicated facilities and assigned characters. Advancement resources gate mid/high tiers. Factions advance through the same military tech tiers independently — controlling advancement resource deposits is both an economic and military objective. Crafted item quality is influenced by character skill, input material quality, and facility level. Multiple knowledge sources: self-research, purchased schematics, reverse engineering, faction contracts, raiding.

### Open World (`open-world.md`)
Static handcrafted map — same geography every playthrough. Dense with authored **points of interest**: ruins, traveler camps, contested sites, resource deposits, faction outposts, criminal locations, and unique landmarks. POIs are exploration scenes with loot, enemies to clear, and neutral NPCs offering recruit, enslave, or minor quest interactions. Wildlife danger is static and zone-defined — does not scale over time. Full day/night cycle affecting stealth, patrol visibility, and travel risk. No fast travel. Fog of war reveals through exploration.

### Save System (`save-system.md`)
Saves the full world simulation state: faction status, economy values, fog of war, named NPC alive/dead, character skills and injuries, base state, and in-progress events. Multiple save slots plus rolling autosave backups. Save files reference content by ID for mod and version compatibility. Breaking schema changes ship with migration scripts.

### Player Experience (`player-experience.md`)
Tutorial teaches controls only — not strategy. Codex records faction terminology, discovered locations, and encountered items as the player builds knowledge. Full settings (graphics, audio, controls, UI, gameplay). Accessibility is a baseline requirement: colorblind modes, full rebinding, hold-vs-toggle, pause anywhere. All strings externalized for localization from day one.

### Content and Release (`content-and-release.md`)
Co-op 2–4 players, self-hosted server (listen server or dedicated). Each player manages their own squad in a shared simulated world. **Multiplayer must be designed in from the start** — every C++ gameplay system requires server-authority and replication. DLC is a long-term goal (additional biomes, lineages, game systems) using the same data-driven architecture as mods.

---

## Known Gaps / Future Notes

- **Setting and theme not yet determined.** Once chosen, this document should be updated with a brief world-context section that other arch docs can reference. Many systems have content deferred pending this decision (lineage roster, biome specifics, tech tree nodes, weapon types).
- **Starting scenario variety** (equivalent to Kenshi's start options) is not yet designed.
- **End-game objective specifics** are deferred until setting and systems are further developed. The categories above are placeholders.
