Combat: Health, Attack, Downed/Recovery, and NPC Aggro (Variant_Strategy)

Context

No combat system exists today. `AStrategyUnit` (StrategyUnit.h/.cpp) is purely commanded -
it never acts on its own, only reacting to `MoveToLocation`/`Interact` calls issued by
`AStrategyPlayerController`. Selection/targeting already distinguishes three click
outcomes in `DoSelectCommand` (StrategyPlayerController.cpp:730-858): a player pawn (added
to `ControlledUnits`), an NPC (`SetSelectedNPC`, highlight-only, never commandable today),
or a container (`SetSelectedContainer`). Containers already have a working proximity-gated
open+loot flow (`FindContainerInRange`/`ToggleContainer`/`OpenContainer`,
StrategyPlayerController.cpp:460-530) built on `AStrategyContainer::IsUnitInRange`
(StrategyContainer.cpp:45-48, checks distance against the container's own
`InteractionRange` sphere component) and an `UInventoryComponent` + drag-and-drop transfer
system (see `9-6-26-inventory-drag-drop-grid.md`, already implemented) that works on any
two `UInventoryComponent*` pointers regardless of owner type.

The Mannequin content pack (`Content/Characters/Mannequins/Anims/`) already has
`Unarmed/Attack/MM_Attack_01`, `_02`, `_03`, `MM_ChargedAttack` (raw anim sequences, not
yet montages) and `Death/MM_Death_Front_01..03`, `_Back_01`, `_Left_01`, `_Right_01`. There
is no "get up" animation anywhere in the pack - the downed/recovery beat has to be built
without one (see step 4).

Resolved during brainstorming

- Flat 25 damage per hit, either direction (player-on-NPC or NPC-on-player). Default 100
  max health for every unit, NPC and player alike.
- Downed for 15 seconds (not 5 - gives looting time), then auto-recovers to full health.
- New `UHealthComponent` on `AStrategyUnit`, mirroring the existing `UInventoryComponent`
  precedent rather than raw properties on the unit class.
- New `Passive`/`Aggressive` disposition on `AStrategyUnit` (a behavior/targeting concept,
  not health math, so it does not live on `UHealthComponent`).
- Input: left-click selection semantics are unchanged (container/NPC/player-pawn
  targeting all still work exactly as today). A new **A** key attacks the
  currently-selected NPC only while it is Passive (flips it to Aggressive and issues the
  attack). Once an NPC is Aggressive, plain left-click on it also issues the attack
  directly - no A needed. A has no effect on a selected container or player pawn.
- An attack command sends **every** unit in `ControlledUnits` to engage the target (not
  just the lead unit) - a squad-wide engage, unlike the spread-out formation move command.
- Combat resolves continuously once engaged, matching the `combat.md` design pillar
  ("automated once engaged") rather than one manual click per swing: whoever is attacking
  (a player squad member or an Aggressive NPC) keeps re-swinging the same target
  automatically until that target goes Downed, the attacker is invalid, or the attacker is
  given a new command (a new move or a new attack retargets/interrupts it).
- Aggressive NPCs self-initiate: this is the first autonomous (not player-commanded)
  behavior `AStrategyUnit` gets. An Aggressive NPC periodically finds the nearest
  non-Downed player-controlled pawn and engages it.
- A Downed unit cannot be damaged further (attacks against it no-op / it's excluded from
  all targeting searches), but stays selectable and keeps showing in the selection label
  exactly as today, and - specifically for a Downed NPC - becomes lootable.
- Aggressive -> Passive reset, exactly two triggers, otherwise Aggressive is permanent
  (no leash/range-based disengage):
  1. The Aggressive NPC itself goes Downed, then recovers -> resets to Passive on
     recovery.
  2. While hunting, every player-controlled pawn in the level is currently Downed (nothing
     left to fight) -> resets to Passive.
- Looting a Downed NPC reuses the existing container-open pipeline verbatim (same "O"
  key, same `ContainerWidget`, same drag-and-drop transfer code) rather than building a
  parallel loot UI. Proximity is gated the same way containers already gate it: a
  controlled unit must be within an interaction-range sphere of the target. This is also
  the general rule going forward - "any player pawn trading items with an NPC or
  container must be close to it," already true for containers, now also true for looting.
- No "get up" animation exists in the content pack. The downed hold+recover is built from
  the Death anim via a two-section Montage (fall once, then loop the ground pose) rather
  than needing new animation content; blending back to locomotion on recovery is a plain
  `Montage_Stop`, accepting a small pop as a v1 tradeoff.
- Attack damage timing uses an `AnimNotify` at the montage's hit frame, not a raw timer -
  the standard Unreal idiom for decoupling "when the swing looks like it connects" from
  "when damage actually applies."

