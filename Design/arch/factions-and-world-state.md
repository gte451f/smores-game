# Factions and World State

> Conceptual arch doc. See `game-pillars.md` for design philosophy. Full implementation design belongs in `systems/`.

---

## Purpose

Factions are the political and social fabric of the world. They hold territory, control trade, project military power, and pursue their own agendas independent of the player. World state is the aggregate condition of those factions — who is strong, who is weakening, who is at war, and who controls what. Player actions are one input into a simulation that runs continuously and would produce outcomes even if the player did nothing.

---

## Faction Types

Factions come in three varieties distinguished by their relationship to physical territory.

### Major Factions
Control one or more towns. Towns are the highest-value settlements in the world — population centers with markets, garrisons, and economic output that make Major Factions the dominant political and military powers.

- Have named leaders whose death or removal has world-state consequences
- Maintain their own internal economy (production, taxation, trade through controlled towns)
- Can declare war, negotiate truces, form alliances, conduct raids, and launch full assaults
- May absorb Minor Factions or reduce them to Nomadic status through conquest
- Losing all towns reduces a Major Faction to Minor or Nomadic status — they do not disappear automatically, but their power and reach diminish sharply

**Troop Budget:** Each Major Faction has a finite pool of soldiers — their total military strength. This pool is allocated between two uses:
- **Town garrison** — troops stationed inside each controlled town, defending it during an assault
- **Patrols** — troops deployed into the surrounding region to project presence, interdict threats, and escort trade

The allocation is a strategic tradeoff. A heavily garrisoned town is hard to take but the faction has weak regional presence. A faction that commits most troops to patrols is aggressive on the map but its towns are soft targets. As soldiers are lost to combat, the pool shrinks and the faction must make harder choices about what to protect.

**Troop recovery** is automatic and passive — factions recruit and replenish over time without player involvement. Recovery is slow enough to make losses meaningful but does not require the player or the simulation to manage it explicitly. This keeps the faction simulation feeling alive without turning it into a full strategy game. A faction that suffers a catastrophic defeat will be visibly weakened for a significant stretch of in-game time before it is back to full strength.

### Minor Factions
Control at least one outpost or minor settlement, but no towns. They have a fixed presence in the world but lack the population and resources of a Major Faction.

- Bandit clans, small religious communities, independent militia outfits, local guilds, remote homesteads
- May be allied with, tributary to, or hostile to Major Factions
- Can be elevated to Major Faction status if they capture a town (by world events or player-engineered opportunity)
- Losing their outpost or settlement reduces them to Nomadic status

**Troop Budget:** Same model as Major Factions at a smaller scale. Troops are split between garrisoning the outpost and fielding patrols. A Minor Faction's total troop pool is small enough that a single bad engagement can strip its garrison dangerously thin. Recovery is the same automatic process as Major Factions, but the pool is smaller and a Minor Faction at low strength is a much more tempting target for opportunistic neighbors.

### Nomadic Factions
Not tied to any fixed location. They move through the world, operate across regions, and cannot be dislodged by capturing territory — because they hold none.

- **Trader guilds** — merchant convoys and traveling merchants with no home base; relationships are purely transactional
- **Criminal enterprises** — smuggling networks, slaver organizations, and thieves' guilds that operate from safehouses and shifting contacts rather than claimed territory
- **Displaced factions** — former Major or Minor Factions that have been driven from their holdings and now operate as a mobile remnant; they may attempt to reclaim lost territory or fade away entirely depending on remaining strength
- **Independent wanderers** — mercenary companies, pilgrims, and other groups with no territorial ambition

Nomadic Factions cannot be eliminated through territory denial. Destroying them requires hunting down their members, dismantling their networks, or cutting off their economic lifelines.

**Behavior during assault:** Nomadic Factions present in a town or outpost under attack do not come to the defense of that location — it is not their home. Their response depends on their standing with the attacking force:
- If **Neutral or better** with the attacker, they stand aside, avoid combat, and wait out the assault. Once the dust settles they assess whether they can operate under the new regime.
- If they have **good standing with the incoming faction**, they continue their operations in the town without interruption and may even benefit from the transition.
- If the incoming faction is **hostile to them**, they use the chaos to flee and reestablish elsewhere.
- If the attacker is **the player** and the Nomadic Faction has neutral or better standing with the player, they will not resist and may acknowledge the player's new dominance of the location — potentially opening new interactions.

### Faction Type Transitions
Faction type is not permanent. World events and player actions can change a faction's classification:

| Transition | Cause |
|---|---|
| Minor → Major | Minor Faction captures a town |
| Nomadic → Minor | Nomadic Faction establishes or captures an outpost |
| Nomadic → Major | Nomadic Faction captures a town directly, bypassing Minor status |
| Major → Minor | Major Faction loses all towns but retains an outpost |
| Major/Minor → Nomadic | Faction loses all fixed holdings; remnant survives as mobile group |
| Any → Eliminated | All members killed or captured; no surviving members remain |

### Faction Promotion

Promotion is the upward movement of a faction's type driven by territorial acquisition. It is a first-class world event — when a faction is promoted, its capabilities, relationships, and threat level change meaningfully.

**Causes of promotion** fall into two categories:

