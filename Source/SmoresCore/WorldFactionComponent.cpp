// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "WorldFactionComponent.h"
#include "FactionDefinition.h"
#include "SmoresCore.h"
#include "SmoresDefinitionLibrary.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UWorldFactionComponent::UWorldFactionComponent()
{
	// nothing here moves on its own - standing only changes when something changes it
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UWorldFactionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!HasOwnerAuthority())
	{
		// clients receive the records through replication, like any other change
		return;
	}

	// every faction the Asset Manager knows about exists in the world. There is no per-level
	// faction list to wire - adding a faction is authoring its asset, nothing else.
	TArray<FName> FactionIds;
	USmoresDefinitionLibrary::GetDefinitionIds(UFactionDefinition::DefinitionType, FactionIds);

	TArray<const UFactionDefinition*> Definitions;

	for (const FName& FactionId : FactionIds)
	{
		const UFactionDefinition* Definition = Cast<UFactionDefinition>(
			USmoresDefinitionLibrary::FindDefinition(UFactionDefinition::DefinitionType, FactionId));

		if (Definition)
		{
			Definitions.Add(Definition);
		}
		else
		{
			UE_LOG(LogSmoresCore, Warning, TEXT("Faction id %s is registered but does not resolve to a UFactionDefinition - skipped."), *FactionId.ToString());
		}
	}

	InitializeFromDefinitions(Definitions);
}

void UWorldFactionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWorldFactionComponent, Records);
	DOREPLIFETIME(UWorldFactionComponent, PairStandings);
}

bool UWorldFactionComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();

	return Owner && Owner->HasAuthority();
}

void UWorldFactionComponent::OnRep_Factions()
{
	OnFactionsChanged.Broadcast();
}

int32 UWorldFactionComponent::InitializeFromDefinitions(const TArray<const UFactionDefinition*>& Definitions)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresCore, Warning, TEXT("InitializeFromDefinitions called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return INDEX_NONE;
	}

	// id order, so the records read in a stable order and a relation authored on both sides
	// resolves identically every run rather than depending on the asset registry's scan order
	TArray<const UFactionDefinition*> Sorted;

	for (const UFactionDefinition* Definition : Definitions)
	{
		if (!Definition || Definition->DefinitionId.IsNone())
		{
			UE_LOG(LogSmoresCore, Warning, TEXT("Skipping a faction definition with no DefinitionId (%s)."), *GetNameSafe(Definition));
			continue;
		}

		Sorted.Add(Definition);
	}

	Sorted.StableSort([](const UFactionDefinition& A, const UFactionDefinition& B)
	{
		return A.DefinitionId.Compare(B.DefinitionId) < 0;
	});

	Records.Reset();
	PairStandings.Reset();

	for (const UFactionDefinition* Definition : Sorted)
	{
		if (IsKnownFaction(Definition->DefinitionId))
		{
			UE_LOG(LogSmoresCore, Warning, TEXT("Two faction definitions share the id %s - keeping the first, skipping %s."),
				*Definition->DefinitionId.ToString(), *Definition->GetName());
			continue;
		}

		FFactionRecord& Record = Records.AddDefaulted_GetRef();
		Record.FactionId = Definition->DefinitionId;
		Record.Tier = Definition->StartingTier;
	}

	// relations second, once every record exists - a relation can name a faction that sorts after it
	for (const UFactionDefinition* Definition : Sorted)
	{
		for (const FFactionStartingRelation& Relation : Definition->StartingRelations)
		{
			const FName OtherId = Relation.Faction ? Relation.Faction->DefinitionId : NAME_None;

			if (!IsKnownFaction(OtherId) || OtherId == Definition->DefinitionId)
			{
				UE_LOG(LogSmoresCore, Warning, TEXT("Faction %s has a starting relation naming %s, which is not another known faction - skipped."),
					*Definition->DefinitionId.ToString(), *OtherId.ToString());
				continue;
			}

			FName FirstId = Definition->DefinitionId;
			FName SecondId = OtherId;
			OrderPair(FirstId, SecondId);

			if (const FFactionPairStanding* Existing = FindPair(FirstId, SecondId))
			{
				if (Existing->Standing != SmoresStanding::Clamp(Relation.Standing))
				{
					UE_LOG(LogSmoresCore, Warning, TEXT("The %s / %s starting standing is authored on both factions with different values (%d, %d) - using %d."),
						*FirstId.ToString(), *SecondId.ToString(), Existing->Standing, Relation.Standing, SmoresStanding::Clamp(Relation.Standing));
				}
			}

			WritePair(Definition->DefinitionId, OtherId, Relation.Standing);
		}
	}

	// authority doesn't run OnRep on itself, so broadcast directly
	OnFactionsChanged.Broadcast();

	return Records.Num();
}

