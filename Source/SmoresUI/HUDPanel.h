// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HUDPanel.generated.h"

/**
 *  Which of the HUD's panels a request is about.
 *
 *  This is the shared vocabulary between the nav rail (which asks for a panel by name), the
 *  player controller (which owns the windows and decides what opening one means), and each
 *  panel window (which carries its own id so the controller can tell them apart without a
 *  cast per class). It lives in its own header rather than beside the interface because both
 *  the interface and the widgets need it, and neither should have to include the other.
 *
 *  Inventory is in here even though it is the one panel that predates the rail and has its own
 *  window path - the whole point of the rail is that pressing `I` and clicking the rail button
 *  run the same code, so it has to be nameable the same way.
 */
UENUM(BlueprintType)
enum class EHUDPanel : uint8
{
	/** No panel. The "nothing is active" value, not something the player can ask for. */
	None      UMETA(DisplayName = "None"),

	/** The squad roster. Stub until the squad panel has real content. */
	Squad     UMETA(DisplayName = "Squad"),

	/** The selected pawn's pack and paperdoll - the existing inventory window path. */
	Inventory UMETA(DisplayName = "Inventory"),

	/** The world map. Stub until there is a world map to draw. */
	Map       UMETA(DisplayName = "Map"),

	/** Research / tech. Stub until there is a tech tree. */
	Research  UMETA(DisplayName = "Research"),

	/** The keybind list. Not a stub - it is the cheapest panel here and the only immediately useful one. */
	Help      UMETA(DisplayName = "Help")
};
