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
(the designated thief takes illicit jobs, the designated laborer works the forge). There
is no hard squad size cap — wages create the real, self-regulating economic ceiling (see
`economy.md`).

## Open Design Questions Worth Tracking

- The specific lineage roster, attribute modifiers, and traits are deferred until setting
  is chosen — the framework above is meant to be stable regardless of what fills it in.
- Whether a player-created starting character (vs. recruited-only) is supported isn't
  decided.
- Personality/trait depth (e.g., a character who flees without orders) isn't fully
  designed.
- The skill-to-outcome proportions (70/20/10) are a stable design constraint; the actual
  roll resolution math and coefficients are iterative implementation work, not fixed by
  this document.
