// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "ItemModifierDefinition.h"

// Has to match the PrimaryAssetType in Config/DefaultGame.ini's PrimaryAssetTypesToScan entry
// for modifiers, or USmoresDefinitionLibrary::FindDefinition will never resolve a modifier id.
// Spelled out rather than derived from the class name so the two are visibly the same string.
const FPrimaryAssetType UItemModifierDefinition::DefinitionType = FPrimaryAssetType(TEXT("ItemModifierDefinition"));

TArray<EItemModifierSlot> UItemModifierDefinition::GetAllModifierSlots()
{
	// material first, then quality - this is the order multipliers compose in and, more visibly,
	// the order names do: "Masterwork Bronze Spear", never "Bronze Masterwork Spear"
	return { EItemModifierSlot::Material, EItemModifierSlot::Quality };
}
