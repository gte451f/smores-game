# Open World

> Conceptual arch doc. See `game-pillars.md` for design philosophy, `factions-and-world-state.md` for territorial ownership, and `economy.md` for how regions connect through trade.

---

## Purpose

The open world is the canvas on which all other systems operate. It is a large, continuous landmass divided into regions with distinct characteristics — terrain, resources, climate, faction presence, and danger level. The world has no loading screens between regions. The player can go anywhere from the start, but not everywhere safely. Distance, terrain, and threat level are the real barriers, not invisible walls.

The world is not designed around the player. It has its own internal logic — factions hold territory for reasons, resources exist where the geography makes sense, danger is distributed according to the behavior of the inhabitants. The player learns the world by exploring it, and that knowledge is a form of power.

---

## Map Design

The world map is **static and handcrafted**. Every playthrough uses the same geography — the same regions, terrain features, settlement locations, resource deposits, and points of interest. This is a deliberate design choice:

- Experienced players carry real knowledge from prior runs; learning the map is a form of mastery
- Faction pre-dispositions, trade routes, and authored NPC placements all depend on a consistent world
- Mod-added regions or locations can extend the map without breaking static-world assumptions

Procedural generation is used for NPC populations and economy values within the static world, not for the world's shape or structure.

---

## World Scale and Regions

The map is large enough that traversal takes meaningful in-game time and creates genuine logistical challenges. It is divided into named regions, each with:

- **Dominant terrain type** — plains, forest, desert, mountains, wetlands, etc. (specifics TBD by setting)
- **Resource profile** — what raw materials are naturally present and in what abundance
- **Climate / environmental hazards** — conditions that affect character health, movement, and base operation
- **Faction presence** — which factions hold or contest the region, and how densely they patrol it
- **Danger rating** — an emergent property of the inhabitants; not a UI label, but something the player learns from observation and reputation

Regions are not isolated — they bleed into each other. Moving from a forested region into a mountain range is gradual, not a hard seam. But the distinction matters for resource extraction, base placement, and faction behavior.

### Biomes and Resource Distribution

Resources are distributed across biomes in a common-sense manner. No biome produces everything. This uneven distribution is the physical foundation of the trade economy — goods are cheap where they are produced and expensive where they are not, creating the price gradients that make trade profitable and transport risky worth considering.

**Design principle:** A player who understands the biome map understands the economic map. The resource geography is consistent every playthrough and learnable.

| Biome | Resource character |
|---|---|
| **Arid / Desert** | Scarce food and water; may yield unique minerals or extractables; high import demand for agricultural goods |
| **Forest / Jungle** | Animal materials, timber, foraging goods; dangerous wildlife raises extraction cost; dense cover favors stealth operations |
| **Plains / Grassland** | Highest farming yield; food surplus; primary export region for agricultural goods; low danger, easy traversal |
| **Mountain / Highland** | Ore and stone rich; food-poor; terrain slows movement and raises transport cost for all goods |
| **Wetland / Coastal** | Fishing and water-adjacent materials; goods unavailable inland; moderate danger; limited agriculture |

Specific resource names are TBD until setting is chosen. The table above describes resource *character*, not a list of items.

**Danger scales with resource value.** The most resource-rich biomes tend to be the most dangerous — dense forests shelter predators and bandits, mountains isolate caravans, arid regions attract desperate factions. This is intentional: high-value resources require real effort and risk to extract and transport, which is what gives them their price premium in distant markets.

---

## Points of Interest

The world map is dense with authored points of interest. POIs are the primary content of exploration — discrete locations with their own physical layout, inhabitants, loot, and interaction potential. The player discovers, clears, loots, negotiates, or avoids them as they choose. There is no quest marker directing the player to most of them; stumbling across a POI while traveling is a normal and intended experience.

**POIs are static and authored.** Their location, layout, inhabitants, and loot profile are the same every playthrough. A ruin that contains a named NPC and a cache of rare materials will always be at that location with that content. This is what makes world knowledge a form of mastery — an experienced player knows where things are.

The world should feel full. Dead space between settlements is an opportunity for a traveler camp, a contested site, an abandoned structure, or a resource deposit. Players moving through the world should encounter something of interest regularly enough that travel feels like exploration, not transit.

