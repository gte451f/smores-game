Chest Interactable (Variant_Strategy)

Context

The Strategy variant already has a minimal inventory system: FInventoryItem (a plain USTRUCT), UInventoryComponent (a generic ActorComponent that holds items and exposes AddItem/RemoveItemAt/OnInventoryChanged), and UInventoryWidget (a UUserWidget that binds to any UInventoryComponent and displays its contents as text). Today the only thing carrying a UInventoryComponent is AStrategyUnit, and the only display path is the player's own inventory, toggled via AStrategyPlayerController::ToggleInventory.

We're extending this to a placeable world object — a chest — that holds its own seeded inventory and can be viewed (read-only, no add/remove/transfer yet) once a selected unit is close enough. This is explicitly scoped down in behavior (no click-to-walk-then-auto-open flow, no item transfer yet), but the user wants the class design to anticipate a fuller container system later: Chest should be one concrete type of a generic, reusable container concept, alongside future types like Barrel or Bag, without redoing this work when those show up.

Approach

1. New C++ classes: AStrategyContainer (base) + AStrategyChest (concrete type)

New files, flat under Variant_Strategy/ (matching StrategyUnit.h/StrategyPawn.h placement — not nested under Inventory/ or UI/, since these are actors, not data/display concerns):
- Source/smores/Variant_Strategy/StrategyContainer.h / .cpp
- Source/smores/Variant_Strategy/StrategyChest.h / .cpp

AStrategyContainer — the reusable base every container type (Chest, and later Barrel/Bag) derives from. Holds everything generic: the mesh/interaction/inventory plumbing and the proximity check. Mirrors the AStrategyUnit/AStrategyPlayerUnit split already in the codebase (generic base + type-specific subclass), and reuses UInventoryComponent unmodified — it's already generic, not unit-specific.

UCLASS(abstract)
class AStrategyContainer : public AActor

