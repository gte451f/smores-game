# Player Experience

## Purpose

Player experience systems sit between the player and the game's content — how they learn
the game, tune it to their preferences, and are accommodated regardless of need or
hardware. None of this touches simulation outcomes, but it determines whether a player
can engage with the simulation at all.

## Onboarding Philosophy

This game does not hold the player's hand, consistent with the punishing, systems-mastery
design described in `vision-and-pillars.md`. Onboarding teaches **controls and mechanics
only — never strategy**; teaching strategy would undercut the core fantasy of learning a
hostile world through lived experience. The concrete bar: a new player should be able to
move their squad, interact with an NPC, and understand a tooltip within the first ten
minutes. What to do with that knowledge is the game's challenge to pose, not the
tutorial's job to answer.

## Codex

An in-game reference the player builds through play rather than starting with — entries
across mechanics, factions, locations, items/equipment, characters, and tech/crafting
unlock as the player actually encounters or discovers the related content. It records
facts the player has personally learned; it is explicitly **not** a quest log or a
strategy guide — analysis and strategy stay the player's job, always.

## Tooltips

Every stat, attribute, skill, UI element, and item has one, describing what a thing *is*
and *does* — never how to optimize it — and always accessible in-game, so a player never
has to alt-tab to a wiki to understand a number. Depth is configurable: a brief one-line
mode and an extended full-description mode, defaulting to extended for new players.

## Settings, as a Baseline Expectation

Full graphics, audio, control, and UI configurability, plus gameplay-level knobs —
notably an autosave interval, an optional ironman toggle (see `save-system.md`), and a
time-flow control offering discrete speed tiers rather than a continuous slider: pause,
1/3x, 1/2x, 3/4x, 1x (normal), 2x, 4x, and 8x. Available anywhere, at any time, with no
mode-specific exemptions — see **Accessibility** below and `base-building.md`'s Building
Mode for why that matters.

## Accessibility

Accessibility is a baseline requirement, never a stretch goal: colorblind modes, a
high-contrast UI mode, UI/font scaling independent of resolution, subtitles for any
information-carrying audio, full key/button rebinding with no action locked to a specific
input, a toggle alternative for anything that would otherwise require holding a key,
sensitivity and dead-zone tuning, pause-anywhere (the simulation should never demand
real-time reflexes to process), the full 1/3x–8x speed range described under **Settings**
above, and notifications that persist until dismissed rather than auto-expiring — the
persistence is what lets the player safely run at high speed without missing something
that needed attention, rather than the game forcing a slowdown on their behalf.

## Localization

A day-one architectural commitment, not a post-launch addition — retrofitting
localization onto a large codebase is expensive; building it in from the start isn't.
Every player-facing string is externalized from day one, fonts must support the character
sets of any targeted market (including CJK or RTL scripts), and UI layouts must tolerate
the 30–40% length variance that languages like German or Russian introduce relative to
English.

## Open Design Questions Worth Tracking

- The tutorial's scenario framing (what situation the player starts in, what the first
  objective is) isn't designed yet.
- Voice acting, if added, would add localization and subtitle requirements beyond the
  current text-only assumption.
- Full controller-native UI is a later pass — keyboard and mouse is the initial target.
- A proper accessibility audit with players who have relevant needs should happen before
  ship; the requirements above are a baseline, not a complete list.
- The exact triggers that unlock a Codex entry (first encounter vs. first interaction vs.
  first trade) need a design pass.
- Whether any tiers above 1x are restricted in specific contexts (e.g. capped during an
  active raid) isn't decided — current lean is no restriction, relying on persistent
  notifications instead, but this needs playtesting once combat exists.
