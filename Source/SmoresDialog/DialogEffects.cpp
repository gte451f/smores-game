// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogEffects.h"
#include "DialogFacts.h"
#include "DialogHost.h"
#include "DialogMemoryComponent.h"
#include "FactionDefinition.h"
#include "PlayerStandingComponent.h"
#include "SmoresDefinitionLibrary.h"
#include "WalletComponent.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "SmoresDialogEffects"

namespace
{
	/** A whole number, strictly - "20" and "-10" yes, "20g" and "2.5" no */
	bool ParseEffectInteger(const FString& Text, int32& OutValue)
	{
		const FString Trimmed = Text.TrimStartAndEnd();

		if (Trimmed.IsEmpty() || !Trimmed.IsNumeric() || Trimmed.Contains(TEXT(".")))
		{
			return false;
		}

		OutValue = FCString::Atoi(*Trimmed);
		return true;
	}

	/** True for an argument a script fills in while it runs ({$amount}) - it can only be checked then */
	bool IsSubstituted(const FString& Argument)
	{
		return Argument.Contains(TEXT("{"));
	}

	/** Checks a gold amount written into a script */
	void CheckAmount(const TArray<FString>& Arguments, TArray<FString>& OutErrors)
	{
		int32 Amount = 0;

		if (Arguments.Num() > 0 && !IsSubstituted(Arguments[0]) && (!ParseEffectInteger(Arguments[0], Amount) || Amount <= 0))
		{
			OutErrors.Add(FString::Printf(TEXT("'%s' isn't an amount of gold - a whole number, 1 or more"), *Arguments[0]));
		}
	}

	/** The faction's name as players see it, or its id when it has none */
	FText GetFactionDisplayName(FName FactionId)
	{
		const USmoresDefinition* Faction = USmoresDefinitionLibrary::FindDefinition(UFactionDefinition::DefinitionType, FactionId);

		return Faction && !Faction->DisplayName.IsEmpty() ? Faction->DisplayName : FText::FromName(FactionId);
	}

	FDialogEffect MakeSetFlagEffect()
	{
		FDialogEffect Effect;
		Effect.Name = TEXT("SetFlag");
		Effect.MinArguments = 1;
		Effect.MaxArguments = 2;
		Effect.Usage = TEXT("SetFlag <flag> [true|false]");
		Effect.Description = TEXT("sets a flag this squad remembers (false clears it); Flag(name) reads it back");
		Effect.Check = [](const TArray<FString>& Arguments, const FDialogKnownIds*, TArray<FString>& OutErrors, TArray<FString>&)
		{
			if (!IsSubstituted(Arguments[0]) && !SmoresDialog::IsValidFlagName(Arguments[0]))
			{
				OutErrors.Add(FString::Printf(TEXT("'%s' isn't a flag name - letters, digits and _"), *Arguments[0]));
			}

			if (Arguments.Num() > 1 && !IsSubstituted(Arguments[1])
				&& !Arguments[1].Equals(TEXT("true"), ESearchCase::IgnoreCase) && !Arguments[1].Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				OutErrors.Add(FString::Printf(TEXT("SetFlag's second word is true or false, not '%s'"), *Arguments[1]));
			}
		};
		Effect.Run = [](const FDialogEffectContext& Context, const TArray<FString>& Arguments)
		{
			FDialogEffectOutcome Outcome;
			UDialogMemoryComponent* Memory = UDialogMemoryComponent::Get(Context.Player);

			if (Memory)
			{
				const bool bValue = Arguments.Num() < 2 || !Arguments[1].Equals(TEXT("false"), ESearchCase::IgnoreCase);

				// setting a flag that was already set did what was asked, so it counts as done
				Memory->SetFlag(FName(*Arguments[0]), bValue);
				Outcome.bDone = Memory->HasFlag(FName(*Arguments[0])) == bValue;
			}

			return Outcome;
		};

		return Effect;
	}

