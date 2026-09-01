# Input Model

## Shared Pattern

The project uses Enhanced Input. Input actions should route into small command methods so mouse, keyboard/gamepad, and touch controls can share gameplay behavior.

## Strategy

`AStrategyPlayerController` chooses a mouse or touch mapping context based on `InputMode`.

Mouse and touch have separate binding paths because their gestures differ, but they converge on shared commands:

- `DoSelectionCommand()`
- `DoSelectAllOnScreenCommand()`
- `DoDeselectAllCommand()`
- `DoDragScrollCommand()`
- `DoMoveUnitsCommand()`

Touch tap and double-tap are manually detected with timing fields because touch tap trigger behavior is not equivalent to mouse click behavior.

## Rules for Future Input Work

- Keep input parsing separate from gameplay commands.
- Add a shared command method before adding parallel mouse/touch gameplay behavior.
- Update `Design/player-facing/controls.md` when visible controls change.
- Update the relevant system doc when input changes affect gameplay rules.
