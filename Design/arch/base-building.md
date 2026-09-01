# Base Building

> Conceptual arch doc. See `game-pillars.md` for design philosophy, `factions-and-world-state.md` for how faction relationships affect base security, and `economy.md` for production chain economics.

---

## Purpose

A base is the player's footprint in the world. The game is not a city builder — the player is always a squad-level actor and infrastructure serves that role, not the other way around. To keep scope appropriate, the player has two distinct forms of physical presence in the world, each with deliberate constraints:

- **One active outpost** — a freeform constructed installation in the open world. The player may have only one at a time.
- **Town buildings** — purchased space inside an existing faction-controlled town. Limited in capability but available earlier and in multiple locations simultaneously.

Together these two modes cover the progression from early-game foothold to mid- and late-game self-sufficiency, without expanding into city-builder territory.

---

## Town Buildings

A town building is a purchased workspace inside a faction-controlled town. The player does not own the building outright — they hold an occupancy agreement with the controlling faction. It is a stepping stone, not a replacement for a full outpost.

### What a Town Building Provides
- **Limited crafting** — small-scale production of basic goods; enough to sustain the squad or produce trade goods, but not full production chains
- **Limited tech progression** — lower-tier research is possible at a town building; mid- and high-tier research requires a full outpost with dedicated research facilities
- **Storage** — modest on-site storage for materials and finished goods
- **A presence in the town** — the player has a reason to be in and around the town, access to local markets without standing requirements, and a base of operations for nearby operations

### What a Town Building Does Not Provide
Full production chains, high-tier research, defensive fortifications, and the capacity to house and support a large squad are all outpost-only capabilities. A player relying solely on town buildings will eventually hit a ceiling that only an outpost can resolve.

### Building Limits by Faction Standing

| Standing with controlling faction | Buildings purchasable |
|---|---|
| Neutral | 1 building |
| Friendly | 2 buildings |
| Allied | 2 buildings (same cap; the benefit of allied standing is elsewhere) |

A second building in the same town significantly expands what is possible — a player might use one building for crafting and one for research, or one for storage and one for production. Reaching Friendly standing before purchasing a second building is the intended progression gating mechanism.

### The Risk: Buildings Are Tied to Faction Control

A town building is only as secure as the faction that controls the town. If the faction loses the town — to a rival faction assault, to world-state collapse, or to player-engineered power vacuum — the player's building is lost with it. Goods stored inside may be looted or inaccessible.

This is intentional. It gives the player a material stake in the town's survival independent of faction standing. A player who has invested in a town building will find that defending that town when it comes under assault is not abstract political alignment — it is protecting their own infrastructure. This creates a natural on-ramp to the assault participation system (see `factions-and-world-state.md`).

The player cannot fortify or defend their town building directly. Their influence on the town's survival is indirect: through their squad's participation in defense, through their faction standing, and through the strength of the controlling faction they chose to invest in.

### Purchasing and Ongoing Lease

Buildings are purchased from the controlling faction for a one-time acquisition cost that scales with the town's size and wealth — a building in a major trade hub costs more than one in a small outpost town. The player cannot negotiate price; it is set by the faction.

Beyond the purchase price, **buildings require regular periodic lease payments** to the controlling faction. Lease payments are an ongoing currency sink for as long as the player holds the building. Non-payment follows the same pattern as unpaid outpost rent: the faction relationship deteriorates, and if left unresolved the building may be repossessed and the player's access revoked without refund.

**Lease payments are intentionally higher than outpost rent.** This is a deliberate design philosophy: town buildings provide substantial value that the player does not pay for in any other way — the faction defends the town at its own cost, the player has free access to market infrastructure, trade routes, and recruitment pools that an outpost cannot replicate. The premium lease cost reflects those embedded benefits. A player who wants cheap ongoing costs must accept the non-cash burden of running their own outpost.

| Form of presence | Cash cost | Non-cash cost |
|---|---|---|
| **Town building** | High lease (ongoing) + one-time purchase | None — faction handles defense, no construction or repair burden |
| **Outpost (friendly territory)** | Lower rent (ongoing) | Construction, garrison wages, fortifications, repair, full upkeep |
| **Outpost (unclaimed territory)** | No rent | Same non-cash costs plus no patrol deterrence |

The net cost of running an outpost is not necessarily lower than a town building — the non-cash burden of building, garrisoning, supplying, and repairing an outpost is significant. The difference is the *type* of cost: town buildings drain currency steadily; outposts demand time, labor, and squad commitment. Players choose which resource they can better afford at a given stage of the campaign.

