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
  Components on an `APlayerState` replicate exactly as they do on a pawn.

## Known Gaps

Session/connect flow, dedicated server packaging, and the Linux cross-compile toolchain
itself aren't built yet — see the `game-design` skill's `multiplayer-and-content.md` for
what's scheduled vs. deferred. Nothing has been tested with an actual second client, so
these rules are followed on inspection rather than proven by play.
