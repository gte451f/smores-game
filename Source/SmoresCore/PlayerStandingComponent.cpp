// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "PlayerStandingComponent.h"
#include "SmoresCore.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UPlayerStandingComponent::UPlayerStandingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UPlayerStandingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// a player state is owned by its player controller, so OwnerOnly means "that player's machine"
	DOREPLIFETIME_CONDITION(UPlayerStandingComponent, Standings, COND_OwnerOnly);
}

bool UPlayerStandingComponent::HasOwnerAuthority() const
{
	const AActor* Owner = GetOwner();

	return Owner && Owner->HasAuthority();
}

void UPlayerStandingComponent::OnRep_Standings()
{
	OnStandingChanged.Broadcast();
}

int32 UPlayerStandingComponent::GetStanding(FName FactionId) const
{
	const FPlayerFactionStanding* Entry = Standings.FindByPredicate([FactionId](const FPlayerFactionStanding& Candidate)
	{
		return Candidate.FactionId == FactionId;
	});

	return Entry ? Entry->Standing : SmoresStanding::Neutral;
}

bool UPlayerStandingComponent::SetStanding(FName FactionId, int32 NewStanding)
{
	if (!HasOwnerAuthority())
	{
		UE_LOG(LogSmoresCore, Warning, TEXT("SetStanding called on a non-authority machine for %s - ignored."), *GetNameSafe(GetOwner()));
		return false;
	}

	if (FactionId.IsNone())
	{
		return false;
	}

	const int32 Clamped = SmoresStanding::Clamp(NewStanding);

	FPlayerFactionStanding* Entry = Standings.FindByPredicate([FactionId](const FPlayerFactionStanding& Candidate)
	{
		return Candidate.FactionId == FactionId;
	});

	if (Entry && Entry->Standing == Clamped)
	{
		return true;
	}

	// a first write that lands on neutral changes nothing a query could see, so it isn't worth an entry
	if (!Entry && Clamped == SmoresStanding::Neutral)
	{
		return true;
	}

	if (!Entry)
	{
		Entry = &Standings.AddDefaulted_GetRef();
		Entry->FactionId = FactionId;
	}

	Entry->Standing = Clamped;

	// authority doesn't run OnRep on itself, so broadcast directly
	OnStandingChanged.Broadcast();

	return true;
}

bool UPlayerStandingComponent::AdjustStanding(FName FactionId, int32 Delta)
{
	// widen before adding, so a delta near INT32_MAX clamps rather than wrapping negative
	const int64 Target = static_cast<int64>(GetStanding(FactionId)) + Delta;

	return SetStanding(FactionId, static_cast<int32>(FMath::Clamp<int64>(Target, SmoresStanding::Min, SmoresStanding::Max)));
}
