# Strategy Camera and Selection

## Purpose

The Strategy camera and selection system provides an RTS-style camera pawn, mouse and touch camera movement, zoom, unit selection, drag selection, and double-tap selection behavior.

## Player Surface

Players control a floating orthographic camera rather than a character. They can pan, zoom, reset zoom, select units individually, box-select units, deselect units, and on touch devices double-tap to select or deselect all visible units. Pressing Tab cycles control to the next placed player pawn; each pawn keeps its own camera position and zoom level independently, and cycling works with any number of pawns placed in the level. On touch, a brief hold-then-drag pans the camera.

## Core Rules

- `AStrategyPawn` owns an orthographic camera with a default `OrthoWidth` of 1500.
- Camera movement is handled by movement input on the floating pawn.
- Zoom clamps between `MinZoomLevel` and `MaxZoomLevel`.
- Reset zoom returns the camera to the possessed pawn camera's initial `OrthoWidth`.
- Mouse selection uses cursor hit testing against `SelectionTraceChannel`.
- Selection uses a vertical sphere sweep from the cached world point and checks pawn object types.
- In mouse mode, selection clears previous selection unless the selection modifier is active.
- **A single click is for picking a target, not for issuing an order against it** — it selects
  whatever is under the cursor and does nothing else. One shipped behavior currently breaks
  this (clicking an already-Aggressive NPC issues a squad attack) and is scheduled for removal;
  see `input-and-keybinds.md`'s "Existing defaults worth revisiting".
- Selecting an already-selected unit toggles it off.
- Drag selection is displayed by `AStrategyHUD::DragSelectUpdate()` and applied through `DragSelectUnits()`.
- Touch uses custom tap and double-tap timing because Enhanced Input tap triggers behave differently on touch.
- Touch double-tap toggles all recently rendered units on screen unless box selection is active.
- Touch secondary input acts as the selection modifier and drives box selection.
- Touch primary hold pans the camera once held past `TouchDragScrollHoldTime` (0.15s), via `DoCameraDragScrollCommand()`.
- `CyclePawn()` rebuilds `PlayerPawns` (sorted by stable object name, since actor-iteration order isn't stable across runs), resumes from the currently-selected pawn's index if one is selected, then advances with wraparound and re-selects the next pawn.

## C++ Implementation

- `AStrategyPawn`
  - Owns `UCameraComponent` and `UFloatingPawnMovement`.
  - `SetZoomModifier()` updates orthographic width.
- `AStrategyPlayerController`
  - Chooses mouse or touch input mapping context from `InputMode`.
  - Owns selected units in `ControlledUnits`.
  - Handles camera movement with `MoveCamera()`, `ZoomCamera()`, and `ResetCamera()`.
  - Handles mouse selection with `SelectHold*()`, `SelectClick()`, and `SelectionModifier()`.
  - Handles touch selection with `TouchPrimaryHold*()` and `TouchSecondary*()`.
  - Applies selection changes through `DoSelectionCommand()`, `DoSelectAllOnScreenCommand()`, `DoDeselectAllCommand()`, and `DragSelectUnits()`.
  - `CyclePawn()` (bound to `CyclePawnAction`) cycles selection across `PlayerPawns` (an `AStrategyPlayerUnit` list refreshed via `RefreshPlayerPawns()`), tracked by `CurrentPlayerPawnIndex`.
  - `TouchPrimaryHoldStarted/Triggered/Completed()` drive touch camera drag-scroll through `DoCameraDragScrollCommand()`.
- `AStrategyHUD`
  - Displays and clears the drag selection box.

## Blueprint / Asset Dependencies

- Level: `Content/Variant_Strategy/LVL_Strategy.umap`
- HUD class derives from `AStrategyHUD`.
- UI widgets derive from `UStrategyUI`.
- Mouse and touch mapping contexts are assigned by Blueprint defaults on the Strategy player controller subclass.
- Unit selection and deselection visuals are implemented through `AStrategyUnit` Blueprint events.

## Extension Points

- Add selection filters in `DoSelectionCommand()` if future factions, ownership, or unit categories are introduced.
- Add richer drag-select behavior in `AStrategyHUD` and route final selected units through `DragSelectUnits()`.
- Keep touch and mouse input paths converged at command methods when adding new selection behavior.

## Known Gaps

- **Double-clicking a *living* NPC falls through to select-all-on-screen.** Confirmed as not
  viable long term: Slice 8 makes a living NPC a double-click target in its own right (trade,
  or later dialog), so the gesture will mean "that person" rather than "everyone". Double-click
  on empty ground keeps select-all. See `inventory-roadmap.md`'s Slice 8.
- **A single click on an Aggressive NPC issues a squad attack order**, contradicting the
  single-click-only-selects rule above. Scheduled for removal.
- C++ does not currently show how `AStrategyHUD` determines the list of units inside the drag box.
- `DoSelectAllOnScreenCommand()` relies on `WasRecentlyRendered(0.2f)`, which may include units not actually intended to be selectable in future occlusion or faction systems.
- Camera movement uses hard-coded rotation assumptions in drag scroll.
