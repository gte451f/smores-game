// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FactionDefinition.h"
#include "Tests/SmoresDefinitionRules.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  The **per-type** layer of the definition content sweeps, for factions. Tests/
 *  SmoresDefinitionAssetTest.cpp already asserts what every definition shares - an id, a name, no
 *  duplicate id, and that the id resolves - and picked these assets up the moment the type existed.
 *  This file adds only what is true of a faction.
 *
 *  Two of its rules are about zero-ish values that fail silently, which is what earns a per-type
 *  file its place (see game-data.md):
 *
 *   - **A colour authored fully transparent** draws nothing at all, and reads as a faction with a
 *     missing banner rather than as a bad asset.
 *   - **A starting relation authored on both factions with different numbers.** Standing is
 *     symmetric, so one of the two is silently discarded at campaign start. Only a sweep over every
 *     faction at once can see it, which is why it lives here rather than in the per-asset check.
 *
 *  See SmoresItems' ItemDefinitionAssetTest.cpp for why these sweeps run against real content and
 *  why the rules themselves are proved against in-memory definitions instead.
 */

/** Every rule one faction definition satisfies on its own, base rules first. Returns false and names the first problem. */
inline bool ValidateFactionDefinition(const UFactionDefinition* Faction, FString& OutProblem)
{
	if (!ValidateSmoresDefinition(Faction, OutProblem))
	{
		return false;
	}

	if (Faction->ShortName.IsEmpty())
	{
		OutProblem = TEXT("ShortName is empty");

		return false;
	}

	if (Faction->Colour.A <= 0.0f)
	{
		OutProblem = TEXT("Colour is fully transparent, so the faction would draw as nothing");

		return false;
	}

	TSet<FName> SeenRelations;

	for (const FFactionStartingRelation& Relation : Faction->StartingRelations)
	{
		if (!Relation.Faction)
		{
			OutProblem = TEXT("a StartingRelations entry names no faction");

			return false;
		}

		if (Relation.Faction == Faction || Relation.Faction->DefinitionId == Faction->DefinitionId)
		{
			OutProblem = TEXT("a StartingRelations entry names the faction itself");

			return false;
		}

		if (Relation.Standing < SmoresStanding::Min || Relation.Standing > SmoresStanding::Max)
		{
			OutProblem = FString::Printf(TEXT("the starting relation with %s is %d, outside %d..%d"),
				*Relation.Faction->DefinitionId.ToString(), Relation.Standing, SmoresStanding::Min, SmoresStanding::Max);

			return false;
		}

		bool bAlreadySeen = false;
		SeenRelations.Add(Relation.Faction->DefinitionId, &bAlreadySeen);

		if (bAlreadySeen)
		{
			OutProblem = FString::Printf(TEXT("StartingRelations names %s twice"), *Relation.Faction->DefinitionId.ToString());

			return false;
		}
	}

	return true;
}

/**
 *  The cross-faction rule: a pair authored on both sides has to agree. Returns false and names
 *  the first disagreeing pair.
 */
