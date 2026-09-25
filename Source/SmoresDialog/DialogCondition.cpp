// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogCondition.h"
#include "DialogFacts.h"

namespace
{
	enum class EDialogTokenKind : uint8
	{
		Identifier,
		Number,
		String,
		Op,
		LParen,
		RParen,
		Semicolon
	};

	struct FDialogToken
	{
		EDialogTokenKind Kind = EDialogTokenKind::Identifier;

		/** An identifier's or a string's text */
		FString Text;

		EDialogCompareOp Op = EDialogCompareOp::Equal;

		double Number = 0.0;

		/** Where the token sits in the condition text, so a clause can be quoted back as written */
		int32 Start = 0;
		int32 End = 0;
	};

	/** What a clause's side was written as, before validation decides whether it is a fact or a value */
	struct FDialogRawOperand
	{
		enum class EKind : uint8 { Identifier, Call, Number, String } Kind = EKind::Identifier;

		FString Text;

		FString Argument;

		double Number = 0.0;
	};

	bool IsDialogIdentifierStart(TCHAR Character)
	{
		return FChar::IsAlpha(Character) || Character == TEXT('_');
	}

	bool IsDialogIdentifierPart(TCHAR Character)
	{
		return FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('.');
	}

	bool TokenizeDialogCondition(const FString& Text, TArray<FDialogToken>& OutTokens, FString& OutError)
	{
		const int32 Length = Text.Len();
		int32 Index = 0;

		auto Peek = [&Text, Length](int32 At) -> TCHAR
		{
			return At < Length ? Text[At] : TEXT('\0');
		};

		while (Index < Length)
		{
			const TCHAR Character = Text[Index];

			if (FChar::IsWhitespace(Character))
			{
				++Index;
				continue;
			}

			FDialogToken Token;
			Token.Start = Index;

			if (Character == TEXT(';') || Character == TEXT('(') || Character == TEXT(')'))
			{
				Token.Kind = Character == TEXT(';') ? EDialogTokenKind::Semicolon
					: Character == TEXT('(') ? EDialogTokenKind::LParen
					: EDialogTokenKind::RParen;
				++Index;
			}
			else if (Character == TEXT('"'))
			{
				const int32 Close = Text.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index + 1);

				if (Close == INDEX_NONE)
				{
					OutError = TEXT("a quoted value is missing its closing \"");
					return false;
				}

				Token.Kind = EDialogTokenKind::String;
				Token.Text = Text.Mid(Index + 1, Close - Index - 1);
				Index = Close + 1;
			}
			else if (Character == TEXT('=') || Character == TEXT('!') || Character == TEXT('<') || Character == TEXT('>'))
			{
				const bool bFollowedByEquals = Peek(Index + 1) == TEXT('=');

				Token.Kind = EDialogTokenKind::Op;

				if (Character == TEXT('='))
				{
					if (!bFollowedByEquals)
					{
						// the one mistake every writer makes once - say what to type instead
						OutError = TEXT("use '==' to compare, not '='");
						return false;
					}

					Token.Op = EDialogCompareOp::Equal;
				}
				else if (Character == TEXT('!'))
				{
					if (!bFollowedByEquals)
					{
						OutError = TEXT("'!' on its own means nothing here - write '!=' or '== false'");
						return false;
					}

					Token.Op = EDialogCompareOp::NotEqual;
				}
				else if (Character == TEXT('<'))
				{
					Token.Op = bFollowedByEquals ? EDialogCompareOp::LessEqual : EDialogCompareOp::Less;
				}
				else
				{
					Token.Op = bFollowedByEquals ? EDialogCompareOp::GreaterEqual : EDialogCompareOp::Greater;
				}

				Index += bFollowedByEquals ? 2 : 1;
			}
			else if (FChar::IsDigit(Character) || (Character == TEXT('-') && FChar::IsDigit(Peek(Index + 1))))
			{
				int32 Cursor = Index + 1;

				while (FChar::IsDigit(Peek(Cursor)))
				{
					++Cursor;
				}

				if (Peek(Cursor) == TEXT('.') && FChar::IsDigit(Peek(Cursor + 1)))
				{
					Cursor += 2;

					while (FChar::IsDigit(Peek(Cursor)))
					{
						++Cursor;
					}
				}

				// "20abc" is neither a number nor an id; refusing it beats reading it as 20
				if (IsDialogIdentifierPart(Peek(Cursor)))
				{
					int32 WordEnd = Cursor;

					while (IsDialogIdentifierPart(Peek(WordEnd)))
					{
						++WordEnd;
					}

					OutError = FString::Printf(TEXT("'%s' isn't a number, and an id can't start with a digit"), *Text.Mid(Index, WordEnd - Index));
					return false;
				}

				Token.Kind = EDialogTokenKind::Number;
				Token.Number = FCString::Atod(*Text.Mid(Index, Cursor - Index));
				Index = Cursor;
			}
			else if (IsDialogIdentifierStart(Character))
			{
				int32 Cursor = Index + 1;

				while (IsDialogIdentifierPart(Peek(Cursor)))
				{
					++Cursor;
				}

				Token.Kind = EDialogTokenKind::Identifier;
				Token.Text = Text.Mid(Index, Cursor - Index);
				Index = Cursor;
			}
			else
			{
				OutError = FString::Printf(TEXT("unexpected '%c'"), Character);
				return false;
			}

			Token.End = Index;
			OutTokens.Add(MoveTemp(Token));
		}

