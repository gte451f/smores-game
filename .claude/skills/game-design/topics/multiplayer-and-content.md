# Multiplayer, Updates, and Content

## Co-Op Vision

Self-hosted co-op for **up to 8 players**, modeled on Minecraft's listen-server pattern —
one player hosts (listen server or standalone dedicated process), others connect over LAN
or internet. No matchmaking, no central publisher-run server, no live-service dependency,
and no ongoing server cost to the developer. This is meant to fit the game's independent,
community-driven character, not to turn it into a live-service product.

Each player in a session:

- Controls their own squad — their characters, equipment, wages, assignments.
- Owns their own base(s), or shares by choice with other players.
- Holds their **own independent faction standing** — two players can have different
  relationships with the same faction at the same time.
- Shares one server-authoritative world: economy, faction simulation, and world events are
  the same for everyone in the session.

Players can cooperate (trading between squads, joint assaults, shared base
infrastructure) or compete (cornering markets, raiding each other's caravans where faction
alignment allows) — the simulation doesn't enforce either; it just puts multiple players
in the same living world (see `factions-and-world-state.md`).

**Deliberately out of scope:** matchmaking/server browser, direct PvP (the world is meant
to supply enough conflict on its own), and cross-platform play (deferred).

A standalone dedicated server should ship as a first-class, player-facing option (home
machine or rented host), with self-hosting documentation as a real deliverable, not an
afterthought. The dedicated server is expected to target **Linux** specifically — hosting
a headless simulation process has no need for a GPU or a Windows Server license, so Linux
hosting is meaningfully cheaper for whoever rents a box to run it. This is independent of
the client's own platform stance (Windows-primary, Steam Deck via Proton — see
`input-and-platforms.md`): the server is a separate build target from the same source, and
a Linux dedicated server does not imply or require a native Linux client.

**Built multiplayer-aware from day one, even though co-op itself may not be playable until
later.** The reasoning is cost asymmetry, not present-day demand: retrofitting multiplayer
onto code that assumed a single player throughout is expensive and risky, while writing
code that already respects multiplayer's constraints costs comparatively little as you go.
The intent is that Unreal's built-in networking (replication, RPCs, server authority)
supplies the large majority of what multiplayer actually needs — the responsibility on the
project's side is engineering *discipline* (no singleton-player assumptions, authority
checks on shared-state mutation, using replication instead of inventing ad hoc sync), not
building networking infrastructure from scratch. That discipline is implementation
guidance, not design intent, so it's codified in `CLAUDE.md` for anyone (human or LLM)
writing gameplay code, rather than restated here.

## What Players Should Be Able to Expect From Updates

Four update categories, distinguished by how much they're allowed to disturb an existing
save:

| Type | What changes | Save compatibility promise |
|---|---|---|
| Hotfix | Bug/crash fixes, critical balance corrections | Must not break existing saves |
| Balance patch | Tuning of economy, combat, skills, wages | Should not break saves; new values apply on load |
| Content update | New authored locations, NPCs, events in the existing world | Should not break saves; new content is simply discovered normally, not retrofitted into old saves |
| Major version | New systems, breaking schema changes | May require migration, with the original save backed up first |

The player-facing commitment is that patch notes always distinguish bug fixes from
balance changes from content additions, so a player can tell at a glance whether their
current strategy is affected or a save migration is coming.

## Player-Contributed Mods and Paid DLC

Mods and DLC are both additive content layers on top of the base game, but they are **not
treated as equivalent** — DLC is official, paid content with **preferential treatment over
mods**: capability, engine-hook access, and support guarantees a community mod isn't
expected to have. (This supersedes an earlier "DLC has no privileged capability over
modding" stance, which treated the two as symmetric.)

### Player-contributed mods

- Community-authored content and systems, distributed independently of official
  storefronts (mod repository / community channels), not through the same pipeline as DLC.
- Best-effort compatibility: not guaranteed to survive a major-version update, and not
  held to the same save-compatibility promises official updates carry (see "What Players
  Should Be Able to Expect From Updates" above).

### Paid DLC

- Official, paid content delivered through **typical storefronts** (Steam, GOG, Epic,
  etc. — see `input-and-platforms.md`), not a mod repository.
- Gets preferential treatment over mods: DLC can rely on capabilities, deeper engine
  integration, or support guarantees that aren't extended to community mods.
- Candidate DLC content: **new areas** (biomes/regions with distinct resource/faction/
  hazard profiles), **new races/lineages** (with associated faction content), **new
  items**, and **expanded gameplay systems** (e.g., mercenary contracts, political
  intrigue, seafaring) — additive, extending player options rather than changing how
  existing systems already work.
- A player without a given DLC pack should still be able to join a shared co-op session
  hosted by someone who has it — DLC content is gracefully hidden or falls back to a
  base-game equivalent, and the player is told clearly that content was stripped rather
  than having the join silently refused.
- DLC win conditions should be optional add-ons; base-game end-game objectives (see
  `vision-and-pillars.md`) always remain reachable without any DLC.
- Each DLC pack is meant to be a **self-contained layer** that integrates with what exists
  without requiring any other DLC pack.
- DLC itself is a **long-term goal, not a launch deliverable** — the base game must stand
  on its own; DLC extends it, never completes it.

## Open Design Questions Worth Tracking

- Session management (invite flow, LAN vs. direct IP vs. invite code, reconnect handling)
  isn't designed yet.
- Co-op economy balance is called out as its own tuning pass — multiple players in one
  economy may destabilize markets faster than solo tuning assumes.
- DLC sequencing (which packs first, timing relative to base-game launch) isn't designed
  yet. Packs are paid and storefront-delivered (decided); per-pack pricing isn't.
- If the game ever ships on console, platform certification requirements for
  updates/DLC/saves would add constraints not captured here.
