// Copyright 2026 Jim Jenkins. All Rights Reserved.


#include "SquadActivityWatcher.h"
#include "SmoresActivityLog.h"
#include "StrategyUnit.h"
#include "StrategyPlayerUnit.h"
#include "HealthComponent.h"

#define LOCTEXT_NAMESPACE "SquadActivityWatcher"

namespace
{
	/** The name to print for whoever dealt a hit, or empty if it wasn't anybody nameable */
	FText GetInstigatorName(const AActor* DamageInstigator)
	{
		const AStrategyUnit* Unit = Cast<AStrategyUnit>(DamageInstigator);

		return Unit ? Unit->GetHolderDisplayName() : FText::GetEmpty();
	}
}

void USquadActivityWatcher::Watch(AStrategyUnit* InUnit, USmoresActivityLog* InLog, bool bInPlayerSquad)
{
	Unwatch();

	Unit = InUnit;
	Log = InLog;
	bPlayerSquad = bInPlayerSquad;
	bHurtByPlayerSquad = false;

	if (!IsValid(InUnit))
	{
		return;
	}

	UHealthComponent* Health = InUnit->GetHealth();

	if (!Health)
	{
		// a unit that can't be hurt has no news, which is a legitimate state rather than a
		// misconfiguration - nothing to warn about
		return;
	}

	Health->OnDamaged.AddUniqueDynamic(this, &USquadActivityWatcher::HandleDamaged);
	Health->OnDowned.AddUniqueDynamic(this, &USquadActivityWatcher::HandleDowned);
	Health->OnRecovered.AddUniqueDynamic(this, &USquadActivityWatcher::HandleRecovered);
	Health->OnDied.AddUniqueDynamic(this, &USquadActivityWatcher::HandleDied);
}

void USquadActivityWatcher::Unwatch()
{
	if (AStrategyUnit* CurrentUnit = Unit.Get())
	{
		if (UHealthComponent* Health = CurrentUnit->GetHealth())
		{
			Health->OnDamaged.RemoveDynamic(this, &USquadActivityWatcher::HandleDamaged);
			Health->OnDowned.RemoveDynamic(this, &USquadActivityWatcher::HandleDowned);
			Health->OnRecovered.RemoveDynamic(this, &USquadActivityWatcher::HandleRecovered);
			Health->OnDied.RemoveDynamic(this, &USquadActivityWatcher::HandleDied);
		}
	}

	Unit = nullptr;
}

FText USquadActivityWatcher::GetUnitName() const
{
	const AStrategyUnit* CurrentUnit = Unit.Get();

	if (!IsValid(CurrentUnit))
	{
		return FText::GetEmpty();
	}

	const FText Name = CurrentUnit->GetHolderDisplayName();

	// a unit nobody got round to naming still has to be identifiable in the feed, the same way
	// FStrategyTargetInfo::bHasTarget exists so an unnamed actor doesn't read as a broken panel
	return Name.IsEmpty() ? LOCTEXT("UnnamedUnit", "Someone") : Name;
}

void USquadActivityWatcher::HandleDamaged(AActor* DamageInstigator)
{
	USmoresActivityLog* ActivityLog = Log.Get();

	if (!ActivityLog)
	{
		return;
	}

	const bool bByPlayerSquad = Cast<AStrategyPlayerUnit>(DamageInstigator) != nullptr;

	if (bByPlayerSquad)
	{
		bHurtByPlayerSquad = true;
	}

	const FText AttackerName = GetInstigatorName(DamageInstigator);

	if (bPlayerSquad)
	{
		const FText Line = AttackerName.IsEmpty()
			? LOCTEXT("SquadDamagedUnknown", "Took damage")
			: FText::Format(LOCTEXT("SquadDamagedBy", "Took damage from {0}"), AttackerName);

		ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Bad, Line, GetUnitName());

		return;
	}

	if (!bByPlayerSquad)
	{
		// a fight the player had no part in. See bHurtByPlayerSquad.
		return;
	}

	ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Good,
		FText::Format(LOCTEXT("SquadDealtDamage", "Hit {0}"), GetUnitName()), AttackerName);
}

void USquadActivityWatcher::HandleDowned()
{
	USmoresActivityLog* ActivityLog = Log.Get();

	if (!ActivityLog)
	{
		return;
	}

	if (bPlayerSquad)
	{
		ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Bad,
			LOCTEXT("SquadDowned", "Is down"), GetUnitName());

		return;
	}

	if (bHurtByPlayerSquad)
	{
		ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Good,
			LOCTEXT("EnemyDowned", "Is down"), GetUnitName());
	}
}

void USquadActivityWatcher::HandleRecovered()
{
	USmoresActivityLog* ActivityLog = Log.Get();

	// only the player's own squad getting back up is news. An NPC standing back up is news too,
	// but it is news about a fight that is still running, and the damage lines already say that.
	if (!ActivityLog || !bPlayerSquad)
	{
		return;
	}

	ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Good,
		LOCTEXT("SquadRecovered", "Is back on their feet"), GetUnitName());
}

void USquadActivityWatcher::HandleDied()
{
	USmoresActivityLog* ActivityLog = Log.Get();

	if (!ActivityLog)
	{
		return;
	}

	if (bPlayerSquad)
	{
		ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Bad,
			LOCTEXT("SquadDied", "Has been killed"), GetUnitName());

		return;
	}

	if (bHurtByPlayerSquad)
	{
		ActivityLog->Post(EActivityCategory::Squad, EActivitySeverity::Good,
			LOCTEXT("EnemyDied", "Has been killed"), GetUnitName());
	}
}

#undef LOCTEXT_NAMESPACE
