# Save Data

The project does not currently define a save/load system in C++.

## Current Persistent State

No gameplay state is documented as persistent across sessions.

## Runtime State That May Become Save Data

- Strategy selected units should probably remain runtime-only.
- Strategy unit positions may become save data if the open-world RTS direction persists unit placement.

## Future Design Questions

- What game mode owns long-term progression?
- Should Strategy units be saved individually, by squad, or by world region?
- Should player-facing help/tutorial state be saved?
- Will save data be Blueprint-only, C++ `USaveGame`, or a custom serialization layer?

Create a dedicated save system design doc before adding save/load implementation.
