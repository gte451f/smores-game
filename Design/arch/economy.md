# Economy

> Conceptual arch doc. See `game-pillars.md` for design philosophy and `factions-and-world-state.md` for how factions shape trade.

---

## Purpose

The economy is a living simulation of supply, demand, production, and trade across the world. It is not a shop menu — it is a system the player can read, predict, and exploit. Prices reflect real scarcity. Shortages create opportunities. Trade routes can be disrupted or protected. The player participates in the economy as a trader, producer, raider, or some combination, and their actions have measurable effects on regional prices and availability.

The economy also functions as the primary pressure system keeping the player engaged at every stage. Squad wages, base upkeep, equipment costs, and research expenditures are continuous drains. Generating income is not optional.

---

## Core Model

### Goods
The world economy is built around a set of tradeable goods. Each good has:

- **A base value** — a reference floor price; the actual market price is always derived from local supply and demand, not from this floor
- **A production source** — the biome, region, faction, or building type that creates it
- **A consumption source** — who buys it, uses it, or needs it to function
- **A weight/volume** — directly affects how much can be transported and at what risk

Goods fall into broad categories:
- **Raw materials** — ore, fiber, food crops, timber, fuel, water; biome-sourced (see `open-world.md`)
- **Processed goods** — refined metals, cloth, preserved food, components
- **Finished goods** — weapons, armor, tools, medicine, luxury items
- **Illicit goods** — stolen property, contraband, slaves
- **Advancement resources** — rare, high-value materials that drive technological progression and unlock higher crafting tiers (see below)

### Goods and Player Utility

Every good in the game must have at least one condition under which a player would rationally choose to craft it, trade it, or buy it. A good with no such condition is wasted design space — it consumes content budget without contributing to gameplay. This is the **player utility requirement**.

Player utility for a good can come from any of the following axes. A good needs only one, but more axes makes it more versatile and more likely to feel relevant across different playstyles and campaign stages:

| Utility axis | What it means |
|---|---|
| **Direct consumption** | The player needs this to operate — food, medicine, repair materials, fuel |
| **Equipment value** | The player or squad equips it — weapons, armor, tools |
| **Crafting input** | This good feeds into a recipe the player wants to produce; its value is derived from what it unlocks |
| **Research input** | Consumed in tech advancement; rare versions are advancement resources |
| **Trade margin** | High enough sale price in at least one market condition that producing or transporting it is economically worthwhile, even if the player never personally uses it |
| **Independence value** | Cheap to buy under normal conditions, but crafting it provides protection against supply disruption — the player who can produce their own food doesn't starve when a trade route closes |

**The luxury good solution.** A good the player would never personally use — a decorative item, a cultural artifact, a prestige consumable — still has player utility if its sale price is high. The incentive is the margin, not the use. Luxury goods justify their existence through trade value alone, and they fill a useful economic role: they give wealthy factions something to spend currency on, which keeps money circulating in the simulation.

**The always-cheap good problem.** A good that is always available at low cost creates a disincentive to craft it. This is only a problem if the good has no other utility axis. Solutions that prevent it from becoming a dead good:

- **Zone variation** — the good is cheap in its producing region but expensive elsewhere; the crafting incentive exists for a player based far from the source
- **Disruption scarcity** — normally cheap, but supply can be interrupted by world events (war, raid, faction collapse); the player who can self-produce is insulated when the market fails
- **Quality differential** — the market version is baseline quality; a skilled crafter can produce a superior version that the market cannot match, creating demand for crafted goods even when the basic version is available cheaply

**Zone-appropriate crafting.** Not every good makes sense to craft in every location. A player building cloth production in a mountain region with no fiber inputs is fighting the world's resource geography. This is intentional design signal — the input availability tells the player what the zone is suited for. A player in a plains region should be thinking about food processing and agricultural goods. A player in a highland region should be thinking about ore refinement and metal goods. Zone-appropriate crafting is more efficient and creates a natural logic to where players build outposts.

**The dead good anti-pattern.** A good fails the player utility requirement if:
- No faction wants to buy it
- The player has no use for it
- It feeds into no crafting recipe
- It has no meaningful sale margin in any market condition
- Its absence or presence creates no interesting economic state

