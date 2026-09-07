# Factions and World State

## Purpose

Factions are the political and social fabric of the world: they hold territory, control
trade, project military power, and pursue their own agendas independent of the player.
World state is the aggregate condition of those factions — who is strong, who is
weakening, who is at war, who controls what. The player is one input into a simulation
that would keep producing outcomes even if they did nothing. This is the concrete
expression of the **Living World** pillar (see `vision-and-pillars.md`).

## Three Kinds of Territorial Standing

- **Major Factions** hold towns — the highest-value settlements — and are the dominant
  political/military powers. They have named leaders, run their own internal economy, and
  can war, ally, raid, and assault.
- **Minor Factions** hold an outpost or minor settlement but no town — bandit clans,
  small religious communities, local guilds. Can be promoted to Major by capturing a town,
  or reduced to Nomadic by losing their holding.
- **Nomadic Factions** hold no fixed territory at all — trader guilds, criminal
  enterprises, displaced remnants of fallen factions, independent wanderers. They can't be
  dislodged by taking territory; eliminating one means hunting down its members or cutting
  off its economic lifelines.

Faction type is a *state*, not an identity — types transition in both directions as the
world simulation runs (a Minor Faction that seizes a town becomes Major; a Major Faction
that loses every town becomes Minor or Nomadic). **Promotion is meant to be visible and
consequential**: a newly promoted faction should visibly begin projecting patrols,
striking new trade relationships, and drawing the attention of established powers. This is
one of the primary mechanisms by which a long campaign should feel like it has moved —
"a player returning to a region after many in-game weeks finds the balance of power
meaningfully different" is the target experience, not an edge case.

## The Player's Fixed Role

**The player never becomes a faction.** No town ownership, no taxation, no declared wars,
no trade route control — this is a firm scope boundary tied directly to the **Not a 4X**
line in `vision-and-pillars.md`, not an incidental limitation. All player influence over
the world is expressed through squad-level action and its reputation consequences: who
will trade with them, who will hire them, who will let them pass. Starting outside every
faction hierarchy is meant to read as an asset, not a handicap — an unaffiliated squad can
work for mutually hostile parties, at least until they find out.

## Why the World Shouldn't "Paint the Map"

A deliberate design goal: no faction should plausibly conquer the whole map in a typical
playthrough. The intended feel is a world that shifts slowly and believably — borders
move, towns change hands, factions rise and fall, but over campaign-length time, not in an
afternoon. This comes from a structural bias toward defense over offense (low population,
expensive and slow-recovering militaries, costly garrisons, strained supply lines,
defensive terrain advantage, and mandatory recovery time after a major action) — the
*reasons* matter for design coherence even though the specific tuning knobs are
implementation. Autonomous faction warfare (raids and full assaults happening whether or
not the player is present) is what makes the world feel alive without needing the player
to referee it; off-screen assaults resolve and the outcome is discovered later, by design.

Unlike factions, **the player is not bound by these attrition constraints** — a
sufficiently capable and motivated player genuinely can exterminate a Major Faction. That
should be a rare, late-game, costly undertaking with real world-state consequences (power
vacuum, reputation shifts among ideologically aligned/opposed factions, economic
disruption, permanent loss of any named NPCs in that faction) — not something achievable
casually.

## Player Standing, Not Player Power

Standing with each faction — independently tracked, not a single global reputation — is
the lever the player actually has: trade access, whether patrols attack on sight,
recruitment access, contract access, territory access. It's meant to move through direct
action, trade volume, political actions, gifts/bribes, and — distinctly — illicit actions
that get traced back. Illicit play is a first-class path (see **Player Fantasy** in
`vision-and-pillars.md`), but the design intent is that it's never risk-free: getting
caught costs standing and can escalate to a bounty; a sustainable criminal operation is
about *managing exposure* across factions, not about having high stealth skill.

## Assault Intelligence: Investment Should Buy Foresight

The design intent for how players learn an assault is coming is a **layered intelligence
system** where no single channel is mandatory or guaranteed — a player who invests in none
of them gets surprised; a player who invests in several rarely does. The channels are
meant to be cumulative and to reward different kinds of investment:

- **Physical presence/scouting** — being there, or sending a scout, is the earliest and
  most precise channel, and requires no other systems.
- **Tavern/market rumors** — freely available to any player who circulates and interacts
  with settlements; imprecise but low-investment.
- **Criminal network contacts** — paid, passive, strategic-only warning; the player still
  has to do the legwork of confirming details.
- **Standing relationships** — a late-game payoff for relationship investment: named NPCs
  the player has built trust with proactively send word.

No channel should ever hand over a full tactical briefing — intel is meant to be a trigger
for player engagement, not a substitute for it. Channel 1 (presence) is the baseline and
should ship first; the rest are additive enrichments to the same intent.

## Faction Military Progression, as a Player-Facing Signal

The design goal here is **readability without a UI**: a faction's growing strength should
be something the player can *see* in the field — troop tier mix, terminology, visible
elite units — not something they have to check a stats screen to understand. Two
independent axes (soldier tier, and whether the faction has hit its resource+tech "gear
gate") combine so that a low-tier faction with the gear gate met can still "punch above
its weight," and factions should feel distinct from each other through their own
in-fiction rank terminology, not a raw number.

The player is meant to be a participant in this same race, not a spectator: they can raid
research facilities to delay a faction, or contest the resource deposits that gate
advancement, and — critically — **there is no catch-up mechanic**. A player who neglects
their own tech progression while a faction advances should genuinely be able to fall
behind and find a previously-manageable region has outgrown them. That's an intended
outcome of the simulation, consistent with the punishing tone described in
`vision-and-pillars.md`, not a balance bug to eliminate.

## Open Design Questions Worth Tracking

- How factions offer the player work (bounties, escorts, supply contracts) isn't designed
  yet.
- Named-leader succession (scripted tree vs. simulated power struggle) is undecided.
- How fast information (crimes, battles, world events) propagates between factions/regions
  is an open question — slow propagation is explicitly preferred over instant global
  knowledge, for strategic-play reasons.
- How a player actually tracks down a Nomadic Faction (especially a displaced faction in
  hiding) needs a concrete mechanic — informants, intel gathering, supply-chain
  following — that makes the hunt possible without being trivial.
