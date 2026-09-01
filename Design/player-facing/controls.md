# Controls

This file describes player-visible controls at the design level. Exact bindings live in Enhanced Input assets and Blueprint defaults.

## Strategy

- Pan the orthographic camera with camera movement input.
- Zoom in and out with zoom input.
- Reset zoom returns to the camera's starting orthographic width.
- Click a unit to select or toggle it.
- Use the selection modifier to add/remove units without clearing the current selection.
- Drag to show a selection box through the Strategy HUD.
- Click a target location with selected units to issue a move command.
- On touch, tap selects, longer touch drags the camera, secondary touch enables box selection, and double-tap toggles all visible units.
- Press Tab to cycle camera control to the next pawn. Each pawn holds its own position and zoom level independently. Works with any number of pawns placed in the level.
