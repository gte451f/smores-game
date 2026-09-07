# Combat

## Purpose

Real-time melee combat shared by every `AStrategyUnit` — NPCs and player pawns alike, since
`AStrategyPlayerUnit` inherits it unchanged. Covers health, damage, the Downed/recovery cycle,
NPC self-initiated hunting, player-issued squad attacks, and auto-retaliation for a unit hit
while otherwise idle. Looting a Downed NPC's inventory is a related but separate system — see
`inventory.md`.

## Player Surface

- Selecting an NPC (`SelectedNPC`) and pressing the Attack key (`AttackAction`, bound to `H` in
  current testing) commands every currently selected unit (`ControlledUnits`) to attack it,
  provided the NPC isn't already Aggressive.
- Clicking directly on an NPC that's already Aggressive re-issues the same squad-wide attack
  immediately, without needing the Attack key again.
- Any unit that takes damage while not already fighting someone swings back at whoever hit it —
  this applies even to a bystander pawn that was never part of the original squad command.
- A unit at zero health goes Downed: it falls, holds a grounded pose, can't move, can't rotate to
  face anyone, can't attack or be attacked further, and automatically recovers to full health
  after a fixed delay (`DownedDurationSeconds`, default 15s).

## Core Rules

- Every `AStrategyUnit` owns a `UHealthComponent` (`Health`/`MaxHealth`, `DownedDurationSeconds`,
  `IsDowned()`).
- `TakeDamage(Amount, DamageInstigator)` no-ops while already Downed or if `Amount <= 0`.
  Otherwise it subtracts `Amount`, then either goes Downed (health at or below zero) or
  broadcasts `OnDamaged(DamageInstigator)` for a hit that was survived.
- Going Downed broadcasts `OnDowned` and starts a one-shot recovery timer; the timer calls
  `Recover()`, which restores full health and broadcasts `OnRecovered`.
- `AStrategyUnit` reacts to its own `Health` component's delegates:
  - `OnHealthDowned` stops movement (including cancelling any live `AIController` move request),
    clears its current/pending attack-target state and its self-hunting retarget timer, and plays
    `DownedMontage`.
  - `OnHealthRecovered` stops the Downed montage and, if the unit itself was Aggressive, resets
    it back to Passive.
  - `OnHealthDamaged` is the auto-retaliation hook: if the unit isn't already mid-engagement (no
    `CurrentAttackTarget`, not `bAttackOnArrival`) and the instigator is a valid, non-Downed
    `AStrategyUnit`, it calls `AttackTarget(Attacker)`. Applies uniformly regardless of whether
    the unit is selected, player-controlled, or an NPC.
- `Disposition` (`Passive`/`Aggressive`) drives NPC self-initiated hunting. `SetAggressive(true)`
  starts a repeating 0.5s timer (`AggroRetargetTimerHandle`) driving
  `TryEngageNearestPlayerPawn()`, which finds the nearest non-Downed `AStrategyPlayerUnit` and
  calls `AttackTarget` on it (skipped if already engaged with that same target).
  `SetAggressive(false)` clears the timer and any attack target.
- `AttackTarget(Target)` bails early if the attacker or `Target` is Downed, or `Target` is
  invalid. In range, it calls `PerformAttack` immediately; out of range, it issues a
  `MoveToLocation` toward `Target` and sets `bAttackOnArrival`/`PendingAttackTarget` so
  `HandleMoveFinished` re-issues the attack on arrival.
- `PerformAttack` rotates the attacker to face `Target`, sets `CurrentAttackTarget`, and plays a
  random montage from `AttackMontages`, binding a fresh per-swing `Montage_SetEndDelegate` rather
  than a persistent `BeginPlay`-time binding (an `AnimInstance` recreation elsewhere would
  silently orphan that binding — see the NOTE in `StrategyUnit.cpp`'s `BeginPlay`).
- `UAnimNotify_AttackHit`, placed at the swing-connect frame of each attack montage, calls
  `ApplyAttackDamage()` on the montage's owning unit. `ApplyAttackDamage` re-checks range at the
  hit frame (not the swing-start frame) and whiffs silently if the target moved out of
  `AttackRange` or is no longer valid; otherwise it calls `TakeDamage(25, this)` on the target.
- `OnAttackMontageEnded` ignores montages that aren't one of `AttackMontages` (e.g. the Downed
  montage ending) and otherwise keeps re-swinging `CurrentAttackTarget` automatically as long as
  it's still valid and not Downed — this self-perpetuating loop drives both NPC self-hunting and
  player-issued attacks.
- Player-issued attacks: `AStrategyPlayerController::DoAttackCommand(Target)` sets
  `Target->SetAggressive(true)` then calls `AttackTarget(Target)` on every unit in
  `ControlledUnits` — a squad-wide engage, distinct from move commands' spread-to-formation
  behavior.
