# Characters and Squads

## Purpose

Characters are the player's primary instrument in the world — they fight, sneak, steal,
trade, build, and research. They are individuals with distinct skill histories, physical
conditions, and wage expectations, never interchangeable units. The squad's composition,
capability, and cost is the central ongoing management challenge of the game (see
**Squad Identity** in `vision-and-pillars.md`).

## Lineage

Every character belongs to a **lineage** — a deliberately neutral term covering race,
species, or ethnic variant depending on setting, scaling from subtle human regional
variants (low fantasy) up to fully distinct non-human or alien species (high
fantasy/sci-fi). Lineage is never a class or career — it shapes a character's starting
point and skill ceiling, it never locks a path; any lineage can develop any skill through
practice. Effects are deliberately modest: small attribute modifiers (no lineage should be
strictly better than another), small skill-ceiling shifts, one or two fixed innate traits,
and visible physical characteristics that can draw social attention independent of formal
faction standing — a rare or foreign-looking lineage in an intolerant region attracts
suspicion even from a faction the player is otherwise in good standing with.

Lineage also shifts **growth rate**, distinct from the ceiling shift above: a given lineage
may advance certain skills or attributes faster than others do (e.g. naturally quicker to
develop a combat skill, slower on a crafting one), on top of any starting-point difference.
Like every other lineage effect, this stays subject to the no-lineage-is-strictly-better
constraint — rate advantages on some skills should come paired with disadvantages on
others, not a lineage that simply learns everything faster.

Factions range from mono-lineage (may distrust, refuse to trade with, or actively
persecute other lineages) to cosmopolitan (recruit and trade freely across all of them);
some high-value recruits are lineage-gated. A faction's lineage stance is static, authored
identity, consistent every playthrough — learnable, exploitable player knowledge.

## Attributes vs. Skills

**Skill determines competence. Attributes determine ceiling and rate.** Attributes
(Strength, Endurance, Agility, Perception, Intelligence, Willpower, Charisma) drift slowly
through sustained behavior — roughly a fifth the rate of skills — and set the ceiling and
modifier a skill can draw on; they are never the primary measure of ability. Skills are
the fast-moving, foreground measure of competence, and they only improve through use —
no skill points, no level-up screen, no shortcuts.

