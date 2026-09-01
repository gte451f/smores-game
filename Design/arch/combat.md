# Combat

> Conceptual arch doc. See `game-pillars.md` for design philosophy, `characters-and-squads.md` for skills and injuries, and `factions-and-world-state.md` for political consequences of combat outcomes.

---

## Purpose

Combat is the mechanism through which physical conflict is resolved. It is real-time and largely automated — characters fight on their own using their skills and equipped weapons. The player's role is not to control individual attacks but to make high-level decisions: where to position the squad, when to engage, when to retreat, and which targets to prioritize. Good combat outcomes flow from preparation, positioning, and knowing when not to fight — not from reaction speed or manual skill.

Combat has lasting consequences. Injuries persist. Squad members can be captured or killed. Faction standings shift based on who was attacked and who witnessed it. The decision to fight should never be trivial.

---

## Combat Model

### Automated Resolution
Once characters are in range of enemies and ordered to engage (or auto-engage by behavior settings), they attack, defend, and maneuver without direct player input. Attack and defense outcomes are resolved continuously using the relevant combat skills of both parties, weapon stats, armor values, and a degree of randomness that prevents perfectly predictable results.

The player watches the fight unfold and issues corrective orders — redirecting a character who is being overwhelmed, pulling back a wounded member, committing a reserve. Combat is won or lost on the decisions made before and during the fight, not on twitch input.

### Player Control During Combat
While combat is automated, the player retains:
- **Movement orders** — reposition individual characters or the whole squad mid-fight
- **Target priority** — direct a character to focus a specific enemy
- **Engage / disengage** — order a character or the squad to stop fighting and retreat
- **Item use** — direct a character to use a healing item or switch weapons

These are the strategic layer of combat. A player who ignores them and lets the squad auto-fight will lose engagements they could have won with active management.

---

## Pre-Combat Decisions

The most important combat decisions happen before the fight starts.

### Engagement Choice
Avoiding combat is always an option. The player can route around threats, wait for patrols to pass, or use stealth to bypass a dangerous encounter entirely. Choosing to fight should be a deliberate decision based on expected outcome and what the player stands to gain or lose.

### Squad Composition
Bringing the right characters matters. A squad built for stealth and theft is not optimized for open combat. Equipping every member with appropriate armor and weapons before a dangerous expedition is basic preparation. Underprepared squads die.

### Terrain and Positioning
Combat initiated from an advantageous position (higher ground, chokepoint, cover) yields better outcomes than a flat-ground brawl. Scouting an area before committing is rewarded.

### Numerical Assessment
The player should be able to estimate the relative strength of an enemy group before engaging. Patrol size, visible equipment, and faction type are observable signals. Attacking a force twice the squad's size without a significant tactical advantage is likely to end badly — and the game will not warn the player that it's a bad idea.

---

## Melee as Primary Combat

Regardless of setting, melee is the primary form of combat. This is a design-level commitment, not a consequence of the setting. Ranged combat exists and is valuable, but it is a supporting role — a force multiplier for melee, not a replacement for it.

**Why melee first:** Melee creates the kind of emergent drama the game is built around. Characters get knocked out, not sniped from across a field. Fights are messy, close, and costly. The injured and unconscious create rescue decisions. Positioning and weight of numbers matter in a way that long-range attrition flattens. Ranged combat that dominates removes all of this.

### Ranged Combat Constraints

Three mechanisms keep ranged in its supporting role. Not all need to be tuned to maximum effect — the goal is that no ranged option becomes the obvious dominant strategy:

**Friendly fire.** Projectiles do not distinguish between friends and enemies. A ranged character firing into a melee where allies are engaged will hit allies. The denser the melee, the higher the risk. This is not a soft penalty — it is real damage to real squad members. Players who want to use ranged must position their shooters carefully outside the melee, which naturally limits how many ranged characters can contribute simultaneously and creates positioning decisions.

**Rate of fire.** All ranged options have meaningful reload or cooldown periods. A character can fire, then must take time to reload, recover, or recharge before firing again. There is no sustained ranged output. Between shots, a ranged character is a melee liability if an enemy reaches them — they switch to a secondary melee weapon or are fighting at a disadvantage. Rate of fire is calibrated so that ranged contributes meaningfully to a fight without replacing the need for melee characters.

**Defense mechanics.** Shields and similar carried defenses (setting-dependent equivalent — buckler, energy screen, magic ward) specifically counter ranged. A shielded melee soldier advancing on a ranged character has meaningful protection that raw armor does not provide against projectiles. Cover (terrain, walls, barricades) also matters — a ranged character without a clear shot is not contributing. These mechanics mean that enemies adapt to ranged fire rather than simply absorbing it.

### Explicit Exclusions

The following are out of scope regardless of setting:

- **Sustained high-rate ranged fire** — no crossbow that fires as fast as a sword swings, no repeating mechanism that negates the reload constraint, no magic that fires continuously
- **Area-of-effect nukes** — no single ranged attack that damages a large area and bypasses friendly fire risk. A fireball, artillery shell, or explosive device that can wipe a melee is not in this game. Small AOE with strong friendly fire risk is the limit.
- **Extreme range** — ranged that operates at distances where melee characters cannot realistically close. Engagements happen at distances where a fast character can cross the gap during a reload cycle. Sniping from beyond engagement range is not a supported tactic.
- **Unblockable ranged** — projectiles that pass through shields or cover unconditionally. Defenses must work or the constraint mechanic fails.