- **World-state driven** — the autonomous faction simulation produces conditions where a Minor or Nomadic Faction can seize a town that has become undefended or weakly held. A prolonged war between two Major Factions may leave a contested town so depleted that a Minor Faction on the periphery walks in. A Nomadic criminal enterprise may exploit a power vacuum to establish a permanent foothold.

- **Player-engineered** — the player creates the conditions deliberately. Destroying a town's garrison, weakening a Major Faction's military, or assassinating a leader may open a specific town to takeover by a faction the player prefers — or fears less. Promoting a friendly Minor Faction to Major status by clearing the way for them is a meaningful strategic tool.

**Nomadic factions can promote directly to Major** if they capture a town outright without first establishing an outpost. A displaced former Major Faction — one with organizational memory, named leaders, and latent military capability — is the most likely candidate for this kind of rapid re-ascendancy. A criminal enterprise with sufficient wealth and ambition could also make this jump.

**Promotion changes the world visibly.** A newly promoted faction:
- Begins projecting patrols from their new town into surrounding territory
- Establishes trade relationships (or hostile ones) with neighboring factions based on pre-existing relationships
- May absorb nearby Minor Factions that were previously independent
- Draws the attention — and possibly the hostility — of established Major Factions who now view them as a competitor

Promotion is one of the primary mechanisms through which the world evolves over a long campaign. A player who has been operating in a region for many in-game weeks may return to find the balance of power meaningfully different from when they left.

---

## Settlements: Towns and Outposts

Settlements are the physical anchors of territorial control. A faction's type is defined by what kind of settlement it holds.

### Towns
A town is a walled settlement — the highest tier of location in the world. Walls are not cosmetic; they are a meaningful defensive feature that channels attackers through gates and makes an uncontested assault significantly more costly than attacking an open outpost.

Every town contains the following features:

| Feature | Description |
|---|---|
| **Shops** | Vendors selling goods appropriate to the town's faction and region. Availability and price reflect the regional economy. |
| **Police / Guard Force** | A standing enforcement presence that maintains law inside the walls. They respond to crimes committed in town — theft, assault, murder — and represent a portion of the faction's garrison. |
| **Leader** | A named NPC who governs the town on behalf of the controlling faction. The leader is a point of interaction for contracts, standing negotiations, and political actions. Their death or removal has local and faction-level consequences. |
| **Trade** | A market or trade post where goods flow in from regional suppliers and out to buyers. The trade feature is what makes a town economically significant — losing it disrupts the faction's income. |
| **Garrison** | The faction's troops stationed inside the walls, drawn from the faction's troop budget. The garrison is the primary defensive force during an assault. |

Towns may have additional features depending on their faction and region — temples, slave markets, black markets, specialized craftsmen — but the five above are universal to all towns.

### Outposts and Minor Settlements
An outpost is a smaller, unwalled installation. It may be a fortified camp, a roadside waystation, a mining operation with a guard post, or a small village. Outposts lack the full feature set of a town:

- May have one or two shops or a single trader, but no full market
- May have a small guard presence but no formal police force
- May have a local commander (named or procedural) but not a formal town leader
- Has a garrison drawn from the controlling faction's troop budget, but smaller than a town garrison

Outposts are less valuable than towns but still represent territorial presence and economic output. They are easier to take and harder to hold — their smaller garrison and lack of walls make them vulnerable to even modest raiding forces.

---

### The Player's Position
The player begins outside the faction hierarchy — unknown, unpredictable, and unaffiliated. **The player never becomes a faction.** They cannot own towns, set taxes, declare war, or control trade routes. Their influence on the world is exercised through squad-level action and the reputation consequences that follow: who will trade with them, who will hire them, who will let them pass. This starting neutrality is an asset — a squad with no allegiances can work for parties that are hostile to each other, at least until those parties find out.

---

## Faction Relationships

Each pair of factions has a relationship state that governs how they behave toward each other:

| State | Behavior |
|---|---|
| Allied | Share intelligence, support each other militarily, coordinate trade |
| Friendly | Trade freely, avoid conflict, may assist if attacked |
| Neutral | Transact at arm's length, do not initiate conflict, watch for opportunity |
| Tense | Compete for resources and territory, skirmish at borders, embargo possible |
| Hostile | Active conflict — patrols attack on sight, trade embargo, potential raids on settlements |

### Lineage Composition
Every faction has an authored lineage composition — the mix of character lineages (see `characters-and-squads.md`) that make up its membership. This ranges from fully mono-lineage factions that actively exclude or persecute outsiders, to cosmopolitan factions that recruit and trade freely across all lineages.

Lineage composition is part of a faction's static authored identity and does not change unless the faction is eliminated or fundamentally transformed by world events. It affects:
- Which NPCs the faction fields as guards, traders, and leaders
- Whether player squad members of certain lineages are treated differently in that faction's territory, independent of the player's overall standing
- Which recruitable NPCs the faction produces (mono-lineage factions produce mono-lineage recruits)
- Diplomatic relationships — factions with incompatible lineage ideologies (one persecuting a lineage the other protects) are more likely to start at Tense or Hostile

### Starting Pre-Dispositions
Every faction pair has an authored starting relationship that reflects the world's history before the player arrives. These are not random — a militarist faction and a neighboring trade republic may start at Tense because of a past border dispute; two criminal networks may start Allied because they share supply chains. Pre-dispositions are part of the static world design and are the same every playthrough, giving experienced players meaningful prior knowledge to exploit.