Approach

1. `UHealthComponent` (new component, mirrors `UInventoryComponent`)

New files `Source/smores/Variant_Strategy/Combat/HealthComponent.h/.cpp`:

```
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UHealthComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = 0, Units = "s"))
    float DownedDurationSeconds = 15.0f;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamage(float Amount);

    UFUNCTION(BlueprintPure, Category = "Health")
    bool IsDowned() const { return bIsDowned; }

    UFUNCTION(BlueprintPure, Category = "Health")
    float GetHealth() const { return Health; }

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthDownedDelegate OnDowned;

    UPROPERTY(BlueprintAssignable, Category = "Health")
    FOnHealthRecoveredDelegate OnRecovered;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "Health")
    float Health = 100.0f;

    bool bIsDowned = false;

    FTimerHandle RecoveryTimerHandle;

    void Downed();
    void Recover();
};
```

`TakeDamage`: no-op if `bIsDowned` or `Amount <= 0`; otherwise
`Health = FMath::Max(Health - Amount, 0.0f)`; if `Health <= 0.0f` call `Downed()`.
`BeginPlay`: `Health = MaxHealth`. `Downed()`: `bIsDowned = true`, broadcast `OnDowned`,
start `RecoveryTimerHandle` for `DownedDurationSeconds` bound to `Recover`. `Recover()`:
`Health = MaxHealth`, `bIsDowned = false`, broadcast `OnRecovered`.
`DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthDownedDelegate)` /
`FOnHealthRecoveredDelegate` alongside the class, same pattern as
`FOnInventoryChangedDelegate` in `InventoryComponent.h`.

2. Disposition, engagement, and downed reaction on `AStrategyUnit`

`StrategyUnit.h`: add

```
UENUM(BlueprintType)
enum class EStrategyDisposition : uint8
{
    Passive,
    Aggressive
};
```

New members: `TObjectPtr<UHealthComponent> Health` (constructed like `Inventory`),
`EStrategyDisposition Disposition = EStrategyDisposition::Passive`,
`TArray<TObjectPtr<UAnimMontage>> AttackMontages` (EditAnywhere, expects the 3 wrapped
`MM_Attack_0X` montages), `TObjectPtr<UAnimMontage> DownedMontage` (EditAnywhere),
`float AttackRange = 150.0f` (EditAnywhere, cm), `TWeakObjectPtr<AStrategyUnit>
CurrentAttackTarget`, `bool bAttackOnArrival = false`, `TWeakObjectPtr<AStrategyUnit>
PendingAttackTarget`, `FTimerHandle AggroRetargetTimerHandle`.

New public API: `UHealthComponent* GetHealth() const`, `bool IsDowned() const` (forwards
to `Health->IsDowned()`), `bool IsAggressive() const`, `void SetAggressive(bool
bAggressive)`, `bool IsUnitInRange(const AStrategyUnit* Unit) const` (mirrors
`AStrategyContainer::IsUnitInRange` - checks distance against this unit's own existing
`InteractionRange` sphere component, StrategyUnit.h:31-32, which is currently created but
never used for a distance check), `void AttackTarget(AStrategyUnit* Target)`,
`void ApplyAttackDamage()` (public - called by the AnimNotify below).

Add `virtual void BeginPlay() override` (new - `AStrategyUnit` has none today): after
`Super::BeginPlay()`, bind `Health->OnDowned.AddDynamic(this,
&AStrategyUnit::OnHealthDowned)` and `Health->OnRecovered.AddDynamic(this,
&AStrategyUnit::OnHealthRecovered)`.

`SetAggressive(bool bAggressive)`: sets `Disposition`; if turning Aggressive, start a
repeating timer (~0.5s) on `AggroRetargetTimerHandle` calling `TryEngageNearestPlayerPawn`
(and call it once immediately); if turning Passive, clear that timer and
`CurrentAttackTarget`/`PendingAttackTarget`.

`TryEngageNearestPlayerPawn()`: gather all `AStrategyPlayerUnit` in the level
(`UGameplayStatics::GetAllActorsOfClass`, same pattern as
`AStrategyPlayerController::RefreshPlayerPawns`), skip any where `IsDowned()`, pick
nearest by `FVector::DistSquared`. If found, `AttackTarget(Nearest)`. If none found,
`SetAggressive(false)` - this is reset trigger #2.

`AttackTarget(AStrategyUnit* Target)`: no-op if `!IsValid(Target) || Target->IsDowned()`.
If `FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= AttackRange`, call
`PerformAttack(Target)` directly. Otherwise set `bAttackOnArrival = true`,
`PendingAttackTarget = Target`, and call the existing `MoveToLocation(Target-
>GetActorLocation(), false, {})` (passing `bInteract=false` - this is a separate arrival
path from the existing interact-on-arrival one, see below).