When a town changes hands:
- The building and all goods stored inside are lost immediately on transfer of control
- The player receives no refund on either purchase price or any prepaid lease
- If the new controlling faction is one the player has sufficient standing with, they may be able to repurchase under the new regime — at the new faction's price

---

## Outpost Construction

The player may have **one active outpost at a time**. This is a deliberate scope constraint — not a progression gate that unlocks more slots later. If the player wants to relocate, they must abandon or lose the current outpost before establishing a new one. Town buildings (see above) are the mechanism for maintaining multiple presences across different locations simultaneously.

Construction is freeform. The player places buildings within a chosen location without a rigid grid or prescribed layout. Terrain, resource proximity, and defensibility are the real constraints — not tile patterns.

### Site Selection
Where a base is built matters significantly:

- **Resource proximity** — placing production buildings near the raw materials they consume reduces transport costs
- **Terrain defensibility** — cliffs, chokepoints, and elevation advantage reduce the force needed to hold against raids
- **Faction territory** — faction relationship with the local controlling faction determines rent obligation and threat level (see below)
- **Region economy** — proximity to trade routes and settlements affects how easily the player can sell surplus production
- **Minimum distance from points of interest** — outposts cannot be placed within a minimum radius of any existing landmark: towns, faction outposts, ruins, resource deposits, criminal locations, unique landmarks, or any other authored point of interest (see `open-world.md`). This prevents the player from building directly on top of world features and keeps the authored world legible. The exact radius is a tuning parameter; it should be large enough that the outpost feels like a distinct new presence in the world, not an extension of an existing location.

### Faction Territory and Rent

Building in or near faction-controlled territory creates an ongoing obligation to that faction. The relationship determines both the cost and the threat:

| Faction relationship | Obligation |
|---|---|
| **Friendly** | Pay rent — a periodic fee to the faction in exchange for tolerance and some patrol deterrence. Lower cost. |
| **Neutral** | Pay rent — same mechanic, higher cost. Neutral factions are less invested in the player's survival; their patrols do not actively deter raiders targeting the player. |
| **Enemy / Hostile** | No rent — the faction will not negotiate. Instead, they send organized raids and, if the player's presence persists and grows, full assault parties to destroy or capture the outpost. |

Rent is a currency sink with a strategic tradeoff: paying it keeps the faction from actively targeting the outpost, but it also means maintaining income sufficient to cover it alongside wages and upkeep. A player who cannot pay rent may find their relationship with the faction deteriorating toward hostile — triggering the raid escalation they were paying to avoid.

Building in unclaimed territory (no controlling faction) avoids rent entirely but forfeits any patrol deterrence and typically means more dangerous opportunistic raider activity.

### Building Placement
Buildings are placed manually by the player. Construction requires materials (type and quantity determined by building) and time (characters assigned to construction work complete it over hours/days of in-game time). Some buildings have adjacency requirements or restrictions (e.g., a smelter needs to be near ore deposits; a well must be near a water source — specifics TBD).

---

## Building Types

Buildings fall into functional categories. The specific building roster is not fully defined at this stage — it will depend on setting and tech progression — but the categories are:

### Resource Extraction
Extract raw materials from the environment. Production rate depends on the richness of the nearby deposit and the skill of assigned laborers. These are the foundation of any self-sufficient base.

