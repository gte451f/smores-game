# Combat

## Purpose

Real-time melee combat shared by every `AStrategyUnit` — NPCs and player pawns alike, since
`AStrategyPlayerUnit` inherits it unchanged. Covers health, damage, the Alive/Downed/Dead state
machine, NPC self-initiated hunting, player-issued squad attacks, and auto-retaliation for a unit
hit while otherwise idle. Looting a body's inventory is a related but separate system — see
`inventory.md`.

## Player Surface

- Selecting an NPC (`SelectedNPC`) and pressing the Attack key (`AttackAction`, bound to `H` in
  current testing) commands every currently selected unit (`ControlledUnits`) to attack it,
  provided the NPC isn't already Aggressive.
- **The Attack key is now the only way to issue an attack.** Clicking an already-Aggressive NPC
  used to re-issue the same squad-wide attack immediately; that was removed, because it broke
  the settled rule that a single click only ever selects what's under the cursor — and because
  the double-click interact gesture fires a select click alongside it, so talking to a hostile
  NPC would have started a fight. See `input-and-keybinds.md`'s "Existing defaults worth
  revisiting".
- Any unit that takes damage while not already fighting someone swings back at whoever hit it —
  this applies even to a bystander pawn that was never part of the original squad command.
- A unit at zero health goes Downed: it falls, holds a grounded pose, can't move, can't rotate to
  face anyone, can't attack or be attacked further, and automatically recovers to full health
  after a fixed delay (`DownedDurationSeconds`, default 15s).
- A unit can also be **Dead**, which looks and behaves exactly like Downed — same grounded pose,
  same inertness, same lootability — except that no recovery is coming. **Nothing in combat
  currently kills anything**: a lethal hit still goes Downed, as it always has. Dead is reachable
  only through `UHealthComponent::Kill()`, exercised by the `SmoresKillNPC` console exec. What
  *should* kill a unit in play — bleeding out while Downed, a finishing blow, a damage threshold
  — is an undecided design question (`character-death-and-permadeath.md` in the game-design skill
  is still a placeholder). The state and the transition are built so that looting a body had
  something real to gate on; whoever settles that rule calls `Kill()`.

## Core Rules

- Every `AStrategyUnit` owns a `UHealthComponent` (`Health`/`MaxHealth`, `DownedDurationSeconds`,
  `HealthState`).
- **State is one replicated `EHealthState { Alive, Downed, Dead }`, not a pair of bools.** Two
  bools would encode four combinations, one of which (Downed *and* Dead) is meaningless, and every
  caller would have to check them in the right order. Three queries read it: `IsDowned()`,
  `IsDead()`, and **`IsIncapacitated()` (Downed or Dead), which is what nearly every gameplay
  check actually wants** — a unit that can't move, fight, be fought, or get up on its own, and
  that can be looted. Reach for the first two only where the difference genuinely matters, which
  today is exactly one place: the loot window's title.
- `TakeDamage(Amount, DamageInstigator)` no-ops while incapacitated or if `Amount <= 0`.
  Otherwise it subtracts `Amount`, then either goes Downed (health at or below zero) or
  broadcasts `OnDamaged(DamageInstigator)` for a hit that was survived.
- Going Downed broadcasts `OnDowned` and starts a one-shot recovery timer; the timer calls
  `Recover()`, which restores full health and broadcasts `OnRecovered`.
- `Kill()` is authority-only, reachable from Alive *or* from Downed, and terminal. It cancels any
  recovery timer in flight (otherwise the corpse stands back up a few seconds later), zeroes
  health, sets `Dead`, and broadcasts `OnDied`. `Recover()` additionally refuses to run while Dead,
  as a second line of defence against a recovery already queued on the timer manager.
- `AStrategyUnit` reacts to its own `Health` component's delegates:
  - `OnHealthDowned` stops movement (including cancelling any live `AIController` move request),
    clears its current/pending attack-target state and its self-hunting retarget timer, and plays
    `DownedMontage`.
  - `OnHealthRecovered` stops the Downed montage and, if the unit itself was Aggressive, resets
    it back to Passive.
  - `OnHealthDied` does exactly what `OnHealthDowned` does — stop moving, drop out of every
    attack loop, play the grounded montage. What makes it death rather than a knockdown is
    entirely that no `OnRecovered` ever follows, so none of it gets undone.
  - `OnHealthDamaged` is the auto-retaliation hook: if the unit isn't already mid-engagement (no
    `CurrentAttackTarget`, not `bAttackOnArrival`) and the instigator is a valid
    `AStrategyUnit` that isn't itself incapacitated, it calls `AttackTarget(Attacker)` — so a
    unit never swings back at a body. Applies uniformly regardless of whether the unit is
    selected, player-controlled, or an NPC.
