# Characters and Squads

> Conceptual arch doc. See `game-pillars.md` for design philosophy, `factions-and-world-state.md` for faction-gated recruitment, and `economy.md` for wage economics.

---

## Purpose

Characters are the player's primary instrument in the world. They fight, sneak, steal, trade, build, and research. They are individuals with distinct skill histories, physical conditions, and wage expectations — not interchangeable units. The squad is the collection of characters currently under the player's direction. Its composition, capability, and cost are the central ongoing management challenge of the game.

---

## Lineage

Every character belongs to a **lineage** — the term used across all arch docs to describe what would be called race, species, or ethnic variant depending on the setting. The word "lineage" is intentionally neutral: it applies equally to human regional variants in low fantasy, classic fantasy races like dwarves or elves in high fantasy, and alien species in science fiction.

### What Lineage Is Not
Lineage is not a class, a career, or a skill path. It does not determine what a character can eventually do. A character of any lineage can develop any skill through practice. Lineage shapes the starting point and the ceiling — it does not lock doors.

### Mechanical Effects

**Attribute modifiers.** Each lineage applies a small set of positive and negative modifiers to base attributes. These are meaningful but not dominant — no lineage should be strictly better than another, and no lineage should make a character unviable in a role they have practiced. A lineage with high strength and low cunning is not a "warrior race" — it is a character who starts combat-adjacent and must work harder to develop social skills.

**Skill ceiling adjustments.** Some lineages have slightly higher or lower ceilings on specific skill categories, driven by their attribute profile. A high-agility lineage reaches a higher stealth ceiling; a high-endurance lineage reaches a higher athletics ceiling. These adjustments are modest — the difference between a skilled practitioner of an advantaged lineage and a skilled practitioner of a neutral lineage should be noticeable but not decisive.

**Innate traits.** Each lineage carries one or two innate traits that are fixed — not chosen and not changeable. These may be physical (enhanced low-light vision, unusual endurance in specific climates, resistance to certain environmental hazards) or behavioral tendencies (not skill modifiers, but flavor that informs how an NPC of that lineage is likely to respond in certain situations). Specific traits are TBD by setting.

**Physical characteristics.** Lineage determines a character's visual appearance. In settings with exotic lineages this may include non-human features. Physical appearance is observable by other characters in the world and can affect social interactions independent of formal faction standing — a rare or foreign-looking lineage in a region where none are common draws attention and may attract suspicion or curiosity.

### Scaling Across Settings

The lineage system is designed to scale gracefully depending on the chosen setting:

| Setting type | Lineage character |
|---|---|
| Low fantasy / grounded | All lineages are human variants — regional, cultural, or physiological differences within the human range. Attribute modifiers are subtle. Innate traits are minor. |
| High fantasy | Distinct non-human lineages (dwarves, elves, and similar) with meaningful mechanical differences — meaningfully different attribute profiles, more distinct innate traits, and stronger visual differentiation. |
| Science fiction | Full alien species with potentially exotic physiology. Wider attribute variance. Innate traits may include genuinely unusual capabilities. Some lineages may be incompatible with certain equipment types by default. |

In all cases the design principle holds: no lineage is strictly optimal for all situations, and character skill remains the dominant factor in competence.

### Lineage and Factions

Factions vary in their lineage composition and tolerance:

- **Mono-lineage factions** are composed entirely or almost entirely of one lineage and may distrust, refuse to trade with, or actively persecute outsiders of other lineages. A player squad member of a persecuted lineage in that faction's territory faces additional standing penalties independent of the player's overall faction standing.
- **Cosmopolitan factions** actively recruit across lineages and impose no lineage-based penalties. These tend to be mercantile or ideologically open factions.
- **Lineage-gated recruitment** — some high-value recruits may only be approachable if the player's squad includes a member of the same lineage, or if the player has sufficiently high standing with a lineage-protective faction.

The lineage composition of a faction is part of its authored identity and is consistent across playthroughs (same static world design). A faction that is hostile to a specific lineage will always be hostile to that lineage — this is exploitable player knowledge.

### Lineage and Visibility

A character whose lineage is rare or foreign in a given region draws passive attention. This is separate from standing — even a player with Friendly standing in a mono-lineage faction's territory may find that a squad member of a persecuted lineage attracts guard attention, limits access to certain locations, or triggers hostile NPC behavior independently of the player's formal relationship with that faction.