### Processing / Production
Convert raw materials into processed or finished goods. Each production building has:
- A required input (raw material type and quantity per cycle)
- An output (processed good type and quantity per cycle)
- A skill requirement (assigned character's relevant craft skill affects output rate and quality)
- A power or fuel requirement (depends on setting)

Chains can be built: extraction → processing → finishing. A fully integrated supply chain within a base is more efficient than buying inputs on the open market, but requires more space, more workers, and more upfront construction investment.

### Storage
Holds goods and materials. Without sufficient storage, production halts when capacity is reached. Storage is not a passive feature — positioning it correctly within the base layout affects how efficiently workers move between production and storage.

### Residential / Support
Housing, medical facilities, and facilities that support squad condition. Characters stationed at the base need somewhere to rest and recover. Medical facilities affect injury recovery rate. Food production or storage affects morale. Under-invested support infrastructure degrades the squad over time.

### Defensive
Walls, gates, guard towers, and other fortifications that increase the cost of attacking the base. Defensive structures do not eliminate the need for defenders — they channel attackers into less favorable positions and buy time. A well-fortified but under-garrisoned base still falls to a determined raid; a well-garrisoned but unfortified base loses its advantage of position.

### Research
Facilities required to advance the tech tree (see `tech-and-crafting.md`). Research buildings are typically late-construction priorities — survival and production come first.

---

## Raids and Attacks

Outposts are not safe. Any base the player constructs is a potential target. The likelihood and severity of attacks is driven by:

### Faction Relationships
Factions whose territory overlaps with or neighbors the base location will raid if their relationship with the player is hostile or sufficiently tense. A base built in the middle of bandit territory will be attacked regularly regardless of other factors. A base built in a friendly faction's region benefits from that faction's patrols as a deterrent — but that protection evaporates if the relationship sours.

### Local Reputation
Beyond formal faction standing, the player's local reputation affects how aggressively nearby groups behave. A player who has raided local caravans, attacked nearby settlements, or otherwise acted aggressively in the region will attract more frequent and more determined raids than one who has maintained peaceful relations.

### Base Value / Visibility
A larger, more productive base is a more attractive target. Factions and opportunistic raiders are drawn to bases that visibly accumulate wealth. A sprawling, wealthy outpost in contested territory will be raided harder and more frequently than a small, inconspicuous one.

### Raid Types
- **Opportunistic raids** — small groups testing the base's defenses; low threat if the base has minimal garrison
- **Organized faction raids** — coordinated attacks by a hostile faction's military; scale with faction strength and the degree of hostility
- **Retaliation raids** — triggered specifically by player actions against a faction; proportional to the severity of provocation
- **Siege** — sustained attack by a major faction force; rare but potentially base-ending; typically preceded by escalating smaller raids

### Defense Response
When a raid occurs, stationed squad members defend automatically. The player can be present and direct the defense actively, or return after the fact to assess damage. A base left completely ungarrisoned will fall to even small raids. Minimum garrison requirements scale with base value and regional threat level.

---

## Supply Chains

A base's production is only valuable if it connects to consumption — internal squad needs, sale into the regional economy, or transfer to another base. Supply chains are the player's responsibility:

- **Internal consumption** — food, medicine, and equipment produced at the base are consumed by stationed characters automatically if stored accessibly
- **Export** — surplus goods need to be transported to market; this requires squad members to carry them (or a logistics mechanism TBD by setting) and the time to travel
- **Inter-base transfer** — goods produced at one base can be sent to another, enabling specialized production sites

Supply chain disruption — a caravan intercepted, a trade route closed, a production building destroyed in a raid — has cascading effects on whatever depended on that supply. A base that runs out of food because its supply route was cut will see morale decline and potentially squad members leaving.

---

## Base Upkeep

Bases have ongoing costs beyond initial construction:
- **Repair** — buildings damaged in raids must be repaired using materials and labor
- **Resource consumption** — some buildings require fuel or power to operate continuously
- **Character wages** — characters stationed at a base still require wages (see `characters-and-squads.md`)
- **Food and medical supplies** — stationed characters consume these; shortages affect morale and recovery

A base that consumes more than it produces (net negative) is a drain on the player's overall economy. Expansion should follow income, not precede it.

---

## Known Gaps / Future Notes

- **Building roster**: The specific list of buildings, their exact input/output ratios, and construction costs are not yet defined. These depend on setting and will be designed during systems implementation.
- **Power systems**: Whether bases require a power infrastructure (generators, fuel — TBD by setting) is open.
- **Lease and rent payment mechanics**: How payments are made (automatic deduction, periodic visit to a faction representative, trade goods instead of currency) and what the grace period looks like before non-payment triggers hostility are not yet designed. Applies to both town building leases and outpost rents.
- **Lease-to-rent ratio tuning**: The design intent is that town building leases are higher than outpost rents. The exact ratio — and whether the premium feels fair relative to the embedded benefits — requires playtesting against the full economy to balance. Too high and town buildings feel like a trap; too low and players never build outposts.
- **Outpost capture vs. destruction**: Whether an enemy faction that defeats the outpost garrison takes the outpost over (and can be recaptured) or simply destroys it is an open question. Capture is more interesting but more complex to implement.
- **Town building capability caps**: The exact boundary between what is possible in a town building vs. what requires a full outpost — which research tiers, which crafting recipes, which production buildings — needs a pass once the tech tree and crafting systems are more defined.
- **Abandoning an outpost**: The mechanics of deliberately abandoning an outpost (what happens to stored goods, buildings, and stationed characters) are not yet designed.
- **Unclaimed territory behavior**: What "unclaimed territory" means in a world where factions control most of the map — whether truly neutral zones exist in sufficient number to offer meaningful site selection — needs review once the world map is designed.
- **POI exclusion radius**: The minimum distance from points of interest is a design and feel question as much as a technical one. Too small and the world feels cluttered; too large and viable build sites become rare. Needs tuning against the actual world map scale once that is determined.
