// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmoresRefusalReason.h"
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
 *  implements it: whether a squad is near, delivering a bark, and opening a shop for the OpenTrade
 *  effect.
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
	 *
	 *  Speaker is who said it, so the client can float the line over them. It travels as an actor
	 *  reference: a client the speaker isn't relevant to receives null, and gets the feed line
	 *  without a bubble.
	 */
	virtual void DeliverBark(FName LineId, AActor* Speaker, const FText& SpeakerName) = 0;

	/**
	 *  Opens Trader's shop on this player's screen - the OpenTrade effect. Server-side. False, with
	 *  nothing opened, if Trader keeps no shop or isn't someone this player may deal with right now.
	 */
	virtual bool OpenTradeWith(AActor* Trader) = 0;

	/** Tells this player why something a conversation tried for them was refused ("Not enough gold"). Server-side. */
	virtual void NotifyDialogRefusal(ESmoresRefusalReason Reason) = 0;
};
