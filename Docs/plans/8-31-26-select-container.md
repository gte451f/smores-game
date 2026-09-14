Container Selection & Visual Highlighting (Variant_Strategy)

Context

The chest/container feature (AStrategyContainer base + AStrategyChest, proximity-gated open, read-only inventory display) is implemented and confirmed working with a single chest in LVL_Strategy.umap. We're now extending it with a second container and a way to disambiguate between them: today AStrategyPlayerController::FindContainerInRange() just returns the first container it finds in range, so with two containers near a unit there'd be no way to pick which one opens. We're adding:

1. A second container in the world (another BP_Chest instance, seeded with different items) elsewhere in the map.
2. A green material scheme so containers read as distinct, interactable world objects.
3. A click-to-select step (reusing the existing left-click select action) so the player can pick which nearby container is "the one" before pressing the open/close key — the selected container renders in a darker shade of green.

This keeps the reusable AStrategyContainer design paying off: selection and material logic live on the base class, so any future container type (Barrel, Bag) gets them for free.

Approach

1. AStrategyContainer — selection state + material swap

Source/smores/Variant_Strategy/StrategyContainer.h / .cpp:

- Add two UPROPERTY(EditAnywhere, Category="Container") TObjectPtr<UMaterialInterface> fields: NormalMaterial, SelectedMaterial. Forward-declare class UMaterialInterface; in the header.
- BeginPlay(): if NormalMaterial is set, apply it via ContainerMesh->SetMaterial(0, NormalMaterial) before seeding StartingItems (unchanged seeding logic stays below it).
- Add void SetSelected(bool bSelected): sets ContainerMesh's material to SelectedMaterial when true, NormalMaterial when false. No new state needed beyond the mesh's current material — the controller is the source of truth for which container is selected (see below), this method just reflects it visually.

2. AStrategyPlayerController — click-to-select + selection-aware opening

Source/smores/Variant_Strategy/StrategyPlayerController.h / .cpp. No new Input Actions or key bindings — this reuses SelectClickAction/SelectClickAdditive (already bound to DoSelectCommand) and the existing ToggleContainerAction.

- New member: TObjectPtr<AStrategyContainer> SelectedContainer; (near ControlledUnits).
- New helper AStrategyContainer* FindContainerAtLocation(const FVector& Location) const: like the existing FindContainerInRange(), gathers all AStrategyContainer via GetAllActorsOfClass, but tests FVector::Dist(Container->GetActorLocation(), Location) <= SelectionRadius (the same click-tolerance radius already used for unit-click overlap) instead of a unit's InteractionRange. This is a click hit-test, distinct from the container's own open-range check.
- New helper void SetSelectedContainer(AStrategyContainer* NewContainer): no-ops if NewContainer == SelectedContainer; otherwise calls SelectedContainer->SetSelected(false) on the old one (if any), assigns the new one, and calls SetSelected(true) on it (if any). Centralizes the highlight toggling so both call sites below stay in sync.
- DoSelectCommand: after the existing unit-overlap loop fails to find a unit (i.e. right before the current return false; at the bottom), add a container check:
  - AStrategyContainer* Clicked = FindContainerAtLocation(SelectLocation);
  - If Clicked: call SetSelectedContainer(Clicked) and return true (click resolved to a container select).
  - Else if !bAdditiveSelection && SelectedContainer: call SetSelectedContainer(nullptr) (clicking empty ground clears the container pick, mirroring the existing deselect-all-units-on-empty-click behavior) — but only for non-additive clicks, so shift-click / touch (which always passes bAdditiveSelection=true) never clears an existing pick.
  - Otherwise fall through to return false unchanged (existing move-fallback behavior for touch/desktop is unaffected).
- FindContainerInRange: change from "return the first in-range container" to "prefer SelectedContainer if it's among the in-range set, otherwise return the first in-range container found" — track a FirstInRange local, and short-circuit-return immediately when the in-range container currently being checked equals SelectedContainer.
- ToggleContainer: after finding NearbyContainer via FindContainerInRange() (now selection-aware) and confirming it's non-null, call SetSelectedContainer(NearbyContainer) before opening it, so opening a container (even via the no-ambiguity auto-fallback path) always leaves it visually highlighted too.

Net effect: with one container in range, behavior is unchanged (auto-opens, now also highlights).e, the first click on either resolves the ambiguity — that one turns darker green — and theopen/close key opens exactly that one.

3. Content / editor wiring (fork, per CLAUDE.md MCP token-discipline guidance)

- Materials: create one parent Material with a Vector Parameter (e.g. Color) feeding both Base Color and Emissive (so it reads clearly green under any lighting), plus two Material Instance children: MI_Container_Green (bright
  green, e.g. (0.05, 0.8, 0.05)) and MI_Container_Green_Selected (darker green, e.g. (0.02, 0.35,
- Set BP_Chest's defaults: NormalMaterial = MI_Container_Green, SelectedMaterial = MI_Container_Green_Selected (via BlueprintTools so the override flag is set and it compiles cleanly).
- Second container: place a second BP_Chest instance in LVL_Strategy.umap, close enough to the fiway) that a single unit standing between them falls inside both InteractionRange spheres — needed toactually exercise the new selection step — while still being a visibly distinct placement rather than stacked on top of the first.
- Override that instance's StartingItems (an instance-editable array, not a Blueprint-default writ, e.g. Rope, Torch, TrapKit (same FInventoryItem(Id, DisplayName) shape used by AStrategyChest'sconstructor).
- Save the level.

Build order

1. Write the StrategyContainer.h/.cpp and StrategyPlayerController.h/.cpp changes above.
2. Cold build is not required this time — no new UCLASS/USTRUCT/UENUM, just new members/methods on existing classes, so Live Coding should pick it up. If Live Coding fails to apply cleanly, fall back to closing the editor and
   a normal cold build.
3. Do the content/MCP wiring pass (materials, BP_Chest defaults, 2nd instance + seed items, level save) in one fork.
4. /compact if the MCP fork's output bloated context.

Verification

- In PIE, confirm both chests render bright green and are easy to pick out from the environment.
- Walk a selected unit to the spot where both InteractionRange spheres overlap.
- Click one chest — it turns darker green. Press the open key — the widget shows that chest's (clher one's.
- Click the other chest — the first reverts to bright green, the second turns dark. Press open — confirms the second chest's different seeded items.
- Click empty ground — the highlighted chest reverts to bright green; pressing open with no expliin range still auto-opens it (single-container case unchanged).
- Walk away from both and confirm opening does nothing (proximity gate still holds).
- Confirm existing unit selection/move-command clicks still work unaffected (container check only check finds nothing).