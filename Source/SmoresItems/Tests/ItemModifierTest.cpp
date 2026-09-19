// Copyright 2026 Jim Jenkins. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "ItemModifierDefinition.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

/**
 *  Material and quality modifiers: the arithmetic they do to an item's weight and value, the
 *  name they compose, the colour they give it, and the one-per-slot rule the whole model rests
 *  on.
 *
 *  These are exactly the shapes testing.md's standing rule names - weight and money arithmetic,
 *  and an all-or-nothing "did it take?" bool. A modifier that silently failed to apply looks the
 *  same as an item that never had one.
 */


//~ Multiplier arithmetic

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierMultipliersTest,
	"Smores.Items.Modifiers.MultipliersCompose",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierMultipliersTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	// a bare spear: 2 kg, 10 gold
	UItemDefinition* Spear = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), /*MaxStack*/ 1, /*Weight*/ 2.0f, /*BaseValue*/ 10);

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"), 1.5f, 2.0f);
	UItemModifierDefinition* Masterwork = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Masterwork"), 1.0f, 3.0f);

	if (!TestNotNull(TEXT("Spear created"), Spear) || !TestNotNull(TEXT("Bronze created"), Bronze) || !TestNotNull(TEXT("Masterwork created"), Masterwork))
	{
		return true;
	}

	// an unmodified item is the baseline the whole model scales from - the item asset's own
	// numbers mean "with no material applied"
	FInventoryItem Bare = MakeTestItem(Spear, 1);

	TestEqual(TEXT("A bare item weighs exactly what its definition says"), Bare.GetUnitWeight(), 2.0f);
	TestEqual(TEXT("...and is worth exactly what its definition says"), Bare.GetUnitBaseValue(), 10);

	FInventoryItem BronzeSpear = MakeTestItem(Spear, 1);
	TestTrue(TEXT("Bronze applied"), BronzeSpear.AddModifier(Bronze));

	TestEqual(TEXT("One modifier scales the unit weight"), BronzeSpear.GetUnitWeight(), 3.0f);
	TestEqual(TEXT("...and the unit value"), BronzeSpear.GetUnitBaseValue(), 20);

	FInventoryItem MasterworkBronzeSpear = BronzeSpear;
	TestTrue(TEXT("Masterwork applied on top"), MasterworkBronzeSpear.AddModifier(Masterwork));

	// multiplied, not added: 2 x 1.5 x 1.0 and 10 x 2 x 3
	TestEqual(TEXT("Two modifiers multiply through rather than accumulating separately"), MasterworkBronzeSpear.GetUnitWeight(), 3.0f);
	TestEqual(TEXT("...and the value multipliers compound"), MasterworkBronzeSpear.GetUnitBaseValue(), 60);

	// the stack total is the unit figure times the quantity, which is what keeps a trader's
	// per-unit price and its line total agreeing
	MasterworkBronzeSpear.Quantity = 4;

	TestEqual(TEXT("Total weight is unit weight x quantity"), MasterworkBronzeSpear.GetTotalWeight(), 12.0f);
	TestEqual(TEXT("Total value is unit value x quantity"), MasterworkBronzeSpear.GetTotalBaseValue(), 240);

	TestWorld.ForwardErrors(this);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierValueRoundingTest,
	"Smores.Items.Modifiers.ValueRoundsPerUnitAndNeverToNothing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierValueRoundingTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Apple = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), /*MaxStack*/ 10, /*Weight*/ 0.1f, /*BaseValue*/ 3);
	UItemDefinition* Junk = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), /*MaxStack*/ 10, /*Weight*/ 0.1f, /*BaseValue*/ 0);

	// a markdown steep enough to take 3 gold below 1
	UItemModifierDefinition* Shoddy = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Shoddy"), 1.0f, 0.1f);

	if (!TestNotNull(TEXT("Apple created"), Apple) || !TestNotNull(TEXT("Junk created"), Junk) || !TestNotNull(TEXT("Shoddy created"), Shoddy))
	{
		return true;
	}

	FInventoryItem ShoddyApple = MakeTestItem(Apple, 10);
	ShoddyApple.AddModifier(Shoddy);

	// anything the designer did price stays worth something - the same rule the trader's own
	// markup already follows, applied one layer down so a stack total can't round away either
	TestEqual(TEXT("A priced item never multiplies down to worthless"), ShoddyApple.GetUnitBaseValue(), 1);
	TestEqual(TEXT("...and the stack total is that floor x quantity, not a re-rounded product"), ShoddyApple.GetTotalBaseValue(), 10);

	FInventoryItem ShoddyJunk = MakeTestItem(Junk, 5);
	ShoddyJunk.AddModifier(Shoddy);

	// an item priced at nothing is genuinely worthless rather than cheap, and no multiplier
	// promotes it to worth a coin
	TestEqual(TEXT("An item with no authored value stays worth nothing"), ShoddyJunk.GetUnitBaseValue(), 0);
	TestEqual(TEXT("...however many of it there are"), ShoddyJunk.GetTotalBaseValue(), 0);

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Slot exclusivity

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierSlotExclusivityTest,
	"Smores.Items.Modifiers.OneModifierPerSlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierSlotExclusivityTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Spear = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), 1, 2.0f, 10);

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"), 1.5f, 2.0f);
	UItemModifierDefinition* Steel = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Steel"), 2.0f, 4.0f);
	UItemModifierDefinition* WellMade = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Well-Made"), 1.0f, 1.5f);

	if (!TestNotNull(TEXT("Spear created"), Spear) || !TestNotNull(TEXT("Steel created"), Steel))
	{
		return true;
	}

	FInventoryItem Item = MakeTestItem(Spear, 1);

	TestTrue(TEXT("The first material is accepted"), Item.AddModifier(Bronze));

	// this is the rule the whole model rests on - an item is made of one thing
	TestFalse(TEXT("A second material into the same slot is refused"), Item.AddModifier(Steel));
	TestEqual(TEXT("...and nothing was added"), Item.Modifiers.Num(), 1);
	TestTrue(TEXT("...and the first material is still the one in the slot"), Item.GetModifier(EItemModifierSlot::Material) == Bronze);

	// a *different* slot is not a conflict
	TestTrue(TEXT("A quality alongside a material is accepted"), Item.AddModifier(WellMade));
	TestEqual(TEXT("Both slots are now filled"), Item.Modifiers.Num(), 2);

	TestFalse(TEXT("A null modifier is refused rather than stored"), Item.AddModifier(nullptr));
	TestEqual(TEXT("...and left the array alone"), Item.Modifiers.Num(), 2);

	// SetModifier is the deliberate replacement path - AddModifier refuses precisely so that
	// losing a material is something a caller has to ask for
	TestTrue(TEXT("SetModifier replaces the occupant of a filled slot"), Item.SetModifier(Steel));
	TestEqual(TEXT("...still one modifier per slot afterwards"), Item.Modifiers.Num(), 2);
	TestTrue(TEXT("...and the replacement is the one in the slot"), Item.GetModifier(EItemModifierSlot::Material) == Steel);
	TestTrue(TEXT("...while the other slot is untouched"), Item.GetModifier(EItemModifierSlot::Quality) == WellMade);

	TestTrue(TEXT("RemoveModifier clears a filled slot"), Item.RemoveModifier(EItemModifierSlot::Material));
	TestNull(TEXT("...leaving it empty"), Item.GetModifier(EItemModifierSlot::Material));
	TestFalse(TEXT("...and removing again reports there was nothing to remove"), Item.RemoveModifier(EItemModifierSlot::Material));

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Name composition

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierNameCompositionTest,
	"Smores.Items.Modifiers.NameComposesInSlotOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierNameCompositionTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Spear = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), 1, 2.0f, 10);

	if (!TestNotNull(TEXT("Spear created"), Spear))
	{
		return true;
	}

	Spear->DisplayName = FText::FromString(TEXT("Spear"));

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"));
	UItemModifierDefinition* Masterwork = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Masterwork"));

	FInventoryItem Bare = MakeTestItem(Spear, 1);
	TestEqual(TEXT("An unmodified item reads as its definition's own name"), Bare.GetDisplayName().ToString(), FString(TEXT("Spear")));

	FInventoryItem BronzeSpear = MakeTestItem(Spear, 1);
	BronzeSpear.AddModifier(Bronze);
	TestEqual(TEXT("A material wraps the item name"), BronzeSpear.GetDisplayName().ToString(), FString(TEXT("Bronze Spear")));

	// the point of composing in *slot* order rather than array order: the quality was added
	// second here and first below, and both have to read the same way
	FInventoryItem MaterialFirst = MakeTestItem(Spear, 1);
	MaterialFirst.AddModifier(Bronze);
	MaterialFirst.AddModifier(Masterwork);

	FInventoryItem QualityFirst = MakeTestItem(Spear, 1);
	QualityFirst.AddModifier(Masterwork);
	QualityFirst.AddModifier(Bronze);

	TestEqual(TEXT("Material then quality reads as Masterwork Bronze Spear"),
		MaterialFirst.GetDisplayName().ToString(), FString(TEXT("Masterwork Bronze Spear")));
	TestEqual(TEXT("...and so does the same pair added the other way round"),
		QualityFirst.GetDisplayName().ToString(), FString(TEXT("Masterwork Bronze Spear")));

	// a translator reorders the pattern, which is the entire reason this is FText::Format and
	// not string concatenation
	UItemModifierDefinition* Suffixed = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("of Quality"),
		1.0f, 1.0f, FLinearColor::White, TEXT("{Item} {Modifier}"));

	FInventoryItem Reordered = MakeTestItem(Spear, 1);
	Reordered.AddModifier(Suffixed);

	TestEqual(TEXT("A pattern putting the modifier last is honoured"),
		Reordered.GetDisplayName().ToString(), FString(TEXT("Spear of Quality")));

	// an asset whose pattern was never filled in still shows its modifier rather than dropping
	// it silently, which is the failure this fallback exists to prevent
	UItemModifierDefinition* Unpatterned = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Iron"),
		1.0f, 1.0f, FLinearColor::White, TEXT(""));

	FInventoryItem IronSpear = MakeTestItem(Spear, 1);
	IronSpear.AddModifier(Unpatterned);

	TestEqual(TEXT("A modifier with no authored pattern falls back rather than vanishing from the name"),
		IronSpear.GetDisplayName().ToString(), FString(TEXT("Iron Spear")));

	TestEqual(TEXT("An empty item still has no name"), FInventoryItem().GetDisplayName().ToString(), FString());

	TestWorld.ForwardErrors(this);

	return true;
}

