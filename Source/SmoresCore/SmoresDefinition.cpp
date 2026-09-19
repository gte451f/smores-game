// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresDefinition.h"

FPrimaryAssetId USmoresDefinition::GetPrimaryAssetId() const
{
	// Two callers land here with nothing to give: the class-default object of an abstract
	// definition type (which must never reach the pure-virtual GetDefinitionType below), and an
	// asset a designer created but hasn't filled in yet. Both fall back to the engine's
	// asset-name behavior. The content sweep in Tests/ is what catches the second case.
	if (DefinitionId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}

	return FPrimaryAssetId(GetDefinitionType(), DefinitionId);
}
