# Character Creation and Appearance

## Purpose

Character creation is where the player authors the small number of characters the game
lets them author, and where a character's visual identity is established. It is
deliberately a **flavor layer, not a power layer** — the player picks who their people
*are*, never how good they are. This is the design guarantee that keeps
`characters-and-squads.md`'s "skills only improve through use, no skill points, ever"
principle intact: if creation could set stats, it would be a level-up screen wearing a
different hat.

Appearance also carries real gameplay weight through lineage: a character's visible
physical characteristics can draw social attention independent of faction standing (see
`characters-and-squads.md`'s Lineage section), so what the player picks at creation has
consequences they'll live with for the campaign.

## When Full Creation Happens

**Once per campaign, at new-game setup.** The chosen starter scenario fixes the starting
squad size, and the player creates each of those characters directly (see
`tutorial-and-scenario-start.md`'s Starting Squad Creation). Every character acquired
afterward is found and recruited, never authored — the player does not design new
characters mid-campaign.

## What the Player Chooses

At full creation, per character:

- **Lineage** — the race/species framework in `characters-and-squads.md`. Carries small
  attribute modifiers, skill-ceiling and growth-rate shifts, one or two innate traits, and
  faction-attitude consequences. The only choice at creation with mechanical weight, and
  it's a tradeoff, never an upgrade — no lineage is strictly better than another.
- **Gender** — a fixed identity property of the character, chosen at creation.
- **Cosmetics** — name, hair, complexion, body size/height, and whatever other visual
  detail the lineage supports.

What the player explicitly does **not** choose: attributes, skills, traits beyond the
lineage's innate ones, starting equipment, or a class/career of any kind. Every fresh
squad member starts from the same common baseline before lineage modifiers apply.

## Mid-Campaign Appearance Editing

A recruited character's appearance is **partly** adjustable after they join. This is
expected to be uncommon in practice, but players will reasonably want to tidy up a
recruit who joined looking nothing like the role they're about to fill.

**Immutable after creation** — lineage and gender. These are who the character *is*.
Lineage additionally has mechanical and social consequences, so letting it be edited would
turn it into a respec; gender is held fixed alongside it for the same identity reason.

**Adjustable** — hair, complexion, and the other surface-level cosmetic detail. The
framing is grooming and upkeep, not redesign: the player is cleaning a character up, not
rebuilding them.

**Adjustable, tentatively** — body size and height. This one is provisional because it is
the least clearly cosmetic of the set. It is only acceptable while build carries **zero**
mechanical weight — no attribute effect, no carry-capacity effect, no reach or hitbox
effect. The moment a bigger character hits harder or carries more, letting the player
adjust build mid-campaign becomes exactly the stat-allocation backdoor this whole system
exists to prevent, and it must become immutable instead.

This also applies to the player's own starting squad after the campaign begins — a
character created at setup is subject to the same fixed/adjustable split from then on.
Nothing about creation is re-openable in full.

## Interface Intent

The mid-campaign edit is intended to be the **same screen** the player already learned at
new-game setup, in a reduced mode with the immutable choices shown but locked, rather than
a second, differently-shaped UI. The player should recognize it immediately.

Whether editing is freely available at any time or gated behind something — a cost, a
settlement service, being at the player's own base — is undecided. A gate is worth
considering purely because free unlimited fiddling makes appearance feel weightless, and
appearance is supposed to be a thing the player commits to.

## Visual Fidelity and Readability

Characters are viewed primarily from a pulled-back tactical camera with a large number of
them potentially on screen at once. That sets the art direction for creation: what matters
is **readability at distance** — silhouette, proportion, and color — not facial micro-detail
the player will never see. A lineage should be identifiable at a glance across a battlefield;
a character should be distinguishable from their squadmates at a glance.

This rules out photorealistic human character pipelines (MetaHuman and equivalents) as the
foundation for squad characters, on two design grounds rather than technical ones: they only
produce humans, which cannot express a lineage roster that scales up to fully non-human
species, and they invest all their fidelity in exactly the close-up detail this camera
throws away. Photoreal humans may still have a place in presentation material or a single
close-up story character — not in the squad.

## Open Design Questions Worth Tracking

- **What gender actually affects, if anything.** It is currently established as a fixed
  identity property with no stated mechanical or social consequence. Whether lineages or
  factions have gendered attitudes — and whether that's a direction this game wants at all
  — is undecided.
- **Whether body size/height stays cosmetic.** See the warning above. This gates whether
  build remains mid-campaign adjustable.
- **Whether appearance editing is gated** by cost, location, or a settlement service.
- **How much cosmetic variety each lineage needs** to keep a squad of a dozen-plus
  characters visually distinct, especially within a single lineage.
- **Whether recruits generated by the world draw from the same cosmetic option set** the
  player sees at creation, or a wider one — a recruit who looks like something the player
  couldn't have made reinforces that they came from the world, but risks looking like a
  bug.
- **Whether the player names recruits.** Distinct from appearance: a found character
  arriving with their own name is a stronger signal that they're a person with a history,
  but players will want to nickname their squad. Undecided.
- The concrete lineage roster remains deferred until setting is chosen (see
  `characters-and-squads.md`).
