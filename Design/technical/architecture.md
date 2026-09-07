# Architecture

## Project Shape

This is a top-down RTS prototype. Core gameplay logic is implemented in C++ under `Source/smores/`. Blueprint subclasses in `Content/` complete class defaults, presentation, asset references, widgets, and map-specific setup.

## Source Layout

- Module entry point lives directly under `Source/smores/`.
- Strategy classes live under `Source/smores/Variant_Strategy/`.

Keep new gameplay code under `Variant_Strategy/` unless it is genuinely module-level.

## C++ and Blueprint Responsibilities

C++ should own:

- Gameplay rules.
- Command routing.
- Movement requests.
- Scoring and counters.
- AI-facing utility logic.
- Selection and interaction rules.

Blueprint should own:

- Visual feedback.
- Asset class assignment.
- Widget layout.
- Animation, particles, sounds, and presentation events.
- Map-specific setup.

## Documentation Responsibilities

When changing behavior, update the closest design doc:

- `Design/systems/` for implementation-backed gameplay systems.
- `Design/player-facing/` for controls, visible rules, and help-file-ready text.
- `Design/technical/` for cross-system patterns and architecture.

If no doc exists for the changed behavior, create one from `Design/templates/system-design-template.md`.