Each meaningful action resolves roughly as skill (~70% of the outcome), a primary attribute
(~20%), and a secondary attribute (~10%) — design proportions, not exposed formulas. A
high-skill, low-attribute character hits reliably but unremarkably; a low-skill,
high-attribute character connects rarely but hits hard when they do; a character strong on
both is the genuinely dangerous one. This skill-first principle is what prevents
attribute-stacking from substituting for practice. Actions are either **contested** (two
characters' scores compared directly — combat, a resisted negotiation) or **threshold**
(a fixed difficulty to clear — picking a specific lock tier, researching a node).

Permanent injury is the primary source of attribute loss (partially reversible with a good
prosthetic); poor conditions — malnutrition, exhaustion — cause temporary, recoverable
suppression instead.

## Recruitment, Wages, and Morale

Every character joins the squad through recruitment as described here, **except the
starting squad**, which the player creates directly at new-game setup instead (see
`tutorial-and-scenario-start.md`'s Starting Squad Creation) — that's the one point in the
campaign where a character is player-authored rather than found, and even there, creation
is scoped to lineage/cosmetic choice only, not stats or skills (every fresh squad starts
from the same common baseline). Recruited characters are never authored this way, but their
surface cosmetics can be adjusted after they join — see `character-creation.md` for the full
fixed-vs-adjustable split.

Recruits are found in settlements, faction-gated locations, criminal networks, slave
markets, or out in the field. Joining costs an upfront hire fee with a skill-based floor
(no charming a free recruit) plus an ongoing wage that scales with current skill level and
a small personality-driven variance. A character whose skills have grown meaningfully will
eventually request a renegotiation the player can accept, counter, or refuse; missed wages
lead to an unhappy state and eventual departure — possibly taking equipment, possibly
damaging the player's recruiting reputation in that region. A liberated slave joins without
a hire fee (already paid for at purchase) at a low but normally-scaling wage.

**Morale** is a collective condition — shaped by recent wins/losses, timely pay, character
deaths, living conditions at base, and the squad leader's skill — that doesn't force
departures directly but widens the conditions under which they happen. A meaningful design
constraint: morale collapse must not be able to cascade into losing the whole squad at
once — a floor (one or two deeply loyal characters who won't leave short of total wipe) is
needed to avoid a soft-lock.

## Injuries

Minor wounds heal with time; major wounds need treatment or worsen; limb damage impairs
function; lost limbs are permanent without a prosthetic, whose quality determines how much
function is restored. Being knocked unconscious opens a window where a character can be
captured, looted, or killed if not recovered in time (see `combat.md`). Injuries treated
but not fully healed can leave small permanent penalties — deliberately, so long-serving
veterans carry a real physical history alongside their high skills.

## Stealth and Illicit Operations

A deliberate design goal underlies this whole section: make theft genuinely rewarding for
players who invest in it, while explicitly closing off the runaway exploit that let a
skilled Kenshi thief empty an entire shop overnight and fence it all locally, skipping the
economic progression the rest of the game is built around.

- **Pickpocketing** is per-target, resolved against skill, target awareness, crowd
  density, and item size; failure is an immediately witnessed theft with immediate
  standing loss.
- **Container/environment theft** is bounded by a real time window (the unguarded gap
  between patrols), cumulative in-session suspicion (each item taken raises the odds of
  detection on the next one), and encumbrance (stolen weight slows the thief and makes
  them noisier) — together these make clearing a location in one session structurally
  impossible.
- **Security escalation** compounds with repeated incidents (extra guard → better locks →
  full lockdown/elite guards) and decays slowly if the location is left alone — repeatedly
  hitting the same target eventually stops paying off.
- **Stolen goods recognition**: items are recognizable as stolen within their origin
  faction's territory. They can be sold at a discount through criminal fences (better with
  criminal-network standing), at full price to sufficiently distant factions unaware of
  the theft, or — after enough in-game time — once the "stolen" flag quietly expires, a
  safety valve rather than a primary strategy.
- **Inventory transfer requires physical proximity** — items can't be handed off to a
  "mule" waiting outside; the thief has to physically exit a guarded location carrying
  what they took, encumbrance and all.
- **Kidnapping** requires subduing a target without killing them, then physically
  carrying the unconscious body to a destination the whole way slow and conspicuous.
- **Save-scumming** is not blocked outright (no forced ironman), but is made unrewarding:
  a retry under identical conditions produces an identical result — only a genuine change
  (skill, elapsed time, a different approach) yields a different outcome — and failure is
  designed to be a recoverable consequence rather than a campaign-ender, which removes
  most of the incentive to reload in the first place.

## Squad Management

The player manages the squad through assignment (patrol vs. base vs. labor), equipping
each member for their role, broad formation/behavior orders, and role specialization
(the designated thief takes illicit jobs, the designated laborer works the forge). The
specific work a character is queued to carry out, and the order they work through it, is
owned by `orders-and-jobs.md`; what they do with no instruction at all is
`ai-and-behavior.md`. There is no hard squad size cap — wages create the
real, self-regulating economic ceiling (see
`economy.md`).

## Squad Divisions

The roster isn't managed or moved as one indivisible block. The player can organize
characters into multiple named divisions — a scouting party, a trade caravan, a base
garrison, a raiding force — each capable of acting independently and simultaneously in
different parts of the world, Kenshi-style. Division membership is fluid: characters can
be reassigned between divisions freely (subject to travel time to physically regroup if
they aren't already together), and a division can be as small as one character.

Divisions are an organization/control layer, not a second economic system — wages, hire
fees, and morale (see **Recruitment, Wages, and Morale** above) still apply at the level
of the individual character and the roster as a whole; splitting into divisions doesn't
create separate budgets or separate morale pools. See `player-interface.md` for how the
player views and switches between divisions on screen.

## Open Design Questions Worth Tracking

- The specific lineage roster, attribute modifiers, and traits are deferred until setting
  is chosen — the framework above is meant to be stable regardless of what fills it in.
  Concrete skill examples floated in discussion (e.g. Blacksmithing, One-Handed Weapons,
  Marksmanship, Lockpicking) are directional, not a committed roster — a full skill list
  likely needs to be assembled per-domain (`combat.md`, `tech-and-crafting.md`, and the
  Stealth section here) rather than enumerated centrally in one place.
- Personality/trait depth (e.g., a character who flees without orders) isn't fully
  designed.
- The skill-to-outcome proportions (70/20/10) are a stable design constraint; the actual
  roll resolution math and coefficients are iterative implementation work, not fixed by
  this document.
- Whether physically separated divisions still count as one roster for morale purposes
  even when far apart with no in-person contact isn't tested — current stance is yes (see
  **Squad Divisions**), but this may need revisiting once long-distance division play and
  co-op are actually testable.
