// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogFacts.h"
#include "StrategyUnit.h"
#include "HealthComponent.h"
#include "CharacterDefinition.h"
#include "FactionDefinition.h"
#include "PlayerStandingComponent.h"
#include "DialogMemoryComponent.h"
#include "WalletComponent.h"
#include "SmoresDefinitionLibrary.h"
#include "GameFramework/PlayerState.h"

namespace SmoresDialog
{
	FName FactionDomain()
	{
		return FName(TEXT("Faction"));
	}

	FName CharacterDomain()
	{
		return FName(TEXT("CharacterDefinition"));
	}

	FName RoleDomain()
	{
		return FName(TEXT("Role"));
	}
}

namespace
{
	/** The unit standing in for Subject, or null - a fact about someone who isn't there answers unset */
	const AStrategyUnit* GetFactSubjectUnit(const FDialogContext& Context, EDialogSubject Subject)
	{
		return Cast<AStrategyUnit>(Context.GetCharacter(Subject));
	}

	/**
	 *  The six facts every character subject answers. Registered once per subject with its own
	 *  prefix, so "Speaker.Role" and "Event.Victim.Role" are the same question asked of two people.
	 */
	void RegisterCharacterFacts(FDialogFactRegistry& Registry, EDialogSubject Subject, const FString& Prefix, const FString& Who)
	{
		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".Definition"));
			Fact.Type = EDialogValueType::Name;
			Fact.Reads = Subject;
			Fact.ContentDomain = SmoresDialog::CharacterDomain();
			Fact.Description = FString::Printf(TEXT("the character definition id %s stands in for (Bandit, Trader), or None"), *Who);
			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);

				if (!Unit)
				{
					return FDialogValue::MakeUnset(EDialogValueType::Name);
				}

				const UCharacterDefinition* Definition = Unit->GetCharacterDefinition();

				return FDialogValue::MakeName(Definition ? Definition->DefinitionId : NAME_None);
			};

			Registry.Register(MoveTemp(Fact));
		}

		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".Role"));
			Fact.Type = EDialogValueType::Name;
			Fact.Reads = Subject;
			Fact.ContentDomain = SmoresDialog::RoleDomain();
			Fact.Description = FString::Printf(TEXT("the role id on %s's character definition (trader), or None"), *Who);
			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);

				if (!Unit)
				{
					return FDialogValue::MakeUnset(EDialogValueType::Name);
				}

				const UCharacterDefinition* Definition = Unit->GetCharacterDefinition();

				return FDialogValue::MakeName(Definition ? Definition->RoleId : NAME_None);
			};

			Registry.Register(MoveTemp(Fact));
		}

		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".Faction"));
			Fact.Type = EDialogValueType::Name;
			Fact.Reads = Subject;
			Fact.ContentDomain = SmoresDialog::FactionDomain();
			Fact.Description = FString::Printf(TEXT("%s's faction id (Raiders), or None when unaffiliated - squad members are"), *Who);
			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);

				return Unit ? FDialogValue::MakeName(Unit->GetFactionId()) : FDialogValue::MakeUnset(EDialogValueType::Name);
			};

			Registry.Register(MoveTemp(Fact));
		}

		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".Name"));
			Fact.Type = EDialogValueType::Name;
			Fact.Reads = Subject;
			Fact.Description = FString::Printf(TEXT("%s's name as the game shows it; quote it if it has a space (\"Merchant Ada\")"), *Who);
			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);

				return Unit ? FDialogValue::MakeName(FName(*Unit->GetHolderDisplayName().ToString())) : FDialogValue::MakeUnset(EDialogValueType::Name);
			};

			Registry.Register(MoveTemp(Fact));
		}

		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".LifeState"));
			Fact.Type = EDialogValueType::Name;
			Fact.Reads = Subject;
			Fact.Description = FString::Printf(TEXT("Alive, Downed or Dead, for %s"), *Who);

			// the vocabulary is EHealthState's, spelled the way the enum spells it, so the two can't drift
			const UEnum* HealthStates = StaticEnum<EHealthState>();

			for (int32 Index = 0; Index < HealthStates->NumEnums() - 1; ++Index)
			{
				Fact.AllowedNames.Add(FName(*HealthStates->GetNameStringByIndex(Index)));
			}

			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);
				const UHealthComponent* Health = Unit ? Unit->GetHealth() : nullptr;

				if (!Health)
				{
					return FDialogValue::MakeUnset(EDialogValueType::Name);
				}

				return FDialogValue::MakeName(FName(*StaticEnum<EHealthState>()->GetNameStringByValue(static_cast<int64>(Health->GetHealthState()))));
			};

			Registry.Register(MoveTemp(Fact));
		}

		{
			FDialogFact Fact;
			Fact.Name = FName(Prefix + TEXT(".Health"));
			Fact.Type = EDialogValueType::Number;
			Fact.Reads = Subject;
			Fact.Description = FString::Printf(TEXT("%s's health as a fraction, 0 to 1 (Health < 0.3 is badly hurt)"), *Who);
			Fact.Answer = [Subject](const FDialogContext& Context, FName)
			{
				const AStrategyUnit* Unit = GetFactSubjectUnit(Context, Subject);
				const UHealthComponent* Health = Unit ? Unit->GetHealth() : nullptr;

				if (!Health || Health->MaxHealth <= 0.0f)
				{
					return FDialogValue::MakeUnset(EDialogValueType::Number);
				}

				return FDialogValue::MakeNumber(FMath::Clamp(Health->GetHealth() / Health->MaxHealth, 0.0f, 1.0f));
			};

			Registry.Register(MoveTemp(Fact));
		}
	}
}

