 Fix chest highlight color + add double-click-to-open

 Context

 Two BP_Chest instances exist in LVL_Strategy.umap, built on AStrategyContainer / AStrategyChest. The click-to-select/highlight feature (Design/plans/8-31-26-select-container.md) is already implemented in C++: AStrategyContainer::SetSelected() swaps ContainerMesh's material between NormalMaterial/SelectedMaterial, and AStrategyPlayerController already has SelectedContainer, FindContainerAtLocation, SetSelectedContainer, and calls it from both DoSelectCommand (click-to-select) and ToggleContainer (the "O" key handler). Verified the two material assets are correctly authored: MI_Container_Green = bright green (0.05, 0.8, 0.05), MI_Container_Green_Selected = dark green (0.02, 0.35, 0.02) — so the color values themselves aren't the bug. The last commit ("partially working color on chests") suggests the wiring from the placed BP_Chest instances to these materials is incomplete or broken; the implementation pass below verifies and fixes NormalMaterial/SelectedMaterial on the Blueprint CDO and both placed instances via MCP.

 Additionally, the user wants double-clicking a chest to do what the "O" key already does (open its inventory) when a player-controlled pawn is nearby, and to highlight it dark green the same way a single click does.

 1. Fix the chest color wiring (content, MCP fork)

 - Re-check BP_Chest_C's CDO NormalMaterial/SelectedMaterial and both placed instance overrides (BP_Chest_C_UAID_74D4DD1BA5635CFD02_2032240856, BP_Chest_C_UAID_74D4DD1BA5635AFD02_1791099503 in LVL_Strategy).
 - Set/repair via BlueprintTools (not raw CDO writes) so the override flag is set and it compiles: NormalMaterial = MI_Container_Green, SelectedMaterial = MI_Container_Green_Selected on BP_Chest_C. If either placed instance carries its own stale/empty override for these properties, clear it so it inherits from the Blueprint, or set it to match.
 - Have the user verify + save BP_Chest in the editor afterward (CDO writes via MCP don't reliably set the BP override flag — existing project guidance).

 2. Double-click-to-open a chest (C++)

 Source/smores/Variant_Strategy/StrategyPlayerController.h/.cpp. No new UCLASS/USTRUCT/UENUM — Live Coding should pick this up.

 - Extract the widget-creation/show/NotifyOpened body currently inlined at the bottom of ToggleContainer (StrategyPlayerController.cpp:403-421) into a new protected helper void OpenContainer(AStrategyContainer* Container). Declare it in the header near ToggleContainer/CloseContainer.
 - ToggleContainer becomes: find NearbyContainer (unchanged) → SetSelectedContainer(NearbyContainer) → OpenContainer(NearbyContainer).
 - Change SelectAllDoubleClick(const FInputActionValue& Value) (StrategyPlayerController.cpp:492-495), which currently unconditionally calls DoSelectAllUnitsOnScreenCommand():
   - Get the world location under the cursor via GetLocationUnderCursor.
   - If FindContainerAtLocation(CursorLocation) finds a container Clicked:
     - SetSelectedContainer(Clicked) — highlights it dark green, mirroring single-click behavior, regardless of range.
     - Loop ControlledUnits; if any is Clicked->IsUnitInRange(...), call OpenContainer(Clicked). This mirrors the same proximity gate ToggleContainer/FindContainerInRange already use (a selected unit must be in range) rather than any player pawn in the level.
     - Return without falling through.
   - Otherwise, keep existing behavior: DoSelectAllUnitsOnScreenCommand().
 - This reuses the existing SelectAllDoubleClickAction (already bound to the native Enhanced Input double-tap trigger on left-click) — no new Input Action or IMC key binding, avoiding the IMC round-trip limitation called out in CLAUDE.md. Net behavior change: double-clicking directly on a chest now selects+opens it instead of triggering "select all on screen" (double-clicking a chest was never a meaningful select-all gesture).

 Build order

 1. Make the C++ changes above; Live Coding compile (no structural/type changes needed).
 2. Fork the MCP content pass (material wiring fix + BP save) per CLAUDE.md token-discipline guidance.
 3. /compact if the MCP fork's output bloated context.

 Verification (PIE)

 - Both chests render bright green on level start.
 - Clicking one chest turns it dark green; the other stays bright green.
 - Clicking empty ground reverts the highlighted chest to bright green.
 - With a unit selected and in range, pressing "O" opens the chest and highlights it dark green (existing behavior, unaffected).
 - Double-clicking a chest with a selected unit in range: chest turns dark green AND its inventory opens.
 - Double-clicking a chest with no selected unit in range: chest turns dark green but inventory does NOT open.
 - Double-clicking empty ground or a unit: existing "select all units on screen" behavior is unchanged.