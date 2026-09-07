# Economy

## Purpose

The economy is a living simulation of supply, demand, production, and trade across the
world — not a shop menu. Prices reflect real scarcity, shortages create opportunities, and
trade routes can be disrupted or protected. The player participates as a trader, producer,
raider, or some combination, and their actions measurably move regional prices and
availability. The economy is also the primary pressure system keeping the player engaged
at every stage: wages, base upkeep, equipment, and research are all continuous drains, and
generating income is never optional (see `vision-and-pillars.md`).

## Goods and the Player Utility Requirement

Every good has a base value (a reference floor, not the actual market price), a
production source, a consumption source, and a weight/volume that governs transport risk.
Categories: raw materials, processed goods, finished goods, illicit goods, and
**advancement resources** (rare materials that gate tech progression, below).

**Every good must have at least one condition under which a player would rationally craft,
trade, or buy it** — a good with no such condition is wasted content budget. Utility can
come from direct consumption, equipment value, feeding a crafting recipe, feeding
research, a trade margin worth the effort even if the player never uses the good
personally (the luxury-good case — margin alone is a valid reason to exist), or
independence value (cheap normally, but self-producing it insures against supply
disruption). A good that is always cheap everywhere needs zone variation, disruption
scarcity, or a quality differential to stay relevant, or it becomes a dead good — no
faction wants it, the player has no use for it, it feeds no recipe, it has no margin
anywhere, and its presence or absence changes nothing. Any good meeting all of those
conditions should be cut or redesigned. Zone-appropriate crafting (don't build cloth
production in a mountain region with no fiber) is the resource geography telling the
player what a zone is good for.

## Emergent, Marginal Pricing

There are no fixed NPC prices. Every market's price is derived from its own local supply
and demand and moves as production, consumption, and trade flow shift:

- **Price crash** — oversupply (NPC production, player output, or a flood of imports)
  drives price down; flooding a market hurts everyone selling into it, including the
  player.
- **Shortage spike** — undersupply drives price up, proportional to severity; a severe
  shortage can disable faction functions that depend on the good.
- **Price lag** — information and goods both travel at caravan speed, so a shortage
  doesn't instantly propagate to neighboring regions — that lag *is* the arbitrage window.

Prices also update **per unit transacted**, not once per session: the first units sold
into a shortage command the highest price, and each subsequent unit earns less as the
market stabilizes — a player dumping a large stockpile self-corrects the shortage (and
their own margin) before they finish selling. Partial selling — take the early premium,
let the market recover, come back later — is a genuine strategy. Curve steepness is a
per-good tuning lever: staple goods stabilize fast, rare goods absorb more volume before
normalizing. In co-op, two players selling into the same shortage compete for the
premium purely through timing, with no explicit PvP mechanic needed.

Supply and demand are tracked **per market**, never globally: supply is local production
plus incoming shipments; demand is local consumption plus military/construction need plus
outbound trade demand. A trade route only clears a profit once its price differential
covers both **distance cost** (time not spent earning elsewhere) and **danger cost** (risk
of losing the cargo outright) — which naturally splits the world into tight-spread safe
markets and wide-spread dangerous ones that few traders will service.

## Trade Hierarchy

Two tiers, used identically by NPC factions and the player:

- **Tier 1 — outpost caravans.** Short range (nearest town only), low risk tolerance,
  small capacity, same-faction or friendly/neutral destinations only. The backbone of a
  faction's internal supply chain — disrupting these starves a town without attacking it
  directly.
- **Tier 2 — town caravans.** Long range, moderate risk tolerance, high capacity,
  **friendly-or-better relationships only** — the mechanism that equalizes prices between
  factions. A relationship sliding from Friendly to Neutral shuts this off and produces
  shortages on both sides.

Independent minor factions trade with whoever will accept them, giving them economic
resilience as early trade partners. The player's own outpost behaves as an automated
Tier-1 caravan; beyond that, the player can run manual trade runs (transport capacity +
escort squad + travel) to catch spikes the automated system is too slow for, move illicit
goods no automated caravan would carry, or reach markets faction caravans avoid — but can
never command an NPC caravan directly, only escort or intercept one.