The test: if a player can win a fight by keeping their entire squad at range without any of them entering melee, the ranged balance is wrong.

### Weapons and Armor

Weapons have stats (damage output, attack speed, reach, weapon class) that interact with the attacker's skill in that weapon class. Using a weapon class the character has low skill in imposes a significant penalty — a trained swordsman handed a polearm is much less effective until they train with it.

Melee weapons are the primary weapon class. Ranged weapons are a secondary slot — a character carries a ranged option alongside a melee weapon, not instead of one. When enemies close to melee range, ranged characters fight with their melee weapon.

Armor reduces incoming damage and provides protection against specific damage types. Heavy armor imposes movement and stamina penalties. The tradeoff between protection and mobility is a meaningful equipment decision, especially for characters in roles that require stealth or rapid repositioning. Shields occupy the off-hand slot and provide active ranged defense at the cost of an off-hand weapon.

Equipment degrades through use and requires repair. A squad that neglects equipment maintenance goes into fights at a disadvantage.

---

## Injury and Incapacitation

When a character takes sufficient damage, they are knocked unconscious rather than killed outright. An unconscious character on an active battlefield:
- Can be looted by enemies
- Can be captured and taken prisoner
- Can be killed by enemies who choose to finish them
- Can be rescued and carried to safety by a squad member

Death is possible but is typically the result of enemies actively executing an unconscious character or the character receiving massive damage in a single blow. This creates a window for rescue and recovery that makes individual fights survivable at the squad level even when they go badly.

Characters who survive major injuries carry those injuries forward (see `characters-and-squads.md`). A squad that fights frequently without adequate medical support will accumulate debilitating injuries over time.

---

## Capture and Prisoner Mechanics

### Player Squad Captured
Unconscious squad members left on the battlefield may be taken prisoner by the victorious faction. Prisoners are held at a faction location. Rescue operations — infiltrating a location to extract a prisoner — are a supported scenario.

### Player Capturing Enemies
The player can take enemies prisoner through the kidnapping mechanics (see `characters-and-squads.md`). Captured enemies can be:
- Sold into slavery (where the slave trade is active)
- Ransomed back to their faction (standing and currency exchange)
- Released (may generate small standing improvement with that faction)
- Recruited (rare; depends on individual disposition and how they were treated)

---

## Combat Consequences

### Faction Standing
Attacking faction members reduces standing with that faction. The severity depends on:
- Whether the attack was witnessed by other faction members or allies
- Whether the player initiated or was defending
- Whether the attacked party survives or is killed

Killing faction members is a larger standing hit than defeating them. Attacking a faction's caravan in an isolated area with no witnesses carries less immediate political cost than fighting their patrol in view of a settlement.

### World State
Large-scale combat — destroying a faction's military force, eliminating a leader, raiding a major settlement — feeds directly into world state changes (see `factions-and-world-state.md`). Individual skirmishes do not shift world state but accumulate into faction military strength changes over time.

### Squad Attrition
A squad that fights constantly without rest, resupply, and medical care degrades. Injuries accumulate, morale suffers after losses, and equipment wears down. Sustained conflict requires active management of the squad's condition between engagements.

---

## Retreat and Escape

Retreating is a legitimate and often correct choice. Characters ordered to retreat will attempt to disengage and move away from the enemy. Fast enemies may pursue; slow or heavily armored enemies may not. Dropping carried weight (including unconscious squad members) increases escape speed — a decision the player may have to make under pressure.

A full squad rout — everyone fleeing — leaves unconscious members behind. This is the worst-case scenario and the cost of being badly outmatched.

---

## Known Gaps / Future Notes

- **Ranged specifics**: The constraint principles are documented (friendly fire, rate of fire, defense mechanics, explicit exclusions). The specific mechanics — range bands, line-of-sight rules, cover interaction, exact reload timing — are implementation details for systems design. Tuning will determine how dominant ranged feels in practice; the design intent is that melee remains primary.
- **Shield mechanics**: How shields interact with ranged attacks (damage reduction vs. full block, active vs. passive, stamina cost) is not yet designed. This is a key part of the ranged defense constraint and needs a full pass.
- **Mounted combat**: Whether mounts exist (and whether they affect combat) depends on setting. Deferred.
- **Group behavior AI**: The specifics of how characters choose targets, manage spacing, and decide when to retreat without player orders need full design during systems implementation.
- **Siege / base defense**: Combat in the context of a base being raided (see `base-building.md`) has additional mechanics — fortifications, chokepoints, garrison behavior — not fully covered here.
- **Settlement assault participation**: The player can join faction-vs-faction assaults on towns and outposts (see `factions-and-world-state.md`). The alarm, breach, and defender behavior rules for those engagements are documented there; the underlying combat resolution uses the same model described in this doc.
- **Morale break**: Whether enemies (or the player's squad) can break and flee from overwhelming odds without explicit retreat orders is an open design question.
- **Faction troop tiers in combat**: Faction soldiers are generated with stat/skill profiles within the range defined by their military tech tier (see `factions-and-world-state.md` — Faction Military Progression). The same combat resolution model applies to all soldiers regardless of tier — higher-tier soldiers simply have higher skill and attribute values feeding into the outcome. No separate resolution path is needed; the existing skill-and-stat model handles the difference. The player-facing implication: a fight that was manageable against a faction's Tier 1 soldiers may not be winnable once they field Tier 2.
