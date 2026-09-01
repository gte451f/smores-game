# Content and Release

> Non-gameplay arch doc. See `game-pillars.md` for modding requirements and the data-driven architecture mandate. See `save-system.md` for save version compatibility. See `characters-and-squads.md` for lineage system that DLC races plug into.

---

## Multiplayer

### Decision
The game supports **co-op multiplayer for 2–4 players** using a self-hosted server model — one player hosts, others connect. This is architecturally similar to Minecraft: the host runs a listen server or a standalone dedicated server process, and clients connect over LAN or the internet. There is no matchmaking, no central Anthropic/publisher-run server, and no live service dependency.

### Why This Model
- Self-hosted fits the game's independent, community-driven character
- The squad-based co-op premise is natural: each player manages their own squad in a shared world; the world simulation (factions, economy, world events) runs authoritatively on the server
- It extends replayability significantly — playing through faction conflicts alongside a friend is a different experience than solo play
- No ongoing server costs for the developer

### Co-Op Design
Each player in a co-op session:
- Controls their own squad (their characters, equipment, wages, assignments)
- Owns their own base(s), or may share bases with other players by choice
- Has their own faction standing (players can have different relationships with the same faction)
- Operates in the same simulated world: economy, faction simulation, and world events are shared and server-authoritative

Players can cooperate freely (trading goods between squads, participating in joint assaults, sharing base infrastructure) or compete (cornering markets, raiding each other's trade caravans if faction alignment allows). The simulation does not enforce cooperation — it just puts multiple players in the same world.

### Architectural Requirement
**Multiplayer support must be designed in from the start.** Retrofitting Unreal Engine's replication system onto existing gameplay code is expensive and error-prone. Every core C++ gameplay system must be written with the following in mind:

- **Server authority**: Game state (character health, skills, faction standing, economy values, world events) lives on the server. Clients replicate state; they do not own it.
- **Replicated properties**: Every `UPROPERTY` that clients need to read must be declared `Replicated` or `ReplicatedUsing`. This includes character stats, squad membership, base inventory, and world state.
- **RPCs for player actions**: Player-initiated actions (move order, attack order, base construction, trade transaction) are sent as Remote Procedure Calls from client to server. The server validates and executes them; clients see the replicated result.
- **GameMode on server only**: `AStrategyGameMode` and any equivalent simulation managers (faction AI, economy) run only on the server. Clients receive replicated read-only summaries.

**This constraint must be communicated to engineers before any gameplay system is written.** A system built without replication in mind will need to be partially or fully rewritten to support co-op.

### Dedicated Server
The game should ship a standalone dedicated server binary (Unreal's `-server` build target) that players can run on a home machine or rent from a hosting provider without needing the full game client. Documentation for self-hosting is a player-facing deliverable.

### What Multiplayer Does Not Include
- Matchmaking or a central server browser
- PvP modes (players fighting each other directly is not in scope — the world provides enough conflict)
- Cross-platform play (deferred; platform-specific requirements vary significantly)
- More than 4 simultaneous players (a larger session would require significant economy and faction simulation scaling work)

---

## Updates and Patches

### Update Categories
| Type | Content | Save compatibility |
|---|---|---|
| **Hotfix** | Bug fixes, crash fixes, critical balance corrections | Must not break existing saves |
| **Balance patch** | Tuning changes to economy, combat, skills, wages | Should not break saves; new values apply on load |
| **Content update** | New authored content (locations, NPCs, events) within the existing world | Should not break saves; new content is discovered normally |
| **Major version** | New systems, schema-breaking changes, significant architecture shifts | May require save migration (see `save-system.md`) |

### Patch Notes
Every update ships with structured patch notes that distinguish between bug fixes, balance changes, and content additions. Players need to know if a balance change affects their current strategy or if a save migration is required.

### Save Compatibility Policy
- Hotfixes and balance patches: saves always load without migration
- Content updates: saves load normally; new content is not retroactively present in old saves (a new settlement discovered in a content update will simply not exist in saves predating the patch — by design, not a bug)
- Major versions: migration scripts ship with the update; original save is backed up before migration runs (see `save-system.md`)

---

## Modding and DLC Relationship

### The Shared Architecture
Mods and DLC use the same underlying data-driven architecture. A mod adds content through data assets and Blueprint subclasses; a DLC does the same, just distributed through a different channel (platform store vs. mod repository). This is intentional:

- DLC cannot do things the modding system cannot — DLC is not a privileged content layer
- Content added by DLC is just as replaceable, buildable-on, and referenceable by mods as base game content
- Modders who reverse-engineer DLC content structure can create compatible addons

The distinction between a mod and DLC is business and distribution, not technical capability.

---

## DLC and Expansions

DLC is a long-term goal, not a launch deliverable. The game must ship as a complete, standalone experience. DLC extends it; it does not complete it.

### DLC Design Philosophy
- Each DLC should add a **self-contained layer** — a new biome, a new lineage group, a new game system — that integrates with existing content but does not require other DLC
- DLC must not invalidate existing saves. A player who does not own a DLC should be able to load a shared multiplayer session hosted by someone who does, with DLC content gracefully hidden or replaced with base-game equivalents
- DLC win conditions (new end-game objectives tied to DLC content) should be optional; base game objectives remain accessible without DLC

### Candidate DLC Content Categories

**New Biomes**
Additional map regions with distinct resource profiles, faction presence, environmental hazards, and points of interest. A new biome plugs into the existing regional system and trade economy. It may introduce new base game resources or restrict access in a way that creates new trade dynamics.

Example: a deep-ocean coastal region with unique extractables not available inland, requiring boat-based logistics (a new transport type) to exploit.

**New Lineages**
Additional playable and NPC lineages with distinct attribute profiles, innate traits, and visual identities. New lineages integrate with the lineage system defined in `characters-and-squads.md` — their faction relationships, mono/cosmopolitan composition rules, and recruitment pools all follow the existing framework.

A lineage DLC may include associated faction content: a new faction built around that lineage with authored settlements, named NPCs, and starting relationships.

**New Game Systems**
Deeper simulation layers that are optional to engage with but mechanically significant if pursued. Examples:
- A reputation-based mercenary contract system (take contracts from factions for payment and standing)
- A political intrigue layer (named NPCs have agendas, can be manipulated or blackmailed)
- A seafaring/exploration system (boats, ocean travel, distant islands)

New game systems must be designed to be **additive**, not replacement. They extend what the player can do; they do not change how existing systems work.

### DLC and Save Files
DLC adds new content IDs. Saves created with DLC active may reference those IDs. When loaded without the DLC:
- DLC items are removed from inventories (flagged in load report)
- DLC factions are treated as unknown references (removed from world state with cleanup)
- DLC biome regions are treated as unexplored/inaccessible

The player is informed clearly on load that DLC content was stripped. The game does not refuse to load the save.

---

## Known Gaps / Future Notes

- **Multiplayer session management**: Invite flow, session discovery (LAN vs. direct IP vs. invite code), and reconnection handling after disconnect are not yet designed.
- **Co-op economy balance**: Multiple players in the same economy may destabilize markets faster than the simulation expects. Balance tuning for co-op sessions is a separate pass from solo tuning.
- **DLC content roadmap**: The specific DLC releases, their sequencing, and their business model (paid expansion vs. free content update) are out of scope for this arch doc.
- **Console platform requirements**: If the game ships on console, platform certification requirements for updates, DLC, and save compatibility vary by platform and would add constraints not captured here.
- **Cross-DLC compatibility**: Two DLC packs that each add a new biome should not conflict. The data-driven architecture handles this if IDs are namespaced, but a namespace convention needs to be established.
