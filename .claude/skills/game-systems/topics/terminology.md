# Terminology

Player-surface terms used across the other topics in this skill — the vocabulary a player
or designer would use, independent of implementation.

## Controlled Units

The Strategy player controller's current selected unit list (`AStrategyPlayerController::ControlledUnits`).

## Drag Selection

An RTS-style box selection operation displayed by the Strategy HUD.

## Interaction Radius

Strategy distance used for selection sweeps, formation spread, move acceptance, cursor feedback, and nearby interaction checks.

## Lead Unit

The selected Strategy unit closest to a move command target. It receives the exact target location while other selected units are assigned nearby navigable points.

## Player Surface

The behavior, controls, feedback, and rules visible to players, independent of implementation detail. Each topic in this skill documents both this and the underlying implementation — see the "Player Surface" section of each topic.
