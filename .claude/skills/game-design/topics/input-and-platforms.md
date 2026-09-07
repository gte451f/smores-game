# Input, Platforms, and Storefronts

## Purpose

States the target input methods, platforms, and sales storefronts as an explicit,
single-source decision, rather than leaving it implied by whatever the current prototype
happens to support. This is a scope decision as much as a design one: smores is built and
maintained by a single developer, and the target list below is sized to that reality, not
to what Unreal Engine is technically capable of shipping to.

## Platforms

**Windows PC is the only committed launch platform.** It's where the genre's audience
already is, and it's the platform Unreal is developed and tested against first — every
other platform is additional surface area a solo developer has to personally maintain.

**Steam Deck is a near-free target, not a separate platform.** Deck runs Windows builds
through Proton rather than requiring a native Linux build. Reaching it costs three things
this project already wants for other reasons: no kernel-level anti-cheat, a UI legible at
1280×800, and full controller navigability (see **Input** below). Achieving those earns
Deck compatibility essentially as a side effect, not a dedicated port.

**Native Linux client support is explicitly out of scope for now.** A real native Linux
client build means a second driver/RHI stack to test and support — real ongoing cost for an
audience already served, on Steam, by the Proton path above. Revisit only if post-launch
demand actually shows up; don't build it speculatively. This is a client-only decision — it
does not apply to the dedicated server, which is a separate build target with none of a
client's rendering/driver surface. See `multiplayer-and-content.md` for the dedicated
server's own Linux hosting plan.

**Mobile (iOS/Android) is out of scope**, and not primarily an engine question — UE5.8 does
target both. The blocker is that this game's systems-dense, mouse/multi-window UI would
need a genuine second UX design pass for touch (not just remapped input), plus its own
performance tuning and storefront certification overhead. That's a multi-month project of
its own, not something a one-person team takes on alongside the main game. Revisit only if
team size or scope changes materially.

## Storefronts

**Sell as widely as possible first; store-specific extras come later, if ever.** The
priority is Windows availability across as many storefronts as reasonably possible, not
depth of integration with any one of them.

- **Steam, GOG, and the Epic Games Store are all intended launch storefronts.** For a bare
  Windows build with no store-specific SDK calls, publishing to all three is close to
  free — the same binary uploads to each store's own pipeline. GOG in particular requires
  nothing further since it's DRM-free by policy.
- **Achievements and cloud saves are explicitly de-prioritized**, and treated as a later
  "extra," not a launch requirement — consistent with selling widely first. The one thing
  worth doing early *because* it's cheap now and expensive to retrofit: if/when these get
  built, put them behind a small interface of the game's own rather than calling
  Steamworks/GOG Galaxy/EOS directly, so supporting them on GOG or Epic later doesn't
  require reworking gameplay code. This is a "when you get to it" note, not a commitment to
  build the abstraction now.
- **Console storefronts (PlayStation/Xbox/Switch) are out of scope** — each requires
  platform holder certification, dev kits, and a certification/compliance process that
  isn't realistic for a solo developer to carry alongside the main game.

## Input

- **Keyboard + mouse is the primary, initial-release input method** — this is what the
  current Strategy-variant control scheme (camera pan/zoom, click/drag-box selection, move
  commands) is designed around.
- **Basic controller support is a pre-launch goal, not a stretch goal** — unlike a full
  console port, it's required to reach Steam Deck (see **Platforms** above) and serves PC
  players using Big Picture/couch play. Scope for this pass: camera control, unit
  selection, and issuing commands — not a redesigned interface.
- **Full controller-native UI (menu/dialog navigation without a mouse cursor) stays
  deferred**, consistent with `player-experience.md`'s existing open question — this is a
  materially bigger lift than binding gameplay actions to a gamepad, and doesn't block
  Deck compatibility the way basic gameplay input does.
- **Touch is already implemented at the prototype level** (inherited from the Strategy
  template — see `game-systems`), but touch is not a targeted platform in its own right;
  it's incidental to the current control scheme, not a commitment to ship on a touch
  platform.

## Open Design Questions Worth Tracking

- Whether to actually pursue Steam Deck's "Verified" compatibility badge (vs. just letting
  Proton compatibility happen informally) isn't decided.
- The point at which full controller-native UI gets scheduled, if ever, isn't decided.
- Native Linux, mobile, and console storefronts are marked out of scope for the current
  solo-dev scale of the project — this should be revisited explicitly if that scale
  changes, not assumed permanent.
- When (or whether) achievements/cloud saves get built at all isn't decided — they're
  parked as a possible post-launch extra, not scheduled.