		return true;
	}

	/** Reads one side of a clause starting at Cursor, advancing past it */
	bool ParseDialogOperand(const TArray<FDialogToken>& Tokens, int32 End, int32& Cursor, FDialogRawOperand& OutOperand, FString& OutError)
	{
		if (Cursor >= End)
		{
			OutError = TEXT("a comparison is missing its right-hand side");
			return false;
		}

		const FDialogToken& Token = Tokens[Cursor];

		switch (Token.Kind)
		{
		case EDialogTokenKind::Number:
			OutOperand.Kind = FDialogRawOperand::EKind::Number;
			OutOperand.Number = Token.Number;
			++Cursor;
			return true;

		case EDialogTokenKind::String:
			OutOperand.Kind = FDialogRawOperand::EKind::String;
			OutOperand.Text = Token.Text;
			++Cursor;
			return true;

		case EDialogTokenKind::Identifier:
			OutOperand.Kind = FDialogRawOperand::EKind::Identifier;
			OutOperand.Text = Token.Text;
			++Cursor;

			// the call form: Flag(met_kess)
			if (Cursor < End && Tokens[Cursor].Kind == EDialogTokenKind::LParen)
			{
				// Cursor is on the '(' - the argument follows it, then the ')', all inside this clause
				const bool bWellFormed = Cursor + 2 < End
					&& (Tokens[Cursor + 1].Kind == EDialogTokenKind::Identifier || Tokens[Cursor + 1].Kind == EDialogTokenKind::String)
					&& Tokens[Cursor + 2].Kind == EDialogTokenKind::RParen;

				if (!bWellFormed)
				{
					OutError = FString::Printf(TEXT("'%s(' needs one id and a closing ')'"), *Token.Text);
					return false;
				}

				OutOperand.Kind = FDialogRawOperand::EKind::Call;
				OutOperand.Argument = Tokens[Cursor + 1].Text;
				Cursor += 3;
			}

			return true;

		case EDialogTokenKind::Op:
			OutError = TEXT("two comparisons in a row");
			return false;

		default:
			OutError = TEXT("unexpected bracket");
			return false;
		}
	}

	FString DescribeDialogType(EDialogValueType Type)
	{
		switch (Type)
		{
		case EDialogValueType::Number:
			return TEXT("a number");

		case EDialogValueType::Bool:
			return TEXT("a yes/no value");

		case EDialogValueType::Name:
		default:
			return TEXT("a name");
		}
	}

	FString DescribeDialogDomain(FName Domain)
	{
		if (Domain == SmoresDialog::FactionDomain())
		{
			return TEXT("faction");
		}

		if (Domain == SmoresDialog::CharacterDomain())
		{
			return TEXT("character definition");
		}

		if (Domain == SmoresDialog::RoleDomain())
		{
			return TEXT("role");
		}

		return Domain.ToString();
	}

	/** A clause with case and spacing folded away, for spotting the same clause written twice */
	FString NormalizeDialogClause(const FString& ClauseText)
	{
		FString Normalized;
		Normalized.Reserve(ClauseText.Len());

		for (const TCHAR Character : ClauseText)
		{
			if (!FChar::IsWhitespace(Character))
			{
				Normalized.AppendChar(FChar::ToLower(Character));
			}
		}

		return Normalized;
	}

	/**
	 *  Turns a parsed side into a fact or a value, checking the fact exists and asks only about
	 *  people this event carries. ExpectedType is the left fact's type when resolving the right-hand
	 *  side, so "true" reads as a yes/no value only where one is wanted.
	 */
	bool ResolveDialogOperand(
		const FDialogRawOperand& Raw,
		const FDialogFactRegistry& Facts,
		EDialogSubject AvailableSubjects,
		bool bIsLeft,
		const TOptional<EDialogValueType>& ExpectedType,
		FDialogOperand& OutOperand,
		EDialogValueType& OutType,
		FString& OutError)
	{
		auto ResolveFact = [&](const FString& FactText, const FString* Argument) -> bool
		{
			const FName FactName(*FactText);
			const FDialogFact* Fact = Facts.Find(FactName);

			if (!Fact)
			{
				OutError = FString::Printf(TEXT("'%s' isn't a fact the game knows - SmoresDialogReport facts lists them"), *FactText);
				return false;
			}

			if (Fact->bTakesArgument && !Argument)
			{
				OutError = FString::Printf(TEXT("'%s' needs an id in brackets, like %s(some_id)"), *FactText, *FactText);
				return false;
			}

			if (!Fact->bTakesArgument && Argument)
			{
				OutError = FString::Printf(TEXT("'%s' doesn't take anything in brackets"), *FactText);
				return false;
			}

			const EDialogSubject Missing = Fact->Reads & ~AvailableSubjects;

			if (Missing != EDialogSubject::None)
			{
				OutError = FString::Printf(TEXT("'%s' asks about %s, and this event only has %s"),
					*FactText, *SmoresDialog::DescribeSubjects(Missing), *SmoresDialog::DescribeSubjects(AvailableSubjects));
				return false;
			}

			OutOperand.bIsFact = true;
			OutOperand.Fact = Fact->Name;
			OutOperand.Argument = Argument ? FName(**Argument) : NAME_None;
			OutType = Fact->Type;

			return true;
		};

		switch (Raw.Kind)
		{
		case FDialogRawOperand::EKind::Call:
			return ResolveFact(Raw.Text, &Raw.Argument);

		case FDialogRawOperand::EKind::Number:
			if (bIsLeft)
			{
				OutError = FString::Printf(TEXT("a clause has to start with a fact, like 'StandingWithSpeaker <= -20' - '%s' isn't one"), *FString::SanitizeFloat(Raw.Number));
				return false;
			}

			OutOperand.Literal = FDialogValue::MakeNumber(Raw.Number);
			OutType = EDialogValueType::Number;
			return true;

		case FDialogRawOperand::EKind::String:
			if (bIsLeft)
			{
				OutError = FString::Printf(TEXT("a clause has to start with a fact, like 'Speaker.Name == \"%s\"'"), *Raw.Text);
				return false;
			}

			OutOperand.Literal = FDialogValue::MakeName(FName(*Raw.Text));
			OutType = EDialogValueType::Name;
			return true;

		case FDialogRawOperand::EKind::Identifier:
		default:
			break;
		}

		if (bIsLeft)
		{
			return ResolveFact(Raw.Text, nullptr);
		}

		// the right-hand side: a yes/no word where one is wanted, then a fact, then an id
		const bool bIsTrueWord = Raw.Text.Equals(TEXT("true"), ESearchCase::IgnoreCase);
		const bool bIsFalseWord = Raw.Text.Equals(TEXT("false"), ESearchCase::IgnoreCase);

		if ((bIsTrueWord || bIsFalseWord) && ExpectedType.IsSet() && ExpectedType.GetValue() == EDialogValueType::Bool)
		{
			OutOperand.Literal = FDialogValue::MakeBool(bIsTrueWord);
			OutType = EDialogValueType::Bool;
			return true;
		}

		if (Facts.Find(FName(*Raw.Text)))
		{
			return ResolveFact(Raw.Text, nullptr);
		}

		// no id of any kind has a dot in it, so a dotted word that isn't a fact is a fact misspelled -
		// reading it as an id would give a rule that silently never matches
		if (Raw.Text.Contains(TEXT(".")))
		{
			OutError = FString::Printf(TEXT("'%s' isn't a fact the game knows - SmoresDialogReport facts lists them"), *Raw.Text);
			return false;
		}

		OutOperand.Literal = FDialogValue::MakeName(FName(*Raw.Text));
		OutType = EDialogValueType::Name;
		return true;
	}

	bool CompareDialogValues(const FDialogValue& Left, EDialogCompareOp Op, const FDialogValue& Right)
	{
		if (!Left.bIsSet || !Right.bIsSet || Left.Type != Right.Type)
		{
			return false;
		}

		switch (Left.Type)
		{
		case EDialogValueType::Number:
			switch (Op)
			{
			case EDialogCompareOp::Equal:        return Left.Number == Right.Number;
			case EDialogCompareOp::NotEqual:     return Left.Number != Right.Number;
			case EDialogCompareOp::Less:         return Left.Number < Right.Number;
			case EDialogCompareOp::LessEqual:    return Left.Number <= Right.Number;
			case EDialogCompareOp::Greater:      return Left.Number > Right.Number;
			case EDialogCompareOp::GreaterEqual: return Left.Number >= Right.Number;
			default:                             return false;
			}

		case EDialogValueType::Bool:
			return Op == EDialogCompareOp::Equal ? Left.bBool == Right.bBool
				: Op == EDialogCompareOp::NotEqual ? Left.bBool != Right.bBool
				: false;

		case EDialogValueType::Name:
		default:
			// FName compares case-insensitively, so "guard" matches a role authored as "Guard"
			return Op == EDialogCompareOp::Equal ? Left.Name == Right.Name
				: Op == EDialogCompareOp::NotEqual ? Left.Name != Right.Name
				: false;
		}
	}

	FDialogValue ResolveDialogValue(const FDialogOperand& Operand, const FDialogContext& Context, const FDialogFactRegistry& Facts)
	{
		return Operand.bIsFact ? Facts.Ask(Operand.Fact, Context, Operand.Argument) : Operand.Literal;
	}
}