bool FDialogKnownIds::IsKnown(FName Domain, FName Id) const
{
	if (Id.IsNone())
	{
		return true;
	}

	const TSet<FName>* Ids = IdsByDomain.Find(Domain);

	return !Ids || Ids->Contains(Id);
}

FDialogKnownIds FDialogKnownIds::GatherFromGame()
{
	FDialogKnownIds Known;

	TArray<FName> FactionIds;
	USmoresDefinitionLibrary::GetDefinitionIds(UFactionDefinition::DefinitionType, FactionIds);
	Known.IdsByDomain.Add(SmoresDialog::FactionDomain(), TSet<FName>(FactionIds));

	TArray<FName> CharacterIds;
	USmoresDefinitionLibrary::GetDefinitionIds(UCharacterDefinition::DefinitionType, CharacterIds);
	Known.IdsByDomain.Add(SmoresDialog::CharacterDomain(), TSet<FName>(CharacterIds));

	// there is no role asset - a role is whatever some character definition says it is, so the
	// known roles are exactly the ones authored
	TSet<FName>& Roles = Known.IdsByDomain.Add(SmoresDialog::RoleDomain());

	for (const FName CharacterId : CharacterIds)
	{
		const UCharacterDefinition* Definition = Cast<UCharacterDefinition>(USmoresDefinitionLibrary::FindDefinition(UCharacterDefinition::DefinitionType, CharacterId));

		if (Definition && !Definition->RoleId.IsNone())
		{
			Roles.Add(Definition->RoleId);
		}
	}

	return Known;
}

bool FDialogFactRegistry::Register(FDialogFact Fact)
{
	if (Fact.Name.IsNone() || IndexByName.Contains(Fact.Name))
	{
		return false;
	}

	IndexByName.Add(Fact.Name, Facts.Num());
	Facts.Add(MoveTemp(Fact));

	return true;
}

const FDialogFact* FDialogFactRegistry::Find(FName Name) const
{
	const int32* Index = IndexByName.Find(Name);

	return Index ? &Facts[*Index] : nullptr;
}

