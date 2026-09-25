# Multiplayer Discipline

## Purpose

Co-op is designed in from day one (self-hosted listen-server or dedicated server, up to 8
players, server-authoritative simulation — see the `game-design` skill's
`multiplayer-and-content.md` for the design intent), even though multiplayer itself isn't
wired up or testable yet. Unreal's built-in networking (replication, RPCs, server
authority) is meant to supply the large majority of what multiplayer needs; the
responsibility on the code side is **discipline now**, since retrofitting these habits
later is far more expensive than following them from the start.

Unlike most topics in this skill, this one isn't documenting a finished system — it's the
standing set of rules every piece of gameplay code is written against. **Read it before
writing or changing gameplay state.**

## The Rules

- **No singleton-player assumptions.** Never assume there is exactly one
  `PlayerController`, camera, squad, or HUD in the world. Key state and lookups off the
  owning `PlayerController`/`PlayerState`, not a global/singleton reference — the single
  most expensive habit to retrofit later.
- **Gate shared-state mutation on authority.** Anything that changes world state other
  than the local player's own cosmetic/UI state — health, inventory, faction standing,
  squad membership, item ownership — must check `HasAuthority()` (or run through a
  `Server`-flagged RPC) before mutating it. Never assume client == server.
- **Replicate through the engine's mechanisms, not ad hoc sync.** Shared state goes
  through `UPROPERTY(Replicated)` + `GetLifetimeReplicatedProps` (with `RepNotify` where
  clients react to a change); actions go through RPCs (`Server`/`Client`/`NetMulticast`).
  Don't invent a custom sync path when replication already covers the case.
- **Decide data ownership before writing a system.** Before adding new gameplay state,
  decide who authoritatively owns it (usually the server) and who merely holds a
  replicated copy — this determines whether it needs to be `Replicated` at all.