- `Disposition` (`Passive`/`Aggressive`) drives NPC self-initiated hunting. `SetAggressive(true)`
  starts a repeating 0.5s timer (`AggroRetargetTimerHandle`) driving
  `TryEngageNearestPlayerPawn()`, which finds the nearest still-standing `AStrategyPlayerUnit` and
  calls `AttackTarget` on it (skipped if already engaged with that same target).
  `SetAggressive(false)` clears the timer and any attack target.
- **An Aggressive unit with nothing to fight stands itself back down.** If
  `TryEngageNearestPlayerPawn()` finds no still-standing player pawn at all, it calls
  `SetAggressive(false)` on itself — so hostility ends when the last target dies or goes Downed,
  without anything else having to notice. This is the *second* reset trigger, alongside
  `OnHealthRecovered` resetting a recovering unit to Passive; both exist so nothing stays
  permanently hostile with no one to be hostile at.
  - **The consequence that surprises people:** `SetAggressive(true)` on a unit in a world with no
    player pawns does not stick. It is Aggressive for up to 0.5s and then Passive again, because
    the first retarget tick finds nothing and stands it down. It looks exactly like the call
    having been ignored. This bit an automation test, and is why
    `ATestStrategyNPC::MakeHostileForTest` sets `Disposition` directly instead — see
    `testing.md`.
  - `Disposition` is *not* replicated, so this is server-side state; anything a client needs to
    know about hostility has to come from something that is.
- `AttackTarget(Target)` bails early if the attacker or `Target` is incapacitated (Downed or
  Dead — neither a corpse nor a knocked-down unit fights, or is worth swinging at), or `Target`
  is invalid. In range, it calls `PerformAttack` immediately; out of range, it issues a
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
  it's still valid and not incapacitated — this self-perpetuating loop drives both NPC
  self-hunting and player-issued attacks.
- Player-issued attacks: `AStrategyPlayerController::DoAttackCommand(Target)` sets
  `Target->SetAggressive(true)` then calls `AttackTarget(Target)` on every unit in
  `ControlledUnits` — a squad-wide engage, distinct from move commands' spread-to-formation
  behavior.
- An incapacitated unit is fully inert: `MoveToLocation` and `AttackTarget` both refuse to act on
  it (as attacker or target), and `Interact()` skips rotating it to face whoever interacts with
  it. Every one of those guards reads `IsIncapacitated()`, so Dead inherited the whole set for
  free rather than needing a parallel check added at each site.

## C++ Implementation

- **Primary classes:** `UHealthComponent`, `AStrategyUnit`, `AStrategyPlayerUnit` (inherits
  combat unchanged), `AStrategyPlayerController`, `UAnimNotify_AttackHit`
- **Important methods:**
  - `UHealthComponent::TakeDamage` — applies damage, resolves Downed vs. survived-hit
  - `UHealthComponent::Downed` / `Recover` — private; broadcast `OnDowned`/`OnRecovered` and
    manage the recovery timer
  - `UHealthComponent::Kill` — the one transition into `Dead`; authority-only and terminal
  - `UHealthComponent::IsIncapacitated` — Downed-or-Dead, the query nearly all gameplay uses
  - `UHealthComponent::OnRep_HealthState` — non-authority machines' reaction to a replicated
    state change, dispatching `OnDowned`/`OnDied`/`OnRecovered` by the new state
  - `AStrategyUnit::AttackTarget` — in-range swing vs. move-then-swing, with Downed guards
  - `AStrategyUnit::PerformAttack` — plays a random attack montage, binds the end delegate
  - `AStrategyUnit::ApplyAttackDamage` — called by `UAnimNotify_AttackHit` at the hit frame
  - `AStrategyUnit::OnAttackMontageEnded` — the auto-attack continuation loop
  - `AStrategyUnit::SetAggressive` / `TryEngageNearestPlayerPawn` — NPC self-hunting
  - `AStrategyUnit::OnHealthDowned` / `OnHealthRecovered` / `OnHealthDied` / `OnHealthDamaged`
    — reactions to the owned `Health` component's delegates
  - `AStrategyPlayerController::SmoresKillNPC` (console exec) — kills the currently-targeted NPC
    via `Server_DebugKill`, since health state is server-owned. The only way to reach `Dead`
  - `AStrategyPlayerController::DoAttackCommand` — squad-wide player-issued attack
  - `AStrategyPlayerController::AttackKeyPressed` — entry point from input, gated on `SelectedNPC`
- **Runtime ownership:** `UHealthComponent` is a default subobject of `AStrategyUnit`, alongside
  `Inventory`.
- **Data flow (player-issued):** Attack key (`H`) with an NPC targeted →
  `DoAttackCommand(Target)` → `Target->SetAggressive(true)` + `AttackTarget()` on each
  `ControlledUnits` member → in range: `PerformAttack` → `Montage_Play` +
  `Montage_SetEndDelegate` → `UAnimNotify_AttackHit` fires `ApplyAttackDamage` → `TakeDamage` →
  `OnDamaged` (auto-retaliate) or `Downed` (`OnHealthDowned`) → `OnAttackMontageEnded` re-swings
  or stops the loop.
