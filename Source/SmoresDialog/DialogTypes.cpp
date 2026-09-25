// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogTypes.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"

FDialogValue FDialogValue::MakeNumber(double InNumber)
{
	FDialogValue Value;
	Value.Type = EDialogValueType::Number;
	Value.bIsSet = true;
	Value.Number = InNumber;
	return Value;
}

FDialogValue FDialogValue::MakeName(FName InName)
{
	FDialogValue Value;
	Value.Type = EDialogValueType::Name;
	Value.bIsSet = true;
	Value.Name = InName;
	return Value;
}

FDialogValue FDialogValue::MakeBool(bool bInBool)
{
	FDialogValue Value;
	Value.Type = EDialogValueType::Bool;
	Value.bIsSet = true;
	Value.bBool = bInBool;
	return Value;
}

FDialogValue FDialogValue::MakeUnset(EDialogValueType InType)
{
	FDialogValue Value;
	Value.Type = InType;
	Value.bIsSet = false;
	return Value;
}

FString FDialogValue::ToString() const
{
	if (!bIsSet)
	{
		return TEXT("(none)");
	}

	switch (Type)
	{
	case EDialogValueType::Number:
		return FString::SanitizeFloat(Number);

	case EDialogValueType::Bool:
		return bBool ? TEXT("true") : TEXT("false");

	case EDialogValueType::Name:
	default:
		return Name.ToString();
	}
}

const AActor* FDialogContext::GetCharacter(EDialogSubject Subject) const
{
	switch (Subject)
	{
	case EDialogSubject::Speaker:
		return Speaker.Get();

	case EDialogSubject::Listener:
		return Listener.Get();

	case EDialogSubject::Victim:
		return Victim.Get();

	default:
		return nullptr;
	}
}

FString FDialogProblem::ToString() const
{
	const TCHAR* SeverityLabel = Severity == EDialogProblemSeverity::Error ? TEXT("error") : TEXT("warning");

	FString Where = Package;

	if (!File.IsEmpty())
	{
		Where += TEXT(" ") + File;

		if (Line > 0)
		{
			Where += FString::Printf(TEXT(":%d"), Line);
		}
	}

	return FString::Printf(TEXT("%s: %s: %s"), SeverityLabel, *Where, *Message);
}

namespace SmoresDialog
{
	const TArray<EBarkEvent>& GetAllBarkEvents()
	{
		static const TArray<EBarkEvent> Events = {
			EBarkEvent::Hurt,
			EBarkEvent::Downed,
			EBarkEvent::WitnessedDeath,
			EBarkEvent::TradeOpened,
			EBarkEvent::NothingToSay,
			EBarkEvent::Approached
		};

		return Events;
	}

	FString GetBarkEventName(EBarkEvent Event)
	{
		return StaticEnum<EBarkEvent>()->GetNameStringByValue(static_cast<int64>(Event));
	}

	bool ParseBarkEvent(const FString& Text, EBarkEvent& OutEvent)
	{
		const FString Trimmed = Text.TrimStartAndEnd();

		for (const EBarkEvent Event : GetAllBarkEvents())
		{
			if (Trimmed.Equals(GetBarkEventName(Event), ESearchCase::IgnoreCase))
			{
				OutEvent = Event;
				return true;
			}
		}

		return false;
	}

	EDialogSubject GetEventSubjects(EBarkEvent Event)
	{
		switch (Event)
		{
		case EBarkEvent::Hurt:
			// the listener is whoever landed the hit, and the player is theirs when it was a squad member
			return EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player;

		case EBarkEvent::Downed:
			// OnDowned carries no instigator, so there is nobody to address
			return EDialogSubject::Speaker;

		case EBarkEvent::WitnessedDeath:
			return EDialogSubject::Speaker | EDialogSubject::Victim;

		case EBarkEvent::TradeOpened:
		case EBarkEvent::NothingToSay:
			return EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player;

		case EBarkEvent::Approached:
			// the listener is the squad member who came near, and the player is theirs - so a line
			// can depend on who walked up and on standing, like TradeOpened
			return EDialogSubject::Speaker | EDialogSubject::Listener | EDialogSubject::Player;

		default:
			return EDialogSubject::Speaker;
		}
	}

	FString DescribeSubjects(EDialogSubject Subjects)
	{
		TArray<FString> Names;

		if (EnumHasAnyFlags(Subjects, EDialogSubject::Speaker))
		{
			Names.Add(TEXT("Speaker"));
		}

		if (EnumHasAnyFlags(Subjects, EDialogSubject::Listener))
		{
			Names.Add(TEXT("Listener"));
		}

		if (EnumHasAnyFlags(Subjects, EDialogSubject::Player))
		{
			Names.Add(TEXT("Player"));
		}

		if (EnumHasAnyFlags(Subjects, EDialogSubject::Victim))
		{
			Names.Add(TEXT("Event.Victim"));
		}

		return Names.Num() > 0 ? FString::Join(Names, TEXT(", ")) : TEXT("nobody");
	}
}
