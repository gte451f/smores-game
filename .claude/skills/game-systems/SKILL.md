---
name: game-systems
description: Reference smores' implemented gameplay systems — what they do, where they live in C++/Blueprint, and their known gaps — including the player-visible surface (controls, game modes, terminology) for each. Use before changing camera/selection, unit commands, or inventory behavior, or when documenting a newly implemented gameplay system or control. This is implementation-level detail (unlike the game-design skill); pair with reading the actual source files before editing.
---

# Game Systems

This skill documents smores' **implemented** gameplay systems: what each system does, the
player-facing rules it enforces, where it lives in C++ and Blueprint, and its known gaps.
Unlike the `game-design` skill (non-technical, desired end states), these topics describe
current, real behavior and point at concrete classes and assets. There is no separate
"player-facing" skill — the player surface (controls, visible game modes, terminology) is
documented as part of each system topic, in its "Player Surface" section, plus the shared
`game-modes.md` and `terminology.md` topics below for content that cuts across systems.

## Topics

| Topic | Covers |
|---|---|
| [`topics/strategy-camera-and-selection.md`](topics/strategy-camera-and-selection.md) | RTS-style camera pawn, mouse/touch camera movement and zoom, unit selection, drag selection, double-tap, pawn cycling |
| [`topics/strategy-unit-commands.md`](topics/strategy-unit-commands.md) | Move commands, lead-unit/formation targeting, movement completion, interaction triggering |
| [`topics/inventory.md`](topics/inventory.md) | `UItemDefinition` shared item-type assets vs. `FInventoryItem` carried instances, `UInventoryComponent` 2D grid storage with rectangular footprints, rotation and per-holder stacking, `UInventoryWidget` drag-and-drop display, `UEquipmentComponent` worn slots and the `UEquipmentWidget` paperdoll, `AWorldItem` loose world pickups, server-side sort repack and client-side category filter, PlayerController-managed open/close/toggle for pawn/container/loot/equipment |
| [`topics/inventory-roadmap.md`](topics/inventory-roadmap.md) | *(forward-looking)* Target design for the inventory slices still ahead — a unified proximity-gated `IInventoryHolder` transfer interface (loot-dead/trade/purchase/theft), a `SmoresEconomy` wallet and pricing layer, and grid sort/filter — plus a dependency-ordered, one-slice-per-session Implementation Order with status and a Resolved Design Decisions log |
| [`topics/refusals-and-feedback.md`](topics/refusals-and-feedback.md) | The shared "why was that refused?" line — the `ESmoresRefusalReason` vocabulary in `SmoresCore`, the single place refusals are worded, the client-side vs. server-RPC routes to it, and the rule that preventing a refusal beats explaining one. **Read before making any action fail silently.** |
| [`topics/combat.md`](topics/combat.md) | `UHealthComponent` health/damage/Downed-recovery, NPC self-hunting, player-issued squad attacks, auto-retaliation |
| [`topics/input-and-keybinds.md`](topics/input-and-keybinds.md) | Every bound key and mouse button with its action asset and context, the reserved-key list for systems not built yet, the Enhanced Input wiring rule every binding must follow, and the checklist for adding one. **Check before adding any player-facing control.** |
| [`topics/game-modes.md`](topics/game-modes.md) | Visible game modes and what they let the player do |
| [`topics/terminology.md`](topics/terminology.md) | Player-surface terms used across topics (Controlled Units, Drag Selection, Interaction Radius, Lead Unit, Player Surface) |
| [`topics/multiplayer-discipline.md`](topics/multiplayer-discipline.md) | The standing server-authority rules all gameplay code is written against — no singleton-player assumptions, `HasAuthority()` gating, replication/RPCs over ad hoc sync, deciding data ownership up front, no speculative prediction, and the Linux dedicated-server constraint. **Read before writing or changing gameplay state.** |
| [`topics/unreal-module-organization.md`](topics/unreal-module-organization.md) | *(forward-looking)* Current single-module state, a proposed target module map sized against the full `game-design` scope, DLC/mod-as-Plugin guidance, and concrete triggers for when to actually split |
| [`topics/testing.md`](topics/testing.md) | How to run the automated test suite (in-editor and headless), what it covers, the standing rule for when finished work should add a test, and the conventions for writing one. **Read when finishing work that changes a counted return, an all-or-nothing operation, or a state machine.** |
| [`topics/testing-roadmap.md`](topics/testing-roadmap.md) | *(forward-looking)* Target design for automated testing — the harness choice (in-module automation tests, no new module or build plumbing), the catalogue of what's worth asserting across inventory/economy/combat/equipment, what stays in PIE permanently, and a dependency-ordered, one-slice-per-session Implementation Order with a Resolved Design Decisions log |

## Working in this skill

- **Describe current implementation, not aspiration.** Put future ideas under a "Known
  Gaps" section in the relevant topic, not into the main description. The deliberate
  exceptions are `topics/unreal-module-organization.md`, `topics/inventory-roadmap.md` and
  `topics/testing-roadmap.md`, each explicitly a forward-looking reference rather than a
  record of built behavior — all three say so up front. `topics/multiplayer-discipline.md`
  and `topics/testing.md` are a different exception: standing rules for how code gets
  written, not a description of one system. When a roadmap topic's content
  ships, move it into the paired current-implementation topic and trim the roadmap
  accordingly.
- **Update when behavior changes.** Any change to gameplay behavior, controls, camera
  behavior, selection rules, or UI-visible rules should be reflected here in the same
  change, alongside the code.
- **Consider a test when finishing gameplay logic.** `topics/testing.md` holds the standing
  rule and the run commands. A function returning a count or a "did it work" bool, an
  all-or-nothing operation, a state-machine transition, arithmetic on money/weight/quantity,
  a rule a comment calls deliberate, or a bug just fixed — each warrants a test, written in
  the same commit as the code. Anything needing a mesh, a montage, a widget, or a human's
  eyes does not; that stays in PIE.
- **Prefer concrete references** — actual C++ classes, Blueprint assets, maps, and input
  assets — over vague descriptions. Don't document binary asset internals unless
  confirmed in the editor or via source-controlled asset metadata.
- **Document both layers** when a system has player-facing behavior and C++
  implementation — see the existing topics for the expected split (Purpose / Player
  Surface / Core Rules / C++ Implementation / Blueprint-Asset Dependencies / Extension
  Points / Known Gaps).
- **Input routing**: all input uses Enhanced Input. Route input actions into small shared
  command methods (`DoSelectionCommand()`, `DoMoveUnitsCommand()`, etc.) so mouse,
  touch, and any future gamepad input share the same gameplay behavior — keep input
  parsing separate from gameplay rules, and add a shared command method before adding
  parallel per-device gameplay logic. **Before adding any new key or button, read
  `topics/input-and-keybinds.md`** — it holds the current bindings, the keys reserved for
  systems not built yet, and the wiring rule that keeps a binding player-rebindable.
- **New system, new topic file.** Follow the same structure as the existing topics.
  **New game mode or control surface**: add a topic, or a section to `game-modes.md`.
