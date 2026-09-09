// Copyright Epic Games, Inc. All Rights Reserved.


#include "DamageNumberActor.h"
#include "DamageNumberWidget.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/RotationMatrix.h"
#include "SmoresCombat.h"

ADamageNumberActor::ADamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawSize(FVector2D(200.0f, 80.0f));
	WidgetComponent->SetTwoSided(true);
}

void ADamageNumberActor::Initialize(float DamageAmount)
{
	UUserWidget* ResolvedWidget = WidgetComponent->GetUserWidgetObject();
	CachedWidget = Cast<UDamageNumberWidget>(ResolvedWidget);

	UE_LOG(LogSmoresCombat, Warning, TEXT("[Combat] %s Initialize(%.0f): GetUserWidgetObject()=%s, CachedWidget valid=%s"),
		*GetName(), DamageAmount,
		ResolvedWidget ? *ResolvedWidget->GetClass()->GetName() : TEXT("null"),
		CachedWidget.IsValid() ? TEXT("true") : TEXT("false"));

	if (CachedWidget.IsValid())
	{
		CachedWidget->SetDamageAmount(DamageAmount);
	}

	// safety net in case the Tick-driven Destroy() call below is ever skipped (e.g. paused world)
	SetLifeSpan(Lifetime + 1.0f);
}

void ADamageNumberActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;

	const float Alpha = FMath::Clamp(ElapsedTime / Lifetime, 0.0f, 1.0f);

	AddActorWorldOffset(FVector(0.0f, 0.0f, (FloatDistance / Lifetime) * DeltaSeconds));

	// keep facing the viewer - the widget is two-sided, so this only affects legibility
	// (mirrored vs. upright text), not visibility, if the camera rotates
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCamera = CameraManager->GetCameraLocation() - GetActorLocation();
		SetActorRotation(FRotationMatrix::MakeFromX(ToCamera).Rotator());
	}

	if (Alpha >= FadeStartFraction && CachedWidget.IsValid())
	{
		const float FadeAlpha = (Alpha - FadeStartFraction) / FMath::Max(1.0f - FadeStartFraction, KINDA_SMALL_NUMBER);
		CachedWidget->SetRenderOpacity(1.0f - FadeAlpha);
	}

	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}
