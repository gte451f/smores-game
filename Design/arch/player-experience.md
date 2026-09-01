# Player Experience

> Non-gameplay arch doc. See `game-pillars.md` for the difficulty philosophy and modding requirements. See `content-and-release.md` for localization release considerations.

---

## Purpose

Player experience systems are the layer between the player and the game's content — how they learn the game, how they configure it to their preferences, and how the game accommodates different needs and hardware. These systems do not affect simulation outcomes but significantly affect whether players can engage with the simulation at all.

---

## In-Game Help and Onboarding

### Design Philosophy
This game does not hold the player's hand. The design pillars explicitly embrace a punishing, systems-mastery experience. Onboarding exists to teach *controls and mechanics*, not *strategy*. Teaching strategy would undercut the core fantasy of learning a hostile world through experience.

The onboarding goal: a new player can move their squad, interact with NPCs, and understand what a tooltip is saying within the first 10 minutes. What to do with that knowledge is the game's challenge, not the tutorial's lesson.

### Tutorial
An optional, skippable tutorial scenario introduces:
- Camera movement and squad selection
- Issuing move orders and target priority
- The basic interaction model with NPCs (trade, recruit, talk)
- The inventory and equipment screen
- One combat encounter with explicit framing of the automated combat model ("your squad fights on their own — direct them, don't control individual attacks")

The tutorial does not teach the economy, faction relationships, base building, or skill progression. These are discoverable systems. The tutorial's job is controls only.

Players who skip the tutorial or finish it and are later confused can revisit any tutorial topic through the Codex.

### Codex (In-Game Encyclopedia)
The Codex is an in-game reference that the player builds through play. Entries unlock as the player encounters or discovers related content — they do not start with a full encyclopedia.

**Entry categories:**
- **Mechanics** — how systems work (combat resolution, skill progression, economy, faction standing)
- **Factions** — discovered factions, their known relationships, and what the player has learned about them through interaction or reputation
- **Locations** — discovered settlements, outposts, and points of interest, with player notes capability
- **Items and Equipment** — items the player has encountered, with stats and crafting origins if known
- **Characters** — named NPCs the player has interacted with, their faction affiliation, and known status (alive/dead)
- **Tech and Crafting** — unlocked or discovered recipes and research nodes

The Codex is not a quest log or a strategy guide. It records facts the player has encountered. Analysis and strategy remain the player's job.

### Tooltips
Every stat, attribute, skill, UI element, and item in the game has a tooltip. Tooltips describe *what a thing is and what it does*, not how to optimize it. They are always present and accessible — the player should never have to leave the game to look up what a number means.

Tooltip depth is configurable: a "brief" mode shows a one-line summary; an "extended" mode shows the full mechanical description. Extended mode is the default for new players.

---

## Settings and Options

### Graphics
- Resolution, display mode (fullscreen / borderless / windowed), refresh rate
- Quality presets (Low / Medium / High / Ultra) and individual overrides (shadow quality, texture quality, draw distance, effects quality, ambient occlusion)
- V-sync, frame rate cap
- Field of view slider (camera zoom range, separate from gameplay zoom limits)

### Audio
- Master, music, ambient, effects, and UI volume sliders (independent)
- Mute background audio when window is not in focus toggle

### Controls
- Full key rebinding for keyboard and mouse
- Controller support with full rebinding
- Mouse sensitivity (horizontal and vertical, independent)
- Toggle vs. hold for contextual actions (e.g., hold to drag-select vs. toggle selection mode)

### UI
- UI scale slider (for high-resolution displays or accessibility needs)
- HUD element visibility toggles (hide individual HUD components for screenshot or immersive play)
- Font size (independent of UI scale)
- Minimap size and opacity
- Notification verbosity (all events / important events only / critical only)

### Gameplay
- Autosave interval
- Ironman mode toggle (one save slot, autosave only — see `save-system.md`)
- Time compression rate slider (within a designed range — the 1x default is 2 real hours per in-game day; a faster option compresses further)
- Combat log verbosity

---

## Accessibility

Accessibility is not optional. The following are baseline requirements, not stretch goals:

### Visual
- **Colorblind modes**: Protanopia, Deuteranopia, Tritanopia color correction modes for UI elements and world indicators
- **High contrast UI mode**: increases contrast on text, icons, and critical HUD elements
- **UI and font scaling**: independent of resolution scaling; supports both small and large displays
- **Subtitles**: any in-game spoken dialogue or audio cue that carries narrative or gameplay information must have a text equivalent

### Control
- **Full key and button rebinding**: no action is locked to a specific input
- **Hold vs. toggle**: any action that requires holding a key/button should have a toggle alternative
- **Mouse sensitivity and dead zone tuning** for imprecise input devices

### Cognitive
- **Pause anywhere**: the game can always be paused. Real-time simulation does not require real-time response from the player.
- **Speed controls**: time compression is adjustable; players can slow the simulation to a crawl if needed to process complex situations
- **Notification persistence**: important notifications should persist until dismissed, not auto-expire

---

## Localization

Localization is a day-one architectural requirement, not a post-launch addition. Retrofitting a large codebase for localization is expensive; building it in from the start is not.

**Requirements:**
- All player-facing strings are externalized into a localization table — no hardcoded text in C++ or Blueprint
- String IDs reference the localization table; the table ships in a format that translators can edit without code access
- Fonts must support the character sets of target languages (including CJK and RTL scripts if those markets are targeted)
- UI layouts must accommodate string length variation — text in German or Russian can run 30–40% longer than English equivalents
- Dates, numbers, and currency formats follow locale conventions

**Target languages at launch:** TBD. English is the development language. Additional languages are a business and budget decision outside the scope of this doc.

---

## Known Gaps / Future Notes

- **Tutorial scenario specifics**: The tutorial's scenario framing (what situation the player is in, what the first objective is) is not designed. It may share structure with a "tutorial" starting scenario option.
- **Voice acting**: If the game includes voiced NPCs or narration, additional localization and subtitle requirements apply. Currently assumed text-only.
- **Controller UI**: Full controller-native UI (navigating menus without a mouse) is a significant design and implementation effort. The initial target is keyboard and mouse; controller support is a later pass.
- **Accessibility audit**: A proper accessibility audit by players with relevant needs should occur before ship. The above requirements are baseline, not comprehensive.
- **Codex unlock triggers**: The exact conditions that unlock Codex entries (first encounter, first interaction, first trade, first kill) need a design pass during systems implementation.