Managing lineage visibility is a minor but real tactical consideration for squads with mixed lineage composition in intolerant regions.

---

## Character Attributes

Attributes are the fixed underlying characteristics of a character — their physical and mental makeup. They change slowly over time through sustained behavior, but they are not the primary measure of competence. That role belongs to skills. Attributes set the ceiling for how high a skill can go and provide modifiers that amplify or constrain skill outcomes. A high-strength character is not automatically a good fighter; they are a character who will become a *better* fighter given the same amount of combat practice as a lower-strength character.

### Attribute List

**Physical**

| Attribute | Governs |
|---|---|
| **Strength** | Melee damage output, carry capacity, labor output, construction speed, subdual in kidnapping |
| **Endurance** | Health pool, stamina pool, injury recovery rate, resistance to environmental conditions |
| **Agility** | Movement speed, attack and parry speed, dodge, stealth movement noise, fine motor tasks (lockpicking, pickpocketing) |
| **Perception** | Detection range, ranged accuracy, spotting threats and hidden objects, reading target awareness states |

**Mental**

| Attribute | Governs |
|---|---|
| **Intelligence** | Research speed, skill learning rate, crafting technique quality, understanding complex mechanisms |
| **Willpower** | Morale resistance, pain tolerance (effectiveness while injured), resistance to intimidation and capture |
| **Charisma** | Negotiation outcomes, leadership effectiveness, initial NPC impression, persuasion floor |

### Attribute Progression

Attributes change slowly — at roughly one-fifth the rate of skills under equivalent practice. They are not static but move on a different timescale.

**How attributes increase:**
- **Strength** increases through sustained heavy physical activity: combat, construction labor, carrying heavy loads
- **Endurance** increases through surviving hardship: prolonged exertion, recovering from injuries, operating in harsh environments
- **Agility** increases through repetitive precise physical activity: combat, stealth operations, crafting fine goods
- **Perception** increases through sustained observational activity: scouting, ranged combat, tracking
- **Intelligence** increases through research and complex problem-solving: time spent at research facilities, high-tier crafting
- **Willpower** increases through surviving adversity: recovering from near-death, enduring captivity, fighting while significantly injured
- **Charisma** increases slowly through social interactions: sustained trade, negotiation, leadership

**How attributes decrease:**
- Permanent injury is the primary source of attribute loss. Losing a limb reduces Strength and Agility. Severe head trauma may reduce Perception or Intelligence. Chronic, untreated injuries reduce Endurance. These losses are permanent unless partially offset by prosthetics.
- Sustained poor conditions (malnutrition, illness, exhaustion) cause temporary attribute suppression that recovers with rest and care — distinct from permanent injury loss.

**The pace distinction:** A character who fights every day will see their combat skills improve noticeably within a week of in-game time. The same character's Strength will have barely shifted. Attributes are background drift; skills are foreground growth.

---

## Skill System

Skills are the primary measure of a character's competence. All skills improve through use — there are no skill points to allocate, no level-up screens, and no shortcuts. A character becomes a better fighter by fighting. A character becomes a better thief by stealing.

### Skill Categories

**Combat**
- Melee attack, melee defense, ranged attack, ranged defense
- Each weapon class (blade, blunt, polearm, ranged — specifics TBD) may have independent sub-skills

**Physical / Field**
- Athletics (movement speed, stamina, encumbrance tolerance)
- Stealth (detection avoidance, noise reduction, shadow use)
- First aid (healing speed and quality in the field)

**Illicit**
- Pickpocket (success rate and item-size ceiling for pocket theft)
- Lockpicking (lock tier ceiling, speed)
- Thievery (general theft success — container looting, item removal from environment)
- Kidnapping (subdual without killing, victim transport)

**Crafting / Labor**
- Each crafting discipline (smithing, fabrication, cooking, etc. — specifics TBD by setting) has an independent skill
- Labor skills improve production speed and output quality at base facilities

**Trade / Social**
- Negotiation (buy/sell price modifiers)
- Persuasion (faction standing interactions, recruit conversations)
- Leadership (squad morale modifier when this character is present — see Morale below)