`HandleMoveFinished()` (StrategyUnit.cpp:181-220): check `bAttackOnArrival` *before* the
existing `bInteractOnArrival` branch - if set, clear the flag and call
`AttackTarget(PendingAttackTarget.Get())` again (now in range, so this re-entry attacks
immediately via the branch above). Existing `bInteractOnArrival` behavior is untouched.

`PerformAttack(AStrategyUnit* Target)` (private): `SetActorRotation(FindLookAtRotation(...))`
toward `Target` (same helper `Interact()` already uses); set `CurrentAttackTarget =
Target`; play a random entry from `AttackMontages` via
`GetMesh()->GetAnimInstance()->Montage_Play(...)`; bind (or reuse a single bound handler
for) `Montage_Play`'s returned handle to `UAnimInstance::OnMontageEnded` so that, once the
montage finishes, if `Disposition == Aggressive` and `Target` is still valid and not
Downed, call `AttackTarget(Target)` again (continues the auto-attack loop) - this is what
makes combat "resolve continuously once engaged" per the design pillar, symmetric for
player-issued attacks and NPC self-attacks alike. A player-issued attack that finishes
because the target went Downed simply stops (nothing to continue).

`ApplyAttackDamage()` (called by the AnimNotify, see step 3): validates
`CurrentAttackTarget.IsValid()` and it's still within `AttackRange` (re-check at the hit
frame, not the swing-start frame, so a target that fled mid-swing doesn't take a phantom
hit); if valid, `CurrentAttackTarget->GetHealth()->TakeDamage(25.0f)`.

