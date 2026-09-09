# smores-game
Let's build a game...maybe

## Perform a cold build
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" smoresEditor Win64 Development "C:\dev\smores\smores.uproject" -waitmutex
```

## Package a standalone Windows build

Produces a runnable folder (`smores.exe` + content paks) under `Saved\PackagedBuild` — no
installer, just the standalone player. Maps cooked come from Project Settings > Packaging
> List of Maps to Include (`Lvl_MainMenu`, `LVL_Strategy`). Safe to run with the editor open.

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="C:\dev\smores\smores.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="C:\dev\smores\Saved\PackagedBuild"
```