FDialogValue FDialogFactRegistry::Ask(FName Name, const FDialogContext& Context, FName Argument) const
{
	const FDialogFact* Fact = Find(Name);

	if (!Fact || !Fact->Answer)
	{
		return FDialogValue::MakeUnset(Fact ? Fact->Type : EDialogValueType::Name);
	}

	FDialogValue Value = Fact->Answer(Context, Argument);

	// a fact answering in the wrong type is a code bug, but it must fail the clause rather than
	// compare a number against a name
	if (Value.Type != Fact->Type)
	{
		return FDialogValue::MakeUnset(Fact->Type);
	}

	return Value;
}

FDialogFactRegistry FDialogFactRegistry::MakeBuiltIn()
{
	FDialogFactRegistry Registry;

	RegisterCharacterFacts(Registry, EDialogSubject::Speaker, TEXT("Speaker"), TEXT("the speaker"));
	RegisterCharacterFacts(Registry, EDialogSubject::Listener, TEXT("Listener"), TEXT("whoever is spoken to"));
	RegisterCharacterFacts(Registry, EDialogSubject::Victim, TEXT("Event.Victim"), TEXT("the one who died"));

	{
		FDialogFact Fact;
		Fact.Name = FName(TEXT("StandingWithSpeaker"));
		Fact.Type = EDialogValueType::Number;
		Fact.Reads = EDialogSubject::Speaker | EDialogSubject::Player;
		Fact.Description = TEXT("this player's standing with the speaker's faction, -100 to 100 (0 when the speaker has none)");
		Fact.Answer = [](const FDialogContext& Context, FName)
		{
			const AStrategyUnit* Speaker = GetFactSubjectUnit(Context, EDialogSubject::Speaker);
			const APlayerState* Player = Context.Player.Get();
			const UPlayerStandingComponent* Standing = Player ? Player->FindComponentByClass<UPlayerStandingComponent>() : nullptr;

			if (!Speaker || !Standing)
			{
				return FDialogValue::MakeUnset(EDialogValueType::Number);
			}

			return FDialogValue::MakeNumber(Standing->GetStanding(Speaker->GetFactionId()));
		};

		Registry.Register(MoveTemp(Fact));
	}

	{
		FDialogFact Fact;
		Fact.Name = FName(TEXT("Gold"));
		Fact.Type = EDialogValueType::Number;
		Fact.Reads = EDialogSubject::Player;
		Fact.Description = TEXT("how much gold this player has");
		Fact.Answer = [](const FDialogContext& Context, FName)
		{
			const APlayerState* Player = Context.Player.Get();
			const UWalletComponent* Wallet = Player ? Player->FindComponentByClass<UWalletComponent>() : nullptr;

			return Wallet ? FDialogValue::MakeNumber(Wallet->GetGold()) : FDialogValue::MakeUnset(EDialogValueType::Number);
		};

		Registry.Register(MoveTemp(Fact));
	}

	{
		FDialogFact Fact;
		Fact.Name = FName(TEXT("Flag"));
		Fact.Type = EDialogValueType::Bool;
		Fact.Reads = EDialogSubject::Player;
		Fact.bTakesArgument = true;
		Fact.Description = TEXT("Flag(name): true once this squad's conversations have set that flag (<<SetFlag name>>)");
		Fact.Answer = [](const FDialogContext& Context, FName Argument)
		{
			const UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Context.Player.Get());

			return Memory ? FDialogValue::MakeBool(Memory->HasFlag(Argument)) : FDialogValue::MakeUnset(EDialogValueType::Bool);
		};

		Registry.Register(MoveTemp(Fact));
	}

	{
		FDialogFact Fact;
		Fact.Name = FName(TEXT("Seen"));
		Fact.Type = EDialogValueType::Bool;
		Fact.Reads = EDialogSubject::Player;
		Fact.bTakesArgument = true;
		Fact.Description = TEXT("Seen(conversation): true once this squad has had that conversation - its title, or package.title");
		Fact.Answer = [](const FDialogContext& Context, FName Argument)
		{
			const UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Context.Player.Get());

			return Memory ? FDialogValue::MakeBool(Memory->HasSeen(Argument)) : FDialogValue::MakeUnset(EDialogValueType::Bool);
		};

		Registry.Register(MoveTemp(Fact));
	}

	return Registry;
}
