---
name: game-systems
description: Reference smores' implemented gameplay systems — what they do, where they live in C++/Blueprint, and their known gaps. Use before changing camera/selection, unit commands, or inventory behavior, or when documenting a newly implemented gameplay system. This is implementation-level detail (unlike the game-design skill); pair with reading the actual source files before editing.
---

# Game Systems

This skill documents smores' **implemented** gameplay systems: what each system does, the
player-facing rules it enforces, where it lives in C++ and Blueprint, and its known gaps.
Unlike the `game-design` skill (non-technical, desired end states), these topics describe
current, real behavior and point at concrete classes and assets.

## Topics

| Topic | Covers |
|---|---|
| [`topics/strategy-camera-and-selection.md`](topics/strategy-camera-and-selection.md) | RTS-style camera pawn, mouse/touch camera movement and zoom, unit selection, drag selection, double-tap |
| [`topics/strategy-unit-commands.md`](topics/strategy-unit-commands.md) | Move commands, lead-unit/formation targeting, movement completion, interaction triggering |
| [`topics/inventory.md`](topics/inventory.md) | `UInventoryComponent` item storage, `UInventoryWidget` display, HUD-managed open/close/toggle |

## Working in this skill

- **Describe current implementation, not aspiration.** Put future ideas under a "Known
  Gaps" section in the relevant topic, not into the main description.
- **Update when behavior changes.** Any change to gameplay behavior, controls, camera
  behavior, selection rules, or UI-visible rules should be reflected here in the same
  change, alongside the code.
- **Prefer concrete references** — actual C++ classes, Blueprint assets, maps, and input
  assets — over vague descriptions. Don't document binary asset internals unless
  confirmed in the editor or via source-controlled asset metadata.
- **Document both layers** when a system has player-facing behavior and C++
  implementation — see the existing topics for the expected split (Purpose / Player
  Surface / Core Rules / C++ Implementation / Blueprint-Asset Dependencies / Extension
  Points / Known Gaps).
- **New system, new topic file.** Follow the same structure as the existing three.