bool FDialogCondition::EvaluateClause(const FDialogClause& Clause, const FDialogContext& Context, const FDialogFactRegistry& Facts)
{
	const FDialogValue Left = ResolveDialogValue(Clause.Left, Context, Facts);

	if (Clause.Op == EDialogCompareOp::IsTrue)
	{
		return Left.bIsSet && Left.Type == EDialogValueType::Bool && Left.bBool;
	}

	return CompareDialogValues(Left, Clause.Op, ResolveDialogValue(Clause.Right, Context, Facts));
}

bool FDialogCondition::Evaluate(const FDialogContext& Context, const FDialogFactRegistry& Facts, int32* OutFailedClause) const
{
	for (int32 Index = 0; Index < Clauses.Num(); ++Index)
	{
		if (!EvaluateClause(Clauses[Index], Context, Facts))
		{
			if (OutFailedClause)
			{
				*OutFailedClause = Index;
			}

			return false;
		}
	}

	if (OutFailedClause)
	{
		*OutFailedClause = INDEX_NONE;
	}

	return true;
}

FString FDialogCondition::ToString() const
{
	if (Clauses.Num() == 0)
	{
		return TEXT("(always)");
	}

	TArray<FString> Texts;

	for (const FDialogClause& Clause : Clauses)
	{
		Texts.Add(Clause.Text);
	}

	return FString::Join(Texts, TEXT("; "));
}

