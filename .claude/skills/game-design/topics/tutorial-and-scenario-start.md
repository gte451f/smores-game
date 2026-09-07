# Tutorial and Scenario Start

## Purpose

States the concrete starting situation a new campaign drops the player into, and resolves
the tutorial question definitively rather than leaving it open. This is the logical
endpoint of `player-experience.md`'s Onboarding Philosophy (mechanics-only teaching, no
hand-holding): if strategy is never taught and only controls/mechanics are, the opening
scenario itself can carry that teaching without needing a separate tutorial structure at
all.

## No Tutorial

There is no tutorial in the traditional sense — no dedicated tutorial mode, no scripted
walkthrough level, no forced "click here to continue" sequence gating the start of the
game. The player is dropped directly into the game with their starting squad; whatever
mechanics-only teaching happens (per `player-experience.md`) happens through ambient
tooltips and Codex discovery during real play, not through a separate teaching vehicle. This
is a pillar-level choice, not a resource-saving shortcut for a solo developer: a scripted
tutorial would contradict the "learn a hostile world through lived experience" fantasy in
`vision-and-pillars.md` — teaching through a safe, artificial first level is the opposite of
that fantasy, not a neutral convenience toward it.

## Starter Scenarios

Rather than one fixed starting condition, the player chooses among multiple **starter
scenarios** at new-game setup (see `main-menu-and-meta-flow.md` for where this sits in the
overall flow). This resolves this topic's original open question ("single fixed starting
scenario vs. player-chosen starting conditions") in favor of player choice.

Each scenario is built around a **squad backstory** — a fictional frame explaining who the
starting squad is, why they're together, and why the squad is the size it is (e.g. "you and
a friend escaped a slaver camp together" explains, and justifies, a two-member start).

**Backstory is cosmetic only — it has no mechanical effect of any kind.** It doesn't grant
different starting stats or skills, and it doesn't grant different starting faction
standing; every fresh squad starts from the identical stats/skills baseline described in
**Starting Squad Creation** below, regardless of which scenario/backstory the player picked.
Backstory's job is entirely presentational: it explains the squad's size and relationships,
and supplies the flavor around naming/appearance choices made during creation. If faction
relations or starting resources ever end up varying by scenario, that would be a deliberate
addition on top of this baseline, not implied by backstory itself — treat it as undecided
unless explicitly designed later.

## Starting Squad Creation

Each starter scenario implies a fixed starting squad size (its backstory sets that count —
the escaped-slaves example above implies exactly two). For each slot the scenario defines,
**the player creates that character directly** rather than receiving a pre-rolled one — but
creation is narrower than a full stat-builder:

- The player chooses each member's **lineage** (`characters-and-squads.md`'s race/species
  framework) and, presumably, cosmetic details (name, appearance) — the flavor layer that
  pairs with the scenario's backstory.
- **Every fresh squad member starts from the same common stats/skills baseline**, before
  lineage's own small, fixed modifiers apply (per `characters-and-squads.md`'s Lineage
  section) — there is no manual attribute allocation and no player-chosen starting skills.
  Growth from that shared baseline happens entirely through play, consistent with
  `characters-and-squads.md`'s "no skill points, ever" principle — starting squad creation
  is explicitly not a backdoor around that rule.

This resolves `characters-and-squads.md`'s open question of whether player-created starting
characters are supported — they are, but scoped specifically to the starting squad at
new-game setup, and scoped further to lineage/cosmetic choice only, not stat/skill choice.
Every character recruited later in the campaign remains found/recruited per that file's
Recruitment, Wages, and Morale section, not player-authored.

## Open Design Questions Worth Tracking

- How many starter scenarios ship, and how much they actually vary beyond squad
  size/backstory flavor (e.g. different starting location or resources) isn't decided.
- The concrete first objective and starting resources, beyond "a chosen scenario," aren't
  designed yet.
- How this hands off from new-game setup (`main-menu-and-meta-flow.md`).
