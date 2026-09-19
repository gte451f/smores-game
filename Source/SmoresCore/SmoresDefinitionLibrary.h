// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SmoresDefinitionLibrary.generated.h"

class USmoresDefinition;

/**
 *  "Give me the definition whose id is Sword."
 *
 *  Everything that stores a definition by id rather than by asset pointer - a record, a loot
 *  table entry, a crafting recipe's inputs, a save file - needs this one lookup to turn that id
 *  back into the asset. It wraps UAssetManager, whose registry is populated from the
 *  PrimaryAssetTypesToScan entries in Config/DefaultGame.ini; a definition type with no entry
 *  there is invisible here no matter how correct its GetDefinitionType() is.
 *
 *  **The load is synchronous, deliberately.** A few dozen small data assets cost nothing to pull
 *  in on demand, and an async handle would force every caller to deal with "not yet". If the
 *  definition count ever reaches the thousands, or definitions start carrying heavy art, this is
 *  the seam that becomes an async preload - the callers keep asking the same question either way.
 */
UCLASS()
class SMORESCORE_API USmoresDefinitionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  The definition registered under DefinitionType with this DefinitionId, loading it if it
	 *  isn't in memory yet. Null when the id resolves to nothing - which is a legitimate outcome,
	 *  not an error: it is what loading a save without the mod that wrote it looks like.
	 */
	UFUNCTION(BlueprintCallable, Category = "Smores|Definitions")
	static USmoresDefinition* FindDefinition(FPrimaryAssetType DefinitionType, FName DefinitionId);

	/** Every id the Asset Manager knows for one definition type, whether or not it is loaded. */
	UFUNCTION(BlueprintCallable, Category = "Smores|Definitions")
	static void GetDefinitionIds(FPrimaryAssetType DefinitionType, TArray<FName>& OutDefinitionIds);

	/**
	 *  Every registered PrimaryAssetType whose base class is a USmoresDefinition - i.e. the
	 *  project's own definition types, and none of the engine's (Map, PrimaryAssetLabel,
	 *  GameFeatureData). Each new definition type joins this list by being registered in
	 *  Config/DefaultGame.ini, with nothing here to update.
	 */
	UFUNCTION(BlueprintCallable, Category = "Smores|Definitions")
	static void GetDefinitionTypes(TArray<FPrimaryAssetType>& OutDefinitionTypes);
};
