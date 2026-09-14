Free-Look Camera for Variant_Strategy

Context

The Strategy variant's camera was a fixed-angle, orthographic top-down/isometric view: AStrategyPawn held a Camera in Orthographic mode with no rotation control, "zoom" just resized OrthoWidth, and RMB-hold panned the camera using a hardcoded -45-degree isometric offset. The actual need is stronger than a traditional RTS camera: this is squad-based control in a 3D world, and players need to freely rotate the view — including tilting down to a near-street-level vantage to see roughly what one of their squad members sees, not just look straight down at a fixed angle.

This called for a real 3D free-look camera (perspective projection, yaw+pitch rotation, dolly zoom, independent height control) while preserving the existing RTS-style commands (click-to-select, drag-box-select, right-click-to-move) exactly as they behaved before. Worked out collaboratively: RMB was chosen for rotation (freeing WASD as the sole pan method), Q/E for raise/lower (a genuinely independent axis from zoom once the camera can tilt), and a fix for a real engine quirk in drag-box selection (actors behind the camera can spuriously satisfy the screen-space rectangle test). Initial pitch/zoom clamps were added, then explicitly removed after playtesting — see below.

Touch controls, custom keybinding UI, and any future first-person "see through a squad member's eyes" mode are explicitly out of scope.

Approach

AStrategyPlayerController stays the single owner of camera input state and tunables (mirrors the pre-existing CameraZoom pattern). GetControlRotation()/SetControlRotation() is the one source of truth for pitch/yaw — no duplicate rotation state — and AStrategyPawn's Camera component's relative rotation/location is kept in sync with it. The pawn's root actor transform owns position only (WASD pan via AddMovementInput, Q/E height via direct Z), so panning and rotation stay decoupled.

Source/smores/Variant_Strategy/StrategyPawn.h/.cpp

