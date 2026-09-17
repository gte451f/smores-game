// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GamePace.generated.h"

/**
 *  How fast the simulation is running - the shared world "speed" the player sets from the HUD's
 *  time-pace strip.
 *
 *  **The values are declared slowest-to-fastest on purpose.** Stepping the ladder with `-` and
 *  `=` is ordinal arithmetic over this enum, so the declaration order *is* the ladder; reordering
 *  it would silently reorder what those keys do. Add a new tier in its speed position, never at
 *  the end.
 *
 *  Pause is the bottom rung rather than a separate mechanism. AGameModeBase::SetPause is a
 *  single-player idea and doesn't survive co-op; a dilation tier does, because
 *  AWorldSettings::TimeDilation replicates on its own. See UTimePaceComponent.
 *
 *  It lives in SmoresCore because pace is shared world state with no dependencies - combat,
 *  economy and AI may all want to read it, and SmoresCore is the one module every module sees.
 */
UENUM(BlueprintType)
enum class EGamePace : uint8
{
	/** Frozen. Not literally 0x - see UTimePaceComponent::GetDilationForPace. */
	Paused        UMETA(DisplayName = "Paused"),

	/** A third of real time. */
	Third         UMETA(DisplayName = "1/3x"),

	/** Half of real time. */
	Half          UMETA(DisplayName = "1/2x"),

	/** Three quarters of real time. */
	ThreeQuarters UMETA(DisplayName = "3/4x"),

	/** Real time. The tier the world starts in. */
	Normal        UMETA(DisplayName = "1x"),

	/** Twice real time. */
	Double        UMETA(DisplayName = "2x"),

	/** Four times real time. */
	Quadruple     UMETA(DisplayName = "4x"),

	/** Eight times real time. The top of the ladder. */
	Octuple       UMETA(DisplayName = "8x")
};