const FFactionRecord* UWorldFactionComponent::FindRecord(FName FactionId) const
{
	if (FactionId.IsNone())
	{
		return nullptr;
	}

	return Records.FindByPredicate([FactionId](const FFactionRecord& Record) { return Record.FactionId == FactionId; });
}

bool UWorldFactionComponent::GetFactionTier(FName FactionId, EFactionTier& OutTier) const
{
	const FFactionRecord* Record = FindRecord(FactionId);

	if (!Record)
	{
		return false;
	}

	OutTier = Record->Tier;

	return true;
}

void UWorldFactionComponent::OrderPair(FName& InOutFirst, FName& InOutSecond)
{
	// lexical, not FName's own operator< - that compares name-table indices, which differ between
	// runs and machines, and a saved pair has to land under the same key when it loads
	if (InOutSecond.Compare(InOutFirst) < 0)
	{
		Swap(InOutFirst, InOutSecond);
	}
}

FFactionPairStanding* UWorldFactionComponent::FindPair(FName FirstId, FName SecondId)
{
	return PairStandings.FindByPredicate([FirstId, SecondId](const FFactionPairStanding& Pair)
	{
		return Pair.FirstId == FirstId && Pair.SecondId == SecondId;
	});
}

const FFactionPairStanding* UWorldFactionComponent::FindPair(FName FirstId, FName SecondId) const
{
	return const_cast<UWorldFactionComponent*>(this)->FindPair(FirstId, SecondId);
}

int32 UWorldFactionComponent::GetStandingBetween(FName FactionA, FName FactionB) const
{
	const bool bBothKnown = IsKnownFaction(FactionA) && IsKnownFaction(FactionB);

	if (!bBothKnown)
	{
		// a faction nobody has heard of - or one a stripped mod took with it - is regarded neutrally
		return SmoresStanding::Neutral;
	}

	if (FactionA == FactionB)
	{
		return SmoresStanding::Max;
	}

	OrderPair(FactionA, FactionB);

	const FFactionPairStanding* Pair = FindPair(FactionA, FactionB);

	return Pair ? Pair->Standing : SmoresStanding::Neutral;
}

bool UWorldFactionComponent::WritePair(FName FactionA, FName FactionB, int32 NewStanding)
{
	OrderPair(FactionA, FactionB);

	const int32 Clamped = SmoresStanding::Clamp(NewStanding);

	FFactionPairStanding* Pair = FindPair(FactionA, FactionB);

	if (!Pair)
	{
		Pair = &PairStandings.AddDefaulted_GetRef();
		Pair->FirstId = FactionA;
		Pair->SecondId = FactionB;
	}
	else if (Pair->Standing == Clamped)
	{
		return false;
	}

	Pair->Standing = Clamped;

	return true;
}

bool UWorldFactionComponent::SetStandingBetween(FName FactionA, FName FactionB, int32 NewStanding)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresCore, Warning, TEXT("SetStandingBetween called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!IsKnownFaction(FactionA) || !IsKnownFaction(FactionB) || FactionA == FactionB)
	{
		return false;
	}

	if (WritePair(FactionA, FactionB, NewStanding))
	{
		OnFactionsChanged.Broadcast();
	}

	return true;
}

bool UWorldFactionComponent::AdjustStandingBetween(FName FactionA, FName FactionB, int32 Delta)
{
	// widen before adding, so a delta near INT32_MAX clamps rather than wrapping negative
	const int64 Target = static_cast<int64>(GetStandingBetween(FactionA, FactionB)) + Delta;

	return SetStandingBetween(FactionA, FactionB, static_cast<int32>(FMath::Clamp<int64>(Target, SmoresStanding::Min, SmoresStanding::Max)));
}

bool UWorldFactionComponent::SetFactionTier(FName FactionId, EFactionTier NewTier)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresCore, Warning, TEXT("SetFactionTier called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return false;
	}

	FFactionRecord* Record = Records.FindByPredicate([FactionId](const FFactionRecord& Candidate) { return Candidate.FactionId == FactionId; });

	if (!Record || FactionId.IsNone())
	{
		return false;
	}

	if (Record->Tier != NewTier)
	{
		Record->Tier = NewTier;

		OnFactionsChanged.Broadcast();
	}

	return true;
}