- A Downed unit is fully inert: `MoveToLocation` and `AttackTarget` both refuse to act on it (as
  attacker or target), and `Interact()` skips rotating it to face whoever interacts with it.

## C++ Implementation

- **Primary classes:** `UHealthComponent`, `AStrategyUnit`, `AStrategyPlayerUnit` (inherits
  combat unchanged), `AStrategyPlayerController`, `UAnimNotify_AttackHit`
- **Important methods:**
  - `UHealthComponent::TakeDamage` — applies damage, resolves Downed vs. survived-hit
  - `UHealthComponent::Downed` / `Recover` — private; broadcast `OnDowned`/`OnRecovered` and
    manage the recovery timer
  - `AStrategyUnit::AttackTarget` — in-range swing vs. move-then-swing, with Downed guards
  - `AStrategyUnit::PerformAttack` — plays a random attack montage, binds the end delegate
  - `AStrategyUnit::ApplyAttackDamage` — called by `UAnimNotify_AttackHit` at the hit frame
  - `AStrategyUnit::OnAttackMontageEnded` — the auto-attack continuation loop
  - `AStrategyUnit::SetAggressive` / `TryEngageNearestPlayerPawn` — NPC self-hunting
  - `AStrategyUnit::OnHealthDowned` / `OnHealthRecovered` / `OnHealthDamaged` — reactions to the
    owned `Health` component's delegates
  - `AStrategyPlayerController::DoAttackCommand` — squad-wide player-issued attack
  - `AStrategyPlayerController::AttackKeyPressed` — entry point from input, gated on `SelectedNPC`
- **Runtime ownership:** `UHealthComponent` is a default subobject of `AStrategyUnit`, alongside
  `Inventory`.
- **Data flow (player-issued):** Attack key or click on an Aggressive NPC →
  `DoAttackCommand(Target)` → `Target->SetAggressive(true)` + `AttackTarget()` on each
  `ControlledUnits` member → in range: `PerformAttack` → `Montage_Play` +
  `Montage_SetEndDelegate` → `UAnimNotify_AttackHit` fires `ApplyAttackDamage` → `TakeDamage` →
  `OnDamaged` (auto-retaliate) or `Downed` (`OnHealthDowned`) → `OnAttackMontageEnded` re-swings
  or stops the loop.
- **Data flow (NPC self-hunting):** `SetAggressive(true)` → repeating
  `TryEngageNearestPlayerPawn` timer → `AttackTarget()` on the nearest non-Downed player pawn,
  same swing loop as above.

## Blueprint / Asset Dependencies

- Unit Blueprint subclasses (`BP_StrategyUnit`, `BP_PlayerUnit`) assign `AttackMontages` (an
  array, one chosen at random per swing) and `DownedMontage`.
- Each attack montage needs a `UAnimNotify_AttackHit` placed at its swing-connect frame.
- `DownedMontage` and every attack montage must share the same anim slot
  (`DefaultGroup.DefaultSlot`) — a slot mismatch was previously the root cause of what looked like
  a corrupted/glitched attack animation, regardless of which montage asset played, because two
  montages targeting different slots can stomp each other's pose on the same `AnimInstance`.
- `AttackRange`, `MaxHealth`, `DownedDurationSeconds` are `EditAnywhere` and tunable per-Blueprint.
- Strategy player controller Blueprint assigns `AttackAction` to an `IA_*` input asset, bound to a
  key in the mouse mapping context.

## Extension Points

- `ApplyAttackDamage`'s damage amount (currently a hardcoded `25`) could become a `UPROPERTY` for
  per-unit or per-weapon damage variation.
- `Disposition` is only ever toggled by `SetAggressive`; a faction/allegiance system could gate
  who auto-retaliates against whom, rather than every unit treating every other unit as a valid
  target.
- `OnHealthDamaged`'s auto-retaliation always targets the single instigator of the most recent
  hit; there's no threat table for multi-attacker scenarios.
- `AttackTarget`'s out-of-range branch reuses the same `MoveToLocation` path as player move
  commands (EQS-refined destination), so any future movement-behavior change affects combat
  approach too.

## Known Gaps

- Damage is a flat, hardcoded `25` per hit; no weapon or damage-type variation.
- No death state — a unit cycles Downed → Recover indefinitely and can never permanently die.
- No threat table — auto-retaliation only ever targets the most recent instigator, dropping any
  earlier attacker.
- `AttackMontages` is currently populated with only 2 montages (`AM_Attack_01`/`02`) after
  `AM_Attack_03` was removed from both unit Blueprints following a previously-diagnosed animation
  glitch; the doc comment on `AttackMontages` in `StrategyUnit.h` still says "Expects the 3
  wrapped MM_Attack_0X montages."
- No ranged/projectile combat path — `AttackRange` is purely a melee proximity check.
- Combat and looting both gate on the Downed state but are implemented as fully separate systems
  (see `inventory.md`), with no shared "interactable while Downed" interface.
