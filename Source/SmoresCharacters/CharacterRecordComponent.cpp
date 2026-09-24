// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "CharacterRecordComponent.h"
#include "CharacterDefinition.h"
#include "FactionDefinition.h"
#include "WorldFactionComponent.h"
#include "SmoresDefinitionLibrary.h"
#include "SmoresCharacters.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

UCharacterRecordComponent::UCharacterRecordComponent()
{
	// records only change when an actor writes back - nothing here moves on its own
	PrimaryComponentTick.bCanEverTick = false;

	// server-owned; see the class comment for why clients get no copy
	SetIsReplicatedByDefault(false);
}

UCharacterRecordComponent* UCharacterRecordComponent::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;

	return GameState ? GameState->FindComponentByClass<UCharacterRecordComponent>() : nullptr;
}

bool UCharacterRecordComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();

	return Owner && Owner->HasAuthority();
}

FGuid UCharacterRecordComponent::CreateRecord(const UCharacterDefinition* Definition, const FGuid& RequestedId, const FText& NameOverride)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("CreateRecord called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return FGuid();
	}

	if (RequestedId.IsValid() && FindRecord(RequestedId))
	{
		UE_LOG(LogSmoresCharacters, Warning, TEXT("CreateRecord refused: record %s already exists - adopt it instead."), *RequestedId.ToString());
		return FGuid();
	}

	if (Definition && Definition->bUnique && HasRecordOfDefinition(Definition->DefinitionId))
	{
		// the whole point of a unique character: authoring two Kesses, or a script trying to make
		// a second one, must not produce a second Kess - and a dead one stays dead
		UE_LOG(LogSmoresCharacters, Error, TEXT("CreateRecord refused: %s is unique and already has a record."), *Definition->DefinitionId.ToString());
		return FGuid();
	}

	FCharacterRecord& Record = Records.AddDefaulted_GetRef();
	Record.RecordId = RequestedId.IsValid() ? RequestedId : FGuid::NewGuid();

	if (!Definition)
	{
		Record.Name = NameOverride;

		return Record.RecordId;
	}

	Record.DefinitionId = Definition->DefinitionId;
	Record.FactionId = Definition->DefaultFactionId;
	Record.Attributes = Definition->BaseAttributes;

	if (Definition->bUnique)
	{
		Record.Name = Definition->DisplayName;
	}
	else if (!NameOverride.IsEmpty())
	{
		Record.Name = NameOverride;
	}
	else
	{
		Record.Name = RollName(Definition, Record.RecordId);
	}

	if (!Record.FactionId.IsNone() && !IsFactionKnown(Record.FactionId))
	{
		// allowed on purpose - a save whose faction came from a mod that isn't installed any more
		// has to keep loading - but worth saying, because the common cause is a typo
		UE_LOG(LogSmoresCharacters, Warning, TEXT("Character %s (%s) belongs to faction %s, which this world doesn't know - kept anyway."),
			*Record.Name.ToString(), *Record.DefinitionId.ToString(), *Record.FactionId.ToString());
	}

	return Record.RecordId;
}

const FCharacterRecord* UCharacterRecordComponent::FindRecord(const FGuid& RecordId) const
{
	if (!RecordId.IsValid())
	{
		return nullptr;
	}

	return Records.FindByPredicate([&RecordId](const FCharacterRecord& Record) { return Record.RecordId == RecordId; });
}

FCharacterRecord* UCharacterRecordComponent::EditRecord(const FGuid& RecordId)
{
	if (!HasOwnerAuthority())
	{
		return nullptr;
	}

	return const_cast<FCharacterRecord*>(FindRecord(RecordId));
}

bool UCharacterRecordComponent::HasRecordOfDefinition(FName DefinitionId) const
{
	if (DefinitionId.IsNone())
	{
		return false;
	}

	return Records.ContainsByPredicate([DefinitionId](const FCharacterRecord& Record) { return Record.DefinitionId == DefinitionId; });
}

bool UCharacterRecordComponent::BindActor(const FGuid& RecordId, AActor* Actor)
{
	if (!HasOwnerAuthority() || !Actor || !FindRecord(RecordId))
	{
		return false;
	}

	const AActor* Existing = GetBoundActor(RecordId);

	if (Existing && Existing != Actor)
	{
		return false;
	}

	BoundActors.Add(RecordId, Actor);

	return true;
}

void UCharacterRecordComponent::UnbindActor(const FGuid& RecordId, const AActor* Actor)
{
	if (GetBoundActor(RecordId) == Actor)
	{
		BoundActors.Remove(RecordId);
	}
}

AActor* UCharacterRecordComponent::GetBoundActor(const FGuid& RecordId) const
{
	const TWeakObjectPtr<AActor>* Bound = BoundActors.Find(RecordId);

	return Bound ? Bound->Get() : nullptr;
}

FText UCharacterRecordComponent::RollName(const UCharacterDefinition* Definition, const FGuid& RecordId)
{
	if (!Definition)
	{
		return FText::GetEmpty();
	}

	if (Definition->NamePool.Num() == 0)
	{
		return Definition->DisplayName;
	}

	// the id's own hash rather than a random stream: the same individual always lands on the
	// same entry, on every machine and every run
	const int32 Index = static_cast<int32>(GetTypeHash(RecordId) % static_cast<uint32>(Definition->NamePool.Num()));

	const FText& Rolled = Definition->NamePool[Index];

	return Rolled.IsEmpty() ? Definition->DisplayName : Rolled;
}

bool UCharacterRecordComponent::IsFactionKnown(FName FactionId) const
{
	if (FactionId.IsNone())
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	const UWorldFactionComponent* WorldFactions = Owner ? Owner->FindComponentByClass<UWorldFactionComponent>() : nullptr;

	// the world component seeds at its own BeginPlay, which may not have run yet when the first
	// unit registers - an empty world component is "not seeded", not "no factions exist"
	if (WorldFactions && WorldFactions->GetRecords().Num() > 0)
	{
		return WorldFactions->IsKnownFaction(FactionId);
	}

	return USmoresDefinitionLibrary::FindDefinition(UFactionDefinition::DefinitionType, FactionId) != nullptr;
}
