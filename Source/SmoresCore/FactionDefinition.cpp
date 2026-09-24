// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "FactionDefinition.h"

// Has to match the PrimaryAssetType in Config/DefaultGame.ini's PrimaryAssetTypesToScan entry
// for factions, or USmoresDefinitionLibrary::FindDefinition will never resolve a faction id.
// Spelled out rather than derived from the class name so the two are visibly the same string.
const FPrimaryAssetType UFactionDefinition::DefinitionType = FPrimaryAssetType(TEXT("FactionDefinition"));
