# Localization

## Purpose

Localization is treated as a **day-one architectural commitment**, not a post-launch
addition, in direct support of the "sell as widely as possible first" storefront priority
in `input-and-platforms.md` — reaching non-English-speaking markets is part of selling
widely, not a separate later initiative. The reasoning is the same asymmetric-cost logic as
`multiplayer-and-content.md`'s multiplayer stance: retrofitting localization onto a large
codebase of hardcoded, scattered strings is expensive; building the architecture to avoid
that from day one is comparatively cheap, even though the actual translated-language list
can (and should) grow gradually.

## Day-One Architectural Commitment

- **Every player-facing string is externalized from day one** — no hardcoded UI text
  anywhere in code, data, or Blueprints. This is the single habit that makes everything else
  possible, and it's cheap to maintain from the start and expensive to retrofit once a large
  surface of UI has grown assuming English strings live inline.
- **Font assets must support the character sets of any targeted market** — including CJK and
  Cyrillic glyph coverage and RTL (right-to-left) scripts — decided early, since swapping in
  font support after UI is built around a Latin-only assumption is a real rework, not a
  content swap.
- **UI layouts must tolerate significant length variance** — roughly 30–40% text expansion
  for languages like German or Russian relative to English — and, for RTL languages, layout
  mirroring, not just mirrored text direction. Fixed-width UI built assuming English string
  lengths is exactly the kind of thing that's cheap to avoid now and expensive to fix later.

## Solo-Dev Scope Reality: Architecture Now, Language List Later

The architecture above is what has to happen on day one; the actual list of shipped
languages does not. Professional translation is the real bottleneck for a one-person team —
not the code — so the realistic path is: keep the game translation-ready from the start, but
let the actual set of shipped languages grow after launch rather than trying to ship many
languages at once. This mirrors the modding stance in `multiplayer-and-content.md`: leaning
on community-contributed translation (the way mods extend content) is a plausible way to
reach more languages without personally funding professional translation for all of them,
though this isn't a committed plan yet — see open questions.

## Open Design Questions Worth Tracking

- The launch language list isn't decided — likely starts small (e.g. English plus one or two
  major markets) and grows post-launch, but the specific list and sequencing aren't chosen.
- Whether community/crowd-sourced translation tooling is supported (vs. contractor-only
  translation) isn't decided.
- How far RTL support goes — mirrored text only, vs. fully mirrored UI layouts — isn't
  decided.
- Voice acting, if ever added, would introduce localization and subtitle requirements well
  beyond the current text-only assumption (see `player-experience.md`) and should be
  re-evaluated against this topic if it comes up.