### NPC Presence and Interactions

Many POIs contain NPCs who are not affiliated with any major faction — survivors, wanderers, isolated communities, desperate individuals, or groups that exist outside the political map. These characters offer a distinct interaction set that differs from faction NPC interactions:

**Recruit** — Neutral NPCs at POIs are a recruitment source outside of faction settlements. A skilled survivor encountered in a ruin, a stranded mercenary at a traveler camp, an escaped slave hiding in the wilderness — these individuals can potentially be convinced to join the squad. Recruitment terms follow the same hire-fee and wage model as other recruits; the difference is context and availability. Some POI recruits have unusual skill profiles unavailable through normal faction channels.

**Enslave** — Neutral NPCs can be subdued and taken prisoner for sale into the slave trade (see `economy.md`). This is mechanically identical to kidnapping any NPC — bring their health low without killing them, carry the unconscious body, transport to a slave market. Whether this is witnessed affects faction standing consequences. A POI outside faction patrol range provides a more isolated environment for this, reducing exposure risk.

**Minor quests** — Some NPCs at POIs offer small tasks in exchange for payment, goods, or standing. These are not authored quest lines — they are systemic, context-driven interactions: an NPC who needs escort to the nearest settlement, a group that will pay for cleared enemies nearby, an individual with a specific item to trade for a specific other item. Completing these provides currency, goods, or a small standing benefit. They are incidental content, not a quest system.

The three interaction types are not mutually exclusive and are not guaranteed at every POI. A given POI may support all three, one, or none depending on who is there and what state the location is in.

### POI Categories

#### Settlements and Towns
Populated locations controlled by factions. They function as trade hubs, recruitment pools, and faction interaction points. Settlement size and wealth reflects the faction that controls them. Settlements can be destroyed or change hands through world-state events (see `factions-and-world-state.md`).

Settlements contain a mix of **named NPCs** — authored characters with defined roles, faction significance, and permanent death — and **procedural NPCs** generated to fill roles like Shop Keeper, Guard, or Laborer. Named NPCs are placed in specific settlements by design; their location is consistent across playthroughs. Procedural NPCs repopulate over time; named NPCs do not return once killed.

#### Ruins and Abandoned Sites
Former settlements, outposts, or facilities no longer actively occupied. May contain loot, salvageable materials, historical information about the world, or dangerous squatters. Some contain research specimens relevant to the tech tree. Ruins are among the richest exploration content — their value is in what was left behind, and what moved in after.

Ruins may be fully abandoned (loot and environmental hazard only), occupied by hostile groups (clear before looting), or home to neutral survivors with interaction potential.

#### Traveler Camps and Isolated Survivors
Small groups or individuals encountered in the wilderness — travelers who never made it to their destination, survivors of faction conflict, isolated communities that predate the current political map. These are primary sites for POI-based NPC interactions: recruitment, minor quests, and illicit activity in isolation from faction witnesses.

#### Resource Deposits
Specific high-quality or rare resource nodes — rich ore veins, unique raw materials, fertile ground — that exceed the baseline resource profile of the region. Meaningful enough to build a base around (subject to minimum distance rules — see `base-building.md`) or contest with other factions. Some resource deposits have prior occupants that must be dealt with before extraction can begin.

#### Contested and Active Sites
Locations where conflict is in progress or recently concluded — two NPC groups fighting over a resource, a caravan under attack by bandits, the aftermath of a faction skirmish. These are dynamic in appearance but static in location: the conflict at a given site is the same type of conflict every playthrough, though exact NPC composition varies. Contested sites create tactical opportunities: the player can pick a side, wait for one group to weaken the other and then engage, or avoid entirely.

#### Faction Outposts and Installations
Military and economic infrastructure belonging to factions — forward camps, watchtowers, resource extraction operations, trade waypoints. Attacking these has direct faction standing consequences. Observing them provides intelligence on faction strength and patrol patterns. Some faction outposts are hostile to the player regardless of standing (bandit camps, enemy faction forward positions).

#### Criminal and Illicit Locations
Hidden markets, smuggling waypoints, slave markets, fence contacts. These locations are not marked on any starting map. They are discovered through faction standing with criminal networks, word of mouth from certain NPCs, or direct exploration. Operating in these locations requires either criminal network standing or willingness to engage on those terms.

