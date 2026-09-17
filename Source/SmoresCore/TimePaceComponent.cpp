// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "TimePaceComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "TimePaceComponent"

UTimePaceComponent::UTimePaceComponent()
{
	// nothing here needs a tick - the pace only changes when someone asks for a different one
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

const TArray<EGamePace>& UTimePaceComponent::GetPaceLadder()
{
	// EGamePace's declaration order, made iterable. Keep the two in step.
	static const TArray<EGamePace> Ladder = {
		EGamePace::Paused,
		EGamePace::Third,
		EGamePace::Half,
		EGamePace::ThreeQuarters,
		EGamePace::Normal,
		EGamePace::Double,
		EGamePace::Quadruple,
		EGamePace::Octuple
	};

	return Ladder;
}

float UTimePaceComponent::GetDilationForPace(EGamePace Pace)
{
	switch (Pace)
	{
	case EGamePace::Paused:        return 0.0001f;	// see the header - this is the engine's own floor
	case EGamePace::Third:         return 1.0f / 3.0f;
	case EGamePace::Half:          return 0.5f;
	case EGamePace::ThreeQuarters: return 0.75f;
	case EGamePace::Normal:        return 1.0f;
	case EGamePace::Double:        return 2.0f;
	case EGamePace::Quadruple:     return 4.0f;
	case EGamePace::Octuple:       return 8.0f;
	default:                       return 1.0f;
	}
}

FText UTimePaceComponent::GetPaceLabel(EGamePace Pace)
{
	switch (Pace)
	{
	case EGamePace::Paused:        return LOCTEXT("PacePaused", "Paused");
	case EGamePace::Third:         return LOCTEXT("PaceThird", "1/3x");
	case EGamePace::Half:          return LOCTEXT("PaceHalf", "1/2x");
	case EGamePace::ThreeQuarters: return LOCTEXT("PaceThreeQuarters", "3/4x");
	case EGamePace::Normal:        return LOCTEXT("PaceNormal", "1x");
	case EGamePace::Double:        return LOCTEXT("PaceDouble", "2x");
	case EGamePace::Quadruple:     return LOCTEXT("PaceQuadruple", "4x");
	case EGamePace::Octuple:       return LOCTEXT("PaceOctuple", "8x");
	default:                       return FText::GetEmpty();
	}
}

EGamePace UTimePaceComponent::StepPace(EGamePace Pace, int32 Steps)
{
	const TArray<EGamePace>& Ladder = GetPaceLadder();

	const int32 CurrentIndex = Ladder.IndexOfByKey(Pace);

	if (CurrentIndex == INDEX_NONE)
	{
		// a tier that isn't on the ladder can't be stepped from; hand back real time rather than
		// guessing, so a missed entry in GetPaceLadder shows up as "the keys stop working" rather
		// than as an off-by-one nobody traces
		return EGamePace::Normal;
	}

	return Ladder[FMath::Clamp(CurrentIndex + Steps, 0, Ladder.Num() - 1)];
}

void UTimePaceComponent::SetPace(EGamePace NewPace)
{
	// shared world state, so the same gate everything else in this project uses. A client that
	// reaches here has taken a wrong route in - Server_RequestPace is the front door.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (Pace == NewPace)
	{
		return;
	}

	// remember where to come back to *before* moving, so unpausing restores the speed that was
	// running rather than whatever the default happens to be
	if (NewPace == EGamePace::Paused)
	{
		ResumePace = Pace;
	}

	Pace = NewPace;

	ApplyDilation();
}

void UTimePaceComponent::BeginPlay()
{
	Super::BeginPlay();

	// the world starts at whatever tier this component defaults to. Normally 1x, so this is a
	// no-op - but it is what makes a level that ships paused, or a saved pace restored later,
	// actually take effect rather than being a number nothing read.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ApplyDilation();
	}
}

void UTimePaceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTimePaceComponent, Pace);
	DOREPLIFETIME(UTimePaceComponent, ResumePace);
}

void UTimePaceComponent::ApplyDilation() const
{
	// AWorldSettings::TimeDilation is itself replicated, so the server setting it is the whole of
	// what clients need - there is no multicast here and there shouldn't be one
	UGameplayStatics::SetGlobalTimeDilation(this, GetDilationForPace(Pace));
}

#undef LOCTEXT_NAMESPACE
