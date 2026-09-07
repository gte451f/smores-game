# Strategy Unit Commands

## Purpose

The Strategy unit command system moves selected units to target locations, spreads groups around the target, reports movement completion, and triggers interaction behavior when a selected unit reaches an interactable target area.

## Player Surface

Players select one or more units and command them to move by clicking or tapping a target location. The closest selected unit takes the lead target. Other selected units move to nearby random navigable points. If a moved unit reaches the interaction location and finds an interactive unit nearby, both units can play interaction behavior.

## Core Rules

- Commands operate on `AStrategyPlayerController::ControlledUnits`.
- Mouse mode uses `CachedInteraction` as the move goal.
- Touch mode uses `CachedSelection` as the move goal.
- The lead unit is the selected unit closest to the move goal in 2D distance.
- Non-lead units receive random navigable points around the move goal using `InteractionRadius * 0.66f`.
- Each commanded unit is stopped before receiving a new move request.
- Unit movement is executed by that unit's `AAIController`.
- Move requests allow partial paths, require pathfinding, project goals to navigation, and require a navigable end location.
- Failed move requests mark the interaction as failed for cursor feedback.
- On movement completion, the controller unsubscribes from that unit's completion delegate.
- Only one completion path can trigger interaction after each command because `bAllowInteraction` is cleared after the first allowed completion.
- Interaction checks for `ECC_WorldDynamic` overlaps near `CachedInteraction`, ignoring selected units and the moved unit.
- Unit interaction rotates one unit toward the other and calls Blueprint interaction behavior on both participants.

## C++ Implementation

- `AStrategyPlayerController`
  - `InteractClickStarted()` resets interaction gating.
  - `InteractClickCompleted()` moves units when selection and cursor hit are valid.
  - `DoMoveUnitsCommand()` chooses goals, binds movement completion, issues moves, and calls `BP_CursorFeedback()`.
  - `OnMoveCompleted()` handles interaction detection and gates repeated interaction.
  - `GetClosestSelectedUnitToLocation()` picks the lead unit.
- `AStrategyUnit`
  - Auto-possesses AI when placed or spawned.
  - Caches `AAIController` in `NotifyControllerChanged()`.
  - Subscribes to `UPathFollowingComponent::OnRequestFinished`.
  - `MoveToLocation()` builds and submits `FAIMoveRequest`.
  - `StopMoving()` immediately stops character movement.
  - `Interact()` rotates and calls Blueprint interaction behavior on both units.

## Blueprint / Asset Dependencies

- Unit Blueprint subclasses implement:
  - `BP_UnitSelected()`
  - `BP_UnitDeselected()`
  - `BP_InteractionBehavior()`
- Strategy player controller Blueprint implements or configures:
  - `BP_CursorFeedback()`
  - Input mapping contexts and action assets.
- A valid NavMesh is required for group movement and random navigable formation points.

## Extension Points

- Add formation logic inside `DoMoveUnitsCommand()` where non-lead goals are assigned.
- Add ownership, command permissions, or ability checks before units are added to `ControlledUnits` or before `MoveToLocation()` is called.
- Add interaction types by replacing or extending the `OnMoveCompleted()` overlap handling.
- Keep unit presentation in Blueprint events and command authority in C++.

## Known Gaps

- Interaction overlap currently casts overlapped actors to `AStrategyUnit` even though the object query targets `ECC_WorldDynamic`; future interactables may need a dedicated interface.
- Only the first valid moved unit after a command can trigger interaction because of `bAllowInteraction`.
- Group formation is randomized rather than deterministic.