- **Decide *whose* state it is, and put it on the matching actor.** Three homes, and picking
  the wrong one is the expensive kind of mistake:

  | Scope | Home | Example |
  |---|---|---|
  | One per pawn | a component on the pawn | `UHealthComponent`, `UInventoryComponent` |
  | One per player | a component on their `APlayerState` | `UWalletComponent` |
  | One per **session**, same for everybody | a component on the **`AGameStateBase`** | `UTimePaceComponent` |
  | One per player, cosmetic/UI only | the local `APlayerController` or a widget — **not replicated at all** | which HUD panels are open |
  | One per **local** player, client-side only | a **`ULocalPlayerSubsystem`** | `USmoresActivityLog` (the activity feed's record) |

  The GameState is the one most likely to be reached for wrongly, in both directions: it is
  not a convenient global for things that are really per-player, and a genuinely session-wide
  value put on a player state means eight players holding eight disagreeing copies.
- **A client can only RPC an actor it owns**, which in practice means its own
  `APlayerController` and its own pawn. It does **not** own the GameState, the GameMode, or
  another player's anything. So a client-initiated change to session-wide state routes
  *through its own controller*: widget → `Server_` RPC on the `APlayerController` → the
  authoritative component. `AStrategyPlayerController::Server_RequestPace` →
  `UTimePaceComponent::SetPace` is the worked example. A `Server_` RPC declared on the
  GameState itself compiles, runs on a listen server, and silently does nothing from a client
  — which is the worst possible failure shape, because it works for whoever is hosting.
- **Prefer a property the engine already replicates over one of your own.** The pace sets
  `UGameplayStatics::SetGlobalTimeDilation` on the server and stops there, because
  `AWorldSettings::TimeDilation` is itself replicated — so there is no multicast, and there
  shouldn't be one. Look for the engine-side property before adding a broadcast.
- **A "request" is not a setter, and the naming should say so.** `RequestPace` may be refused,
  arrives a round trip later, and must not be assumed to have taken effect: the pace buttons
  don't repaint themselves on click, they ask and let the next frame's replicated value move
  the readout. A widget that updates optimistically shows a state the world isn't in exactly
  when the player most needs the truth — and that is prediction, which this project has
  decided not to do speculatively (see the rule above).
- **Don't add prediction machinery speculatively.** The point-and-click command scheme
  (`Variant_Strategy`) is latency-tolerant by design — no client-side
  prediction/reconciliation unless a specific system proves it's needed.
- **Dedicated server hosting targets Linux**, cross-compiled from the same C++ source as
  the Windows client. Avoid Windows-only APIs/dependencies in gameplay code, and watch
  asset-reference case sensitivity (Linux is case-sensitive, Windows isn't) once a Linux
  cook is attempted.

## Related Guidance

- **Module boundaries aren't authority boundaries.** Unreal compiles the same module for
  client and server; server-only logic is expressed with `WITH_SERVER_CODE`/authority
  checks *inside* a module, not a separate module per side. See
  `unreal-module-organization.md`'s guiding principles.
- **Per-player state belongs in a replicated component**, not inline on the player
  state — also `unreal-module-organization.md` ("Framework Classes vs. Feature Modules").
  Components on an `APlayerState` replicate exactly as they do on a pawn, and so do
  components on an `AGameStateBase`.
- **A `ULocalPlayerSubsystem` makes the no-singleton rule structural rather than intended.** When
  client-side state is genuinely per-local-player, this is the best home available: it is keyed to
  a local player *by construction*, so there is no global to reach for and no way to write the
  singleton-player bug into it. `USmoresActivityLog` is the example, and its accessor is the other
  half of the pattern — `Get(const APlayerController*)` takes a controller rather than a world, so
  a remote controller correctly gets nothing back. **A producer that can't name a player is a
  producer about to assume there is only one**, which is why no world-context overload exists.
- **The HUD is the standing example of per-local-player state that is deliberately *not*
  replicated** — which panels a player has open is nobody else's business. `hud-and-panels.md`
  draws the contrast with the pace, which sits on the same screen and is shared by everyone.
  Being on the HUD says nothing about scope; decide it per piece of state.

## Worked Examples In The Codebase

Read these rather than re-deriving the pattern:

| Pattern | Where |
|---|---|
| Per-pawn replicated state, authority-gated mutators | `UHealthComponent` (`SmoresCombat`) |
| Per-player state on the player state | `UWalletComponent` on `AStrategyPlayerState` |
| Session-wide state on the game state | `UTimePaceComponent`, `UWorldFactionComponent` on `AStrategyGameState` |
| Per-player state replicated to its owner only | `UPlayerStandingComponent` on `AStrategyPlayerState` (`COND_OwnerOnly`) |
| Session-wide state that is deliberately **not** replicated, because clients already see its copy | `UCharacterRecordComponent` on `AStrategyGameState` - each record's replicated copy *is* its unit's components; see `game-data.md` |
| Session-wide state that is deliberately **not** replicated, because a client knowing it would be a leak | `UWorldSeedComponent` on `AStrategyGameState` - every roll that reads it is authority-only, and a client holding the seed could work out every chest's contents before opening one; see `game-data.md` |
| Client → own controller → authoritative component | `Server_RequestPace`, `Server_MoveUnits`, `Server_MoveInventoryItem` |
| Server → owning client, for a decision only the server could make | `Client_NotifyRefusal`, `Client_NotifyActivity` |
| Deliberately unreplicated local UI state | `AStrategyPlayerController::PanelWidgets` |
| Per-local-player client-side state, enforced by construction | `USmoresActivityLog` (`SmoresCore`) |

## Known Gaps

Session/connect flow, dedicated server packaging, and the Linux cross-compile toolchain
itself aren't built yet — see the `game-design` skill's `multiplayer-and-content.md` for
what's scheduled vs. deferred. Nothing has been tested with an actual second client, so
these rules are followed on inspection rather than proven by play.