	FDialogEffect MakeChangeStandingEffect()
	{
		FDialogEffect Effect;
		Effect.Name = TEXT("ChangeStanding");
		Effect.MinArguments = 2;
		Effect.MaxArguments = 2;
		Effect.Usage = TEXT("ChangeStanding <faction> <amount>");
		Effect.Description = TEXT("moves this player's standing with a faction by amount (-10, 5), kept within -100..100");
		Effect.Check = [](const TArray<FString>& Arguments, const FDialogKnownIds* KnownIds, TArray<FString>& OutErrors, TArray<FString>& OutWarnings)
		{
			int32 Delta = 0;

			if (!IsSubstituted(Arguments[0]) && KnownIds && !KnownIds->IsKnown(SmoresDialog::FactionDomain(), FName(*Arguments[0])))
			{
				OutWarnings.Add(FString::Printf(TEXT("'%s' isn't a faction any loaded content has - kept, in case it comes from another mod"), *Arguments[0]));
			}

			if (!IsSubstituted(Arguments[1]) && (!ParseEffectInteger(Arguments[1], Delta) || Delta == 0))
			{
				OutErrors.Add(FString::Printf(TEXT("'%s' isn't a change in standing - a whole number like -10 or 5"), *Arguments[1]));
			}
		};
		Effect.Run = [](const FDialogEffectContext& Context, const TArray<FString>& Arguments)
		{
			FDialogEffectOutcome Outcome;
			UPlayerStandingComponent* Standing = Context.Player ? Context.Player->FindComponentByClass<UPlayerStandingComponent>() : nullptr;
			int32 Delta = 0;

			if (Standing && ParseEffectInteger(Arguments[1], Delta))
			{
				const FName FactionId(*Arguments[0]);
				const int32 Before = Standing->GetStanding(FactionId);

				Standing->AdjustStanding(FactionId, Delta);

				const int32 After = Standing->GetStanding(FactionId);

				Outcome.bDone = true;

				// what actually moved, which the clamp can make less than asked
				if (After != Before)
				{
					Outcome.FeedLine = FText::Format(LOCTEXT("StandingChanged", "{0} standing {1}{2} (now {3})"),
						GetFactionDisplayName(FactionId), FText::FromString(After > Before ? TEXT("+") : TEXT("")), FText::AsNumber(After - Before), FText::AsNumber(After));
				}
			}

			return Outcome;
		};

		return Effect;
	}

	FDialogEffect MakeTakeMoneyEffect()
	{
		FDialogEffect Effect;
		Effect.Name = TEXT("TakeMoney");
		Effect.MinArguments = 1;
		Effect.MaxArguments = 1;
		Effect.Usage = TEXT("TakeMoney <amount>");
		Effect.Description = TEXT("takes gold from this player, all or nothing - guard the choice with <<if gold() >= amount>>");
		Effect.Check = [](const TArray<FString>& Arguments, const FDialogKnownIds*, TArray<FString>& OutErrors, TArray<FString>&)
		{
			CheckAmount(Arguments, OutErrors);
		};
		Effect.Run = [](const FDialogEffectContext& Context, const TArray<FString>& Arguments)
		{
			FDialogEffectOutcome Outcome;
			UWalletComponent* Wallet = Context.Player ? Context.Player->FindComponentByClass<UWalletComponent>() : nullptr;
			int32 Amount = 0;

			if (!Wallet || !ParseEffectInteger(Arguments[0], Amount) || Amount <= 0)
			{
				return Outcome;
			}

			// all or nothing: TrySpendGold leaves the wallet untouched when it can't cover the amount.
			// A script that forgot its <<if gold() >= ...>> guard lands here and is refused.
			if (!Wallet->TrySpendGold(Amount))
			{
				Outcome.Refusal = ESmoresRefusalReason::CannotAfford;
				return Outcome;
			}

			Outcome.bDone = true;
			Outcome.FeedLine = FText::Format(LOCTEXT("PaidGold", "Paid {0} gold"), FText::AsNumber(Amount));
			return Outcome;
		};

		return Effect;
	}