- **Data flow (NPC self-hunting):** `SetAggressive(true)` → repeating
  `TryEngageNearestPlayerPawn` timer → `AttackTarget()` on the nearest still-standing player pawn,
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
  target. Faction standing is now *stored* (`factions.md`) but units have no faction yet and
  nothing reads it — deriving hostility from it is exactly this extension point.
- `OnHealthDamaged`'s auto-retaliation always targets the single instigator of the most recent
  hit; there's no threat table for multi-attacker scenarios.
- `AttackTarget`'s out-of-range branch reuses the same `MoveToLocation` path as player move
  commands (EQS-refined destination), so any future movement-behavior change affects combat
  approach too.
- **`UHealthComponent`'s four delegates now have a second consumer, and it wants the victim's
  identity.** `AStrategyUnit` binds them for its own behavior (retaliation, going inert); the HUD's
  activity feed binds them through `USquadActivityWatcher` to write the fight record. Two things
  worth knowing before changing them:
  - **Three of the four carry no parameters** (`OnDowned`, `OnRecovered`, `OnDied`), so a listener
    cannot tell *who* the event was about. That is the entire reason the feed needs one watcher
    object per unit rather than four handlers on the player controller. Adding the owner as a
    parameter would collapse that; it would also change every existing binding, so it is a
    deliberate decision rather than a tidy-up.
  - **They fire on whichever machine ran the damage**, which is the server, since `TakeDamage` is
    authority-gated. Anything client-side that listens to them therefore works standalone and on a
    listen server's own screen and nowhere else — see `hud-and-panels.md`'s Known Gaps.

## Known Gaps

- **`Disposition` is not replicated, and something on the HUD now reads it.** `AStrategyUnit`
  declares no `GetLifetimeReplicatedProps` at all, so hostility is server-only state. Health is
  replicated (`UHealthComponent` has both `OnRep_`s), so "is this a body?" is correct everywhere —
  but "is this person hostile?" is correct only on the server or a listen-server host.
  **`AStrategyPlayerController::BuildTargetInfo` calls `IsAggressive()` client-side**, to classify
  the target as `PERSON - HOSTILE` and to decide whether the target panel's Talk and Attack
  buttons are enabled. On a remote client every NPC would therefore read as neutral, Talk would
  look available against someone actively attacking you, and Attack would look available against
  someone already fighting.
  - **This is a display defect, not a security hole.** Every rule that matters is re-checked
    server-side — `TryTradeItem` re-checks hostility before a single item moves, and
    `Server_AttackCommand` runs on the server — so a client acting on the wrong-looking row gets
    refused, not rewarded.
  - **It is latent**: multiplayer isn't wired up or testable yet, and in a single-player PIE
    session the client *is* the authority, so nothing about it is visible today.
  - **The fix is one line plus a `DOREPLIFETIME`** — mark `Disposition` `UPROPERTY(Replicated)`
    and give `AStrategyUnit` a `GetLifetimeReplicatedProps`. Worth doing with the first real co-op
    session rather than speculatively, but it should not be *discovered* then. Per
    `multiplayer-discipline.md`, this is the "decide who owns new state before writing it" rule
    catching up with state that predates the rule.

- Damage is a flat, hardcoded `25` per hit; no weapon or damage-type variation.
- **Nothing in the damage path ever kills.** `Dead` exists, replicates, and is honoured by every
  gameplay guard, but a lethal hit still goes Downed — the only route to `Dead` is the
  `SmoresKillNPC` debug exec. A deliberate boundary, not an oversight: this slice needed the state
  so bodies could be looted, and what *should* kill a unit is an undecided design question.
- **A dead unit never despawns.** It stays in the level permanently, holding its inventory. Body
  lifetime hasn't been designed.
- No threat table — auto-retaliation only ever targets the most recent instigator, dropping any
  earlier attacker.
- `AttackMontages` is currently populated with only 2 montages (`AM_Attack_01`/`02`) after
  `AM_Attack_03` was removed from both unit Blueprints following a previously-diagnosed animation
  glitch; the doc comment on `AttackMontages` in `StrategyUnit.h` still says "Expects the 3
  wrapped MM_Attack_0X montages."
- No ranged/projectile combat path — `AttackRange` is purely a melee proximity check.
- Combat and looting still read health state from opposite sides: combat asks
  `IsIncapacitated()` to decide inertness, looting asks
  `AStrategyPlayerController::IsLootableNPC`. `IInventoryHolder` unified the *proximity* half of
  what they share (see `inventory.md`) but deliberately not the state half — what counts as
  lootable is an inventory question, what counts as inert is a combat one, and they only happen
  to agree today.