`OnHealthDowned()` (bound to `Health->OnDowned`): `StopMoving()`; clear
`CurrentAttackTarget`/`PendingAttackTarget`/`bAttackOnArrival` (stop being anyone's live
attack target and stop this unit's own attack loop); play `DownedMontage`.

`OnHealthRecovered()` (bound to `Health->OnRecovered`):
`GetMesh()->GetAnimInstance()->Montage_Stop(0.25f, DownedMontage)` (blends back to
locomotion - accepted small pop, no "get up" anim exists); if `Disposition ==
EStrategyDisposition::Aggressive`, call `SetAggressive(false)` - this is reset trigger #1.

3. `AStrategyPlayerController`: attack input, attack command, loot generalization

New `UInputAction* AttackAction` (Input category, alongside the other `IA_Strategy_*`
properties). Bind in `SetupInputComponent` (StrategyPlayerController.cpp:68-154, desktop-
only like `CyclePawnAction`/`ToggleInventoryAction`):
```
if (AttackAction)
{
    EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &AStrategyPlayerController::AttackKeyPressed);
}
```

New `void AttackKeyPressed(const FInputActionValue&)`: if `SelectedNPC` is valid and
`!SelectedNPC->IsAggressive()`, call `DoAttackCommand(SelectedNPC)`. No-op otherwise
(covers "no effect on a selected container or player pawn" for free, since those never
populate `SelectedNPC`).

`DoSelectCommand` (StrategyPlayerController.cpp:828-832), change the NPC branch:
```
else
{
    // NPCs are targetable (highlighted, shown in the selection label) but never commandable
    SetSelectedNPC(NearestUnit);

    // an already-aggressive NPC is attacked directly by the same click that targets it
    if (NearestUnit->IsAggressive())
    {
        DoAttackCommand(NearestUnit);
    }
}
```

New `void DoAttackCommand(AStrategyUnit* Target)`: no-op if `!IsValid(Target) ||
Target->IsDowned()`. Call `Target->SetAggressive(true)` (harmless if already Aggressive -
this is what flips a Passive NPC on the A-key path). Loop `ControlledUnits`, calling
`CurrentUnit->AttackTarget(Target)` for every valid one - every selected unit engages the
same target, unlike `DoMoveUnitsCommand`'s spread-to-nearby-points formation logic.

Loot generalization - extend the existing "O" key path rather than building a new one:

New `AStrategyUnit* FindLootableNPCInRange() const`: if `SelectedNPC` is valid and
`SelectedNPC->IsDowned()`, loop `ControlledUnits` calling `SelectedNPC->IsUnitInRange
(CurrentUnit)`; return `SelectedNPC` on the first match, else `nullptr`. Mirrors
`FindContainerInRange`'s shape (StrategyPlayerController.cpp:1127-1161).

`ToggleContainer` (StrategyPlayerController.cpp:460-481): after `FindContainerInRange()`
returns null, also try `FindLootableNPCInRange()`; if that returns a unit, call a new
`OpenLoot(AStrategyUnit* LootTarget)` instead of `OpenContainer`. `OpenLoot` mirrors
`OpenContainer`'s body (StrategyPlayerController.cpp:496-530) but sources
`LootTarget->GetInventory()` / a title built from `LootTarget->GetUnitDisplayName()`
(e.g. `"{0} (Downed)"`) instead of `AStrategyContainer`'s equivalents, reusing the same
`ContainerWidget`/`ContainerWidgetClass` - this means the existing paired-inventory-
opening and drag-and-drop transfer code (from `9-6-26-inventory-drag-drop-grid.md`) works
on a Downed NPC's items with zero further changes. `CloseContainer` is unchanged - loot
uses the same widget instance, so the same close key/behavior applies.

4. New `AnimNotify` for attack-hit timing

New files `Source/smores/Variant_Strategy/Combat/AnimNotify_AttackHit.h/.cpp`:
```
UCLASS()
class UAnimNotify_AttackHit : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
```
`Notify()`: cast `MeshComp->GetOwner()` to `AStrategyUnit`, call `ApplyAttackDamage()` if
valid.

5. Animation/content wiring (fork, per CLAUDE.md MCP token-discipline guidance)

- Wrap `MM_Attack_01`, `_02`, `_03` into 3 separate `UAnimMontage` assets (`AM_Attack_01/
  02/03`, matching the existing `MM_Pistol_Fire_Montage` naming precedent), each with one
  `UAnimNotify_AttackHit` placed at the swing-connect frame.
- Build `AM_Downed` from one `MM_Death_Front_0X` sequence: a "Fall" section plays it once;
  a "Ground" section loops a short clip of its final pose (`bLooping` true, own
  `NextSectionName`, `bEnableAutoBlendOut` false) so it holds until code calls
  `Montage_Stop`.
- Set `AttackMontages` (all 3), `DownedMontage`, `AttackRange`, and confirm
  `DownedDurationSeconds` reads 15 on every unit Blueprint (NPC and player pawn Blueprints
  alike, since these fields live on the shared `AStrategyUnit` base).
- Create `IA_Strategy_Attack` (Boolean `UInputAction`, duplicate an existing Boolean IA
  per the CLAUDE.md IMC caveat) and assign it to the player controller Blueprint's new
  `AttackAction` property. **Hand the actual A-key binding in `IMC_Strategy_Mouse` to the
  user** - per CLAUDE.md, IMC key mappings can't be safely round-tripped via MCP.
- No new widget assets needed for loot - `ContainerWidgetClass` is reused as-is.
- Get user confirmation before saving changes to these project assets, per CLAUDE.md.

Build order

1. Write all new C++: `Combat/HealthComponent.h/.cpp`, `Combat/AnimNotify_AttackHit.h/.cpp`,
   the `StrategyUnit.h/.cpp` changes (new `EStrategyDisposition` UENUM, new `BeginPlay`,
   new members/methods above), the `StrategyPlayerController.h/.cpp` changes above.
2. Close the editor, cold build via Visual Studio (new `UCLASS`/`UENUM` - Live Coding
   won't register them).
3. Reopen the editor, do the content/input wiring pass (step 5) in one fork.
4. `/compact` if the wiring fork's MCP output bloated context.

Verification (PIE)

- Select a Passive NPC, press A: it flips Aggressive, and every currently-selected unit
  moves in and starts swinging; damage lands in 25-point steps.
- Left-click an already-Aggressive NPC (without pressing A): same attack command fires
  directly.
- A key does nothing when a container or a player pawn is selected instead of a Passive
  NPC.
- Four hits downs either side; the Downed unit falls and holds a grounded pose for 15
  seconds, then stands and is back to full health.
- While Downed, further attacks against that unit are no-ops (health doesn't drop below
  0/no double-downing), but it's still selectable and still shows in the selection label.
- Walk a pawn within range of a Downed NPC and press O: its loot panel opens (title reads
  "<Name> (Downed)"), paired with the closest pawn's own inventory, and items can be
  dragged between them exactly like a container.
- Trying to loot a Downed NPC while no controlled unit is within its interaction range
  does nothing (same proximity rule as containers).
- An Aggressive NPC with no target selected by the player still seeks out and attacks the
  nearest non-Downed player-controlled pawn on its own.
- Down every player-controlled pawn against one Aggressive NPC: once all are Downed, that
  NPC reverts to Passive (stops hunting).
- Down the Aggressive NPC itself (player wins the fight); once it recovers 15 seconds
  later, it's Passive again, not still hunting.
- An NPC that stays Aggressive throughout (never itself Downed, always has a valid
  target) never reverts on its own.