### Skill Progression Rate
Skills improve faster at lower levels and slow as they approach the ceiling. Practice under adversity (fighting stronger opponents, stealing from well-guarded targets) yields faster gains than safe repetition. There is no explicit experience multiplier visible to the player — the effect is observable through behavior, not a UI number.

Skills have a soft cap influenced by the character's relevant attribute. A low-agility character can still develop stealth, but will reach a lower ceiling than a high-agility character with equivalent practice time.

---

## Stats, Skills, and Outcomes

Every meaningful action a character takes produces an outcome influenced by two inputs: their **skill** in the relevant discipline and their **attributes**. Skill is always the dominant factor. Attributes are modifiers — they amplify or constrain what skill can achieve, but they do not substitute for it.

### The Principle

> **Skill determines competence. Attributes determine ceiling and rate.**

A character with high sword skill and low Strength will hit reliably but deal modest damage. A character with low sword skill and high Strength will hit infrequently but hurt badly when they do connect. A character with high sword skill and high Strength is the dangerous one. The skill-first principle prevents attribute-stacking from replacing practice.

### Outcome Resolution Model

Each action resolves against one **primary attribute** and optionally one **secondary attribute**. Skill contributes the majority of the outcome score; attributes contribute the remainder as modifiers.

Conceptually:
- Skill provides ~70% of the outcome score
- Primary attribute provides ~20%
- Secondary attribute provides ~10%

These are design proportions, not exposed formulas. The exact coefficients are implementation concerns; the principle — skill dominant, attributes supplementary — is the design constraint.

### Skill-to-Outcome Reference

| Skill | Primary Attribute | Secondary Attribute | What it determines |
|---|---|---|---|
| Melee attack | Agility (strike speed and precision) | Strength (force behind strikes) | Hit chance, hit quality |
| Melee defense | Agility (reaction speed) | Perception (reading attacks) | Parry/block rate |
| Ranged attack | Perception (accuracy, wind reading) | Agility (steady hands) | Hit chance at range |
| Athletics | Endurance (stamina) | Agility (speed) | Movement speed, encumbrance tolerance |
| Stealth | Agility (quiet movement) | Perception (avoiding detection zones) | Detection avoidance |
| First Aid | Intelligence (technique) | Perception (wound assessment) | Healing speed and quality |
| Pickpocket | Agility (hand speed) | Perception (reading target state) | Success chance, item size ceiling |
| Lockpicking | Agility (fine motor control) | Intelligence (mechanism understanding) | Lock tier ceiling, speed |
| Thievery | Perception (spotting watchers) | Agility (quiet hands) | Detection avoidance, success rate |
| Kidnapping | Strength (subdual force) | Agility (speed of grab) | Subdual success without killing |
| Crafting (heavy) | Intelligence (technique) | Strength (physical output) | Output rate, quality ceiling |
| Crafting (fine) | Intelligence (technique) | Agility (precision) | Output rate, quality ceiling |
| Labor | Strength (output) | Endurance (sustained work) | Production rate |
| Negotiation | Charisma (impression) | Intelligence (value assessment) | Buy/sell price modifier |
| Persuasion | Charisma (likability) | Willpower (conviction) | Standing interaction outcomes |
| Leadership | Charisma (inspiration) | Willpower (steadiness under pressure) | Morale modifier for nearby squad |
| Research | Intelligence (primary) | Perception (observational insight) | Research speed, node unlock rate |

### Damage and Harm

Damage is a separate resolution from hit chance. A hit is established first (melee attack skill + Agility vs. melee defense skill + Agility); if a hit connects, damage is then calculated:

- **Base damage** comes from the weapon's stats
- **Strength modifier** scales the damage output up or down
- **Armor mitigation** from the defender's equipped armor reduces the final damage

A skilled, weak attacker hits often but needs more hits to incapacitate. A clumsy, strong attacker lands fewer blows but each one counts. Both are viable combat approaches with different risk profiles.

### Contested vs. Threshold Actions

Outcomes fall into two types:

**Contested** — two characters competing directly. Both characters' skill+stat scores are computed and compared. Combat (attack vs. defense), pickpocketing against an alert target, and negotiation against a resistant NPC are contested. Randomness is present but bounded — a vastly superior score wins reliably, not occasionally.

**Threshold** — a character acting against a fixed difficulty. Lockpicking a specific lock tier, crafting an item, researching a tech node. The threshold is set by the lock/item/node; the character either clears it or doesn't, with partial progress possible on close margins.