#### Unique Landmarks
Locations with special significance — historical, economic, or strategic. Unique in the world; there is only one of each. Some are tied to end-game objectives (controlling a landmark, destroying one, or holding it for a sustained period). Others are simply significant world features that reward exploration with context, lore, or unique items.

### POI Density and Pacing

POI density should vary by region in a way that reflects the world's logic. Settled, patrolled regions have fewer derelict ruins and isolated survivors — people don't tend to get stranded where law and trade function. Dangerous, remote, or contested regions have more — abandoned infrastructure, survivors of conflict, groups that operate outside faction oversight.

The intent is that the player never crosses a large stretch of map without encountering something worth noticing. Whether they stop and engage is their choice.

---

## Travel and Visibility

### Movement
The squad moves across the world in real time. Movement speed is affected by terrain (rough terrain slows movement), encumbrance (heavily loaded characters move slower), and character athletics skill.

There is no fast travel in the traditional sense — the world does not teleport the player. However, established routes and known paths are navigated more efficiently than pathless terrain. The player may be able to use faction-controlled roads where relationships permit.

### Fog of War
The player has visibility into areas currently occupied or recently visited. Unvisited regions are unknown — their resources, faction presence, and points of interest are not shown until discovered. Visited areas gradually lose currency if the player has not returned — faction control shifts, settlements change, new outposts appear.

Price information from distant markets, intelligence on faction movements, and the location of illicit contacts all require maintaining a presence or network of contacts in those areas.

### Scouting
Sending small elements of the squad ahead to scout terrain before committing is a supported tactic. A high-perception, high-stealth character can gather intelligence on a location without triggering combat. Information gathered through scouting (enemy positions, patrol routes, settlement garrison size) is actionable.

---

## Time Scale and Day/Night Cycle

### Day/Night Cycle
The game has a full day/night cycle. Darkness is not cosmetic — it affects stealth detection, patrol visibility, wildlife behavior, and the general danger of travel. Operating at night is riskier but provides meaningful cover for illicit activity. The cycle is consistent and predictable; players learn its rhythm and plan around it.

### Time Compression Ratio
One in-game day lasts **40 real-time minutes** as a current baseline. This is subject to further reduction during development — 40 minutes is a safe starting point, not a final commitment.

| In-game period | Real time |
|---|---|
| 1 in-game hour | ~1 min 40 sec |
| 1 in-game day | 40 real minutes |
| 1 in-game week | ~4 hrs 40 min |
| 1 in-game month (~30 days) | ~20 real hours |
| 1 in-game year | ~243 real hours (~10 real days) |

A 1-year campaign at this ratio is approximately 243 hours of play — comparable to a long Kenshi playthrough, which is the intended reference. A 5-year campaign is the upper ceiling, not the expected average.

### Setting and Calendar Units

The ratio above assumes an Earth-like day (24 hours) and year (365 days). If the game is set on another planet, neither of those units is guaranteed:

