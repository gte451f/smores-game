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
1/3x, 1/2x, 3/4x, 1x (normal), 2x, 4x, and 8x. In single-player these are available
anywhere, at any time, with no mode-specific exemptions — see **Accessibility** below and
`base-building.md`'s Building Mode for why that matters.

**In multiplayer the simulation runs at 1x and no player may retime it, decided.** A shared
world where any of eight players can speed it up, slow it down or stop it is unstable for
everyone else, and there is no version of "whoever clicked last wins" that reads as fair.
Time dilation is a single-player feature.

**Pause is inside that lock, decided** — it is a time change like any other, and a shared
world that one of eight players can stop is the same instability as one they can speed up.
There is no host-only or consent-based exemption.

This does cost something real, and it is recorded here rather than hidden: the
pause-anywhere commitment under **Accessibility** below is a **single-player guarantee**.
Pause is the one tier with an accessibility claim on it where the rest carry only a
convenience claim, so co-op asks more of a player than single-player does. That is accepted
as the price of a stable shared world — co-op is opt-in, single-player is the mode the
accessibility baseline is written against, and no other accessibility commitment is
weakened by it.

The lock keys off whether the session is networked at all, not off how many players are
connected. A host sitting alone in a co-op session cannot pause either. That is deliberate:
availability that changed the moment someone joined would be worse than availability that
never existed.

## Accessibility

Accessibility is a baseline requirement, never a stretch goal: colorblind modes, a
high-contrast UI mode, UI/font scaling independent of resolution, subtitles for any
information-carrying audio, full key/button rebinding with no action locked to a specific
input, a toggle alternative for anything that would otherwise require holding a key,
sensitivity and dead-zone tuning, pause-anywhere (the simulation should never demand
real-time reflexes to process), the full 1/3x–8x speed range and pause-anywhere described under
**Settings** above (both single-player; see the multiplayer lock recorded there), and notifications that persist until dismissed rather than auto-expiring — the
persistence is what lets the player safely run at high speed without missing something
that needed attention, rather than the game forcing a slowdown on their behalf.

## Localization

Covered in its own topic, `localization.md` — a day-one architectural commitment (string
externalization, font/script support, layout tolerance for length variance) tied to the
"sell as widely as possible" storefront priority in `input-and-platforms.md`.

## Open Design Questions Worth Tracking

- The tutorial's scenario framing (what situation the player starts in, what the first
  objective is) isn't designed yet.
- Voice acting, if added, would add localization and subtitle requirements beyond the
  current text-only assumption — see the open question in `localization.md`.
- Full controller-native UI is a later pass — keyboard and mouse is the initial target.
- A proper accessibility audit with players who have relevant needs should happen before
  ship; the requirements above are a baseline, not a complete list.
- The exact triggers that unlock a Codex entry (first encounter vs. first interaction vs.
  first trade) need a design pass.
- Whether any tiers above 1x are restricted in specific contexts (e.g. capped during an
  active raid) isn't decided — current lean is no restriction, relying on persistent
  notifications instead, but this needs playtesting once combat exists.