Starting relationships shift over time through world simulation events and player actions. A pair that begins Neutral can become Hostile through a series of escalating skirmishes the player had nothing to do with.

**Relationship asymmetry is valid.** Faction A may be hostile to Faction B while Faction B remains merely tense with Faction A, reflecting different internal politics or information states.

Relationships are dynamic. They shift in response to world events (a border skirmish escalates, a trade deal is struck) and player actions (the player eliminates a faction's rival, or raids a faction's caravan). Factions have memory — an unprovoked attack is not forgotten quickly.

---

## World State

World state is the set of conditions that describe the current balance of power across the map. It is not a single variable — it is a collection of tracked properties per faction and per region that evolve over time.

### Tracked per Faction
- Territory held (regions, key locations, trade routes)
- Military strength (approximate — the player observes this indirectly through patrols, garrison sizes, behavior). Military strength is finite and recovers slowly; it is not an abstracted resource that recharges quickly.
- Military distribution (how many soldiers are committed to garrisons vs. available for offensive action — a faction with all soldiers garrisoning captured towns cannot assault a new target)
- Economic health (production output, trade volume, treasury pressure)
- Leadership stability (who leads, whether succession is contested)
- Active wars and alliances

### State Change Triggers
Events that meaningfully shift world state include:

- **Leader death** — triggers succession, potential internal power struggle, possible faction split or collapse
- **Base or settlement destroyed** — reduces that faction's economic output and territorial control
- **Trade route captured or blocked** — degrades economic health; may force faction into conflict or concession
- **Major military defeat** — reduces military strength; may cause minor factions to defect or opportunist factions to expand
- **Player completing an end-game objective** — specific objectives (installing a leader, destroying a ruling faction) directly rewrite world state

### Ripple Effects
World state changes propagate. A faction weakened by military defeat may lose the ability to protect its trade routes, causing economic decline, which reduces its ability to field patrols, which opens territory to opportunist expansion by a neighboring faction. These cascades are emergent — they arise from the simulation, not from scripted sequences.

