// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogHost.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UDialogHost : public UInterface
{
	GENERATED_BODY()
};

/**
 *  What dialog needs a player's controller to do - implemented by AStrategyPlayerController.
 *
 *  The IStrategySelectionHost pattern (unreal-module-organization.md): nothing depends on the
 *  smores module, so SmoresDialog declares the narrow interface it needs and the controller
 *  implements it. Slice 1 needs two things; Slice 2's OpenTrade effect is expected to be the next.
 *
 *  Every call arrives on the server, one per player controller, and must not assume there is only
 *  one of those.
 */
class SMORESDIALOG_API IDialogHost
{
	GENERATED_BODY()

public:

	/** True if any of this player's own squad is within Range of Location. Server-side; answers from the authoritative units. */
	virtual bool IsSquadMemberWithin(const FVector& Location, float Range) const = 0;

	/**
	 *  Tells this player a bark was said.
	 *
	 *  Server-side. The implementation carries it to the owning client **as the line id**, never as
	 *  text, and the client looks the line up in its own loaded, translated copy - so in co-op each
	 *  player reads it in their own language. SpeakerName is a character's name, not dialog, and
	 *  travels as it is.
	 */
	virtual void DeliverBark(FName LineId, const FText& SpeakerName) = 0;
};
