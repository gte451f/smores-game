// Copyright Epic Games, Inc. All Rights Reserved.


#include "ItemDefinition.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	// fall back to the base behavior (asset name) until a designer authors a stable ItemId
	if (ItemId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}

	return FPrimaryAssetId(UItemDefinition::StaticClass()->GetFName(), ItemId);
}
