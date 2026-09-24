// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresDefinition.h"
#include "FactionTypes.h"
#include "FactionDefinition.generated.h"

class UFactionDefinition;

/**
 *  How a faction regards one other faction when a campaign begins.
 *
 *  Authored on **one** side of a pair only. Standing between factions is symmetric, so writing
 *  "Ironclan hates the Raiders" on Ironclan is the whole of it; the content sweep rejects a pair
 *  authored on both sides with different numbers, because one of the two would be silently lost.
 */
USTRUCT(BlueprintType)
struct FFactionStartingRelation
{
	GENERATED_BODY()

	/**
	 *  The other faction. An asset pointer rather than an id, because this is one definition
	 *  naming another - ordinary content linking - rather than saved data. See game-data.md.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	TObjectPtr<UFactionDefinition> Faction;

	/** Starting standing between the two, on the SmoresStanding scale (-100 to 100) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (ClampMin = -100, ClampMax = 100))
	int32 Standing = SmoresStanding::Neutral;
};

/**
 *  What a faction *is*: its authored identity, identical every campaign.
 *
 *  Everything that can change during play - which tier it currently holds, how it regards other
 *  factions, how each player stands with it - lives in records on UWorldFactionComponent and
 *  UPlayerStandingComponent instead. That split is the whole point of the definition/record
 *  model: StartingTier is here because a faction *begins* as Minor, and nothing about a faction
 *  later seizing a town ever writes back to this asset.
 *
 *  Lives in SmoresCore rather than a future SmoresFactions module because combat, characters and
 *  economy all need to read faction identity, and SmoresCore is the only module all three can
 *  see. SmoresFactions becomes worth cutting when faction *behavior* arrives.
 */
UCLASS(BlueprintType)
class SMORESCORE_API UFactionDefinition : public USmoresDefinition
{
	GENERATED_BODY()

public:

	/** The Asset Manager type factions are registered under - see Config/DefaultGame.ini's PrimaryAssetTypesToScan */
	static const FPrimaryAssetType DefinitionType;

	/** A compact name for tight spaces - a nameplate, a map label, a column header */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction|Identity")
	FText ShortName;

	/**
	 *  The faction's colour on maps, nameplates and banners. Defaults to opaque grey rather than
	 *  Unreal's usual zero so an unfilled asset still shows up; the content sweep rejects one
	 *  authored fully transparent, which would draw nothing and look like a missing faction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction|Identity")
	FLinearColor Colour = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	/** How the faction treats lineages other than its own. Authored identity - it never changes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction|Identity")
	ELineageStance LineageStance = ELineageStance::Cosmopolitan;

	/**
	 *  The tier this faction holds when a campaign begins. Only the *starting* value - the current
	 *  tier is FFactionRecord::Tier, because tier is a state a faction moves through, not who it is.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction|Campaign Start")
	EFactionTier StartingTier = EFactionTier::Minor;

	/** How this faction regards others at campaign start. Any faction not listed starts neutral. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction|Campaign Start")
	TArray<FFactionStartingRelation> StartingRelations;

	//~ Begin USmoresDefinition interface
	virtual FPrimaryAssetType GetDefinitionType() const override { return DefinitionType; }
	//~ End USmoresDefinition interface
};
