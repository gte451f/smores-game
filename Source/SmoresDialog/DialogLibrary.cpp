// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "DialogLibrary.h"

const FBarkLine* FDialogLibrary::FindBark(FName LineId) const
{
	const int32* Index = BarkIndexById.Find(LineId);

	return Index ? &Barks[*Index] : nullptr;
}

TArray<const FBarkLine*> FDialogLibrary::GetBarksForEvent(EBarkEvent Event) const
{
	TArray<const FBarkLine*> Lines;

	if (const TArray<int32>* Indices = BarkIndicesByEvent.Find(Event))
	{
		Lines.Reserve(Indices->Num());

		for (const int32 Index : *Indices)
		{
			Lines.Add(&Barks[Index]);
		}
	}

	return Lines;
}

const FDialogPackageInfo* FDialogLibrary::FindPackage(FName PackageId) const
{
	return Packages.FindByPredicate([PackageId](const FDialogPackageInfo& Package)
	{
		return Package.Id == PackageId;
	});
}

int32 FDialogLibrary::CountProblems(EDialogProblemSeverity Severity, FName PackageId) const
{
	int32 Count = 0;

	for (const FDialogProblem& Problem : Problems)
	{
		if (Problem.Severity == Severity && (PackageId.IsNone() || FName(*Problem.Package) == PackageId))
		{
			++Count;
		}
	}

	return Count;
}

TArray<FString> FDialogLibrary::BuildSummaryLines() const
{
	TArray<FString> Lines;

	for (const FDialogPackageInfo& Package : Packages)
	{
		const int32 Errors = CountProblems(EDialogProblemSeverity::Error, Package.Id);
		const int32 Warnings = CountProblems(EDialogProblemSeverity::Warning, Package.Id);

		FString Line = FString::Printf(TEXT("%s %s"), Package.bIsCore ? TEXT("base game") : TEXT("mod"), *Package.Id.ToString());

		if (!Package.Version.IsEmpty())
		{
			Line += FString::Printf(TEXT(" %s"), *Package.Version);
		}

		if (Package.bLoaded)
		{
			Line += FString::Printf(TEXT(": loaded, %d barks, %d translations"), Package.NumBarks, Package.NumTranslations);
		}
		else
		{
			Line += TEXT(": SKIPPED");
		}

		Line += FString::Printf(TEXT(", %d errors, %d warnings"), Errors, Warnings);

		Lines.Add(MoveTemp(Line));
	}

	return Lines;
}

void FDialogLibrary::RebuildIndex()
{
	BarkIndexById.Reset();
	BarkIndicesByEvent.Reset();

	for (int32 Index = 0; Index < Barks.Num(); ++Index)
	{
		BarkIndexById.Add(Barks[Index].Id, Index);
		BarkIndicesByEvent.FindOrAdd(Barks[Index].Event).Add(Index);
	}
}