//~ Tint

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSmoresItemModifierTintTest,
	"Smores.Items.Modifiers.TintsMultiply",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresItemModifierTintTest::RunTest(const FString& Parameters)
{
	FSmoresTestWorld TestWorld;

	UItemDefinition* Spear = MakeTestItemDefinition(TestWorld, FIntPoint(1, 3), 1, 2.0f, 10);

	const FLinearColor BronzeColour(0.8f, 0.5f, 0.2f, 1.0f);

	UItemModifierDefinition* Bronze = MakeTestModifier(TestWorld, EItemModifierSlot::Material, TEXT("Bronze"), 1.0f, 1.0f, BronzeColour);
	UItemModifierDefinition* NeutralQuality = MakeTestModifier(TestWorld, EItemModifierSlot::Quality, TEXT("Well-Made"));

	if (!TestNotNull(TEXT("Spear created"), Spear) || !TestNotNull(TEXT("Bronze created"), Bronze))
	{
		return true;
	}

	TestEqual(TEXT("An unmodified item tints White, so nothing draws differently than before"),
		MakeTestItem(Spear, 1).GetTint(), FLinearColor::White);

	FInventoryItem BronzeSpear = MakeTestItem(Spear, 1);
	BronzeSpear.AddModifier(Bronze);

	TestEqual(TEXT("A material's colour comes through"), BronzeSpear.GetTint(), BronzeColour);

	// multiplied rather than last-one-wins, which is what makes White genuinely neutral - a
	// quality that isn't about colour must not wash the material out
	BronzeSpear.AddModifier(NeutralQuality);

	TestEqual(TEXT("A White quality leaves the material's colour alone"), BronzeSpear.GetTint(), BronzeColour);

	TestWorld.ForwardErrors(this);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