namespace SmoresDialog
{
	bool CompileCondition(
		const FString& Text,
		const FDialogFactRegistry& Facts,
		EDialogSubject AvailableSubjects,
		const FDialogKnownIds* KnownIds,
		FDialogCondition& OutCondition,
		TArray<FString>& OutErrors,
		TArray<FString>& OutWarnings)
	{
		OutCondition = FDialogCondition();

		TArray<FDialogToken> Tokens;
		FString TokenError;

		if (!TokenizeDialogCondition(Text, Tokens, TokenError))
		{
			OutErrors.Add(TokenError);
			return false;
		}

		const int32 ErrorsBefore = OutErrors.Num();
		TSet<FString> SeenClauses;

		int32 ClauseStart = 0;

		while (ClauseStart <= Tokens.Num())
		{
			int32 ClauseEnd = ClauseStart;

			while (ClauseEnd < Tokens.Num() && Tokens[ClauseEnd].Kind != EDialogTokenKind::Semicolon)
			{
				++ClauseEnd;
			}

			// an empty clause - a trailing ';', or ';;' - says nothing and costs nothing
			if (ClauseEnd > ClauseStart)
			{
				FDialogClause Clause;
				Clause.Text = Text.Mid(Tokens[ClauseStart].Start, Tokens[ClauseEnd - 1].End - Tokens[ClauseStart].Start).TrimStartAndEnd();

				auto Fail = [&OutErrors, &Clause](const FString& Why)
				{
					OutErrors.Add(FString::Printf(TEXT("in '%s': %s"), *Clause.Text, *Why));
				};

				int32 Cursor = ClauseStart;
				FDialogRawOperand RawLeft;
				FString Error;

				if (!ParseDialogOperand(Tokens, ClauseEnd, Cursor, RawLeft, Error))
				{
					Fail(Error);
				}
				else
				{
					EDialogValueType LeftType = EDialogValueType::Name;

					if (!ResolveDialogOperand(RawLeft, Facts, AvailableSubjects, /*bIsLeft*/ true, TOptional<EDialogValueType>(), Clause.Left, LeftType, Error))
					{
						Fail(Error);
					}
					else if (Cursor == ClauseEnd)
					{
						// the bare form - only a yes/no fact can stand on its own
						Clause.Op = EDialogCompareOp::IsTrue;

						if (LeftType != EDialogValueType::Bool)
						{
							Fail(FString::Printf(TEXT("'%s' is %s, so it needs comparing with something"), *RawLeft.Text, *DescribeDialogType(LeftType)));
						}
					}
					else if (Tokens[Cursor].Kind != EDialogTokenKind::Op)
					{
						Fail(FString::Printf(TEXT("expected a comparison after '%s' - separate clauses with ';'"), *RawLeft.Text));
					}
					else
					{
						Clause.Op = Tokens[Cursor].Op;
						++Cursor;

						FDialogRawOperand RawRight;
						EDialogValueType RightType = EDialogValueType::Name;

						if (!ParseDialogOperand(Tokens, ClauseEnd, Cursor, RawRight, Error)
							|| !ResolveDialogOperand(RawRight, Facts, AvailableSubjects, /*bIsLeft*/ false, TOptional<EDialogValueType>(LeftType), Clause.Right, RightType, Error))
						{
							Fail(Error);
						}
						else if (Cursor != ClauseEnd)
						{
							Fail(TEXT("more follows the comparison - separate clauses with ';'"));
						}
						else if (RightType != LeftType)
						{
							Fail(FString::Printf(TEXT("'%s' is %s, and the other side is %s"), *RawLeft.Text, *DescribeDialogType(LeftType), *DescribeDialogType(RightType)));
						}
						else if (LeftType != EDialogValueType::Number && Clause.Op != EDialogCompareOp::Equal && Clause.Op != EDialogCompareOp::NotEqual)
						{
							Fail(FString::Printf(TEXT("'%s' is %s, which only compares with == or !="), *RawLeft.Text, *DescribeDialogType(LeftType)));
						}
						else if (!Clause.Right.bIsFact && LeftType == EDialogValueType::Name)
						{
							// a written name checked against what the left-hand fact can ever answer
							const FDialogFact* LeftFact = Facts.Find(Clause.Left.Fact);
							const FName Literal = Clause.Right.Literal.Name;

							if (LeftFact && LeftFact->AllowedNames.Num() > 0 && !LeftFact->AllowedNames.Contains(Literal))
							{
								TArray<FString> Allowed;

								for (const FName AllowedName : LeftFact->AllowedNames)
								{
									Allowed.Add(AllowedName.ToString());
								}

								Fail(FString::Printf(TEXT("'%s' is one of %s - not '%s'"), *RawLeft.Text, *FString::Join(Allowed, TEXT(", ")), *Literal.ToString()));
							}
							else if (LeftFact && KnownIds && !LeftFact->ContentDomain.IsNone() && !KnownIds->IsKnown(LeftFact->ContentDomain, Literal))
							{
								OutWarnings.Add(FString::Printf(TEXT("in '%s': no %s called '%s' is loaded"), *Clause.Text, *DescribeDialogDomain(LeftFact->ContentDomain), *Literal.ToString()));
							}
						}
					}
				}

				if (!SeenClauses.Contains(NormalizeDialogClause(Clause.Text)))
				{
					SeenClauses.Add(NormalizeDialogClause(Clause.Text));
				}
				else
				{
					Fail(TEXT("this clause is written twice, which would count it twice toward how specific the rule is"));
				}

				OutCondition.Clauses.Add(MoveTemp(Clause));
			}

			ClauseStart = ClauseEnd + 1;
		}

		if (OutErrors.Num() > ErrorsBefore)
		{
			OutCondition = FDialogCondition();
			return false;
		}

		return true;
	}
}
