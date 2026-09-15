# Base Building

## Purpose

A base is the player's footprint in the world — infrastructure that serves the squad, not
a city to manage for its own sake (see **Not a city builder** in `vision-and-pillars.md`).
To keep scope appropriate, the player has exactly two deliberately constrained forms of
physical presence: **one active outpost** (freeform, one at a time) and **town buildings**
(purchased space inside faction towns, available earlier and in multiple locations at
once, but capability-limited). Together they cover the arc from early-game foothold to
late-game self-sufficiency without drifting into city-builder territory.

## Town Buildings

A purchased occupancy agreement inside a faction-controlled town — never outright
ownership. It provides limited crafting, limited (low-tier only) research, modest storage,
and a legitimate presence with free market and recruitment access — but never full
production chains, high-tier research, fortifications, or squad housing at scale; a player
relying solely on town buildings eventually hits a ceiling only a full outpost resolves.
Purchasable building count scales with faction standing (Neutral: 1, Friendly/Allied: 2) —
a deliberate progression gate, since a second building meaningfully expands what's
possible (one for crafting, one for research, say).

**The building is only as secure as the faction that holds the town.** If the faction
loses the town, the building and everything stored in it are gone immediately, with no
refund — this is intentional: it gives the player a *material* stake in that town's
survival, not just a political one, and creates a natural on-ramp into assault
participation (see `factions-and-world-state.md`). The player can't fortify or defend a
town building directly; their only influence is indirect, through their own squad's
participation in the town's defense.

Buying in costs a one-time, faction-set acquisition fee plus an **ongoing lease that is
deliberately priced higher than outpost rent** — because the town is absorbing costs the
player would otherwise carry themselves (defense, market access, trade routes,
recruitment pools). The two paths are a genuine cash-vs-labor tradeoff, not one being
strictly better: town buildings drain currency steadily, outposts demand time, materials,
and squad commitment instead. Non-payment follows the same pattern as outpost rent —
relationship decay, then repossession without refund if left unresolved.

## Outpost Construction

Exactly **one active outpost at a time**, by permanent design choice, not a slot that
unlocks more later — relocating means abandoning or losing the current one first.
Construction is freeform, constrained by terrain, resource proximity, and defensibility
rather than a grid. Site selection matters: resource proximity controls transport cost,
terrain defensibility reduces the force needed to hold against a raid, and outposts must
sit a **minimum distance from every authored point of interest** (see `open-world.md`) so
the player is never building directly on top of a world feature and the authored world
stays legible.

Faction land breaks into two bands, and they behave differently:

- **Zone of control** — the tightly patrolled/administered ground immediately around a
  faction's towns and holdings. Hard-blocked for outpost placement, no exceptions and no
  rent option; a **Town Buildings** plot is the only way to have a presence this close in.
- **Territory** — everything beyond that, loosely claimed rather than administered. Here
  placement is allowed, and it creates an ongoing obligation: pay rent to a Friendly
  faction (cheaper, with some patrol deterrence) or a Neutral one (pricier, no real
  deterrence), or build in Hostile/unclaimed territory and take raids and eventual full
  assaults instead of paying anything. Rent is a real strategic tradeoff — the alternative
  to paying it is not "free," it's exposure.

Even where rent is being paid, the outpost's mere presence costs something: building inside
a faction's territory at all — not just failing to pay — nudges local reputation with
nearby factions downward, on top of whatever raid/rent dynamics already apply (see local
reputation in **Raids and Attacks** below). A faction doesn't have to be hostile to resent
being built next to.

## Building Mode

Construction, teardown, and decoration are all done through one dedicated interaction mode
the player enters to work on their outpost — distinct from ordinary squad-command play, but
not a break from it. Building Mode has no time behavior of its own: it works identically at any pause/speed
state the player already has set, including fully paused — the same universal
pause-anywhere and adjustable-speed controls available everywhere else (see
`player-experience.md`), never restricted or overridden for this mode specifically.
Pausing before doing careful placement work is the normal, expected way to use the mode,
not a loophole — once pause-anywhere exists at all, "a raid can interrupt construction"
is only ever true for a player who chose to leave time running while building.

- **Construct** — place a new building; see **Placement and Building Interiors** below for
  the freeform-with-guardrails rules.
- **Teardown** — remove an existing building. Whether it recovers some fraction of the
  original material cost or is a pure loss isn't decided yet — partial recovery fits the
  game's general aversion to free undos (see **Supply Chains and Upkeep**), but a pure loss
  makes every construction decision higher-stakes, which also fits the tone.
- **Decorate** — place small items on/in a building's surfaces; this is the deferred
  small-item placement system noted below, exposed through the same mode rather than a
  separate one.

## Placement and Building Interiors

Buildings are not decoration — a player pawn can walk inside one, so the exterior and
interior are the same authored piece of space, not a facade backed by a disconnected
interior cell. Entering a building is seamless: no loading transition, and what the
exterior silhouette promises from outside is what's actually inside. This is a deliberate
fit for a single-outpost, small-squad game where the base should read as one comprehensible
physical place, not a city of stage-set fronts.

The direct consequence: a building's exterior scale **is** its interior capacity, authored
together as one piece. Freeform placement therefore only ever offers **position and
rotation**, never non-uniform scaling — scaling a building would desync its interior from
its exterior shell.