### Lineage and Attribute Interaction

Lineage attribute modifiers shift the starting values of the attributes above, which in turn shifts the outcome scores derived from them. A lineage with a Strength bonus produces characters who deal slightly more damage and carry slightly more weight from day one, without any additional skill. The modifier is real but modest — see the Lineage section for the design constraint on magnitude.

---

## Recruitment

### Finding Recruits
Recruitable NPCs exist throughout the world. They are found in:
- Settlements and taverns (openly available, various quality)
- Faction-specific locations (higher-quality or specialized recruits gated behind faction standing)
- Criminal networks (illicit-skilled recruits; require standing with those networks)
- Slave markets (purchasable; willingness to join squad varies — see below)
- In the field (injured or stranded NPCs the player can assist, who may offer to join)

### Hire Fee
Joining the squad costs an upfront hire fee. This reflects the recruit's perceived value, current market conditions, and the player's negotiation skill. The fee is non-negotiable below a floor set by the recruit's skills — players cannot simply charm their way to free recruits.

### Ongoing Wages
Squad members are retained by ongoing wage payment. Wages are paid on a regular cycle (daily, weekly — TBD). Each character's wage is determined by:
- Their current overall skill level (computed from a weighted average of their highest skills)
- A baseline set at recruitment that scales upward as skills grow
- A small individual variation representing personality (a greedy character demands above-market wages)

A character whose skills have grown significantly since recruitment will eventually request a wage renegotiation. The player can accept, counter, or refuse. Refusal risks the character leaving.

### Non-Payment
If wages are not paid on cycle, a character enters an unhappy state. After a short grace period without resolution, they leave — and may take equipment they consider theirs. Squad members leaving due to non-payment may talk, which can affect the player's reputation for reliability with potential future recruits in that region.

### Liberated Slaves
A slave purchased from a market and freed may offer to join the squad. They do not demand a hire fee (they were purchased). Their initial wage is low but scales normally as their skills develop. Their willingness to join depends on how they were treated during transport and their individual disposition.

---

## Morale

Squad morale is a collective condition that affects combat performance, skill learning rate, and the likelihood of characters leaving during hard times.

Morale is influenced by:
- Recent wins and losses (victories raise it; catastrophic defeats lower it)
- Timely wage payment (late or missed payment sharply lowers morale)
- Character deaths (especially veterans the squad knew well)
- Living conditions at base (food availability, shelter quality — see `base-building.md`)
- Leadership skill of a designated squad leader

Low morale does not immediately cause characters to leave, but it widens the conditions under which they will (missed payment, another defeat, harsh conditions). High morale provides a buffer against bad events.

---

## Injuries and Physical Condition

Combat and field accidents produce injuries. Injuries are not healed by resting alone past a threshold — they require treatment.

### Injury Types
- **Minor wounds** — heal over time with or without treatment; slow the character temporarily
- **Major wounds** — require field first aid or medical facility treatment; untreated major wounds worsen
- **Limb damage** — impairs the specific limb's function (reduced combat effectiveness, slower movement, inability to use two-handed weapons)
- **Severed / lost limbs** — permanent loss unless a prosthetic is available and fitted
- **Knocked unconscious** — character is incapacitated; may be captured, looted, or killed by enemies if not recovered

### Prosthetics
Lost limbs can be replaced with prosthetics, which restore partial or full function depending on prosthetic quality. Prosthetics are craftable or purchasable (see `tech-and-crafting.md`). A high-quality prosthetic may restore nearly full function; a crude one may impose permanent penalties. Some characters may start with a prosthetic.

### Permanent Scarring / Stat Effects
Severe injuries that are treated but not fully healed may leave permanent minor penalties. This is intentional — it gives long-serving characters a physical history. A veteran who has survived multiple major injuries may carry small cumulative penalties alongside their high skills.

---

## Stealth and Illicit Operations

Stealth is a field skill, not a mode toggle. A character with high stealth skill moves more quietly, is harder to detect visually, and can remain hidden in partial cover that a low-skill character cannot. Detection is continuous — not a one-time check at a door.

