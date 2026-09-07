# Tech and Crafting

## Purpose

The tech tree defines what the player can build, craft, and field over a campaign.
Crafting converts researched knowledge into physical goods — weapons, armor, tools,
prosthetics, base infrastructure components. Together they're the late-game progression
backbone: the player starts using whatever they can afford or find, and gradually gains
the ability to out-produce the open market.

## Tech Tree Structure

A directed graph of nodes with prerequisites, and occasional branches that force a choice
between paths that are difficult — though not necessarily mutually exclusive — to pursue
together. Conceptual branches (exact content is setting-dependent): materials/processing,
weapons/armor, base infrastructure, medicine/prosthetics, stealth/illicit, trade/logistics.
Branches aren't siloed — a player chasing weapon crafting still needs infrastructure nodes
to build the facilities that require. The design should not assume a single tree; a
setting might warrant more than one (e.g. a conventional crafting tree alongside a
separate salvage/adaptation path, or faction-specific trees with different philosophies).

## Research

Research requires a dedicated facility and an assigned character — there's no passive
research, and a node in progress pauses if no one is assigned. It costs character-hours
plus materials: ordinary goods at low tiers, but mid- and high-tier nodes consume
**advancement resources** (see `economy.md`), making resource access — not time invested —
the real bottleneck for reaching high tech. Speed scales with the assigned character's
relevant skill, and some nodes require a skill floor just to attempt.

Self-research isn't the only path: purchased schematics, reverse-engineering a captured or
purchased item, faction contract rewards, and raiding a rival's own research facility can
all unlock the same node — no player is locked into one slow grind.

## Crafting

Each item category needs a matching facility, whose level caps the achievable quality.
Output quality is jointly a function of crafter skill, input material quality, and
facility level — a master crafter at a top-tier facility using premium (often
advancement-resource) inputs can produce goods that meaningfully outperform anything on
the open market, a genuine late-game achievement requiring sustained investment on both
the skill axis (characters, through use) and the tech axis (facilities, through research)
at once. Surplus crafted goods sell into the regional economy, with high-quality output
commanding a real premium where that item type is scarce — enough volume, though, and a
player can corner and depress a market for competitors.

## Faction Military Tech Progression

The same tree gates the equipment and stat/skill envelope factions field at each military
tier (see **Faction Military Progression** in `factions-and-world-state.md`). This is a
direct design constraint on the tree itself: **each tier needs to produce a soldier
profile the player will actually notice** — meaningfully tougher, not just a bigger
number — because falling behind the world's military tier has to feel like a real
strategic problem. Advancement resources are consequently a shared bottleneck: the player
and every tech-progressing faction draw from the same finite, static-map pool, so
controlling or denying a deposit carries outsized strategic weight relative to its
physical footprint, and a faction cut off from that supply visibly stalls.

## Modding

Tech nodes, recipe definitions, and building requirements must be data-driven — addable
without touching core code — an explicit modding target carried over from
`vision-and-pillars.md`.

## Open Design Questions Worth Tracking

- Exact tree content and branch structure are deferred until setting is chosen; the
  categories above are placeholders, not a final list.
- Whether the game ships more than one tech tree is undecided.
- How the player navigates/visualizes the tree in the UI isn't designed.
- Whether progression needs hard sequential bottlenecks (must clear Tier 2 before any
  Tier 3 node) or can stay a more open graph is an open balance question.
- The detailed mechanics of non-research tech acquisition (raiding, purchase, faction
  reward) need more design during systems work.