- Constructor: Camera->ProjectionMode = Perspective, FieldOfView = 60. Removed the ortho-only OrthoWidth/AutoPlaneShift/bUpdateOrthoPlanes setup.
- New private float DollyDistance (replaces OrthoWidth as the "zoom" representation) and private UpdateCameraDollyOffset(), which sets Camera's relative location to -Camera->GetRelativeRotation().Vector() * DollyDistance.
- SetZoomModifier(float Value) keeps its existing signature (required — StrategyTouchControls calls through the controller's percentage API) but now sets DollyDistance and calls the helper.
- SetCameraRotation(const FRotator&) sets Camera's relative rotation directly (roll is NOT forced to zero — see "Full 360 rotation" below for why), then re-runs UpdateCameraDollyOffset() so the dolly follows the new look direction.
- SetHeight(float)/GetHeight() — sets the pawn's world Z via SetActorLocation, and re-points FloatingPawnMovement's PlaneConstraintOrigin to UpVector * NewHeight, so Q/E can move height without fighting the plane constraint that keeps WASD panning flat.

Source/smores/Variant_Strategy/StrategyPlayerController.h/.cpp

EditAnywhere Category="Camera" tunables: MinCameraPitch/MaxCameraPitch (unused — see below), DefaultCameraPitch = -50, CameraYawSpeed/CameraPitchSpeed = 0.8 (deg/pixel, raised twice from an initial 0.2 during playtesting), CameraRotateDragSuppressWindow = 0.1s, DefaultCameraHeight = 1500, MinCameraHeight = 800, MaxCameraHeight = 3000, HeightScaling = 50. Runtime state: DefaultCameraYaw, LastCameraRotateDragTime, CameraHeight, bIsRotatingCamera, bSkipNextRotateSample. New EditAnywhere Category="Input" property: UInputAction* AdjustHeightAction (bound like CyclePawnAction — guarded if, desktop-only).

Pan fix: MoveCamera originally combined WASD input as (InputVector.X + InputVector.Y) / (InputVector.X - InputVector.Y) fed through Forward/Right rotators built from GetControlRotation(). MCP inspection of BP_StrategyPawn's Camera confirmed its baked relative rotation is pitch -50, yaw 45 — while GetControlRotation() was never set anywhere in C++ before this change, so it was effectively always 0. The X+Y/X-Y remap was a static compensation for that fixed ~45-degree visual yaw assuming control rotation never moves. Once OnPossess/DoCameraRotateCommand started keeping real yaw in GetControlRotation(), that compensation double-applied. Fixed by using InputVector.X/InputVector.Y directly. A second pan bug then surfaced (A panned right instead of left): this IMC's Y axis reads +1 from A and -1 from D, the opposite of the naive assumption, so the right-vector term is negated: AddMovementInput(RightRot.RotateVector(FVector::RightVector), -InputVector.Y).

Rotation (RMB hold, replaces the old RMB-drag-pan) — went through three iterations before landing:

1. First attempt diffed GetMouseLocationForPlayer() (absolute cursor position) and used SetInputMode(FInputModeGameAndUI) with EMouseLockMode::LockAlways to hide/lock the cursor. This broke two ways: changing input mode while a mouse button is actively captured causes Slate to synthesize a release+repress of that same button, re-triggering the Hold action in a feedback loop (visible as MouseLockMode thrashing in the log); and locking the cursor to the viewport means its position saturates at the viewport edge, silently capping how far a single drag can rotate.
2. Second attempt dropped SetInputMode (keeping only bShowMouseCursor = false) and switched to GetInputMouseDelta (raw per-frame mouse movement) read inside InteractHoldTriggered, bound to the InteractHoldAction's Triggered event. This fixed the lock-thrash bug but was still unreliable: Enhanced Input's Triggered callback for a held digital button isn't guaranteed to fire every single engine tick, so most of a drag's movement could be silently dropped between callbacks — reported as "have to drag the whole screen for a small rotation."
3. Final design: AStrategyPlayerController::PlayerTick(float DeltaTime) is overridden and does the sampling itself, guaranteed every tick regardless of Enhanced Input's own firing cadence. InteractHoldTriggered was deleted entirely (no longer bound). InteractHoldStarted just sets bIsRotatingCamera = true, hides the cursor, warps it to the viewport center (GetViewportSize + SetMouseLocation), and flushes one GetInputMouseDelta reading to discard whatever's left in the accumulator from the click itself. PlayerTick, when bIsRotatingCamera is true, reads GetInputMouseDelta, calls DoCameraRotateCommand, then re-centers the cursor again (every tick) so it can never reach a screen edge and saturate. bSkipNextRotateSample additionally discards the very first tick's delta after a hold starts, since that tick can otherwise misread the centering warp itself as a real mouse movement before the OS/Slate has caught up — this was the source of a persistent one-time "flash/reset" on the first RMB use each session. InteractHoldCompleted clears bIsRotatingCamera and restores the cursor.

Full 360 rotation: DoCameraRotateCommand originally updated Pitch and Yaw as independent Euler fields, clamped to MinCameraPitch/MaxCameraPitch (-85/-15 initially). The user asked to drop the clamp entirely for feel-testing ("let's make that unconstrained for now"), which surfaced a deeper problem: a naive Euler pitch/yaw update cannot represent a genuine loop past +/-90 degrees pitch (that requires roll to emerge, which a pure pitch/yaw pair can't produce) and gimbal-locks near vertical (yaw has no visible effect when looking straight up/down). Fixed by composing rotation with quaternions instead — world-space yaw times camera-local-space pitch times the current orientation — and by no longer forcing Roll to zero in AStrategyPawn::SetCameraRotation, since the true continuation of a vertical loop through "upside down" needs it. MinCameraPitch/MaxCameraPitch are left declared but unused; re-introduce a clamp there (and reconsider whether to keep Roll unconstrained) if a bounded range is wanted later.

Height (Q/E): AdjustHeight(const FInputActionValue&) -> DoCameraModifyHeightCommand(Value.Get<float>() * HeightScaling). DoCameraModifyHeightCommand/DoCameraResetHeightCommand mirror the zoom commands, clamping CameraHeight and calling ControlledCameraPawn->SetHeight(CameraHeight). Playtesting surfaced an apparent "Q/E also moves forward/back" complaint — confirmed via code review this is not a bug: SetHeight only ever touches world Z, so the camera genuinely moves in a straight vertical line. The perceived forward/back motion is the ordinary trigonometric consequence of gaining/losing altitude while pitched at a fixed angle (the ground point centered in view necessarily recedes or approaches) — the same behavior as Unreal's own editor fly-camera Q/E, Google Earth, etc. Left as-is; no code change made.

Zoom: ZoomCamera negates its input (DoCameraModifyZoomCommand(-Value.Get<float>() * ZoomScaling)) after playtesting found scroll-up zoomed out instead of in. DoCameraModifyZoomCommand's FMath::Clamp(CameraZoom, MinZoomLevel, MaxZoomLevel) was removed entirely at the user's request ("capped... please remove") — CameraZoom (and therefore DollyDistance) is now unbounded on the desktop scroll-wheel path. MinZoomLevel/MaxZoomLevel are still used by the untouched touch-percentage API (DoCameraSetZoomPercentageCommand, GetDefaultZoomPercentage), just not by this path. Known caveat, not yet addressed: DollyDistance has no floor, so zooming in far enough goes negative and puts the camera on the wrong side of the pawn (visually glitchy) — flagged for the user, fix deferred until it's actually hit in practice.

Reset & possess: DoCameraResetRotationCommand() resets to FRotator(DefaultCameraPitch, DefaultCameraYaw, 0). DefaultCameraYaw is captured from the pawn's actual resting orientation in OnPossess (not hardcoded to 0), so reset restores whatever facing the level was designed around. ResetCamera calls DoCameraResetZoomCommand() + DoCameraResetHeightCommand() + DoCameraResetRotationCommand(). OnPossess pushes the controller's authoritative rotation/zoom/height defaults onto the newly possessed pawn instead of pulling OrthoWidth from the pawn.

Source/smores/Variant_Strategy/UI/StrategyHUD.cpp

In DrawHUD(), right after the existing GetActorsInSelectionRectangle(BoxStart, BoxCurrentPosition, BoxedPlayerUnits, true) call, filter out actors behind the camera before they reach PC->DragSelectUnits (GetActorsInSelectionRectangle doesn't clip against the near plane, so a behind-camera actor can still satisfy the 2D screen-rect test once the camera can pitch/rotate freely):

FVector CamLoc; FRotator CamRot;
PC->GetPlayerViewPoint(CamLoc, CamRot);
BoxedPlayerUnits.RemoveAll([&CamLoc, &CamRot](AStrategyPlayerUnit* Unit)
{
    return !IsValid(Unit) || FVector::DotProduct(CamRot.Vector(), Unit->GetActorLocation() - CamLoc) <= 0.0f;
});

No changes: StrategyTouchControls.h/.cpp, StrategyUI.h/.cpp

ResetZoom()/SetZoomPercentage()/BP_SetZoomPercentage() operate purely on the existing 0-1 percentage API and never reference OrthoWidth directly, so they needed zero changes even though zoom's internal meaning shifted from ortho-width to dolly-distance, and stayed untouched through the zoom-invert/uncap fixes too. StrategyUI has no camera-orientation-reactive logic at all.

Editor/Blueprint wiring (unreal-mcp, forked per CLAUDE.md token-discipline guidance)

- Created /Game/Variant_Strategy/Input/Actions/IA_Strategy_AdjustHeight by duplicating IA_Strategy_Zoom (matching ValueType: Axis1D). Set AdjustHeightAction on BP_StrategyPlayerController to it via a raw ObjectTools CDO write (verified surviving compile+save; visually confirmed by the user afterward).
- Confirmed BP_StrategyPawn's Camera CDO already had ProjectionMode: Perspective, FieldOfView: 60, and relative rotation pitch -50/yaw 45 (no shadowing BP override; matches the pan-fix finding above).
- User manually mapped Q (-1, Negate modifier) / E (+1) on IA_Strategy_AdjustHeight in IMC_Strategy_Mouse — IMC key mappings can't be safely round-tripped via MCP, so this step was always going to be manual.

Diagnostic incident: stale Live Coding state

Partway through playtesting, several rounds of fixes (the A/D sign flip, the SetInputMode removal, the quaternion rewrite) appeared to have zero effect despite Live Coding reporting "Result: Succeeded" each time. A fork investigation (BlueprintTools/ObjectTools graph inspection + EditorToolset.LogsToolset) ruled out a Blueprint-side override — BP_StrategyPawn's component hierarchy and Event Graph were confirmed clean — and instead found LogLiveCoding had zero compile events recorded for the entire current editor session. Live Coding patches only live in a process's memory and don't survive an editor restart; since the process had been restarted at some point after the very first cold build, every subsequent Live-Coding-reported "success" was suspect. Resolved by doing a full cold build (editor closed, Build.bat) for the PlayerTick rework, which also served as an unambiguous reset point — everything since has been verified via matched cold-build/source file timestamps rather than trusting in-editor Live Coding logs alone.

Build order (as actually executed)

1. Initial C++ pass (new UPROPERTYs: AdjustHeightAction, all Camera tunables) — cold build, succeeded.
2. MoveCamera diagonal-remap fix — Live Coding (confirmed via pasted log).
3. Editor/Blueprint MCP wiring pass (see above).
4. A/D sign fix + first rotation rewrite (SetInputMode removal, GetInputMouseDelta) — Live Coding, confirmed working via playtesting.
5. Pitch range widen + speed double (0.2 -> 0.4) — Live Coding, confirmed working.
6. Pitch clamp removed entirely — Live Coding; this is the point where "no effect" reports started (see diagnostic incident above).
7. Quaternion rewrite + Roll preserved + PlayerTick/bIsRotatingCamera/bSkipNextRotateSample (new members + new virtual override) — required a cold build regardless of the Live Coding question, which also resolved it. Succeeded, confirmed via DLL/source timestamp comparison.
8. Zoom invert + zoom cap removal + speed double again (0.4 -> 0.8) — cold build (bSkipNextRotateSample was added in the same pass as step 7's edits, both landed in this build). User confirmed via playtest: "looks good."

Verification (PIE, as actually confirmed)

- WASD pans correctly relative to yaw at any camera rotation, including after a full RMB rotation.
- RMB-hold rotates smoothly and without a hard limit in either yaw or pitch, including looping past vertical.
- First RMB-hold of a session no longer flashes/resets.
- A quick RMB tap still issues a move-order and does not also rotate the camera.
- Q/E raises/lowers within clamped bounds (the apparent forward/back drift while doing so is expected geometry, not a bug — see above).
- Mouse-wheel zoom dollies in/out, uncapped, with scroll-up now zooming in.
- Reset Camera restores zoom + height + rotation together.
- Drag-box select at a shallow camera pitch near foreground units doesn't pick up units behind the camera.
- Not yet re-verified after the latest changes: touch controls (out of scope for this playtesting pass, should still be unaffected per the "No changes" section above).

Open items

- DollyDistance has no floor and can go negative (camera flips to the wrong side of the pawn) — deferred until actually encountered.
- MinCameraPitch/MaxCameraPitch and MinZoomLevel/MaxZoomLevel (desktop path) are unused but still declared — revisit if a bounded range is wanted later.
- User mentioned other keyboard commands are planned but not yet specified — out of scope for this pass.
