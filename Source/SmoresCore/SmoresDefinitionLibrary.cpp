// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SmoresDefinitionLibrary.h"

#include "Engine/AssetManager.h"
#include "Engine/AssetManagerTypes.h"
#include "SmoresDefinition.h"

USmoresDefinition* USmoresDefinitionLibrary::FindDefinition(FPrimaryAssetType DefinitionType, FName DefinitionId)
{
	if (!DefinitionType.IsValid() || DefinitionId.IsNone())
	{
		return nullptr;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();

	if (!AssetManager)
	{
		return nullptr;
	}

	const FPrimaryAssetId AssetId(DefinitionType, DefinitionId);

	// already in memory - the common case once anything has referenced it once
	if (USmoresDefinition* Loaded = AssetManager->GetPrimaryAssetObject<USmoresDefinition>(AssetId))
	{
		return Loaded;
	}

	// resolve the id to a path and pull it in. TryLoad rather than the Asset Manager's own
	// LoadPrimaryAsset because that hands back a streaming handle the caller would have to keep
	// alive; the definition tree is small enough that ordinary loading is the honest answer.
	const FSoftObjectPath DefinitionPath = AssetManager->GetPrimaryAssetPath(AssetId);

	if (!DefinitionPath.IsValid())
	{
		return nullptr;
	}

	return Cast<USmoresDefinition>(DefinitionPath.TryLoad());
}

void USmoresDefinitionLibrary::GetDefinitionIds(FPrimaryAssetType DefinitionType, TArray<FName>& OutDefinitionIds)
{
	OutDefinitionIds.Reset();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();

	if (!AssetManager || !DefinitionType.IsValid())
	{
		return;
	}

	TArray<FPrimaryAssetId> AssetIds;
	AssetManager->GetPrimaryAssetIdList(DefinitionType, AssetIds);

	OutDefinitionIds.Reserve(AssetIds.Num());

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		OutDefinitionIds.Add(AssetId.PrimaryAssetName);
	}
}

void USmoresDefinitionLibrary::GetDefinitionTypes(TArray<FPrimaryAssetType>& OutDefinitionTypes)
{
	OutDefinitionTypes.Reset();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();

	if (!AssetManager)
	{
		return;
	}

	TArray<FPrimaryAssetTypeInfo> TypeInfos;
	AssetManager->GetPrimaryAssetTypeInfoList(TypeInfos);

	for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
	{
		// AssetBaseClassLoaded is the resolved copy of the config's base class, and is only filled
		// in once the Asset Manager has run FillRuntimeData; fall back to the config soft pointer
		const UClass* BaseClass = TypeInfo.AssetBaseClassLoaded;

		if (!BaseClass)
		{
			BaseClass = TypeInfo.GetAssetBaseClass().Get();
		}

		if (BaseClass && BaseClass->IsChildOf(USmoresDefinition::StaticClass()))
		{
			OutDefinitionTypes.Add(TypeInfo.PrimaryAssetType);
		}
	}
}
