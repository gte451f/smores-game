// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SmoresDefinition.generated.h"

/**
 *  The shared base of every authored "what a kind of thing is" asset in the game - items, item
 *  modifiers, factions, characters, loot tables, and later recipes and buildings.
 *
 *  Three layers hold the game's data, and this is the first of them:
 *
 *    Definition  - what a *kind* of thing is. Authored by hand, identical in every campaign,
 *                  never written to at runtime, ships with the game and is not saved.
 *    Record      - one *particular* thing (this bandit, this health, this inventory). A plain
 *                  USTRUCT in memory; records are what the save file is made of.
 *    Actor       - what is physically standing in the level. A puppet driven by a record, and
 *                  rebuilt from it rather than the other way round.
 *
 *  A definition is never written to, and a record is never a copy of an actor - the actor is a
 *  copy of the record. All three exist for characters (UCharacterDefinition, FCharacterRecord,
 *  AStrategyUnit) and the first two for factions - see game-data.md.
 *
 *  Exactly three fields live here, because they are the only three every definition type
 *  genuinely shares. **Presentation deliberately stays on the subclasses**: an item's Icon is a
 *  small transparent sprite sized to a grid cell, a character's Portrait is a framed face, a
 *  faction's is a crest, and a loot table has no picture at all. One hoisted Icon field would be
 *  permanently null on some types and - the practical cost - impossible to require, because the
 *  content sweep could never assert "this is set" without failing the types that legitimately
 *  have none. Left on the subclass, each type can make its own picture mandatory.
 *
 *  For types the player never sees (a loot table, say) DisplayName and Description are the
 *  designer's own label and notes rather than anything that reaches the screen.
 */
UCLASS(Abstract, BlueprintType)
class SMORESCORE_API USmoresDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/**
	 *  Stable identifier for this kind of thing, independent of the asset's name and path.
	 *  Records, loot tables and saves all reference a definition through this rather than through
	 *  an asset pointer, which is what lets a save written with a mod installed load without it -
	 *  an id that resolves to nothing strips that content instead of refusing to load.
	 *
	 *  Definitions referencing *each other* (a recipe naming its output, a character naming its
	 *  loadout) use ordinary asset pointers - that is normal content linking, it cooks correctly,
	 *  and the editor shows the reference. The id rule applies to saved data only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Definition|Identity")
	FName DefinitionId;

	/** Player-facing name, or the designer's own label on a type the player never sees */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Definition|Identity")
	FText DisplayName;

	/** Player-facing description, or the designer's own notes on a type the player never sees */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Definition|Identity", meta = (MultiLine = "true"))
	FText Description;

	/**
	 *  The Asset Manager type this definition is registered under - the "ItemDefinition" half of
	 *  an FPrimaryAssetId like ItemDefinition:Sword. Each concrete type names itself, and the
	 *  name it returns has to match its PrimaryAssetTypesToScan entry in Config/DefaultGame.ini or
	 *  nothing will ever resolve it by id.
	 */
	virtual FPrimaryAssetType GetDefinitionType() const
		PURE_VIRTUAL(USmoresDefinition::GetDefinitionType, return FPrimaryAssetType(););

	//~ Begin UPrimaryDataAsset interface
	/** {GetDefinitionType(), DefinitionId}, so the asset can be renamed or moved without breaking id references */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UPrimaryDataAsset interface
};
