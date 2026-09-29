// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyTargetInfo.h"

bool FStrategyTargetInfo::DrawsIdenticallyTo(const FStrategyTargetInfo& Other) const
{
	if (bHasTarget != Other.bHasTarget
		|| !DisplayName.EqualTo(Other.DisplayName)
		|| !Classification.EqualTo(Other.Classification)
		|| !ActorName.EqualTo(Other.ActorName)
		|| bHasHealth != Other.bHasHealth
		|| FMath::RoundToInt(DistanceMeters) != FMath::RoundToInt(Other.DistanceMeters)
		|| FMath::RoundToInt(HealthFraction * 100.0f) != FMath::RoundToInt(Other.HealthFraction * 100.0f)
		|| Actions.Num() != Other.Actions.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Actions.Num(); ++Index)
	{
		const FTargetAction& Left = Actions[Index];
		const FTargetAction& Right = Other.Actions[Index];

		if (Left.Id != Right.Id || Left.bEnabled != Right.bEnabled || Left.DisabledReason != Right.DisabledReason
			|| !Left.Detail.EqualTo(Right.Detail))
		{
			return false;
		}
	}

	return true;
}

namespace StrategyTargetAction
{
	FName Open()
	{
		static const FName Id(TEXT("Open"));
		return Id;
	}

	FName Close()
	{
		static const FName Id(TEXT("Close"));
		return Id;
	}

	FName Loot()
	{
		static const FName Id(TEXT("Loot"));
		return Id;
	}

	FName Talk()
	{
		static const FName Id(TEXT("Talk"));
		return Id;
	}

	FName Trade()
	{
		static const FName Id(TEXT("Trade"));
		return Id;
	}

	FName PickUp()
	{
		static const FName Id(TEXT("PickUp"));
		return Id;
	}

	FName Examine()
	{
		static const FName Id(TEXT("Examine"));
		return Id;
	}

	FName Attack()
	{
		static const FName Id(TEXT("Attack"));
		return Id;
	}

	FName Heal()
	{
		static const FName Id(TEXT("Heal"));
		return Id;
	}

	FName Kidnap()
	{
		static const FName Id(TEXT("Kidnap"));
		return Id;
	}

	FName Pickpocket()
	{
		static const FName Id(TEXT("Pickpocket"));
		return Id;
	}

	FName KnockOut()
	{
		static const FName Id(TEXT("KnockOut"));
		return Id;
	}
}
