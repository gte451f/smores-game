// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "SmoresDialogSubsystem.h"
#include "SmoresDialog.h"
#include "DialogLoader.h"
#include "DialogText.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

USmoresDialogSubsystem* USmoresDialogSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<USmoresDialogSubsystem>() : nullptr;
}

void USmoresDialogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Facts = FDialogFactRegistry::MakeBuiltIn();

	Reload();
}

void USmoresDialogSubsystem::Deinitialize()
{
	// a culture preview started from a PIE console would otherwise outlive the session and leave
	// the editor showing game text in French
	SmoresDialog::EndDialogCulturePreview();

	Super::Deinitialize();
}

void USmoresDialogSubsystem::Reload()
{
	TArray<FDialogPackageSource> Sources;
	TArray<FDialogProblem> DiskProblems;

	SmoresDialog::GatherPackagesFromDisk(SmoresDialog::GetCoreDirectory(), SmoresDialog::GetModsDirectory(), Sources, DiskProblems);

	const FDialogKnownIds KnownIds = FDialogKnownIds::GatherFromGame();

	Library = SmoresDialog::LoadPackages(Sources, Facts, &KnownIds);

	// a file that couldn't be read is reported ahead of what the loader made of the rest
	Library.Problems.Insert(DiskProblems, 0);

	SmoresDialog::PublishDialogText(Library);

	for (const FString& Line : Library.BuildSummaryLines())
	{
		UE_LOG(LogSmoresDialog, Display, TEXT("Dialog: %s"), *Line);
	}

	// every problem, errors loud and warnings quieter - a broken mod is the author's to fix, and
	// must never look like the game failing
	for (const FDialogProblem& Problem : Library.Problems)
	{
		if (Problem.Severity == EDialogProblemSeverity::Error)
		{
			UE_LOG(LogSmoresDialog, Warning, TEXT("Dialog %s"), *Problem.ToString());
		}
		else
		{
			UE_LOG(LogSmoresDialog, Display, TEXT("Dialog %s"), *Problem.ToString());
		}
	}

	OnLibraryLoaded.Broadcast();
}

TArray<FString> USmoresDialogSubsystem::GetLoadedPackageVersions() const
{
	TArray<FString> Versions;

	for (const FDialogPackageInfo& Package : Library.Packages)
	{
		if (Package.bLoaded)
		{
			Versions.Add(FString::Printf(TEXT("%s@%s"), *Package.Id.ToString(), *Package.Version));
		}
	}

	return Versions;
}

FText USmoresDialogSubsystem::GetLineText(FName LineId) const
{
	const FBarkLine* Line = Library.FindBark(LineId);

	if (!Line)
	{
		return FText::GetEmpty();
	}

	const FText Published = SmoresDialog::GetPublishedLineText(Line->Id, Line->PackageId);

	// unpublished only if something went badly wrong with the string table; the source text is a
	// better answer than nothing
	return Published.IsEmpty() ? FText::AsCultureInvariant(Line->SourceText) : Published;
}

void USmoresDialogSubsystem::LogReport(bool bIncludeFacts) const
{
	UE_LOG(LogSmoresDialog, Display, TEXT("=== Dialog report: %d packages, %d barks, %d translations, %d errors, %d warnings ==="),
		Library.Packages.Num(), Library.Barks.Num(), Library.Translations.Num(),
		Library.CountProblems(EDialogProblemSeverity::Error), Library.CountProblems(EDialogProblemSeverity::Warning));

	for (const FDialogPackageInfo& Package : Library.Packages)
	{
		const FString Requires = Package.Requires.Num() > 0
			? FString::JoinBy(Package.Requires, TEXT(", "), [](FName Id) { return Id.ToString(); })
			: FString(TEXT("nothing"));

		UE_LOG(LogSmoresDialog, Display, TEXT("  %s (%s) - \"%s\" version '%s' for game '%s', requires %s: %s, %d barks, %d translations"),
			*Package.Id.ToString(), *Package.FolderName, *Package.DisplayName, *Package.Version, *Package.GameVersion, *Requires,
			Package.bLoaded ? TEXT("loaded") : TEXT("SKIPPED"), Package.NumBarks, Package.NumTranslations);
	}

	UE_LOG(LogSmoresDialog, Display, TEXT("  Barks per event:"));

	for (const EBarkEvent Event : SmoresDialog::GetAllBarkEvents())
	{
		UE_LOG(LogSmoresDialog, Display, TEXT("    %-15s %d"), *SmoresDialog::GetBarkEventName(Event), Library.GetBarksForEvent(Event).Num());
	}

	if (Library.Problems.Num() > 0)
	{
		UE_LOG(LogSmoresDialog, Display, TEXT("  Problems:"));

		for (const FDialogProblem& Problem : Library.Problems)
		{
			UE_LOG(LogSmoresDialog, Display, TEXT("    %s"), *Problem.ToString());
		}
	}

	if (!bIncludeFacts)
	{
		UE_LOG(LogSmoresDialog, Display, TEXT("  (SmoresDialogReport facts lists every fact a condition may use)"));
		return;
	}

	UE_LOG(LogSmoresDialog, Display, TEXT("  Who each event carries:"));

	for (const EBarkEvent Event : SmoresDialog::GetAllBarkEvents())
	{
		UE_LOG(LogSmoresDialog, Display, TEXT("    %-15s %s"), *SmoresDialog::GetBarkEventName(Event), *SmoresDialog::DescribeSubjects(SmoresDialog::GetEventSubjects(Event)));
	}

	UE_LOG(LogSmoresDialog, Display, TEXT("  Facts:"));

	for (const FDialogFact& Fact : Facts.GetFacts())
	{
		const TCHAR* TypeName = Fact.Type == EDialogValueType::Number ? TEXT("number")
			: Fact.Type == EDialogValueType::Bool ? TEXT("yes/no")
			: TEXT("name");

		UE_LOG(LogSmoresDialog, Display, TEXT("    %-24s %-7s %s"), *Fact.Name.ToString(), TypeName, *Fact.Description);
	}
}
