# Main Menu and Meta-Flow

**Status:** first pass — title screen structure decided; new-game setup, load/save UX,
credits, and announcements still undesigned.

The title screen is the player's entry point outside an active campaign, with five
entries: **Continue** (resume the most recent campaign), **Load Game** (pick a save from a
list), **Start New Game**, **Options**, and **Exit**. A **Mods** entry is deliberately left
off for now — mod load order, install, and management aren't designed yet (see
`multiplayer-and-content.md`'s mod/DLC relationship), so it doesn't belong on the title
screen until that design exists.

**Options** opens a dedicated interface, not a flat list, organized into four categories:
Keybindings, Audio, Graphics, and Game. This is a structural decision, not a settings-
content one — see `player-experience.md` for what belongs inside each category.

Until `save-system.md`'s save/load system and this topic's own new-game setup (scenario/
squad selection, difficulty/modifier picks — see `difficulty-and-modifiers.md`) exist,
Continue, Load Game, and Start New Game are three doors to the same undifferentiated
campaign start rather than genuinely distinct flows. That's a placeholder state, not the
intended end state — each should eventually do what its name says.

## Open Design Questions Worth Tracking

- Whether announcements are purely in-game text or imply any live backend.
- How new-game setup and the tutorial/scenario start (`tutorial-and-scenario-start.md`)
  hand off to each other.
- What a save-slot list in Load Game should show per entry (timestamp, campaign day,
  squad snapshot, etc.) — depends on `save-system.md`'s save contents.
- Full mod load-order/install/management design, which gates adding a Mods entry here.