Placement is freeform (see **Outpost Construction** above) but guarded against absurd
outcomes — a building sunk low enough that only the roof shows, or a wall buried in a rock
— by a two-tier clipping check the game runs against the world at placement time:

- **Terrain** gets graduated tolerance keyed to each building's authored **foundation
  line**. Anything below that line (the foundation/footing) may embed into a slope freely —
  that's just grading. Anything at or above it — walls, doors, windows, roofline — gets
  zero tolerance; placement is blocked the moment any of that would dip below grade.
- **Static world objects** (POI props, rocks, other placed buildings) get no graduated
  tolerance at all — binary blocked/clear. There's no terrain-like excuse for a wall to
  overlap a rock, so any overlap here simply blocks placement outright.

This means every building prefab needs an authored foundation line, and the world needs to
distinguish "terrain" from "solid obstacle" for the placement check to reference — both are
content/authoring requirements on top of the placement tool itself, not just tooling. Other
buildings — the player's own or a town's — fall under the same "solid obstacle" rule: no
stacking or overlapping, whether that's another outpost building or a plot inside a town the
player doesn't own.

Once placed, a building is a real obstacle to the world, not just to the player: creatures
and NPCs path around it, not through it, the same way the **defensive** building types
below are already meant to channel attackers rather than let them walk through walls. This
is a general property of any solid player structure, not something limited to purpose-built
defenses.

Separately, and later: placing small items in or on a building (furniture, decor, stored
goods visibly sitting on a shelf) is a different, smaller-scale system from building
placement itself — a building is placed *in the world*, an item is placed *relative to a
building's surfaces*. Worth planning for, not yet designed (see Open Design Questions).

## Building Types

Categories (specific roster TBD by setting): **extraction** (raw materials from the
surrounding biome — the foundation of self-sufficiency); **processing/production**
(chainable — extraction → processing → finishing is more efficient than buying inputs, but
costs space, workers, and upfront investment); **storage** (positioning within the base
layout affects worker efficiency, not just capacity); **residential/support** (housing,
medical care, morale — under-investing here degrades the squad over time); **defensive**
(walls, gates, towers — these channel attackers and buy time, they don't replace the need
for actual defenders); **research** (typically a later-priority build, see
`tech-and-crafting.md`).

## Raids and Attacks

No base is safe. Attack likelihood and severity are driven by faction relationship (a
hostile neighbor raids regularly regardless of anything else; a friendly one's patrols
deter raiders until the relationship sours), **local reputation independent of formal
standing** (a player who's been aggressive in the region draws more determined raids), and
**base value/visibility** (a visibly wealthy, sprawling base in contested territory draws
harder, more frequent attacks than a small inconspicuous one). Raid severity ranges from
small opportunistic probes up to a rare, potentially base-ending siege, usually preceded
by escalating smaller raids. An entirely ungarrisoned base falls to even a small raid.

## Supply Chains and Upkeep

Buildings don't run themselves — every production, hauling, construction, and repair
task here is work a named squad member has to be assigned to, through the job queue in
`orders-and-jobs.md`. A building with nobody on it produces nothing.

Production only matters once it connects to consumption — internal squad needs, export to
market, or transfer to another base — and disruption anywhere in that chain (an
intercepted caravan, a destroyed production building) cascades into morale problems and
potential departures (see `characters-and-squads.md`). Ongoing upkeep — repair, fuel/power,
wages for stationed characters, food and medical supply — means a base consuming more than
it produces is a straightforward drain on the player's economy; expansion should always
follow income, never precede it.

## Open Design Questions Worth Tracking

- Exact building roster, input/output ratios, and construction costs are deferred until
  setting is chosen.
- Exact tolerance values for terrain embedding (how much foundation sinking reads as
  natural grading vs. obviously wrong) aren't tuned yet, and need playtesting once
  building meshes with authored foundation lines exist.
- Whether every building type gets the same foundation-line/tolerance profile, or whether
  some (e.g. a dock built to sit partly in water) need their own tolerance rules, is
  undecided.
- Lease and rent payment mechanics (automatic, periodic in-person, goods-in-kind) and the
  grace period before non-payment triggers hostility aren't designed yet.
- Whether the lease-to-rent price premium actually feels fair once playtested against the
  full economy — too high and town buildings feel like a trap, too low and no one bothers
  with an outpost.
- Whether a defeated outpost garrison means the enemy faction can capture and hold the
  outpost (more interesting, more complex) or simply destroys it is undecided.
- Deliberate-abandonment mechanics (what happens to stored goods, buildings, stationed
  characters) aren't designed.
- The point-of-interest exclusion radius is as much a feel question as a technical one —
  too small and the world feels cluttered with bases, too large and viable sites become
  rare — and needs tuning once the actual map scale is set.
- Zone-of-control radius per faction (and whether it scales with faction size/tier) isn't
  defined yet, nor is exactly how much reputation building in someone's territory costs.
- Small-item placement on/in buildings (furniture, decor, visibly stored goods) is
  intentionally deferred — it's a distinct, smaller-scale placement system from building
  placement and hasn't been designed.
- Teardown material recovery (partial refund vs. pure loss) isn't decided — a balance
  question best settled once the full resource economy exists to playtest it against.