The illicit economy is a viable playstyle, but it is not a shortcut. A key design goal is preventing the runaway exploitation that plagued Kenshi — where a skilled thief could empty a shop's entire inventory in a single night, fence everything locally, and bypass the economic progression the rest of the game is built around. The mechanics below are designed together to prevent this while keeping theft genuinely rewarding for players who invest in it.

### Pickpocketing
Attempted against a specific NPC. Success chance depends on pickpocket skill, target awareness state, crowd density, and item size. Failure is detected and treated as a witnessed theft — immediate standing loss with the relevant faction. The item size ceiling rises with pickpocket skill — a novice can lift small coins; an expert can lift equipped accessories or small weapons.

### Theft (Container / Environment)
Removing items from owned containers or buildings. Success chance depends on thievery skill and whether the area is watched. Some items or locations require a minimum lockpicking skill to access.

**Per-session constraints.** Theft is not instant — each item takes time to locate, extract, and conceal. A character cannot clear an entire shop's inventory in a single night. Three factors naturally limit what can be taken in a session:

1. **Time window** — the period during which the location is unguarded or guards are between patrols is fixed. A single patrol gap is not enough time to take everything.
2. **Suspicion accumulation** — each item taken in a session incrementally raises the location's alert level, increasing detection chance for subsequent attempts. The effect compounds: taking one item is low risk; taking ten in a row is nearly guaranteed detection before the last one.
3. **Encumbrance** — stolen items add weight. A heavily loaded thief moves noisier and slower, raising detection risk and reducing escape speed. The player must choose how much to carry out vs. how safely to escape.

### Security Escalation
When a theft is detected — or when an owner notices missing inventory after the fact — the affected location increases its security. Escalation is persistent and cumulative:

| Escalation stage | Security response |
|---|---|
| First incident | Additional guard on overnight duty; more frequent patrol route |
| Second incident | Better locks on containers; guard placement covers previous gap |
| Third incident | Overnight closure or locked perimeter; multiple guards stationed |
| Severe / repeated | Shop may refuse to operate at hours when theft occurred; higher-quality guards hired |

Security escalation decays slowly over time — a location left alone for many in-game days will gradually return toward its baseline state. But a player who repeatedly targets the same location will face a progressively harder environment, eventually reaching a point where that specific location is not worth the risk.

### Shop Restock Time
Shops source their inventory through the trade caravan system (see `economy.md`). If a shop's inventory is significantly depleted — whether by theft, heavy legitimate trading, or supply disruption — restocking depends on the next caravan delivery. Restock time varies by location:

- **Major towns** — frequent caravan visits; depleted stock recovers within a few in-game days
- **Minor outposts and remote settlements** — infrequent caravans; a cleared-out shop may take many in-game days to recover meaningful inventory

A player who clears a shop's inventory gets a one-time gain but leaves an economically disrupted location behind them. Other players in a co-op session, or NPCs who depended on that inventory, are affected. The world records the impact.

### Stolen Goods Recognition
Items stolen within a faction's territory are recognizable as stolen to members of that faction. Attempting to sell locally-flagged stolen goods to a legitimate shop in the same faction risks detection — the shopkeeper may recognize the item, report the player, and trigger standing loss and potential pursuit.

**Where stolen goods can be sold:**
- **Criminal faction fences** — always willing to buy; price is significantly below market value (the fence's margin for risk). Fence quality improves with standing in criminal networks (see `economy.md`).
- **Distant factions** — a faction sufficiently removed from the theft's origin faction does not recognize the goods as stolen and buys at normal market prices. The travel cost and time are the balancing mechanic.
- **Same-faction legitimate shops** — not viable without significant detection risk. Attempting it is a gamble, not a reliable strategy.

Stolen status is not permanent. After sufficient in-game time — long enough that the theft is no longer recent news — items may lose their flagged status and become tradeable normally. This is a design safety valve, not a primary strategy; the time required is long enough to matter.

### Inventory Transfer Proximity Requirement
Items can only be transferred between characters who are physically adjacent — within a short interaction radius. Characters cannot pass items through walls, across rooms, or over distance. This is a hard mechanical constraint, not a soft suggestion.

This closes the primary Kenshi exploit: a thief carrying loot cannot pass it to a "mule" character waiting outside the building, because the handoff requires the two characters to actually be in the same location. Smuggling loot out of a guarded building requires the thief to physically exit with it — at which point their encumbrance, alert level, and patrol timing all apply.

### Kidnapping
Subduing a live NPC for transport. Requires bringing the target's health low without killing them. The unconscious NPC must be carried (reduces the carrying character's movement and combat ability). Witnesses trigger immediate faction response. Like theft, this cannot be resolved instantaneously — the target must be physically transported to a destination, during which the carrying character is slowed and conspicuous.