inline bool ValidateFactionRelationsAgree(const TArray<const UFactionDefinition*>& Factions, FString& OutProblem)
{
	for (const UFactionDefinition* Faction : Factions)
	{
		if (!Faction)
		{
			continue;
		}

		for (const FFactionStartingRelation& Relation : Faction->StartingRelations)
		{
			if (!Relation.Faction)
			{
				continue;
			}

			// look for the same pair authored from the other side
			for (const FFactionStartingRelation& Reverse : Relation.Faction->StartingRelations)
			{
				if (Reverse.Faction && Reverse.Faction->DefinitionId == Faction->DefinitionId && Reverse.Standing != Relation.Standing)
				{
					OutProblem = FString::Printf(TEXT("%s says %d toward %s, but %s says %d back - author it on one side only"),
						*Faction->DefinitionId.ToString(), Relation.Standing, *Relation.Faction->DefinitionId.ToString(),
						*Relation.Faction->DefinitionId.ToString(), Reverse.Standing);

					return false;
				}
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionDefinitionWellFormedTest,
	"Smores.Content.FactionDefinitions.EveryAssetIsWellFormed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionDefinitionWellFormedTest::RunTest(const FString& Parameters)
{
	TArray<FAssetData> Assets;
	GatherDefinitionAssets(UFactionDefinition::StaticClass(), Assets);

	// a sweep that found nothing is green for the wrong reason
	if (!TestTrue(TEXT("At least one UFactionDefinition asset exists under /Game"), Assets.Num() > 0))
	{
		return true;
	}

	TArray<const UFactionDefinition*> Factions;

	for (const FAssetData& AssetData : Assets)
	{
		const UFactionDefinition* Faction = Cast<UFactionDefinition>(AssetData.GetAsset());

		FString Problem;

		if (!ValidateFactionDefinition(Faction, Problem))
		{
			AddError(FString::Printf(TEXT("%s is malformed: %s"), *AssetData.GetSoftObjectPath().ToString(), *Problem));
		}

		Factions.Add(Faction);
	}

	FString Problem;

	if (!ValidateFactionRelationsAgree(Factions, Problem))
	{
		AddError(FString::Printf(TEXT("Faction starting relations disagree: %s"), *Problem));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresFactionDefinitionRuleTest,
	"Smores.Content.FactionDefinitions.MalformedDefinitionIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSmoresFactionDefinitionRuleTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UFactionDefinition* Faction = TestWorld.NewKeptObject<UFactionDefinition>();
	UFactionDefinition* Other = TestWorld.NewKeptObject<UFactionDefinition>();

	if (!TestNotNull(TEXT("Factions created"), Faction) || !TestNotNull(TEXT("..."), Other))
	{
		return true;
	}

	FString Problem;

	TestFalse(TEXT("A faction with no DefinitionId is rejected"), ValidateFactionDefinition(Faction, Problem));

	Faction->DefinitionId = FName(TEXT("RuleProof"));
	Faction->DisplayName = FText::FromString(TEXT("Rule Proof"));

	TestFalse(TEXT("A faction with no ShortName is rejected"), ValidateFactionDefinition(Faction, Problem));
	TestTrue(TEXT("...and the failure names the field"), Problem.Contains(TEXT("ShortName")));

	Faction->ShortName = FText::FromString(TEXT("Proof"));

	TestTrue(TEXT("A filled-in faction is accepted"), ValidateFactionDefinition(Faction, Problem));

	TestEqual(TEXT("A faction's primary asset id is typed FactionDefinition"),
		Faction->GetPrimaryAssetId().ToString(), FString(TEXT("FactionDefinition:RuleProof")));

	Faction->Colour = FLinearColor::Transparent;

	TestFalse(TEXT("A transparent colour is rejected"), ValidateFactionDefinition(Faction, Problem));

	Faction->Colour = FLinearColor::Red;

	FFactionStartingRelation& ToSelf = Faction->StartingRelations.AddDefaulted_GetRef();
	ToSelf.Faction = Faction;

	TestFalse(TEXT("A relation with itself is rejected"), ValidateFactionDefinition(Faction, Problem));

	Faction->StartingRelations.Reset();

	Other->DefinitionId = FName(TEXT("OtherProof"));
	Other->DisplayName = FText::FromString(TEXT("Other Proof"));
	Other->ShortName = FText::FromString(TEXT("Other"));

	FFactionStartingRelation& Outward = Faction->StartingRelations.AddDefaulted_GetRef();
	Outward.Faction = Other;
	Outward.Standing = -30;

	TestTrue(TEXT("A relation with another faction is accepted"), ValidateFactionDefinition(Faction, Problem));
	TestTrue(TEXT("...and authored on one side only, it agrees with itself"), ValidateFactionRelationsAgree({ Faction, Other }, Problem));

	FFactionStartingRelation& Twice = Faction->StartingRelations.AddDefaulted_GetRef();
	Twice.Faction = Other;

	TestFalse(TEXT("Naming the same faction twice is rejected"), ValidateFactionDefinition(Faction, Problem));

	Faction->StartingRelations.Pop();

	FFactionStartingRelation& Back = Other->StartingRelations.AddDefaulted_GetRef();
	Back.Faction = Faction;
	Back.Standing = -30;

	TestTrue(TEXT("Both sides authoring the same number agree"), ValidateFactionRelationsAgree({ Faction, Other }, Problem));

	Back.Standing = 40;

	// the one only a sweep across every faction can see: symmetric storage keeps one of these
	TestFalse(TEXT("Both sides authoring different numbers is rejected"), ValidateFactionRelationsAgree({ Faction, Other }, Problem));

	TestFalse(TEXT("A null faction is rejected rather than crashing the sweep"), ValidateFactionDefinition(nullptr, Problem));

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
