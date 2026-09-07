---
name: player-facing
description: Reference smores' player-visible behavior, controls, game modes, and terminology — the "player surface" independent of implementation. Use when documenting or reasoning about what a player sees/does/presses, or updating these docs after a controls/input/UI-visible change. Skip for C++/Blueprint implementation detail — see the game-systems skill and Source/smores/ for that.
---

# Player-Facing

This skill documents the game's **player surface** — controls, visible game modes, and
the terminology a player (or a designer talking about the game) would use — independent
of how any of it is implemented in C++ or Blueprint.

## Topics

| Topic | Covers |
|---|---|
| [`topics/controls.md`](topics/controls.md) | Player-visible controls at the design level (camera, selection, movement, touch) |
| [`topics/game-modes.md`](topics/game-modes.md) | Visible game modes and what they let the player do |
| [`topics/terminology.md`](topics/terminology.md) | Player-surface terms (Controlled Units, Drag Selection, Interaction Radius, Lead Unit, Player Surface) |

## Working in this skill

- **Player surface, not implementation.** Exact input bindings live in Enhanced Input
  assets and Blueprint defaults; class/method names and implementation detail belong in
  the `game-systems` skill instead.
- **Keep it current.** Update the relevant topic file whenever a control, visible game
  mode, or player-facing term changes — don't wait to be asked.
- **New game mode or control surface, new content.** Add a topic (or a section to
  `game-modes.md`) as new modes are introduced.