### Fencing
Selling stolen goods through criminal intermediary contacts (see above — Stolen Goods Recognition). Requires having established a contact through criminal faction standing. The fence contact's quality and therefore the yield improves with standing in criminal networks.

Fencing is not available everywhere. The player must locate fence contacts through exploration, criminal standing, or word of mouth (see `open-world.md` — Criminal and Illicit Locations). A player who has not invested in criminal faction relationships has limited fencing options and must rely on distant-faction sales instead.

### Save Scumming
Save scumming — attempting a risky theft, failing, reloading a prior save, and trying again — cannot be fully prevented by design without mandatory ironman mode. This is an honest limitation. The design response is not to try to stop it, but to make it less mechanically rewarding:

**Consistent outcomes on reload.** A theft attempt produces the same result if retried under identical conditions — same character skill, same target, same in-game day. Reloading and trying again identically does not generate a new roll. To get a different outcome the player must genuinely change something: improve their skill, wait until the next in-game day, or try a different approach.

**Randomized scenario elements.** Guard patrol timing has variance within its scheduled route — a guard who arrived at a certain time last night may arrive a few minutes later tonight. This prevents memorizing an exact safe window while keeping the world feeling alive rather than clockwork.

**Recoverable failure.** Getting caught stealing is a consequence to manage, not an instant campaign-ender. Standing damage, bounties, and pursuit are serious but survivable. A player who accepts failure and deals with its consequences is not significantly worse off than one who reloaded — which reduces the incentive to reload.

**Ironman mode** (see `content-and-release.md`) is the opt-in solution for players who want to self-enforce. The base game does not force it.

**Design acceptance.** Some players will save scum regardless, and the game should remain fun for them. The theft system is balanced around its mechanics working as intended — a player who save-scums past the constraints is choosing a different (lower-effort) experience, not breaking the economy at the scale that Kenshi's system allowed.

---

## Squad Management

The player manages the squad through:

- **Assignment** — which characters go on a patrol vs. stay at base vs. perform labor
- **Equipment** — arming and equipping each character appropriately for their role
- **Formation and behavior orders** — broad tactical instructions before and during encounters (aggressive, defensive, flee-if-outnumbered — specifics TBD)
- **Role specialization** — deciding which skills to develop through task assignment (send the designated thief on illicit jobs; send the designated laborer to the forge)

The squad has no hard size cap at this design stage, but wages create a natural economic cap — the player can only afford as many skilled members as their income supports.

---

## Known Gaps / Future Notes

- **Lineage roster**: The specific lineages, their attribute modifiers, innate traits, and visual profiles are deferred until setting is chosen. The system design above is the stable framework; the content slots in once the setting is known.
- **Lineage interaction depth**: How granularly lineage-based faction hostility is implemented (blanket standing penalty vs. per-location guard behavior vs. NPC dialogue) needs design during systems implementation.
- **Character generation**: Whether player-created characters (vs. recruited NPCs) are supported is not yet decided. Kenshi allows a custom starting character. This may be tied to starting scenario design.
- **Character personalities and traits**: The set of innate traits and how strongly they govern behavior (e.g., a cowardly character who flees without orders) is not fully designed.
- **Squad behavior AI**: The specifics of automated combat behavior — target priority, retreat conditions, formation spacing — belong in `combat.md`.
- **Wage negotiation UI**: How the player interacts with a wage renegotiation request is not yet designed.
- **Stats, skills, and outcome tuning**: The attribute list and skill-to-outcome framework above are design intent, not final numbers. The 70/20/10 principle (skill dominant, attributes supplementary) is the stable design constraint — the actual implementation coefficients, roll resolution math, and balance are all iterative during systems design. The reference table in "Stats, Skills, and Outcomes" is a starting point, not a specification.
- **Morale cascade risk**: If morale collapse causes multiple characters to leave simultaneously, this could be a soft-lock. Design must include a minimum floor — perhaps one or two deeply loyal characters who will not leave under any condition short of total party wipe.
