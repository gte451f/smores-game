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

## Keybinds

- **Every player-facing control is rebindable by the player.** This is a committed goal, not
  an aspiration, and it is the reason it appears here rather than in a settings backlog: it
  constrains *how every binding is built*, from the first one onward. A control wired outside
  Unreal's Enhanced Input system cannot be surfaced in a keybind screen at all, so retrofitting
  one later means rewriting the binding rather than adding a menu. The enforcement detail lives
  in the `game-systems` skill's `input-and-keybinds.md`.
- **The default set follows PC RPG convention.** The audience arrives with muscle memory from
  Kenshi, Baldur's Gate, Pillars, Diablo and the rest — `I` for inventory, `M` for map, `J`
  for journal, `C` for the character sheet, `Space` to pause, `F5`/`F9` to quicksave and load,
  number keys for squad groups. Matching that costs nothing and is felt immediately; deviating
  from it reads as the game being wrong rather than as a setting waiting to be changed.
- **Defaults must ship conflict-free.** Because most players never open the keybind screen,
  rebindability is not a licence to leave collisions for the player to discover and work
  around. Keys are therefore held in reserve for their conventional meaning *before* the
  system that will use them is built — a system claiming `M` or `Space` for something
  unrelated is a design error even if the map and pause don't exist yet. The reserved list and
  the record of what is currently bound live together in
  `game-systems`/`input-and-keybinds.md`, which is the single place to check before adding a
  control.
- **Context-scoped keys are preferred over inventing new ones.** A key may mean different
  things in different modes (a window open, a build mode active) rather than every new action
  consuming another key from a finite, increasingly awkward pool.
- **Controller bindings inherit the same rule** when basic controller support lands — same
  action definitions, a separate default mapping set.

## Open Design Questions Worth Tracking

- Whether to actually pursue Steam Deck's "Verified" compatibility badge (vs. just letting
  Proton compatibility happen informally) isn't decided.
- Whether the keybind screen exposes *modifier* conventions (Ctrl/Shift/Alt + click inside a
  window) as rebindable, or keeps them fixed as near-universal idioms, isn't decided. Most
  games keep them fixed; making them rebindable drags real complexity into the settings UI.
- Several current defaults fight convention (`H` to attack, `O` to open a container, `Q`/`E`
  spent on camera height) because the conventional keys are already taken by camera movement.
  Whether to rearrange them is not decided — see `input-and-keybinds.md` for the specifics.
- The point at which full controller-native UI gets scheduled, if ever, isn't decided.
- Native Linux, mobile, and console storefronts are marked out of scope for the current
  solo-dev scale of the project — this should be revisited explicitly if that scale
  changes, not assumed permanent.
- When (or whether) achievements/cloud saves get built at all isn't decided — they're
  parked as a possible post-launch extra, not scheduled.