Any good that meets all five conditions should be cut or redesigned. Content review of the goods list should use this checklist explicitly. When a setting is chosen and the specific good roster is designed, each good should be able to answer: *"Under what conditions would a player craft or buy this?"*

---

### Emergent Pricing — No Fixed Prices

There are no fixed NPC vendor prices. Every price in every market is derived from the local balance of supply and demand at that market. The same good costs a different amount in every town, and those prices change over time as production, consumption, and trade flows shift.

This is the Albion Online model applied to a simulated world: prices are what the market makes them, not what a designer set them to be.

**Price crash:** When supply into a market exceeds consumption — whether from NPC faction production, player outpost output, or a flood of imported goods — prices fall. A player who produces the same good as the dominant local faction and tries to sell in that faction's home market will get poor returns. Flooding a market, even with high-quality goods, drives the price down for everyone including the player. First-mover advantage and market selection matter.

**Shortage spike:** When supply drops below demand — due to a disrupted trade route, a production facility destroyed in a raid, a faction at war diverting resources, or seasonal variation — prices spike. The spike is proportional to severity. A mild shortage raises prices modestly. A severe shortage makes the good nearly unavailable at any price and may disable faction functions dependent on it.

**Price lag:** Information and goods both travel at the speed of caravans. A shortage in one region does not instantly raise prices in neighboring regions — it takes time for the information to propagate and for supply to respond. This lag is the window for arbitrage.

### Marginal Pricing — Per-Unit Price Updates

Prices update with each unit transacted, not once per trade session. Every unit a player sells into a market slightly increases that market's inventory, which moves the price curve slightly downward for the next unit. Every unit bought does the reverse.

This is the Mount & Blade: Bannerlord model applied to a simulated world economy. The practical effect:

- The first units sold into a shortage command the highest price
- Each subsequent unit earns slightly less as the market stabilizes
- A player who dumps large volume into a shortage will self-correct the shortage — and their own margins — before they finish selling
- Partial selling becomes a genuine strategy: sell some now at peak prices, let the market recover, return later for a second run

**Example — selling into a grain shortage:**

A town is at acute shortage (3 days of supply). Base price is 10 currency. Shortage multiplier puts current price at 32 currency per unit.

| Units sold | Inventory change | Price received per unit |
|---|---|---|
| Unit 1 | 300 → 301 | 32 |
| Unit 10 | 309 → 310 | 30 |
| Unit 30 | 329 → 330 | 26 |
| Unit 60 | 359 → 360 | 20 |
| Unit 100 | 399 → 400 | 14 |
| Unit 120 | 419 → 420 | 11 — shortage resolved |

The player with 200 units cannot sell all 200 at 32 currency. They receive the area under the curve — good money on early units, diminishing returns as the shortage stabilizes. Choosing when to stop selling is a real decision.

**The curve shape is the primary tuning lever.** A steep curve means even 20 units visibly moves the price; a shallow curve allows more volume before the premium erodes. This is adjusted per-good based on how elastic that good's demand should feel — staple foods might have a steeper curve (markets stabilize quickly) while rare goods might have a shallower one (the market absorbs more supply before price normalizes).

**Co-op implication.** In multiplayer sessions, two players selling into the same shortage compete for the premium: whoever sells first earns more. The second player arrives to a partially stabilized market. This is emergent competition without any explicit PvP mechanic.

### Supply and Demand

Each market (town or outpost with a trade feature) tracks supply and demand independently. Supply and demand are not global.

- **Supply** — local production + incoming trade shipments
- **Demand** — local population consumption + faction military and construction needs + outbound trade demand

A surplus (supply persistently exceeds demand) causes prices to fall until either production decreases or new demand emerges. A shortage (demand exceeds supply) causes prices to rise until new supply arrives or demand contracts.

The player's production at an outpost contributes directly to the supply side of nearby markets. Producing a good in high demand at a nearby market is profitable. Producing a good already in surplus is not.

### Trade Routes and Transport Cost

Goods flow between markets along trade routes, represented in the world by moving caravans and traders. Transport is not free or instant — it carries two real costs:

**Distance cost:** Time spent moving goods is time those goods are not generating income, and it is time the carrying squad members are committed to transport rather than other activities. Longer routes need proportionally larger price differentials to be worth running. A marginal profit on a short safe route may be better than a large profit on a route that takes many in-game days.

**Danger cost:** Moving goods through dangerous territory — hostile faction lands, bandit-heavy biomes, contested regions — risks the goods themselves. A caravan intercepted loses its cargo. The player must weigh the price premium available at a dangerous destination against the probability of losing the shipment entirely. Higher-value goods make this calculus more attractive; bulkier, lower-margin goods may not be worth the risk at all.

These two costs together mean that price differentials between markets must clear a real threshold before a trade run is profitable. This naturally segments the economy: safe, well-connected markets have tighter price spreads; isolated or dangerous markets have wider spreads because fewer traders are willing to service them.

Trade routes are not invisible abstractions — they are represented by caravans and traders moving in the world. A player can watch a trade route, intercept it, protect it, or establish their own.

---

## Trade Hierarchy

Trade operates on two tiers, each with different range, capacity, and relationship requirements. Both NPC factions and the player operate within this same hierarchy.

### Tier 1 — Outpost Caravans (Local Supply)

Outposts and minor settlements generate resources from their surrounding biome and production buildings. They dispatch small caravans to deliver those resources to the **nearest town**, typically one controlled by their own faction or a friendly patron.

- **Range:** Short. These caravans travel to the nearest accessible town and return. They do not attempt long cross-region journeys.
- **Risk tolerance:** Low. Small caravans avoid dangerous routes and will not cross hostile faction territory. If the route to the nearest town becomes unsafe, the caravan delays or cancels rather than attempting an alternative.
- **Capacity:** Limited. Small caravan, small load — primarily raw materials and lightly processed goods.
- **Relationship requirement:** The destination town must be held by the same faction or a friendly/neutral faction. An outpost cut off from a friendly or neutral town by hostile territory stops trading and accumulates surplus locally until a route reopens.

This tier represents the backbone of a faction's internal supply chain. Towns depend on their network of outposts for raw material input. Disrupting outpost caravans starves a town's production without directly attacking it.

### Tier 2 — Town Caravans (Inter-Regional Trade)

Towns dispatch larger, better-equipped caravans that conduct inter-regional and inter-faction trade.

- **Range:** Long. Town caravans travel between major markets across the map, including into other factions' territory.
- **Risk tolerance:** Moderate. Larger caravans can defend themselves against opportunistic raiders but will still avoid active war zones. They carry guards proportional to the cargo value.
- **Capacity:** High. Multiple goods, significant volume — processed goods, finished goods, luxury items, and bulk raw materials.
- **Relationship requirement:** **Friendly factions only.** Town caravans do not trade with Neutral factions — only Friendly or Allied. A faction relationship that slides from Friendly to Neutral causes town-level trade to cease between those factions, creating shortages on both sides and economic pressure toward diplomacy or conflict.

Town caravans are the primary mechanism for inter-faction price equalization. When they run freely, price spreads between connected markets narrow. When they are disrupted — by war, raids, or a deteriorating relationship — spreads widen and shortages emerge.

### Independent Minor Faction Outposts

Outposts held by independent minor factions that are not vassals or members of a Major Faction follow a modified rule:

- They attempt to trade with the **nearest Friendly or Neutral faction** — not just the nearest same-faction town, since they may have no same-faction town at all.
- They are pragmatic: a minor faction with no strong allegiances will send caravans to whoever will accept them.
- If their standing with the nearest market drops to Tense or Hostile, they will seek the next nearest option rather than halting trade entirely.

This gives independent minor factions economic resilience and makes them useful as trade partners for the player even before a strong relationship is established.

### The Player in the Trade Hierarchy

The player's outpost behaves identically to a faction-owned outpost at Tier 1 — it generates goods and dispatches small caravans to the nearest accessible town. These are automated: the player does not need to manually run every supply shipment.

Beyond the automated system, the player can run their own trade caravan manually:

