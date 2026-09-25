// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SmoresDefinition.h"
#include "CharacterRecord.h"
#include "InventoryComponent.h"
#include "CharacterDefinition.generated.h"

class AStrategyUnit;
class UTexture2D;

/**
 *  What a *kind* of character is - "Bandit", "Settler", or one named individual like Warlord
 *  Kess. Authored identity, identical every campaign, never written at runtime.
 *
 *  Everything about one particular character - their name, their health, what they carry,
 *  whether they are alive - lives in an FCharacterRecord instead. That includes a unique
 *  character's condition: "is Kess dead?" is a question about Kess's record, never about this
 *  asset. See game-data.md.
 */
UCLASS(BlueprintType)
class SMORESCHARACTERS_API UCharacterDefinition : public USmoresDefinition
{
	GENERATED_BODY()

public:

	/** The Asset Manager type characters are registered under - see Config/DefaultGame.ini's PrimaryAssetTypesToScan */
	static const FPrimaryAssetType DefinitionType;

	/**
	 *  Exactly one record of this definition may ever exist - a named individual rather than a
	 *  kind of person. The record is named DisplayName and never NamePool. Once created it is
	 *  never created again, alive or dead: uniqueness is authored here, condition is recorded.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity")
	bool bUnique = false;

	/** Who this is. Designer-facing for a kind of character, player-facing for a unique one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity", meta = (MultiLine = "true"))
	FText Backstory;

	/**
	 *  The framed face the squad bar and target panel draw - not an item icon. Optional: a unit
	 *  with no portrait draws initials. A placed unit's own PortraitTexture, when set, wins over
	 *  this one, so a single placed individual can differ from the rest of its kind.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity")
	TObjectPtr<UTexture2D> Portrait;

	/**
	 *  The faction a new record of this kind belongs to, or None for the unaffiliated. An id
	 *  rather than an asset pointer because it is copied straight into the saved record. An id the
	 *  world doesn't know is allowed (a stripped mod) and only warned about; the content sweep
	 *  requires every authored one to resolve.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity")
	FName DefaultFactionId;

	/** "trader", "guard" - an id and nothing more. There is no role system yet: dialog's Role facts read it to choose what someone says, and nothing reads it for behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity")
	FName RoleId;

	/** The seven attributes a new record starts with */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats")
	FCharacterAttributes BaseAttributes;

	/** What a new record of this kind carries - placed into its actor's grid through the ordinary AddItem rules */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats")
	TArray<FInventoryItem> DefaultLoadout;

	/**
	 *  Names a non-unique record is rolled from. The roll is seeded from the record's id, so the
	 *  same individual always gets the same name. Ignored for a unique definition, and overridden
	 *  by a placed unit that has its own UnitDisplayName authored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Identity")
	TArray<FText> NamePool;

	/**
	 *  The actor that stands in for a record of this kind. **Read by nothing yet** - under the soft
	 *  split every actor is placed in the level and nothing spawns one - but it is what the
	 *  world-activity roadmap's spawn-on-approach needs. Soft, so loading a definition doesn't
	 *  drag in a skeletal mesh and its animation set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Actor")
	TSoftClassPtr<AStrategyUnit> ActorClass;

	//~ Begin USmoresDefinition interface
	virtual FPrimaryAssetType GetDefinitionType() const override { return DefinitionType; }
	//~ End USmoresDefinition interface
};