The player should be able to observe these cascades unfolding and exploit them. A player who engineers the conditions for a cascade (assassinates a leader, starves a faction's key supply line) and then positions themselves to capitalize is playing the game correctly.

---

## Player Standing

The player maintains an independent standing value with each faction, ranging from trusted ally to kill-on-sight enemy. Standing determines:

- Whether faction members trade with the player
- Whether faction patrols attack on sight or ignore the player
- Which recruitable NPCs are willing to talk
- Access to faction-specific goods, information, and contracts
- Whether the player can enter faction-controlled territory freely

### Changing Standing
Standing changes through:

- **Direct action** — attacking faction members drops standing; assisting in combat, completing contracts, or delivering wanted goods raises it
- **Trade volume** — sustained, profitable trade with a faction gradually improves standing
- **Political actions** — eliminating a faction's enemy, destroying a rival's base, or brokering a deal on a faction's behalf
- **Illicit actions traced back to the player** — see Detection and Bounties below
- **Gifts and bribes** — direct standing purchase, diminishing returns on repeated use

Standing with one faction can affect standing with its rivals. Becoming allied with Faction A may automatically reduce standing with Faction A's enemies.

---

## Detection, Bounties, and Pursuit

Illicit actions (theft, pickpocketing, kidnapping, murder, slaving operations) are not free. The world has memory and factions have reach.

### Detection
Each illicit action has a detection chance modified by:
- The acting pawn's stealth skill
- Number of witnesses present
- Time of day and lighting conditions — night provides meaningful cover; the day/night cycle directly affects detection risk
- Proximity to faction patrols or settlements

A witnessed action triggers immediate standing loss and may cause patrol response. An unwitnessed action may still be discovered later (missing goods, missing persons reported) with a delayed and lower standing penalty.

### Bounties
When standing with a faction falls below a threshold from criminal activity, a bounty is issued. The bounty represents a price on the player's squad — or a named individual within it.

- Bounty hunters (independent NPCs) will pursue the player in faction-controlled territory
- The bounty amount scales with the severity of offenses
- Bounties can be paid off directly with a faction representative, if the player can reach one without being attacked
- Bounties decay slowly over time if no further offenses are committed and the player stays out of faction territory

### Managing Exposure
A sustainable criminal operation requires managing which factions know what. Fencing stolen goods through a faction-hostile to the victim reduces the risk of unified response. Conducting kidnapping operations far from settlement centers reduces witnesses. Running slaving operations through minor faction intermediaries creates plausible separation.

---

## Faction AI Behavior Priorities

Factions are not static. They pursue goals with autonomous behavior:

- **Expansionist** factions actively patrol borders, raid weakened neighbors, and attempt to establish new outposts
- **Mercantile** factions prioritize trade route security, will negotiate to avoid conflict, and respond strongly to economic threats
- **Militarist** factions build military strength, respond to provocations aggressively, and may launch unprovoked wars when strong
- **Isolationist** factions avoid conflict, maintain defensive postures, and do not expand — but resist encroachment sharply
- **Criminal/Illicit** factions (smuggling networks, slaver organizations) operate in the shadows, have indirect territory, and respond to exposure rather than military threat

A faction's AI priority is a tendency, not a lock. A mercantile faction pushed into a corner will eventually fight.

---

## Faction Autonomous Warfare

Factions conduct raids and full military assaults against each other independent of the player. This is not reactive behavior — it is proactive. An expansionist faction that sees a weakened neighbor will act on it. A militarist faction with a full treasury will look for a war to fight. The player inhabits a world where these conflicts happen around them and without their involvement.

### Raids
Small-scale aggressive actions against enemy settlements, outposts, or caravans. Raids are frequent, low-commitment tests of a rival's defenses. A successful raid weakens the target economically, emboldens the aggressor, and may trigger retaliation or concession.

### Full Assaults
Large-scale military operations aimed at capturing or destroying a settlement or region. Assaults are preceded by observable buildup — troop movement, supply convoys, patrol intensity changes. A player paying attention can anticipate them. An assault that succeeds permanently shifts territorial control.

**Alarm:** When an assault begins, the defending settlement raises an alarm. All available garrison troops mobilize immediately and move to defensive positions. Any off-duty guards or militia present in the town respond as well. The alarm cannot be suppressed once the attack is detected — a stealthy approach may delay detection, but once fighting starts or the gate is breached the alarm is up. This is distinct from the stealth detection system for individual crimes; an assault is an open act of war.

**Breach and clear:** Attackers attempt to break through the settlement's defenses — forcing gates, scaling walls at weak points, or overwhelming guard posts — and then eliminate the garrison. Combat proceeds under the standard rules (see `combat.md`): automated real-time fighting until one side is defeated. Defenders fight to the death or incapacitation. They do not rout or flee unless vastly outnumbered — at that point surviving defenders may attempt to escape through a back gate, blend into the civilian population, or take cover and hide rather than die in a hopeless stand. The threshold for flight is high; defenders are fighting for their home.

**Off-screen simulation:** Assaults that occur while the player is not in the area are simulated rather than played out in full. The outcome is resolved based on the relative military strength of the attacking and defending forces, with some randomness to prevent perfectly predictable results. The player discovers the outcome when they next visit the area or receive word through contacts. Off-screen simulation means the world changes meaningfully without requiring the player to witness every event — returning to a region and finding a town under new management is a legitimate and intended experience.

### Assault Intelligence — How the Player Learns What Is Coming

Faction assaults are not announced to the player. The player must learn about them through the world — by being present, by maintaining relationships, or by paying for information. The design intent is a **layered intelligence system**: multiple channels exist, each requiring a different form of player investment, and together they give engaged players a realistic chance to know what is coming before it happens.

No single channel is mandatory or guaranteed. A player who invests in none will be surprised by assaults; a player who invests in several will rarely be. The channels are cumulative — a player close to a region AND with criminal contacts AND with standing relationships has the clearest picture.

**The overarching principle:** intel precision and lead time scale with how invested the player is in a region. Distance, neglect, and social isolation produce ignorance. Presence, relationships, and money produce forewarning.

**Channel 1 — Physical presence and scouting**
A player or scout in or near a region observes the pre-assault buildup directly: unusual troop concentrations, supply wagons moving toward a border, patrol frequency increasing near a target settlement. A character with high Perception picks this up passively while traveling. A dedicated scout sent into faction territory can confirm it deliberately. This channel provides the earliest and most precise intelligence but requires the player to already be in the area or willing to send someone there.

**Channel 2 — Tavern and market rumors**
NPCs circulating through towns — merchants, travelers, off-duty soldiers — carry fragments of information. A player who trades, recruits, or simply talks to NPCs in settlements will periodically surface warnings: *"I've heard the Iron Fang have been buying more provisions than they need"* or *"The road north has been busier than usual with armed men."* Rumors are imprecise in timing and unconfirmed in scope, but they are freely available to any player who circulates through the world and interacts with it. This channel rewards movement and social engagement.

**Channel 3 — Criminal network contacts**
A player with sufficient standing in a criminal network can pay for advance intelligence on faction military movements. Criminal organizations operate across faction lines and have informants in many locations — their nomadic nature makes them plausible cross-faction intelligence brokers. This channel provides strategic warning (*"an assault is being planned in the northern region within the next few days"*) without tactical detail. The player learns something is coming and roughly where; they must still travel, scout, and assess the situation themselves. This channel costs coin or standing to maintain and is the most passive of the options — the player does not need to be nearby — but deliberately provides less precise information than physical presence.

**Channel 4 — Standing relationships**
A named NPC the player has built a relationship with — a merchant they have traded with repeatedly, a guard captain they assisted — may send word when their settlement is threatened. This is a late-game payoff for relationship investment: the world the player has engaged with starts feeding them information. The warning is local and personal (*"Aldric the merchant sends word that armed men have been spotted massing at the ridge north of Stonegate"*), but it is specific and reliable. This channel is not designed or built in advance — it emerges naturally if the contact and relationship system supports it.

**What intelligence does not do**
No channel provides a full tactical briefing. The player learns that something is coming and approximately when and where. Knowing the exact attack composition, timing, and route requires being there. Intelligence is a trigger for engagement, not a substitute for it.

---

### Player Participation in Faction Assaults

The player may elect to join an assault being conducted by a faction they are Neutral or better with. Participation is voluntary and the player chooses when and how to engage — they are not conscripted.

- **Joining an assault** — the player's squad fights alongside the attacking faction's forces. Kills and contributions count toward the outcome. Assisting a successful assault improves standing with the attacking faction and may earn payment or loot rights.
- **Defending against an assault** — the player may similarly choose to help defend a settlement that is being attacked. Successful defense improves standing with the defending faction. The player's motivation may be that they oppose the attacking faction, value the defending faction, or simply want the loot and standing reward.
- **Risk** — participation means the player's squad is in a full military engagement. Injuries, captures, and deaths are possible. The player cannot extract their squad from an ongoing assault without taking the risks of disengaging from active combat.
- **Neutrality** — the player is never forced to participate. Watching an assault from a distance, looting the aftermath, or simply moving on are all valid choices.

### Territorial Constraints — Why Factions Don't Paint the Map

The world has low population. Military forces are scarce, expensive to maintain, and slow to recover. This creates a structural bias toward defense over offense, preventing any single faction from steamrolling the map.

**Offense is attrition.** Assaulting a defended position costs significant manpower — often more than defending it. Killed soldiers are not immediately replaced. A faction that wins a costly assault may be weakened enough that it cannot defend its new acquisition.

**Garrison cost.** Holding captured territory requires leaving troops behind to garrison it. Every soldier garrisoning a newly captured town is a soldier not available for the next assault. A faction that expands quickly thins its military across more territory.

**Supply line strain.** Territory far from a faction's core holdings is expensive to supply and reinforce. The further a faction expands, the longer and more vulnerable its supply lines become — and the more they cost to maintain relative to the value of the territory.

**Defensive terrain advantage.** Defenders hold terrain they know, with shorter supply lines, interior lines, and prepared positions. The attacker must move to them. All else being equal, a smaller faction defending its home territory is not easy to dislodge.

**Recovery time.** After a major military action — win or lose — a faction needs time to regroup, resupply, and replenish. They do not immediately launch another assault. This creates natural pauses in expansion that give the player and rival factions time to respond.

The net effect is a world that shifts slowly and believably. Borders move, towns change hands, factions rise and fall — but over campaign-length time, not in an afternoon. No faction should plausibly conquer the whole map during a typical playthrough.

### Consequences for the Player
Faction wars create both opportunity and risk for the player:
- A war between two factions creates demand for mercenary work, escorts, and supplies from both sides
- A faction weakened by war may offer better contract terms or accept lower standing requirements
- A faction that wins a major war becomes stronger and more dangerous — potentially a new threat to the player's region
- The player's base or trade routes may be caught in the middle of a conflict they did not start

The faction simulation is not designed to be predictable. Unexpected wars, surprising alliances, and rapid territorial shifts are features, not bugs. The player should feel embedded in a world that has its own momentum.

---

## Player Extermination

Unlike AI factions, which are constrained by attrition, supply lines, and garrison costs (see Territorial Constraints above), the player is not prosecuting a territorial war — they are conducting a sustained campaign of destruction. A sufficiently motivated and capable player can eliminate a faction entirely.

Extermination is achieved by killing or capturing all of a faction's members. There is no diplomatic resolution once the player has committed to this path — the targeted faction will treat the player as an existential threat and respond accordingly.

### Consequences of Extermination
Eliminating a faction is a major world-state event with broad consequences:

- **Power vacuum** — the faction's territory, trade relationships, and contracts become contested; neighboring factions respond to the opportunity
- **Reputation shift** — other factions take note. Factions ideologically aligned with the eliminated faction may become hostile; factions that were enemies of the eliminated faction may improve standing with the player
- **Economic disruption** — goods and trade routes the faction controlled become scarce or contested until the vacuum is filled
- **Named NPC deaths** — any named NPCs within the faction are permanently gone, foreclosing quest lines or contracts tied to them

### The Cost of Extermination
A war of extermination against a Major Faction is not a short campaign. The player's squad faces:

- Sustained military pressure — the faction will hunt the player aggressively once it recognizes the threat
- Possible allied response — factions allied with the target may enter the conflict
- Attrition on the player's own squad — this is the most prolonged and dangerous type of player-initiated conflict

Against a Nomadic Faction, extermination requires locating and eliminating dispersed members who have no fixed point of failure. Hunting a criminal network to extinction is a different challenge than destroying a garrison.

### Minor and Nomadic Faction Extermination
Smaller factions are reachable targets for a mid-game squad. Eliminating a bandit clan or a minor criminal outfit is a plausible objective. Full extermination of a Major Faction is a late-game undertaking and would constitute an end-game objective in its own right.

---

## Town Capture and Territory Transfer

When a town's garrison is destroyed — whether by the player, by a rival faction, or by an autonomous assault — the town becomes vulnerable to occupation. A faction can move into and claim that town if:

1. Their existing territory is **contiguous** with the town's location (they cannot teleport a claim across unconnected territory)
2. They have sufficient military strength to spare for a new garrison
3. No stronger faction acts first

This means the player can deliberately destroy a garrison to engineer a territory transfer — weakening a faction they dislike while strengthening one they prefer, or clearing space for their own operations. The player does not claim the town themselves; the world fills the vacuum according to its own logic.

### Ruin and Rebirth

A captured or abandoned town does not instantly reset under new management. It goes through a **transition period** lasting several in-game days during which the location is in a state of ruin:

- The defeated faction's remaining NPCs flee, scatter, or go into hiding
- The town leader is gone; no law enforcement is active
- Shops are closed or looted; trade has stopped
- The walls remain but the garrison is empty — the town is physically present but socially hollow
- Crime, scavenging, and opportunistic violence are unpoliced during this window

The transition period is dangerous but also an opportunity. Goods can be looted from undefended stores. Fleeing NPCs can be intercepted. The absence of law means the player can operate without the usual detection and standing consequences — at least temporarily.

### Settlement Parties and the Race to Colonize

When a town falls — particularly one vacated by a player assault — nearby factions that qualify (contiguous territory, available troops) may dispatch **settlement parties**: organized groups moving toward the vacant town with the intent to garrison and claim it.

- Settlement parties are visible in the world — the player can observe them moving and interact with or intercept them
- Multiple factions may dispatch parties simultaneously, creating a race to reach and occupy the town first
- If two settlement parties from rival factions arrive in close succession, **they may fight each other** before either can establish control — producing a second battle the player did not initiate and may choose to influence, watch, or exploit
- A settlement party that arrives unopposed begins establishing a garrison; the town exits ruin and begins rebirth under the new faction over the following days
- If no settlement party arrives within a threshold period, the town may be claimed by opportunistic minor groups — bandits, displaced nomads — rather than an organized faction

The player can use settlement party dynamics strategically: escorting a preferred faction's party to ensure they arrive safely, intercepting a rival's party to prevent an unfavorable takeover, or simply watching to see who wins and planning accordingly.

### Rebirth

Once a new garrison is established, the town begins recovering:
- New faction NPCs populate the location over time
- Shops reopen under the new faction's trade profile
- A new leader is installed (named or procedural, depending on the faction)
- Police presence resumes under the new faction's rules
- The regional economy begins adjusting to the new controlling faction's production and trade patterns

A town that has changed hands is meaningfully different to operate in — different goods, different standing requirements, different NPCs, potentially different laws regarding the player's activities.

---

## Faction Military Progression

Factions are not static military forces. Over the course of a campaign, each faction independently advances through military technology tiers — fielding progressively better-equipped, better-trained soldiers. This is not a scripted difficulty ramp. It is an emergent world property: different factions advance at different rates each playthrough, creating an unpredictable competitive pressure that the player must respond to or fall behind.

### What Progression Means: Tier and Gear Are Separate

A soldier's threat level is the product of two independent axes: their **tier** (stat/skill envelope) and their **gear** (what they are actually carrying). These are designed and tracked separately. Two factions can field soldiers at the same tier who are meaningfully different in combat because their gear profiles differ.

**Tier — the stat/skill envelope**

Tier reflects how competent a soldier is as a fighter. It is the baseline capability independent of what they hold in their hands. Procedural soldiers are generated with stats and skills within the range defined by their tier:

| Tier | Stat/Skill Profile |
|---|---|
| **Tier 1** | Low combat skill range; attributes near the minimum for the faction. Relatively easy to kill and outmaneuver. |
| **Tier 2** | Moderate combat skill range; attributes in the middle band. Hit more reliably, harder to put down. |
| **Tier 3** | High combat skill range; attributes near the upper end. Fight well, absorb punishment, and create real problems for an underprepared squad. |
| **Tier 4** | Elite range; maximum stat/skill ceiling for the faction. Dangerous individually; in numbers, a late-game threat. |

A Tier 2 soldier is harder to hit, hits more reliably, and takes more punishment than a Tier 1 soldier — regardless of what either of them is carrying.

**Gear — what they carry**

Gear is a faction-wide binary gate, not a per-soldier calculation. A faction either has enhanced gear or they do not. The condition is simple:

1. Does the faction control a source of the relevant advanced material (a rare resource deposit, production outpost, or trade access to one)?
2. Has the faction progressed their tech tree far enough to make use of it?

If both conditions are met, **all soldiers in that faction — at every tier — carry the enhanced equipment**. A Tier 1 soldier and a Tier 4 soldier in that faction both have +1 swords. The player encounters a uniformly better-equipped force, not a mixed one.

This keeps faction gear readable: a faction either has it or they don't. Two factions at the same tier are straightforwardly comparable unless one has hit the gear gate and the other hasn't.

**The player uses the same system.** Controlling the resource source and having the relevant tech node unlocked means the player's squad can carry the same enhanced gear. The strategic value of a rare resource location is inseparable from the military advantage it confers.

**Actual threat = Tier × Gear gate**

| Tier | Gear gate | Result |
|---|---|---|
| Low tier | Not met | Manageable — routine threat |
| Low tier | Met | Punching above their weight — more dangerous than tier alone suggests |
| High tier | Not met | Skilled but standard — can be matched with good preparation |
| High tier | Met | Late-game serious threat — both capable and well-equipped |

### Faction-Specific Soldier Terminology

Each faction uses its own names for its soldier tiers. The underlying tier number (1–4) is a mechanical abstraction used in game systems; the player never sees "Tier 2" in the world — they see the faction's own term for that rank.

**Each faction authors a name for each of their 1–4 tiers.** Not every faction needs four tiers — a small nomadic criminal outfit might have only two grades of enforcer, while an established major military faction might maintain a full four-tier hierarchy. The number of tiers a faction supports is part of their authored definition.

Examples of how the same underlying tier maps to different faction vocabulary (hypothetical, setting-agnostic):

| Tier | Faction A (militaristic, disciplined) | Faction B (tribal, honor-based) | Faction C (criminal network) |
|---|---|---|---|
| 1 | Conscript | Initiate | Street Thug |
| 2 | Soldier | Warrior | Enforcer |
| 3 | Sergeant | Bloodsworn | Lieutenant |
| 4 | Veteran Commando | Elder Champion | Elite Shock Trooper |

The faction's terminology reflects its culture and identity. A disciplined military faction uses rank language; a criminal enterprise uses underworld hierarchy; a tribal faction uses honor-and-blood language. This makes factions feel distinct from each other even before the player has learned their politics.

**Player readability over time.** The player will not know what an "Elder Champion" represents on their first encounter. Recognizing that a specific title maps to a dangerous Tier 4 soldier is learned knowledge — the same kind of systems mastery the game rewards throughout. The Codex (see `player-experience.md`) records faction terminology as the player encounters it, so veterans can reference what they have learned without leaving the game.

**Data-driven requirement.** Faction tier names are authored in the faction's data asset alongside other faction-specific properties. They must not be hardcoded. Mod-added factions define their own tier names and tier count within the same schema.

### Troop Mix as Player Communication

Factions do not instantly field all higher-tier soldiers when they advance. Progression is gradual: as a faction's military tech tier increases, they begin producing and fielding higher-tier soldiers alongside their existing lower-tier ones. At any given moment, a faction's patrols and garrisons will reflect a **mix of tiers** that directly signals where they are in their military development.

This mix is intentional player transparency. A player who observes a patrol or garrison can immediately read the faction's current strength:

| What the player sees | What it means |
|---|---|
| All Tier 1 soldiers | Faction is early-stage; manageable for a prepared squad |
| Mostly Tier 1, some Tier 2 | Faction is beginning to advance; pressure is growing |
| Even mix of Tier 1 and Tier 2 | Faction is mid-advancement; noticeably more dangerous than early game |
| Mostly Tier 2, few Tier 1 | Faction is well-advanced; requires strong player response |
| Tier 2 and Tier 3 soldiers present | Faction is a serious late-game threat |

The player does not need a UI indicator or faction status screen to know the faction is getting stronger. They can see it on the soldiers they encounter. A region the player visited early in the campaign that now shows more Tier 2 soldiers is a visible warning that the world has not stood still.

**Named / Elite units** — At higher tiers, factions may field a small number of named elite soldiers (authored NPCs rather than procedural) who serve as champion-level threats in key locations. These are rare, visually distinct, and mark the faction's military as fully matured. Killing an elite unit is a meaningful event, not routine combat.

Scouting a faction's patrol composition before engaging is always worthwhile. Committing a squad to a fight and discovering mid-engagement that the enemy has more Tier 2 soldiers than expected is a player error, not an unfair surprise — the information was visible if the player had looked.

### Starting Tiers and Zone Baseline

Factions do not all start at Tier 1. Each faction has an **authored starting tier** that reflects how established and powerful they are at the opening of the campaign:

- Factions in safe, peripheral regions may begin at Tier 1
- Factions in contested or dangerous regions may start at Tier 2 or higher
- This creates the intended zone difficulty gradient: some areas are inherently harder at the start of every playthrough because the factions there are already better equipped

Starting tiers are part of the authored world — they are the same every playthrough. What varies between playthroughs is how quickly each faction advances *from* their starting tier.

### Advancement Rate

Rate of military advancement is not uniform. Each faction advances at a rate driven by two factors:

1. **Access to advancement resources** — the rare, high-value materials that gate mid- and high-tier technology (see `economy.md` and `tech-and-crafting.md`). A faction that controls rich sources of advancement resources will progress faster; a faction cut off from them will stagnate. This rate driver is dynamic — player or world-state actions can change it.

2. **A per-playthrough randomized base rate** — within a designed range, each faction is assigned a slightly different baseline advancement speed at campaign start. This is the primary source of playthrough variance. The same faction might be a Tier 3 military power by mid-campaign in one playthrough and still struggling at Tier 2 in another, depending on their random rate and resource access.

These two factors combine: a faction with a fast base rate that also controls strong resource sources is the most dangerous long-term threat. A faction with a slow base rate that has been deprived of resources barely progresses at all.

### Player Interaction with Faction Progression

The player is not a passive observer. They can influence how quickly factions advance:

- **Raid faction research facilities** — destroying or looting a faction's research buildings delays their progression. The effect is temporary; they rebuild over time. But in a compressed campaign, disrupting a dangerous faction's tech advancement by even a few in-game weeks can be decisive.
- **Control advancement resource sources** — a faction that cannot source the materials for higher-tier production cannot advance. Establishing a base near a key deposit and denying faction access is a strategic objective, not just an economic one.
- **Trade networks** — supplying a faction with advanced goods (even indirectly through trade) may inadvertently help them arm their soldiers. A faction that buys better weapons from player-supplied markets advances faster than one in isolation. This is an intentional emergent consequence, not a bug.

### The Player's Tech Race

The player is in the same arms race. The meaningful measure is not absolute player tech tier — it is the **gap between the player's tech tier and the factions they interact with**.

- A player who advances quickly may find that what was once a dangerous zone is now manageable — the factions there, while locally powerful, have not kept pace
- A player who neglects tech progression while focusing exclusively on base building, economy, or squad size may find that factions they could once fight on equal terms now field visibly superior equipment
- **The player can fall behind.** There is no catch-up mechanic. A campaign in which the player's tech stagnates while major factions advance to Tier 3 becomes significantly harder. The player may need to reduce their objectives, change strategy, or accept that the playthrough is a loss

This dynamic is intentional and consistent with the game's punishing design philosophy. The world does not wait.

### What Progression Does Not Do

- Faction military progression does **not** apply to wildlife. Animals do not acquire equipment or organize. Wildlife danger is fixed by zone from the start of the campaign — some regions have more dangerous wildlife than others by authored design, and this does not change over time. See `open-world.md`.
- Progression does **not** lock zones behind a required player tier. A player can enter any zone at any point. Whether they survive depends on their squad, equipment, and preparation — not on an invisible gate.
- Progression does **not** guarantee that the most dangerous factions stay the most dangerous. A faction that suffers heavy losses, gets cut off from resources, or is targeted for extermination may regress or stagnate while others advance past them.

---

## NPCs: Named and Procedural

### Named NPCs
Specific, authored individuals with defined identities, faction affiliations, skills, and roles. Named NPCs are placed in the static world and have meaningful game-state significance:
- Faction leaders whose death triggers succession events
- High-value recruitment targets with unusual skill profiles
- Unique traders or fence contacts with access to rare goods
- Quest-adjacent characters tied to end-game objectives

Named NPCs do not respawn if killed. Their death is a permanent world-state change. Killing the wrong named NPC can foreclose options the player did not know existed.

### Procedural NPCs
Characters generated at runtime to populate the world with plausible inhabitants. They carry a role label ("Shop Keeper", "Guard", "Laborer", "Traveler") and are assigned randomized names, appearances, and skill profiles within bounds appropriate to their role and faction. Procedural NPCs can be recruited, robbed, kidnapped, and killed without world-state significance — they are the texture of the world, not the bones of it.

Procedural NPCs are generated with a lineage consistent with their faction's composition (see `characters-and-squads.md`). A mono-lineage faction produces guards and shopkeepers of that lineage. A cosmopolitan faction produces a mixed population. The lineage of a procedural NPC affects how they respond to player squad members of different lineages in intolerant regions.

Procedural NPCs respawn over time as settlements naturally recover their populations. The rate of respawn may be slowed in areas that have been heavily disrupted. When a town changes hands, repopulating NPCs reflect the new faction's lineage composition — a visible signal that the settlement has genuinely changed ownership.

---

## Known Gaps / Future Notes

- **Player faction: explicitly excluded by design.** The player cannot found a faction, own a town, set taxes, or control trade routes. This is a firm scope boundary — the game is squad-level, not 4X. Do not design systems that blur this line.
- **Faction events and contracts**: The mechanism by which factions offer the player work (bounties, escort contracts, supply requests) is not yet designed. Likely lives in `economy.md` or a dedicated contracts system.
- **Faction succession**: What happens when a named leader dies needs more detail — scripted succession trees vs. simulated internal power struggles.
- **NPC information propagation**: How fast information (player crimes, world events, battle outcomes) travels between factions and regions is an open simulation design question. Instant global knowledge is unrealistic; slow propagation enables more strategic play.
- **Assault intelligence channels**: The layered intel system documents four channels (scouting, rumors, criminal contacts, standing relationships) as the overarching design intent. Not all channels need to ship together. Channel 1 (physical presence) is the baseline and should ship first; the others are additive. Channel 4 (standing relationships) depends on the NPC contact system being built.
- **Assault simulation detail**: The specific mechanics of how the AI decides to launch a full assault (threshold conditions, force marshalling, approach routing) are not yet designed. This is a significant simulation system.
- **Gear gate complexity**: The current model is intentionally simple — resource + tech node = all soldiers enhanced. Future complexity could include gradual rollout (higher tiers get enhanced gear first), multiple enhancement types (weapon vs. armor separately), or partial availability based on supply disruption. Start simple; add only if the base system feels insufficient.
- **Expansion rate tuning**: The balance between military recovery speed, garrison costs, and offensive thresholds is critical to preventing map-painting. This needs careful simulation testing — if recovery is too fast or garrison costs too low, factions will still steamroll. If too slow, the world stagnates. The goal is slow, believable border movement over a full campaign.
- **Nomadic faction tracking**: How the player locates and pursues a Nomadic Faction's members (especially a displaced faction in hiding) is an open design question. There needs to be some mechanic — informants, intelligence gathering, following supply chains — that makes the hunt possible without being trivial.
- **Faction remnant behavior**: When a Major or Minor Faction is reduced to Nomadic status, how actively it attempts to reclaim territory vs. simply persisting as a diminished group needs design. A faction that fights back to Major status after being nearly eliminated would be a memorable world event.
- **Named NPC authored content scope**: How many named NPCs exist, what depth of authored content they carry, and how they interact with procedural systems is not yet determined.
- **Lineage roster**: Specific lineages per faction are deferred until setting is chosen. The lineage composition framework (mono-lineage vs. cosmopolitan, authored per faction) is stable; the content fills in once setting is decided.