1. **Acquire transport capacity** — pack animals, mounts, or equivalent (specific form TBD by setting) carry goods the squad cannot carry on foot alone. More transport capacity means more cargo and a larger, more visible caravan.
2. **Assemble a protection squad** — the player selects which squad members travel with the caravan. A larger, better-armed escort deters opportunistic attacks; a lean escort moves faster but is more vulnerable.
3. **Travel and trade** — the player physically moves the caravan across the map, buying goods at one market and selling at another. Route choice, timing, and knowledge of current prices are the player's edge over the automated system.

The player **cannot direct or control NPC faction caravans or patrols.** They operate independently. The player can escort a faction caravan for pay (traveling alongside it) or intercept an enemy caravan (attacking it), but they cannot issue orders to it.

Manual trade runs allow the player to exploit price spikes the automated system is too slow to capture, move illicit goods an automated caravan would not carry, and reach markets in dangerous territory that faction caravans avoid.

---

## Trade as World Activity

Trade is not a background abstraction — it is a source of ongoing world activity that the player observes, interacts with, and is affected by. Caravans moving across the map, faction patrols moving to protect or threaten them, and wildlife reacting to traffic through their territory all create spontaneous encounters that make the world feel inhabited and in motion.

### Caravans in the World
NPC caravans — both Tier 1 outpost caravans and Tier 2 town caravans — are physically present on the map as moving groups. They follow routes, travel at caravan speed, and are vulnerable in transit. A player moving through the world will regularly encounter caravans belonging to various factions, and their presence communicates information: which routes are active, which factions are trading, and how much traffic a region supports.

### Patrol and Caravan Interaction
Faction patrols and caravans interact organically:
- A patrol encountering a friendly faction's caravan may escort it through a dangerous stretch
- A patrol encountering a hostile faction's caravan will attack it
- A patrol encountering a neutral faction's caravan will typically ignore it unless standing has deteriorated
- Bandits and hostile minor factions will raid any caravan they can overpower

These interactions happen without player involvement. The player may stumble into an ongoing raid, find a caravan that has already been looted, or witness a faction patrol driving off an attacker. Each is an emergent event, not a scripted encounter.

### Wildlife
Wildlife in appropriate biomes poses a threat to both caravans and patrols. A caravan moving through a dense forest biome may be attacked by predators. A small patrol in a dangerous region may be overwhelmed by local fauna. Wildlife does not distinguish between player and NPC — it responds to proximity and perceived threat.

This creates a layered threat environment on active trade routes: the route may be safe from hostile factions but dangerous due to wildlife, or vice versa. A route that is simultaneously in contested faction territory and passing through a dangerous biome is genuinely hazardous and will command a significant price premium at its destination — if anyone is willing to run it at all.

### Opportunity for the Player
The activity generated by trade creates ongoing opportunities:
- A faction caravan under attack is a chance to assist (earning standing and possible payment) or loot the aftermath
- A patrol weakened by a wildlife encounter is a softer target
- A trade route that has gone quiet — no caravans moving — signals a disruption worth investigating or exploiting
- Following an active caravan can guide the player toward markets and settlements they have not yet discovered

---

## Resource Consumption and World Events

The economy needs enough consumption simulation to produce believable price signals — not enough to model every transaction. The design target is a world that *feels* economically alive, not one that simulates a real economy. When in doubt, prefer a simpler model that generates visible player opportunities over a precise model that runs invisibly.

### Consumption Categories

Rather than tracking individual goods exhaustively, consumption is modeled through three broad categories that respond to world state:

| Category | What drives it | Goods affected |
|---|---|---|
| **Military** | Garrison size, active conflicts, faction troop recovery | Weapons, armor, medicine, food for soldiers |
| **Population** | Settlement size, population growth or loss | Food, basic goods, fuel/heating |
| **Construction** | New buildings, town rebuilding after ruin, outpost expansion | Raw materials, processed goods, tools |

When world-state changes affect one of these categories — a new faction moves in, a town is destroyed, a war breaks out — the affected category's demand adjusts. The price signal follows naturally from the supply/demand model already in place.

### World Events as Demand Shocks

Specific world events create demand spikes or supply collapses that the player can observe and act on:

**Faction moves into a captured town**
The incoming faction needs to garrison troops, equip soldiers, and stock the town. Military goods demand spikes for a period while they establish themselves. A player with weapons or armor to sell, or one who can run a supply caravan to the town quickly, can profit from the transition window before the town's own supply chain catches up.

**Town in ruin / early rebirth**
A recently devastated town starts with minimal stock — shelves empty, caravans not yet restored, production not yet running. Construction goods are in high demand as rebuilding begins. Basic goods command high prices from the remaining population. This is a high-margin but high-risk trading environment: the town may still be contested, law enforcement is absent, and the route to it may still be dangerous.

**Famine**
An agricultural shortfall — caused by drought in an arid biome, a blight, or deliberate disruption of food supply routes — creates a regional food shortage. Prices spike across the affected region. Surrounding food-producing regions see their export value surge. Famine can weaken a faction militarily (hungry soldiers are less effective) and politically (starving populations are unstable). A player who can import food into a famine region profits significantly; a player who engineers a famine through supply disruption can use it as a weapon.

**Bountiful harvest**
An unusually productive agricultural cycle floods the local market with food. Prices crash in the producing region but remain normal or high elsewhere. The opportunity is to buy surplus food cheaply and transport it to regions still paying normal prices — classic arbitrage with a time window before the surplus equalizes across trade routes.

**Prolonged war**
A faction sustaining military operations over time burns through military goods continuously. Medical supplies, weapons, and food for troops are in persistent elevated demand. Supplying a warring faction is lucrative but politically entangling — the other side may view the player as a hostile supplier.

### Simulation Philosophy

The simulation does not need to track every loaf of bread or every sword. It needs to:
1. Adjust demand category levels when world events occur
2. Let those demand changes flow through the existing supply/demand pricing model
3. Produce price signals the player can observe at markets they visit