	FDialogEffect MakeGiveMoneyEffect()
	{
		FDialogEffect Effect;
		Effect.Name = TEXT("GiveMoney");
		Effect.MinArguments = 1;
		Effect.MaxArguments = 1;
		Effect.Usage = TEXT("GiveMoney <amount>");
		Effect.Description = TEXT("gives this player gold");
		Effect.Check = [](const TArray<FString>& Arguments, const FDialogKnownIds*, TArray<FString>& OutErrors, TArray<FString>&)
		{
			CheckAmount(Arguments, OutErrors);
		};
		Effect.Run = [](const FDialogEffectContext& Context, const TArray<FString>& Arguments)
		{
			FDialogEffectOutcome Outcome;
			UWalletComponent* Wallet = Context.Player ? Context.Player->FindComponentByClass<UWalletComponent>() : nullptr;
			int32 Amount = 0;

			if (Wallet && ParseEffectInteger(Arguments[0], Amount) && Amount > 0)
			{
				Wallet->AddGold(Amount);
				Outcome.bDone = true;
				Outcome.FeedLine = FText::Format(LOCTEXT("ReceivedGold", "Received {0} gold"), FText::AsNumber(Amount));
			}

			return Outcome;
		};

		return Effect;
	}

	FDialogEffect MakeOpenTradeEffect()
	{
		FDialogEffect Effect;
		Effect.Name = TEXT("OpenTrade");
		Effect.MinArguments = 0;
		Effect.MaxArguments = 0;
		Effect.Usage = TEXT("OpenTrade");
		Effect.Description = TEXT("opens the NPC's shop and ends the conversation - the trade screen replaces the window; nothing after it runs");
		Effect.bNeedsWindow = true;
		Effect.Check = [](const TArray<FString>&, const FDialogKnownIds*, TArray<FString>&, TArray<FString>&) {};
		Effect.Run = [](const FDialogEffectContext& Context, const TArray<FString>&)
		{
			FDialogEffectOutcome Outcome;

			// the controller knows whether the NPC keeps a shop, and opens it on its own client
			if (Context.Host && Context.Speaker)
			{
				Outcome.bDone = Context.Host->OpenTradeWith(Context.Speaker);
			}

			Outcome.bEndsConversation = Outcome.bDone;
			return Outcome;
		};

		return Effect;
	}
}

bool FDialogEffectRegistry::Register(FDialogEffect Effect)
{
	if (Effect.Name.IsNone() || IndexByName.Contains(Effect.Name))
	{
		return false;
	}

	IndexByName.Add(Effect.Name, Effects.Num());
	Effects.Add(MoveTemp(Effect));

	return true;
}

const FDialogEffect* FDialogEffectRegistry::Find(FName Name) const
{
	const int32* Index = IndexByName.Find(Name);

	return Index ? &Effects[*Index] : nullptr;
}

FDialogEffectOutcome FDialogEffectRegistry::Run(FName Name, const FDialogEffectContext& Context, const TArray<FString>& Arguments) const
{
	const FDialogEffect* Effect = Find(Name);

	// every effect changes shared state, so the gate is here once rather than in each of them
	if (!Effect || !Effect->Run || !Context.Player || !Context.Player->HasAuthority())
	{
		return FDialogEffectOutcome();
	}

	if (Arguments.Num() < Effect->MinArguments || Arguments.Num() > Effect->MaxArguments)
	{
		return FDialogEffectOutcome();
	}

	return Effect->Run(Context, Arguments);
}

FDialogEffectRegistry FDialogEffectRegistry::MakeBuiltIn()
{
	FDialogEffectRegistry Registry;

	Registry.Register(MakeSetFlagEffect());
	Registry.Register(MakeChangeStandingEffect());
	Registry.Register(MakeTakeMoneyEffect());
	Registry.Register(MakeGiveMoneyEffect());
	Registry.Register(MakeOpenTradeEffect());

	return Registry;
}

namespace SmoresDialog
{
	bool FindChoiceReason(FName Key, ESmoresRefusalReason& OutReason)
	{
		// one row per key; the words are the refusal line's (URefusalWidget::GetRefusalText), so a
		// greyed choice and a refused action never describe the same reason two ways. Only add a key
		// some content uses.
		if (Key == FName(TEXT("not_enough_money")))
		{
			OutReason = ESmoresRefusalReason::CannotAfford;
			return true;
		}

		return false;
	}

	TArray<FName> GetChoiceReasonKeys()
	{
		return { FName(TEXT("not_enough_money")) };
	}

	bool IsValidFlagName(const FString& Name)
	{
		if (Name.IsEmpty())
		{
			return false;
		}

		for (const TCHAR Character : Name)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				return false;
			}
		}

		return true;
	}
}

#undef LOCTEXT_NAMESPACE
