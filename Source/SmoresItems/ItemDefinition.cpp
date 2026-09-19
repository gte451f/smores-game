// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ItemDefinition.h"

// Has to match the PrimaryAssetType in Config/DefaultGame.ini's PrimaryAssetTypesToScan entry
// for items, or USmoresDefinitionLibrary::FindDefinition will never resolve an item id. Spelled
// out rather than derived from the class name so the two are visibly the same string.
const FPrimaryAssetType UItemDefinition::DefinitionType = FPrimaryAssetType(TEXT("ItemDefinition"));
