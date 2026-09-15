# Combat

## Purpose

Combat is what happens once a fight has started. What decides *whether* a character
fights at all — who counts as an enemy, what a character notices, and when they
disengage on their own — is `ai-and-behavior.md`.

Combat is the mechanism through which physical conflict resolves — real-time and largely
automated, characters fight on their own using their skills and equipped weapons. The
player's role is high-level: where to position the squad, when to engage, when to retreat,
and which targets to prioritize. Good outcomes are meant to come from preparation,
positioning, and knowing when *not* to fight — never from reaction speed or manual
execution. Combat has lasting consequences: injuries persist, squad members can be
captured or killed, and faction standing shifts based on who was attacked and who
witnessed it. The decision to fight should never be trivial.

## The Combat Model

Once engaged, characters attack, defend, and maneuver on their own, resolved continuously
from both sides' combat skills, weapon stats, armor values, and bounded randomness. The
player watches the fight and issues corrective orders — reposition an overwhelmed
character, pull back the wounded, commit a reserve. This is the strategic layer the player
actually controls: **movement orders, target priority, engage/disengage, and item use.** A
player who lets the squad auto-fight without engaging this layer will lose fights they
could have won.

## Pre-Combat Decisions Matter Most

Avoiding combat is always a valid choice — routing around a threat, waiting out a patrol,
using stealth to bypass entirely. Choosing to fight should be deliberate: the right squad
composition for the job (a stealth/theft squad is not built for open combat), terrain and
positioning (higher ground, a chokepoint, cover), and an honest numerical assessment of the
enemy (patrol size, visible equipment, and faction type are all observable signals — the
game will not warn the player before a bad fight).

## Melee Is Primary, By Deliberate Design

This is a design-level commitment independent of setting, not an incidental consequence of
one. Melee produces the close, messy, costly fights the game is built around — characters
get knocked out rather than sniped from across a field, and the injured and unconscious
create rescue decisions that long-range attrition would flatten out entirely. Ranged
combat is real and valuable, but it stays a **supporting force multiplier**, held there by
three levers, none of which need to be tuned to their maximum to work:

- **Friendly fire** — projectiles don't distinguish friend from foe; a shooter firing into
  a dense melee will hit allies, which naturally limits how many ranged characters can
  contribute at once and forces real positioning decisions.
- **Rate of fire** — every ranged option has a meaningful reload/cooldown; between shots a
  ranged character is a melee liability if an enemy reaches them.
- **Defense mechanics** — shields and cover specifically counter ranged in a way raw armor
  does not, so enemies genuinely adapt to ranged fire rather than just absorbing it.

**Explicitly out of scope regardless of setting:** sustained high-rate ranged fire, any
area-of-effect attack that wipes a melee and bypasses friendly-fire risk, engagement
distances long enough that melee characters can't realistically close, and unblockable
projectiles. The test that decides whether ranged balance is right: *if a squad can win a
fight entirely at range without anyone entering melee, it's wrong.*

Weapon-class skill mismatch imposes a real penalty (a trained swordsman handed a polearm
is markedly worse until they train). Melee is the primary weapon slot; ranged is a
secondary a character carries alongside it, falling back to melee once enemies close.
Armor trades protection for mobility and stamina; equipment degrades and needs upkeep.

## Injury, Incapacitation, and Capture

Sufficient damage knocks a character unconscious rather than killing them outright — an
unconscious character on an active battlefield can be looted, captured, killed outright by
an enemy who chooses to finish them, or rescued and carried to safety. This creates a real
rescue window that keeps individual fights survivable at the squad level even when they go
badly. Death is typically the result of active execution or a single massive hit, not the
default outcome of losing. Injuries that are survived carry forward (see
`characters-and-squads.md`) — a squad fighting constantly without medical support
accumulates real, lasting debilitation.

Captured squad members are held at a faction location and can be rescued through
infiltration. Enemies the player captures can be sold into slavery, ransomed, released
(a small standing gain), or — rarely, depending on disposition and treatment — recruited.

## Consequences

Attacking faction members costs standing, worse if witnessed and worse if lethal — killing
is a bigger hit than merely defeating. Large-scale combat (destroying a faction's
military, eliminating a leader, raiding a major settlement) feeds directly into world
state (see `factions-and-world-state.md`); individual skirmishes accumulate into faction
military strength over time rather than shifting it instantly. Constant fighting without
rest, resupply, and medical care degrades a squad — injuries pile up, morale suffers after
losses, equipment wears down.

## Retreat

Retreat is a legitimate, often-correct choice, not a failure state. Fast enemies may
pursue; slow or heavily armored ones may not. Dropping carried weight — including an
unconscious squad member — trades a life for speed, a real decision the player may have to
make under pressure. A full squad rout leaves the unconscious behind: the worst-case
outcome of being badly outmatched.

## Open Design Questions Worth Tracking

- Exact shield mechanics (damage reduction vs. full block, active vs. passive, stamina
  cost) are undesigned — a key piece of the ranged-defense constraint that needs a full
  pass.
- Whether mounted combat exists depends on setting.
- Whether characters (player's or enemy's) can break and flee from overwhelming odds
  without an explicit order is an open design question — tracked in
  `ai-and-behavior.md` as well, and it needs one answer covering both sides.
- Siege/base-defense combat (see `base-building.md`) and settlement-assault participation
  (see `factions-and-world-state.md`) use this same resolution model but carry additional
  context (fortifications, chokepoints, alarm/breach rules) not fully covered here.
