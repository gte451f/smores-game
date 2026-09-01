# Save System

> Non-gameplay arch doc. See `factions-and-world-state.md` for world state model, `characters-and-squads.md` for character state, and `open-world.md` for fog of war and discovery state.

---

## Purpose

The save system records the full state of a campaign at a moment in time and restores it faithfully on load. Because the game simulates a living world — factions shifting, economies fluctuating, characters aging through injuries — the save file is complex and must be treated as a first-class system, not an afterthought.

The save system must also be version-aware. Players will load saves after game patches and after adding or removing mods. The system needs a defined strategy for handling those cases rather than silently corrupting or crashing.

---

## What Must Be Saved

### World State
- Faction type and status (Major / Minor / Nomadic / Eliminated) for every faction
- Faction relationships (current standing between every faction pair, not just starting values)
- Settlement ownership and current status (intact / ruined / being rebuilt)
- Named NPC alive/dead status and current location
- Fog of war: which regions and points of interest the player has discovered
- Active world events (faction conflicts in progress, trade disruptions, migration movements)
- Economy values: current supply/demand levels and price state per market

### Player Squad and Bases
- Each squad member's current attributes, skills, injuries, equipment, and wage
- Squad assignments (who is at what base, on patrol, or traveling)
- Each base's buildings, building condition, storage inventory, and production assignments
- Active research progress (node in progress, character-hours accumulated, materials already consumed)
- Player reputation and standing with every faction

### In-Progress Events
- Any assault or raid that is in progress (including off-screen simulation state)
- Caravan positions and current routes
- Pending NPC events (scheduled renegotiations, settlement party movements)

---

## Save Slots and Autosave

The game supports **multiple manual save slots** and a separate **autosave** that runs on a configurable interval (default: every in-game day). Autosave does not overwrite the player's manual saves.

A **rolling backup** of the previous N autosave states (minimum 3) is kept so that a player who discovers a mistake several in-game days later has a recovery path. Backup depth is a configurable setting.

There is no single-save (ironman) requirement in the base game. An optional ironman mode (one save slot, autosave only, no manual saves) may be offered as a difficulty modifier — see `content-and-release.md`.

---

## Save File Structure

Save files are structured data (JSON or binary equivalent) organized to mirror the game's data-driven architecture. The structure must:

- **Reference content by ID, not by index.** Faction data, item types, building types, tech nodes, and NPC templates are all referenced by their data asset ID. This is what makes version and mod compatibility tractable — if an ID no longer exists, the system can handle it as a missing reference rather than an offset error.
- **Be self-describing.** Each save file includes a header block listing: game version, list of active mod IDs and their versions, and a schema version number. This header is read before any game state is loaded.
- **Separate static from dynamic.** The save file records what has *changed* from the authored world state, not a full copy of the world. Authored NPC positions, authored faction starting territories, and authored building placements do not need to be saved — only the delta (moved, dead, destroyed, changed ownership) is recorded. This keeps save files manageable and makes it easier to merge authored world updates with saved dynamic state.

---

## Version Compatibility

### Game Version Updates
When a save is loaded with a newer game version:

1. The save header schema version is compared to the current schema version.
2. If the schema is identical or only additive (new fields with defaults), the save loads normally. New fields default to their authored values.
3. If the schema has breaking changes (fields removed, types changed, IDs renamed), a **migration script** runs first. Migration scripts are shipped with the patch and transform the save file to the new schema before loading. The original save is backed up before migration.
4. If migration is not possible (too many breaking changes, old save too far behind), the player is informed clearly — the save is not silently broken.

**Design implication:** Breaking save changes should be rare and deliberate. Avoid schema changes in routine balance patches. Reserve breaking changes for major version milestones.

### Mod Compatibility
When a save is loaded without a mod that was active when it was created:

- Content from the missing mod that is referenced in the save (items, factions, NPCs, buildings) is treated as **unknown references**.
- Unknown references are stripped from inventories and flagged in a load report shown to the player: "X items from missing mod Y were removed."
- The game proceeds with the stripped state rather than refusing to load.
- Characters or factions that *are* the missing mod content (e.g., a mod-added faction that the save records as active) are removed and their world-state effects are cleaned up as gracefully as possible.

When a save is loaded with a *new* mod that was not active when it was created, the mod's content is simply not present in the saved world state. The mod integrates from that point forward.

---

## Corruption Protection

- Save writes are **atomic**: the new save is written to a temp file first, then renamed over the old file. A crash mid-write leaves the old file intact.
- Save files include a **checksum**. On load, if the checksum fails, the game warns the player and offers to load the most recent backup instead of loading a potentially corrupt file silently.
- Cloud sync (Steam Cloud or equivalent) is supported. Local saves are the authoritative source; cloud saves are a backup and cross-device convenience.

---

## Performance Considerations

Saving the full world state on a large map with active faction simulation is non-trivial. Save operations should:

- Run **asynchronously** where possible — the game does not freeze during a save
- Be **rate-limited** — autosave does not trigger during active combat or events where a save mid-sequence would be misleading
- Provide **clear feedback** to the player that a save is in progress and when it completes

---

## Known Gaps / Future Notes

- **Exact schema format**: JSON vs. binary (CBOR, MessagePack, or Unreal's built-in serialization) is an implementation decision. The above requirements are format-agnostic.
- **Save file size budget**: With a large simulated world, economy state, and full character histories, save files could become large. A size target and pruning strategy (e.g., trimming price history beyond N days) needs a pass during implementation.
- **Ironman mode design**: The details of ironman (one save, no reloads, what happens on squad wipe) are not yet designed.
- **Console platform saves**: If the game ships on console, platform-specific save requirements (save data size limits, cloud sync APIs) apply. Deferred until platform targets are confirmed.