## Trade as World Activity

Caravans are physically present, vulnerable, and interceptable. Patrols escort friendly
caravans, attack hostile ones, and ignore neutral ones; bandits raid anyone they can
overpower; wildlife doesn't distinguish friend from foe. None of this requires the player
— it's ambient, emergent activity that produces readable signals (a route gone quiet
means a disruption worth investigating) and opportunities (assist a caravan under attack,
loot the aftermath, follow an active caravan toward an undiscovered market).

## World Events as Demand Shocks

Three broad categories — military, population, construction — absorb world-state changes
without needing to track every individual good. Specific events create legible,
exploitable price shocks: a faction moving into a captured town spikes military-goods
demand during the transition window; a town in ruin has empty shelves and desperate
construction demand; famine spikes food prices regionally and can weaken a faction
militarily and politically; a bountiful harvest crashes local food prices and opens an
arbitrage window before the surplus spreads; a prolonged war keeps a faction's medical and
military-goods demand elevated (and supplying them is lucrative but politically
entangling). **The simulation only needs to feel economically alive, not model a real
economy** — when in doubt, favor a simpler model that produces visible opportunities over
a precise one that runs invisibly.

## Price Signals, Roles, and the Illicit Economy

Information is the player's most powerful economic tool — moving goods ahead of a known
shortage, cornering a surplus before it's widely known, or **deliberately engineering a
shortage** by raiding a supply route are all supported strategies (with real standing and
competitive risk attached). Price visibility is partial: current in places visited or with
active contacts, stale or unknown elsewhere.

Players combine economic roles freely: **trader** (buy low, sell high — low risk,
moderate return), **producer** (build production at an outpost, see `base-building.md` —
reads the regional economy before choosing what to build, since flooding a market the
dominant local faction already exports into crashes the player's own margin),
**raider/disruption agent** (high risk/reward, real standing cost), **contractor**
(reliable, faction-standing-building work), and **criminal operator** (fencing,
contraband, slave trade — higher margin, real exposure management required). Fencing
value scales with criminal-network standing; contraband legality is region-dependent;
slave markets exist only where the controlling faction tolerates the practice, and
operating one where it's prohibited is treated as a harsher form of contraband.

## Currency Sinks

Wages are the primary ongoing pressure (see `characters-and-squads.md`) — a squad that
grows faster than its income is structurally unsustainable. Other sinks: recruitment fees,
base construction/upkeep, research and tech advancement, equipment purchase/repair,
bribes/standing purchases, and bounty payments.

## Biome Resources and Advancement Resources

No region produces everything — biome logic (arid: scarce food/unique minerals; forest:
materials, dangerous wildlife; plains: farming surplus; mountain: ore, food-poor, isolated;
wetland: fishing, moderate danger) creates permanent, learnable trade gradients (see
`open-world.md`). Danger scales with resource value by design — the richest deposits are
the most dangerous to reach, which is what earns them their price premium elsewhere.

**Advancement resources** are a special, structurally scarce category that gates tech
progression (see `tech-and-crafting.md`): found only at specific dangerous or
faction-contested locations, never from ordinary biome production. Both the player and
every faction draw from the same finite, static-map pool, making control or denial of a
deposit a strategic act with outsized impact — a faction cut off from these resources
visibly and observably stagnates technologically over a long campaign.

## Open Design Questions Worth Tracking

- Simulation granularity (per-settlement vs. per-region) and how much price/trend/stock
  information the market UI should surface without turning trading into a spreadsheet
  exercise.
- How factions post economic contracts (supply requests, escort jobs, bounties) isn't
  designed yet.
- Whether transport logistics are abstracted (squad inventory) or require dedicated
  logistics units is deferred to setting — but losing a transport to raiders must always
  mean losing the goods, regardless of the answer.
- Whether a hard price floor exists is an open balance question — too hard a floor
  undermines the crash mechanic; no floor risks some goods going worthless.