- A planetary day may be longer or shorter than 24 hours
- A planetary year may contain more or fewer days than Earth's
- The compression ratio should be defined in terms of **real-world minutes per in-game planetary cycle** (whatever the world's day equivalent is), not anchored to Earth units

When a setting is chosen, the time scale section should be updated with the actual in-world calendar — how many hours in a local day, how many days in a local year — and the real-time ratio recalculated accordingly. The compression principle remains the same; the units that fill it in are setting-dependent.

### Target Campaign Length
A full campaign spans **1–5 in-game years**. This is the time frame within which the player is expected to pursue and complete end-game objectives, grow their squad, build infrastructure, and engage with the faction simulation.

### Time Compression Abstraction
A 1–5 year in-game span compressed into a playable campaign requires deliberate abstraction. Players must suspend disbelief in a few areas:

- **Crop and resource cycles** are dramatically accelerated. Harvests occur on a game-friendly schedule, not a realistic agricultural one.
- **Research and technology** advances in days or weeks of in-game time, not years. A faction developing new weapons technology in a few in-game months is accepted as an abstraction, not simulated realism.
- **Character skill growth** is faster than realistic. A recruit becoming a competent fighter over weeks rather than years is part of the game's compressed timeline.
- **Generational play is not modeled.** Characters do not age, retire, or die of old age within the campaign window. There are no successor mechanics or dynasty systems. The same squad members who start with the player can, in principle, be present at the end.

These abstractions are accepted constraints of the design. The compressed timeline exists to make the game playable, not to simulate history. Systems should be tuned to feel meaningful within the 1–5 year window, not to be realistic on a human timescale.

---

## Wildlife

Wildlife populates the world and represents a distinct category of danger — separate from faction threats and separate from environmental hazards.

**Wildlife danger is static.** Unlike factions, wildlife does not progress through tech tiers or get stronger over time. The danger level of wildlife in a given zone is fixed by the authored world design and is consistent across playthroughs. A region with dangerous predators at campaign start has the same dangerous predators at campaign end. Players who return to an area they explored early in the game will not find more powerful wildlife than before.

**Zone-based distribution.** Some zones are inherently more dangerous than others from the start because their wildlife is more lethal — not because anything has changed, but because the world is designed with meaningful geographic danger gradients. Dense forests and remote highlands may shelter creatures that a starting squad cannot handle. This is terrain to route around early and return to with a capable squad later.

**Wildlife as world texture.** Wildlife creates incidental danger on travel routes, shapes which paths are practically safe, and represents an ongoing extraction cost for resource-rich areas (the most productive biomes often have more dangerous wildlife — see `open-world.md` Biomes). They also create opportunities: patrol routes that avoid wildlife-heavy terrain are predictable; caravans routed through safer corridors pay a distance premium.

Wildlife is not a faction. It has no standing, no political relationship with the player, and no economic behavior. It reacts to proximity and hunger, not player reputation. Managing wildlife is a logistics and route-planning problem, not a diplomatic one.

---

## Environmental Hazards

The world presents ongoing environmental challenges beyond faction and wildlife threats:

- **Harsh terrain** — impassable or very slow terrain that shapes routing and base placement decisions
- **Climate extremes** — extreme heat, cold, or precipitation that affects character stamina and health over time; base construction may need to account for environmental protection
- **Environmental events** — storms, seasonal changes, or setting-specific hazards (specifics TBD) that affect travel and base operation temporarily

Environmental hazards are not random punishments — they are consistent enough to be anticipated and planned around. A player who knows a mountain pass becomes impassable in bad weather will plan their logistics accordingly.

---

## World Events

Beyond faction-driven world state changes, the world generates periodic events that create opportunities and threats:

- **Faction conflicts** — wars break out, borders shift, settlements change hands; observable from a distance before they directly affect the player
- **Trade disruptions** — a route becomes unsafe due to increased bandit activity or faction conflict, creating regional shortages
- **Migration** — population movement between settlements, potentially creating recruitment opportunities or destabilizing a region's economy
- **Rare resource events** — a previously unknown deposit is revealed through exploration, or an existing one is depleted; shifts regional economic incentives. Note: the map is static — no deposits appear spontaneously. "Discovery" means fog-of-war reveal of an existing deposit, not a new one emerging.

World events are not scripted sequences. They emerge from the faction simulation. The player does not receive push notifications — they discover events by being present, by having contacts in a region, or by observing the economic consequences after the fact.

---

## Discovery and Exploration Rewards

Exploration is rewarded concretely, not through abstract experience:

- Finding a rich resource deposit enables better base placement
- Locating an illicit market opens a new economic channel
- Mapping a faction's patrol routes enables safer or more profitable operations in that region
- Discovering ruins may yield salvage, tech tree specimens, or unique items
- Finding a hidden settlement may unlock new recruitable NPCs

There is no exploration XP. The reward for exploring is the information and access itself.

---

## Known Gaps / Future Notes

- **World scale specifics**: The exact map size and region count are not yet determined. These are significant design decisions that affect travel time, logistics complexity, and faction spread.
- **Fast travel or equivalent**: Strict no-fast-travel may be revisited depending on map scale. Faction-controlled transport networks (roads, ferries) could serve as a diegetic equivalent without breaking immersion.
- **Setting-specific terrain**: The specific terrain types, environmental hazards, and region characteristics are deferred until setting is chosen.
- **Map persistence on save**: How the world state (fog of war, discovered locations, faction territory) is saved and loaded is a technical question for `technical/save-data.md`.
