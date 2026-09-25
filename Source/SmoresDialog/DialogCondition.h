// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogTypes.h"

class FDialogFactRegistry;
struct FDialogKnownIds;

/** How a clause compares. IsTrue is the bare form - a yes/no fact on its own means "== true". */
enum class EDialogCompareOp : uint8
{
	IsTrue,
	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual
};

/** One side of a clause: a fact to ask, or a value written into the condition */
struct SMORESDIALOG_API FDialogOperand
{
	bool bIsFact = false;

	/** The fact's name, when bIsFact */
	FName Fact;

	/** The call form's argument - Flag(met_kess) - when the fact takes one */
	FName Argument;

	/** The written value, when !bIsFact */
	FDialogValue Literal;
};

/** One clause: "Speaker.Role == guard" */
struct SMORESDIALOG_API FDialogClause
{
	FDialogOperand Left;

	EDialogCompareOp Op = EDialogCompareOp::IsTrue;

	/** Unused for IsTrue */
	FDialogOperand Right;

	/** The clause as the writer typed it, trimmed - what the explanations quote back */
	FString Text;
};

/**
 *  A parsed, checked condition: a list of clauses, every one of which must hold.
 *
 *  **There is no "or", deliberately.** Write two rules instead. That is what keeps specificity
 *  countable: a rule's specificity is simply how many clauses it has, and "most clauses matched
 *  wins" only means something if every clause of the winner actually held.
 *
 *  An empty condition always holds - that is a bark's generic, fallback line.
 */
struct SMORESDIALOG_API FDialogCondition
{
	TArray<FDialogClause> Clauses;

	/** How many clauses must hold - the number "most specific wins" compares */
	int32 GetSpecificity() const { return Clauses.Num(); }

	/** True if every clause holds. OutFailedClause, if given, is the index of the first that didn't, or INDEX_NONE. */
	bool Evaluate(const FDialogContext& Context, const FDialogFactRegistry& Facts, int32* OutFailedClause = nullptr) const;

	/** One clause against one moment. Every comparison involving an unset value is false. */
	static bool EvaluateClause(const FDialogClause& Clause, const FDialogContext& Context, const FDialogFactRegistry& Facts);

	/** The clauses rejoined, "Speaker.Role == guard; StandingWithSpeaker <= -20", or "(always)" */
	FString ToString() const;
};

namespace SmoresDialog
{
	/**
	 *  Parses and checks a condition in one step, the way the loader does for every row.
	 *
	 *  Clauses are separated by ';'. A clause is a fact, then optionally a comparison
	 *  (== != < <= > >=) and a value or a second fact. Values are numbers (-20, 0.3), ids (guard,
	 *  Raiders), quoted text ("Merchant Ada"), or true/false.
	 *
	 *  Checked here, at load time rather than in play, so a mistake is an error with a line number
	 *  rather than a rule that silently never matches:
	 *  - every fact must be registered, and must only read subjects in AvailableSubjects;
	 *  - both sides must be the same type, and names and yes/no values only compare with == and !=;
	 *  - a name compared against a fixed vocabulary (LifeState) must be one of its words;
	 *  - the same clause may not appear twice, since it would count twice toward specificity.
	 *
	 *  Returns false, with at least one entry in OutErrors, when the condition can't be used. A
	 *  content id that nothing loaded has (checked only when KnownIds is given) lands in OutWarnings
	 *  and does not fail - see FDialogKnownIds.
	 */
	SMORESDIALOG_API bool CompileCondition(
		const FString& Text,
		const FDialogFactRegistry& Facts,
		EDialogSubject AvailableSubjects,
		const FDialogKnownIds* KnownIds,
		FDialogCondition& OutCondition,
		TArray<FString>& OutErrors,
		TArray<FString>& OutWarnings);
}