Events should be **legible** — when prices spike on military goods in a town, the player should be able to reason about why (a new garrison moved in, there's a war nearby) rather than experiencing it as random noise. The economy rewards players who pay attention to the world, not players who run optimization spreadsheets.

---

## Price Signals and Player Exploitation

The most powerful economic tool the player has is information. A player who knows that a region is running short on medicine before prices peak can:
- Transport goods from a surplus region and sell at the shortage price
- Corner supply in a surplus region before the shortage is widely known
- Engineer a shortage deliberately (by disrupting the supply route) and then profit from it

**Deliberate shortage engineering** — raiding or blocking a supply route to create artificial scarcity — is a fully supported strategy. It carries risks: the targeted faction loses economic health and may respond militarily; other traders may fill the gap before the player can exploit it; and the player's role may become known to the faction.

Price information should be partially observable. The player sees prices in locations they visit or have agents in. Prices in unvisited regions are unknown or estimated from last visit. This creates value in scouting and maintaining trade contacts.

---

## Player Economic Roles

The player can participate in the economy through multiple roles simultaneously:

### Trader
Buy low, sell high across regions. Requires transport capacity (squad members carrying goods or controlled vehicles/pack animals — TBD by setting), knowledge of regional prices, and safe routes. Low risk, moderate returns.

### Producer
Build production infrastructure at an outpost (see `base-building.md`). Extract raw materials from the surrounding biome, convert them into processed or finished goods, and sell surplus into regional markets. The outpost's output flows directly into the local supply — which means production decisions have real market consequences.

Producing a good that is in structural shortage in the target market is highly profitable. Producing the same good the dominant local faction already exports into its own market risks a price crash. The smart producer reads the regional economy before choosing what to build. Outpost production is the most capital-intensive economic role but scales well — a well-placed, well-supplied outpost generating goods for a hungry market can become the player's primary income source.

### Raider / Disruption Agent
Attack trade caravans, destroy production facilities, sabotage supply routes. Generates short-term loot and creates economic conditions the player or allied factions can exploit. High risk, high reward, significant faction standing consequences.

### Contractor
Accept economic contracts from factions — supply delivery, caravan escort, resource recovery. Reliable income with lower risk than raiding; standing with the contracting faction improves. Limited by what factions are willing to offer.

### Criminal Operator
Fence stolen goods, run contraband, operate in the slave trade. Often higher margin than legitimate trade but requires managing faction exposure, bounties, and specialist contacts. See illicit systems below.

---

## Illicit Economy

### Fencing
Stolen goods carry reduced value through legitimate channels and may be identified and seized if the player trades in a territory controlled by the victim faction. Fencing contacts — minor faction traders or criminal intermediaries — accept stolen goods without questions at a further discount. The discount reflects their risk. Better fencing contacts (higher standing with criminal networks) yield better rates.

### Contraband
Certain goods are legal in some regions and illegal in others. Transporting them across that boundary creates profit but attracts inspection. Faction patrols in sensitive regions may search the player's squad. High stealth or low-suspicion routes reduce interception risk.

### Slave Trade
Slaves are a tradeable good in regions where the practice is accepted or tolerated by the dominant faction. The player can:
- **Sell** captured or kidnapped NPCs to slave markets
- **Purchase** enslaved NPCs — including for recruitment, if a purchased slave is willing to join the squad; no hire fee applies (the purchase price already paid covers this), though ongoing wages apply once they join
- **Liberate** enslaved NPCs, which may improve standing with anti-slavery factions and provides recruitable candidates

Slave markets exist only in regions where the controlling faction permits them. Operating the slave trade in a faction that prohibits it is treated as contraband — and draws a stronger response than ordinary smuggling.

The slave trade is a meaningful currency source but not a trivially safe one. Kidnapping generates detection risk. Transport of living cargo is slower and higher-profile than goods. Markets may be disrupted by faction events.

---

## Currency and Currency Sinks

Currency is earned through trade, contracts, looting, and production sales. It is consumed by:

| Sink | Notes |
|---|---|
| Squad wages | Ongoing; scales with squad size and individual skill levels — see `characters-and-squads.md` |
| Recruitment fees | One-time cost per new squad member |
| Base construction and upkeep | Building costs plus ongoing resource consumption |
| Research and tech advancement | See `tech-and-crafting.md` |
| Equipment purchase and repair | Arms, armor, tools |
| Bribes and standing purchases | Faction relationship repair |
| Bounty payments | Criminal activity cleanup |

The wage sink is designed to be the primary ongoing pressure. A large, skilled squad is expensive. A player who recruits aggressively without expanding their income is structurally unsustainable — squad members who go unpaid will leave, and their departure may cascade if morale collapses (see `characters-and-squads.md`).

---

## Biome-Sourced Resources and Regional Variation

Resources are distributed across the world in a common-sense manner that reflects the underlying biome. No single region produces everything. This creates permanent trade gradients between regions — the economic geography of the world is as fixed and learnable as its physical geography.

**Biome resource principles** (specific goods TBD when setting is chosen):
- **Dry / arid biomes** — scarce food and water; higher prices for agricultural goods; may have unique mineral or extractable resources that compensate
- **Forest / jungle biomes** — abundant animal-sourced materials, timber, and foraging goods; hostile wildlife raises extraction and transport danger
- **Plains / grasslands** — highest farming potential; food surplus drives prices down locally but creates export opportunity to food-scarce regions
- **Mountain / highland biomes** — rich in ore and stone; poor in food; isolated terrain raises transport cost for everything that moves in or out
- **Wetland / coastal biomes** — fishing and water-adjacent resources; may support goods unavailable inland; often lower danger but limited agricultural output

These principles mean that certain goods are structurally cheap in their native biome and structurally expensive everywhere else. The price gradient between a food-surplus plains region and a food-scarce arid region is baked into the world design — the player can rely on it being there and plan accordingly.

**Additional regional variation** beyond biome type:
- Population size drives demand — large towns consume more of everything
- Faction production focus — a militarist faction's region produces more weapons and armor; a mercantile faction's region may have better trade infrastructure
- Geographic isolation — regions with few connecting routes have higher prices on all imports due to limited supply flow and higher transport risk
- Danger level — high-danger regions are underserviced by NPC traders, creating persistent price premiums for goods delivered there

Regional variation creates a map of trade gradients — the most profitable routes follow the steepest differentials. As the player acts (producing goods, disrupting routes, establishing outposts), these gradients shift locally. A player producing food in a plains outpost and selling into a nearby arid market is exploiting a structural gradient. A player who then attracts three other producers to do the same thing will watch that margin compress as the arid market's shortage is resolved.

---

## Advancement Resources

Advancement resources are a special goods category — rare materials whose primary purpose is fueling technological research and unlocking higher tiers of crafted goods (see `tech-and-crafting.md`). They participate in the trade economy like any other good but are structurally scarcer and more valuable.

### What They Are

The specific form of these resources is determined by the game's setting, not by this document. They are a class with consistent economic and gameplay properties regardless of theme:

| Setting type | Possible form |
|---|---|
| Fantasy / magic | Rare alchemical ingredients, concentrated magical essences, creature-sourced components |
| Science fiction | Salvaged components from a lost civilization, exotic materials, engineered compounds |
| Historical / alt-history | Lost-knowledge artifacts, rare earth materials, recovered manuscripts |

For design and implementation purposes, advancement resources are treated as a unified category. Their in-world appearance is a skin over the same underlying economic and crafting mechanics.

### Economic Properties

**Rare sources.** Advancement resources do not come from common biome production. They are found at:
- Specific high-danger locations — ruins, deep wilderness, contested sites
- Guarded deposits that factions actively contest for control of
- Occasional appearance in the trade inventories of specialized Nomadic faction traders

Their scarcity is structural, not simulated. There are only so many sources on the static map, and those sources produce limited quantities. No amount of economic pressure conjures more of them.

**High value, high risk.** Because advancement resources are rare and in sustained demand from any tech-progressing faction or player, their prices are permanently elevated. The danger of the locations that produce them and the value of the cargo make their transport one of the highest-reward trade runs available — and one of the most attractive targets for interception.

**Faction competition.** Factions that control advancement resource sources gain a long-term economic and military advantage through faster tech progression. Factions may raid or assault locations specifically to gain or deny access to these sources. The player can exploit this: controlling or disrupting an advancement resource source has outsized strategic impact relative to its physical size.

**Demand from factions.** AI factions consume advancement resources to progress their own tech and equipment tiers. A faction that is cut off from these resources will stagnate technologically over a long campaign — their equipment stays at lower tiers while better-supplied factions improve. This creates an observable, exploitable dynamic: a resource-starved faction is a weakening one.

### Player Interaction

The player needs advancement resources for tech tree progression. Their options for acquiring them:

- **Direct extraction** — find and exploit a source location; high danger, no middleman cost
- **Trade purchase** — buy from markets or Nomadic traders where they appear; convenient but expensive and supply-limited
- **Raid or theft** — take them from a faction that holds a source or from a caravan transporting them
- **Contract reward** — some faction contracts may pay in advancement resources rather than currency

Because these resources are a bottleneck for tech progression, the player who secures a reliable supply source — through ownership, trade relationship, or repeated raiding — gains a meaningful and sustained advantage.

---

## Known Gaps / Future Notes

- **Economy simulation granularity**: How frequently supply/demand is recalculated, and at what geographic resolution (per settlement vs. per region), is an open technical question. Per-settlement granularity is more realistic and creates tighter price signals; per-region is simpler to simulate and communicate. Finer granularity rewards players who move goods between nearby settlements; coarser granularity simplifies the player's read of the market.
- **Price display and market UI**: The player needs enough price information to make informed decisions without it becoming a spreadsheet game. Showing current price, recent trend direction, and stock level at visited markets is likely sufficient. Design of the interface is deferred.
- **Faction economic contracts**: The mechanism for factions posting supply requests, escort jobs, or bounties is not yet fully designed.
- **Transport logistics**: Whether goods transport is abstracted (squad inventory) or requires dedicated logistics units (pack animals, wagons) depends on setting. The danger-cost model requires that losing a transport to raiders means losing the goods — that must hold regardless of implementation form. Deferred until setting is chosen.
- **Price floor**: Whether there is a minimum price below which goods cannot fall (preventing complete market collapse) is an open balance question. A hard floor risks undermining the crash mechanic; no floor risks making some goods economically worthless.
- **Specific biome resource lists**: Exact goods per biome are deferred until setting is chosen. The biome principles above are the design constraint; specific resources must conform to them.