Members (mirroring AStrategyUnit's component-setup conventions):
- UStaticMeshComponent* ContainerMesh — VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"), root component. Mesh asset assigned per-type in each BP subclass.
- USphereComponent* InteractionRange — same specifiers, attached to root, default radius (e.g. 250.f), purely for the proximity check and editor visualization (no overlap-event binding needed).
- UInventoryComponent* Inventory — same specifiers, CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory")).
- TArray<FInventoryItem> StartingItems — EditAnywhere, Category="Inventory", empty by default at this level — the seeding mechanism lives here, concrete seed data lives in each type's constructor (see AStrategyChest below).
- virtual void BeginPlay() override — iterates StartingItems, calls Inventory->AddItem(Item) for each (same pattern as AStrategyPlayerUnit::BeginPlay).
- UInventoryComponent* GetInventory() const { return Inventory; } — plain accessor, mirrors AStrategyUnit::GetInventory().
- bool IsUnitInRange(const AStrategyUnit* Unit) const — simple distance check: Unit && FVector::Dist(GetActorLocation(), Unit->GetActorLocation()) <= InteractionRange->GetScaledSphereRadius(). Deliberately a plain distance check rather than a physics overlap query — no collision-profile setup required, easy to reason about, matches the "keep it simple" scope.
- void NotifyOpened() calling BP_ContainerOpened() — a BlueprintImplementableEvent cosmetic hook (same convention as AStrategyUnit::BP_InteractionBehavior), so any container type gets an open animation/sound hook for free later. Called from the controller when a container's widget is opened.

AStrategyChest — UCLASS(abstract) class AStrategyChest : public AStrategyContainer. No new members/overrides beyond its constructor, which populates the inherited StartingItems with 2–3 hardcoded FInventoryItems (same pattern as AStrategyPlayerUnit's constructor). This is the concrete "type" the user asked for: it exists to hold chest-specific default content and to give chest-specific behavior (e.g. a lock/key mechanic later) a home without touching AStrategyContainer or other container types. A future AStrategyBarrel/AStrategyBag would follow the identical pattern.

2. Controller wiring: AStrategyPlayerController

Modify Source/smores/Variant_Strategy/StrategyPlayerController.h / .cpp. Add, alongside the existing InventoryWidgetClass/InventoryWidget/ToggleInventoryAction triplet — named generically after Container, not Chest, so a future Barrel/Bag needs zero controller changes:

- TSubclassOf<UInventoryWidget> ContainerWidgetClass — EditAnywhere, Category="UI". Reuses UInventoryWidget directly (no new widget C++ class) since it already just binds to a UInventoryComponent* — a separate BP child (WBP_ContainerInventory) gives it an independent instance without new code, shared by every container type.
- TObjectPtr<UInventoryWidget> ContainerWidget — active instance, same lazy-create pattern as InventoryWidget.
- UInputAction* ToggleContainerAction — EditAnywhere, Category="Input", bound in SetupInputComponent the same way as ToggleInventoryAction.
- void ToggleContainer(const FInputActionValue& Value):
  - If ContainerWidget is currently in the viewport, call CloseContainer() and return (press-again-to-close, same UX as ToggleInventory).
  - Otherwise call FindContainerInRange(); if it returns a container, lazily create ContainerWidget from ContainerWidgetClass, call ContainerWidget->SetInventory(Container->GetInventory()), AddToViewport(0), then Container->NotifyOpened().
  - If no container in range, do nothing.
- void CloseContainer() — ContainerWidget->ClearInventory(); ContainerWidget->RemoveFromParent(); (mirrors CloseInventory).
- AStrategyContainer* FindContainerInRange() const — UGameplayStatics::GetAllActorsOfClass(GetWoraticClass(), Containers) (picks up every subclass — Chest today, Barrel/Bag later — automatically),then for each container and each unit in ControlledUnits, return the first container where Container->IsUnitInRange(Unit) is true.

This satisfies the proximity requirement (opening does nothing unless a selected unit is within InteractionRange) without touching the existing move/select/EQS interaction pipeline used for unit-to-unit interaction — that
generalization (click a container, path to it, auto-open on arrival) is explicitly deferred.

3. Content / editor wiring (do in a fork, per CLAUDE.md's MCP token-discipline guidance)

- Create BP_Chest (child of AStrategyChest), assign a static mesh to ContainerMesh (reuse an exisengine basic shape), and place one instance in Content/Variant_Strategy/LVL_Strategy.umap (confirmedwith the user as the target map, despite the project's GlobalDefaultGameMode still pointing at BP_TopDownGameMode/Lvl_TopDown).
- Create WBP_ContainerInventory (child of UInventoryWidget, duplicate WBP_Inventory's SlotListTex container type, not chest-specific. Optionally add a "Close" button wired to call the controller'sCloseContainer — not required (the toggle key already closes it) but cheap Blueprint glue if wanted.
- Create IA_Strategy_ToggleContainer (duplicate IA_Strategy_CyclePawn, same Boolean ValueType — s
- Set BP_StrategyPlayerController's ContainerWidgetClass = WBP_ContainerInventory, ToggleContainerAction = IA_Strategy_ToggleContainer.
- Manual step for the user: add the actual key binding for IA_Strategy_ToggleContainer into MousepingContext if desired) — per CLAUDE.md, MCP can't safely round-trip UInputMappingContext keymappings.

Build order

1. Write StrategyContainer.h/.cpp, StrategyChest.h/.cpp, and the StrategyPlayerController changes.
2. Close the editor, cold-build from Visual Studio (new UCLASSes — Live Coding won't pick them up
3. Reopen the editor, do the content/MCP wiring pass in one fork.
4. User adds the IA_Strategy_ToggleContainer key binding by hand.
5. /compact if the MCP fork's output bloated context.

Verification

- In PIE, select a player unit, walk it near BP_Chest, press the bound key — the container widget appears listing the seeded StartingItems.
- Press the key again (or click Close, if added) — widget disappears.
- Walk away and press the key with no unit in range — nothing happens (confirms the proximity gate).
- Confirm the player's own inventory (ToggleInventory) still works unaffected (no shared state bentainerWidget instances).