# Game Modes

## Main Menu

The title screen, loaded via `Lvl_MainMenu` (`AMainMenuGameMode`, no default pawn — the
level is UI only). `AMainMenuHUD` spawns `UMainMenuWidget` on `BeginPlay` and puts the
owning player controller into UI-only input mode with the cursor shown, mirroring how
`AStrategyHUD` owns `UStrategyUI`'s lifecycle.

`UMainMenuWidget` exposes five buttons — Continue, Start New Game, Load Game, Options,
Exit — bound via `BindWidgetOptional` (`ContinueButton`/`NewGameButton`/`LoadGameButton`/
`OptionsButton`/`ExitButton`). There's no save system yet (see `game-design`'s
`save-system.md`), so Continue, Load Game, and Start New Game are all wired to the same
`EnterGame()` call, which opens whatever level is set in the `GameLevel`
`TSoftObjectPtr<UWorld>` property (currently `LVL_Strategy`) via
`UGameplayStatics::OpenLevelBySoftObjectPtr`. Exit calls `UKismetSystemLibrary::QuitGame`.

Before traveling, `EnterGame()` calls `PC->SetInputMode(FInputModeGameAndUI())` on the
owning PlayerController. This isn't optional cleanup: `AMainMenuHUD`'s UI-only input mode
points Slate's focus and mouse-capture path at this widget, and a non-seamless `OpenLevel`
travel doesn't reset that - the `UGameViewportClient` (and Slate's focus/capture state)
outlives the PlayerController and the destroyed widget, so the next level's fresh
PlayerController inherits a dangling capture target and gets no working mouse input at all
(keyboard input still worked, since it doesn't depend on the same focus path). Directly
resetting the viewport client's ignore-input/capture-mode/lock-mode flags is *not*
sufficient fix - only an actual `SetInputMode` call redirects Slate's focus back to the
game viewport. `FInputModeGameOnly` was deliberately avoided here in favor of
`FInputModeGameAndUI`: `GameOnly` also switches to permanent mouse capture, a locked
cursor, and high-precision (relative-delta) mouse movement - an FPS-style mode
`AStrategyPlayerController` never opts into and that breaks its absolute-cursor-position
click/drag selection. Any future UI screen that transitions into a gameplay level (e.g. a
pause menu's "return to game" or "quit to main menu") needs the same reset before
traveling.

Options spawns `UOptionsWidget` (a `UWindowWidget` subclass, reusing its `CloseButton`
chrome as the back affordance) on top and collapses the main menu widget until
`UOptionsWidget::OnOptionsClosed` fires. `UOptionsWidget` has four category buttons
(`KeybindingsButton`/`AudioButton`/`GraphicsButton`/`GameButton`) that each show one of
four placeholder panels (`KeybindingsPanel`/`AudioPanel`/`GraphicsPanel`/`GamePanel`) —
the panels exist and switch correctly, but none of the underlying settings systems
(keybind rebinding, audio/graphics options, gameplay options) are implemented yet.

C++: `Source/smores/MainMenu/` (`MainMenuGameMode`, `MainMenuHUD`) and
`Source/smores/MainMenu/UI/` (`MainMenuWidget`, `OptionsWidget`). Blueprint assets:
`Content/MainMenu/Blueprints/` (`BP_MainMenuGameMode`, `BP_MainMenuHUD`) and
`Content/MainMenu/UI/` (`WBP_MainMenu`, `WBP_Options`). `Lvl_MainMenu` is the project's
`GameDefaultMap` (packaged/game launch); the editor's `EditorStartupMap` is `LVL_Strategy`
(the `Lvl_TopDown` map that used to serve as a faster-loading PIE-iteration level has been
removed along with the rest of `Content/TopDown/`).

**Known Gaps:** no save system, so Continue/Load Game/Start New Game are functionally
identical; Load Game has no save-slot list UI since there's nothing to list yet; the four
Options categories are empty placeholders; no Mods entry (see `game-design`'s
`main-menu-and-meta-flow.md`).

## Strategy

Strategy is an RTS-style command mode. The player controls a floating camera, selects units, and gives movement and interaction commands. Units move through AI controller pathfinding rather than direct player possession.

Visible progression is currently focused on selection, command feedback, and unit interaction behavior. See `strategy-camera-and-selection.md` and `strategy-unit-commands.md` for the underlying rules and implementation.
