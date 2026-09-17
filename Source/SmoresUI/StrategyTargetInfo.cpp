// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "StrategyTargetInfo.h"

namespace StrategyTargetAction
{
	FName Open()
	{
		static const FName Id(TEXT("Open"));
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

	FName Attack()
	{
		static const FName Id(TEXT("Attack"));
		return Id;
	}
}
