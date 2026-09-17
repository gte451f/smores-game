# Automated Testing Roadmap

## Purpose

This is a **roadmap**, not a system reference: read it while implementing one of its slices,
or when Jim points at it. The permanent record of how testing actually works lives in the
`game-systems` skill's `testing.md`.

Unlike that topic, this document isn't describing a built harness — it's a **forward-looking
target design**, the result of a pass over `SmoresItems`, `SmoresEconomy` and `SmoresCombat`
asking one question: *what in this codebase can be verified by code rather than by a human in
PIE?* It exists so testing gets built deliberately, in slices, instead of appearing as three
ad-hoc test files with three different conventions. Treat every section as "what to build next,"
not "what exists" — **except** where a heading is marked SHIPPED, which means that section has
moved to `testing.md` and only its summary line remains here.

The work is cut into **one slice per clean session** (see "Implementation Order" below), the same
convention `inventory-roadmap.md` uses. When a slice ships, move its content into the
`game-systems` skill's `testing.md`, delete it from here, and mark the slice `DONE` in the
order list.

**The project has no tests today.** `Automation_smores.slnx` is not a test setup — it is a
solution file that pulls in Epic's own `UnrealBuildTool` and `EpicGames.*` C# projects, several of
which happen to be named `*.Tests`. None of them test smores.

## What This Is For, and What It Isn't

The honest scope, stated up front so no later session mistakes this for a quality mandate:

**Automated tests here exist to catch silent numerical and state-machine regressions.** Not to
prove the game is fun, not to replace PIE, and not to reach a coverage number. The specific
failure this protects against is the one this project has already hit repeatedly and recorded in
`inventory-roadmap.md`: a function that returns `true` while having done only part of the job.
`AddItem` returning true on a partial add (`inventory-roadmap.md` Slice 6), `MoveItem` doing the
same one level up (its Slice 8), a `Kill()` landing on a Downed unit that leaves a recovery timer
in flight so the corpse stands back up (its Slice 7). Those are that roadmap's slice numbers, not
this one's. Every one of those is invisible on screen until it isn't, and every one is three
lines to assert.

**What stays in PIE, permanently:** drag-and-drop feel, the rotate-while-dragging gesture, window
layout and stacking order, camera pan/zoom tuning, animation and montage timing, EQS destination
quality, NPC behavior, anything about whether a number *feels* right. No slice below tries to
automate any of it, and no future slice should. `inventory-roadmap.md`'s per-slice protocol
already says "the user PIE-tests the player-facing behavior; the agent verifies compile, wiring
and property state" — this roadmap adds a third column to that split, not a replacement for the
first.

The dividing line is worth stating as a rule: **if verifying it requires a human to look at
something, it is not a candidate.** If verifying it means comparing two numbers or checking which
of three states an object is in, it is.

## The Harness

### Where test code lives

**Inside each feature module**, in a `Tests/` subfolder next to the code it tests
(`Source/SmoresItems/Tests/InventoryComponentTest.cpp`), wrapped in
`#if WITH_DEV_AUTOMATION_TESTS`. This is Epic's own layout — the engine keeps
`Runtime/Engine/Private/Tests/` exactly this way.

What this buys, and it is the whole reason for the choice: **it requires no build plumbing at
all.** No new module, no `.uproject` entry, no `Build.cs` change, no new target. Every module
already depends on `Core` (where `Misc/AutomationTest.h` lives) and on `Engine` (where
`Tests/AutomationCommon.h` lives). A new `.cpp` under an existing module's folder is picked up by
UnrealBuildTool automatically, and `WITH_DEV_AUTOMATION_TESTS` is compiled to `0` in Shipping and
Test targets, so nothing written this way ever reaches a packaged build.

### Registration and naming

One macro per test:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSmoresInventoryStackCapTest,
    "Smores.Items.Inventory.StackCapClampsMerge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
```

- **Pretty name is `Smores.<Module>.<Subject>.<Case>`**, dot-separated. The dots are what the
  Automation window renders as a tree, and the leading `Smores.` is what makes
  `Automation RunTests Smores` run the project's tests and none of the engine's several thousand.
- **Exactly one filter flag is mandatory** — `ProductFilter` for everything here. The macro
  carries a `static_assert` that rejects zero or two, and its error text is not obvious.
- **At least one application-context flag is mandatory**, same `static_assert`.
- **The 5.5+ gotcha, which will bite anyone copying from a tutorial:** `EAutomationTestFlags`
  became an `enum class` in UE 5.5, so the combined masks could no longer live inside it. They are
  now free constants with an **underscore**: `EAutomationTestFlags_ApplicationContextMask`, not
  `EAutomationTestFlags::ApplicationContextMask`. Every pre-5.5 example online uses the old
  spelling and will not compile against 5.8.

### Getting an authority-gated component under test

This is the one genuinely load-bearing technical decision, because nearly every mutator worth
testing in this codebase is authority-gated — `UInventoryComponent`, `UEquipmentComponent`,
`UWalletComponent` and `UHealthComponent` all no-op silently off-authority, by design and per
`multiplayer-discipline.md`.

**A test creates a throwaway world with `FTestWorldWrapper` (`Tests/AutomationCommon.h`) and
spawns a plain `AActor` into it to own the component under test.** An actor spawned into a world
with no net driver holds `ROLE_Authority`, so `HasOwnerAuthority()` is true and every mutator runs
its real path rather than its no-op path.

```cpp
FTestWorldWrapper WorldWrapper;
WorldWrapper.CreateTestWorld(EWorldType::Game);
UWorld* World = WorldWrapper.GetTestWorld();
// ... spawn actors, attach components, assert ...
WorldWrapper.ForwardErrorMessages(this);
WorldWrapper.DestroyTestWorld(true);
```

`BeginPlayInTestWorld()` runs `BeginPlay` (needed for `UWalletComponent`'s `StartingGold` and
`UTraderComponent`'s `StartingStock`), and `TickTestWorld(DeltaSeconds)` advances timers (needed
for `UHealthComponent`'s recovery). Both are opt-in — a test that needs neither should skip them
and stay fast.

**The trap this decision exists to avoid, and it is a nasty one:** constructing the component with
`NewObject<UInventoryComponent>(GetTransientPackage())` and no owner at all looks simpler and is
actively dangerous. With no owner, `HasOwnerAuthority()` returns false, every mutator returns
without doing anything, and a test written as "call `RemoveEntry`, assert the grid is unchanged"
**passes for entirely the wrong reason**. A whole suite can be green while testing nothing. The
first test written in Slice 1 is therefore an assertion that `HasOwnerAuthority()` is actually
true in a test world — so that if any of the above is wrong, it is caught once, loudly, instead of
silently poisoning every slice that follows.

### Item definitions in tests

**Tests build their own `UItemDefinition` objects in memory** — `NewObject<UItemDefinition>()`
plus direct field assignment — and never load anything out of `Content/`.

A test that loads a real asset is testing that asset as much as the code, so a designer retuning a
sword's weight breaks an unrelated inventory test and the failure points at the wrong place.
In-memory definitions also let a test state its inputs where the assertion is: a 2x3 footprint
with a stack cap of 10 is visible in the test body rather than requiring someone to open an asset
to find out what the numbers were.

A small shared helper (`MakeTestItemDefinition(Footprint, MaxStack, Weight, BaseValue)`) belongs
in one place rather than being re-written per file — Slice 1 creates it.

## Test Inventory by System

What follows is the catalogue of what is worth asserting, grouped by the module it lives in and
ordered roughly by value. Slices below draw from this list; it is not itself the running order.

### `SmoresItems` — `UInventoryComponent`

The largest and highest-value target. Pure grid arithmetic plus the counted-return family.

**Placement and geometry** (no mutation, fast):

- `CanPlaceAt` rejects a footprint crossing any grid edge, at all four edges.
- `CanPlaceAt` rejects overlap with a placed entry, and accepts a footprint that merely abuts it.
- `CanPlaceAt` with `IgnoreEntryId` accepts an entry re-anchored onto cells it already occupies
  itself — the case that makes a one-cell nudge legal.
- `FindFreePlacement` exhausts the natural orientation across the whole grid before trying the
  rotated one, so an item that fits either way comes back unrotated.
- `FindFreePlacement` returns false and leaves its out-params untouched when nothing fits.
- `FInventoryEntry::CoversCell` is true for every cell of a rotated footprint and false one cell
  past each edge.
- `GetFootprint(bRotated)` swaps width and height, and returns zero for an empty item.

**Stacking:**

- `GetEffectiveMaxStack` is base `MaxStackSize` x `StackMultiplier`, never returns below 1 for a
  real definition, and returns 0 for a null definition.
- `CanStackWith` requires the same definition, requires `IsStackable()`, and requires matching
  `bStolen` — a stolen unit must not launder itself into a clean stack.
- `CanStackWith` **ignores `Condition`**, deliberately. Assert it explicitly, because it reads
  like an oversight to anyone who didn't read the comment and is exactly the kind of thing a
  future session "fixes".

**The counted-return family — the highest-value tests in the project:**

- `AddItemCounted` reports the actual number taken when the grid runs out of room partway, and the
  grid holds exactly that many.
- `AddItem` returns false in that same case while still keeping what fit.
- `AddItemCounted` splits a quantity above the effective cap into as many entries as needed.
- `MoveItemCounted` merging into a destination stack already near its cap reports only what fit,
  not what was asked for — this is the bug `inventory-roadmap.md` Slice 8 found in the purchase
  path, and the one that
  would have overcharged a player.
- `MoveItemCounted` on a pure reposition within one grid reports the whole entry as moved.

**Move semantics:**

- `MoveItem` never swaps: a drop onto a non-stackable occupant is rejected whole, and **both grids
  are byte-identical afterwards**. Assert the non-mutation, not just the `false`.
- A cross-grid move removes from the source in the same operation that adds to the destination —
  no window where the item is in both or neither.
- A partial-quantity move splits the source stack and leaves the remainder in place.

**Entries and lifecycle:**

- `EntryId` survives other entries being removed — place three, remove the middle, the third's id
  is unchanged and still resolves.
- `SetEntryQuantity` clamps to the effective cap; a new quantity of 0 or less removes the entry.
- `RemoveEntry` on an unknown id returns false and mutates nothing.
- `SetGridSize` drops exactly the entries that no longer fit and keeps the rest.
- `GetEntryIdAtCell` returns `INDEX_NONE` for a free cell and for an out-of-bounds cell.

**Weight:**

- `GetTotalWeight` is the sum of unit weight x quantity, and is unrelated to cells occupied —
  assert with one bulky-light and one small-heavy item so the two measures visibly disagree.
- `HasWeightLimit` is false at capacity 0, and `IsOverWeightCapacity` is false at capacity 0 **no
  matter how much is carried**. A chest holding a tonne is not over capacity.
- `IsOverWeightCapacity` is a strict comparison — exactly at capacity is not over.

**Sort and repack (`SortEntries`):**

Added with Slice 9 of `inventory-roadmap.md`. This is the single best-suited thing in the whole
system to an automated test: it is pure, deterministic, all-or-nothing arithmetic over the grid,
and every one of its rules is invisible on screen.

- Each criterion orders descending — build a grid where weight, value and quantity disagree about
  the ordering, sort by each in turn, and assert the resulting anchor-cell order differs as
  expected. A grid where all three agree proves nothing.
- The result is **deterministic**: sorting an already-sorted grid returns false and mutates
  nothing, and two grids built with the same contents added in *different orders* sort to
  identical placements. This is what the `EntryId` tiebreak exists for, and an unstable-sort
  regression would show up nowhere else.
- Consolidation: two partial stacks of the same definition become one; a pair that `CanStackWith`
  rejects (different `bStolen`) stays two; merging never exceeds `GetEffectiveMaxStack`.
- **All-or-nothing**: construct a case where first-fit in criterion order can't re-place
  everything, and assert the grid is byte-for-byte what it was — same ids, same anchors, same
  rotations, same quantities — and that `OnInventoryChanged` did not fire.
- Entry ids survive a repack (a UI holds one across a round trip), and no entry is lost: total
  quantity per definition before equals total after, always.
- Non-authority returns false and mutates nothing, like every other mutator here.

**Refusal reasons (`SortEntriesWithReason`):**

Added with the refusal line — see `refusals-and-feedback.md`. Worth asserting because the whole
point of these variants is that they distinguish two outcomes the bool collapses, and getting
one wrong produces a *wrong message* rather than a visible failure, which is far harder to spot
in PIE than silence was.

- An abandoned repack reports `NoRoom`; an already-sorted grid reports `None`. Both return false,
  and asserting only the bool would pass either way round.
- A non-authority call reports `None`, not a reason — the player didn't do anything wrong.
- The plain `SortEntries` forwarder returns exactly what `SortEntriesWithReason` returns, for
  every one of the above. A forwarder that drifts is the standing hazard of this whole family
  (`AddItem`/`AddItemCounted`, `MoveItem`/`MoveItemCounted`, `Equip`/`EquipWithReason`).

**Delegates:**

- `OnInventoryChanged` fires once per successful mutator and **not at all** on a rejected one —
  including an abandoned `SortEntries`.

### `SmoresItems` — `UEquipmentComponent`

- `GetSlotForItem` returns the definition's `EquipSlot`, and `EEquipSlot::None` for anything not
  wearable.
- `CanEquipItem` is slot-type matching and nothing else — assert that a heavy/worn/stolen item
  still equips, so a later session adding a skill gate here has to delete a test that says not to.
- `Equip` takes **one unit** off a stack and leaves the rest in the grid.
- `Equip(..., EEquipSlot::None)` resolves to the item's own slot.
- `Equip` naming a mismatched slot explicitly fails.
- **The all-or-nothing swap:** equipping into an occupied slot when the grid has no room for the
  displaced item fails, leaves the original item worn, and leaves the grid untouched. Nothing is
  destroyed by running out of room.
- `Unequip` into a full grid fails and leaves the item worn.
- `EquipWithReason` reports `WrongSlot` for a mismatched slot and `NoRoom` when the displaced
  item has nowhere to go — two failures that look identical through the bool, and that now
  produce two different messages on screen. `UnequipWithReason` reports `NoRoom` for a full grid.
  Both plain forwarders must agree with their informative versions.
- `GetTotalWeight` sums worn items and is reported separately from the grid's weight (the known
  seam recorded in `inventory-roadmap.md` Slice 5 — assert current behavior so the pass that merges
  them has to change a test on purpose).

### `SmoresEconomy` — `UWalletComponent` and pricing

Small, pure, and about money, which is the category where a silent error costs most.

- `TrySpendGold` with a balance one short returns false and **does not change the balance**.
- `TrySpendGold` for exactly the balance succeeds and leaves zero.
- `TrySpendGold` with a negative amount returns false and does not credit the wallet.
- `CanAfford(0)` and `CanAfford(-5)` are true (a free thing is always affordable).
- `AddGold` broadcasts `OnGoldChanged` with the new balance; `TrySpendGold` does too on success
  and not on failure.
- `StartingGold` is applied at `BeginPlay` — requires `BeginPlayInTestWorld()`.
- `UTraderComponent::GetUnitBuyPrice` is `BaseValue` x `BuyMarkup`, and **never rounds a saleable
  item down to free** — a `BaseValue` of 1 with a markup that would floor to 0 still costs 1.
- `GetUnitSellPrice` is `BaseValue` x `SellMarkdown`.
- Buy price exceeds sell price at the default markups — the trader's margin, asserted as an
  invariant rather than as two hardcoded numbers.
- `IPricingProvider::GetBuyPrice(Item, Quantity)` is unit x quantity, and a **negative quantity
  prices as zero rather than as a refund**.
- A `BaseValue` of 0 item prices at 0 on both sides (worthless is not free-with-margin).

### `SmoresCombat` — `UHealthComponent`

A three-state machine with a timer, which is precisely the shape that hides bugs.

- `TakeDamage` reduces health by the amount; a zero or negative amount changes nothing.
- Damage that doesn't reach zero broadcasts `OnDamaged` and leaves the state `Alive`.
- Damage reaching or passing zero moves to `Downed`, broadcasts `OnDowned`, and does **not**
  broadcast `OnDied`.
- `TakeDamage` on an already-Downed or Dead component changes nothing and broadcasts nothing.
- `Kill()` from `Alive` moves to `Dead` and broadcasts `OnDied`.
- `Kill()` twice broadcasts `OnDied` once.
- **`Kill()` on an already-Downed component cancels the recovery timer.** Tick past
  `DownedDurationSeconds` afterwards and assert the state is still `Dead`. This is the
  `inventory-roadmap.md` Slice 7 trap — a corpse standing back up a few seconds later — and it is
  the single best argument in this
  document for having tests at all: the failure is silent, delayed, and only reproducible by
  waiting.
- `Recover()` restores full health and broadcasts `OnRecovered` from `Downed`, and refuses to run
  from `Dead`.
- The recovery timer fires after `DownedDurationSeconds`. **Set `DownedDurationSeconds` to
  something small in the test** — the default is 15 seconds, and ticking a test world through 15
  simulated seconds at 100fps is 1500 iterations for no benefit.
- `IsIncapacitated()` is true for both `Downed` and `Dead` and false for `Alive`; `IsDowned()` is
  false once `Dead`.

### `SmoresCombat` — `UCombatComponent` (partial)

Mostly **not** testable, and worth recording why so nobody re-derives it: `PerformAttack` plays a
montage on the owner's anim instance, which needs a skeletal mesh, a skeleton and authored montage
assets. That is a PIE concern.

The one branch that is testable is the one that doesn't reach a montage:

- `AttackTarget` on a target beyond `AttackRange` broadcasts `OnTargetOutOfRange` and sets no
  current target.
- `AttackTarget` after `NotifyOwnerDowned` does nothing.
- `ClearCurrentAttackTarget` clears without touching montage state.

`AttackRange` is `protected` with no setter, so a range test needs either a test-only subclass in
the same module or a small seam. Prefer the subclass — a test-only accessor on production code is
a worse trade than four lines of `class FTestCombatComponent : public UCombatComponent`.

### `smores` — the trade transaction

`AStrategyPlayerController::TryTradeItem` is where the money and the goods actually change hands,
and its all-or-nothing ordering is the load-bearing detail `inventory-roadmap.md` Slice 8 calls
out: price the whole requested quantity, refuse up front if the balance won't cover it, move the
item, then debit for what actually moved — so the debit can never fail after the goods are gone.

That is worth asserting, and it is also the most expensive test in this document, because it needs
an `APlayerController` with an `APlayerState` carrying a `UWalletComponent`, plus a pawn and an NPC
with a `UTraderComponent`. It gets its own slice, late, and only after the cheap ones are paying
for themselves.

### Editor smoke tests

Cheap, high-value, and a different shape from everything above — these run in editor context and
assert that content still loads:

- Every `UItemDefinition` asset under `Content/` has a non-`None` `ItemId`, a non-empty
  `DisplayName`, and a footprint within the grid clamps.
- No two `UItemDefinition` assets share an `ItemId` (they are the stable identity, and a duplicate
  is silent until something resolves the wrong one).
- `LVL_Strategy` and `Lvl_MainMenu` load without error.

The engine already ships a Blueprint-compile smoke test that covers "did a C++ rename break a
Blueprint" — it needs no code here, only to be run. That is a run-instruction, not a slice.

## Explicitly Out of Scope

- **Coverage targets or a required-tests-before-merge rule.** This is a solo project; a gate that
  costs more than the bugs it catches is a net loss. The protocol in `testing.md` is "add a test
  when the shape calls for it," not "every function needs one."
- **UI / widget tests.** UMG widget construction needs a viewport, and what actually breaks in the
  inventory UI is layout, drag payloads and focus — none of which an assertion reaches. PIE owns
  this permanently.
- **Input binding tests.** `input-and-keybinds.md` owns the wiring rule, and what can go wrong (an
  unassigned `UInputAction`, a context added at the wrong priority) is asset state, not logic. An
  editor smoke test asserting "every `EditAnywhere` input action property on the controller BP is
  non-null" is the only version of this worth considering, and it belongs with the other asset
  smoke tests if it is ever wanted.
- **Multiplayer / replication tests.** `IMPLEMENT_NETWORKED_AUTOMATION_TEST` exists and needs two
  connected instances. Multiplayer isn't wired up or testable yet per CLAUDE.md, so there is
  nothing to point it at. The authority *gate* is testable today (see below); the replication
  itself is not.
- **Testing the no-authority path.** Asserting "a mutator no-ops on a client" needs an actor whose
  role is not `ROLE_Authority`, which needs a net driver. Deferred with multiplayer, and noted so a
  session doesn't burn an afternoon discovering the cost. What *is* cheap and worth doing: assert
  every mutator's authority gate exists at all, by asserting the positive path runs.
- **Performance / stress tests.** `PerfFilter` and `StressFilter` exist; nothing here is near a
  performance cliff worth guarding.
- **Gauntlet.** Launching packaged builds on devices needs a build farm. Nothing to gain solo.

## Relationship to the Current Implementation

What makes this cheap, and it is worth being explicit because it is unusual: **nothing in
`SmoresItems`, `SmoresEconomy` or `SmoresCombat` needs to change to be tested.** The module split
that `unreal-module-organization.md` drove for architectural reasons happens to have produced
exactly the property tests need — the gameplay rules live in components that depend on `Core`,
`CoreUObject` and `Engine` and nothing else, with no reference to the player controller, the HUD,
or any widget. `UInventoryComponent` can be exercised without a controller existing.

Three specific pieces of existing design carry most of the weight:

- **`static` `MoveItem`/`MoveItemCounted`** take both inventories as parameters, so a cross-grid
  transfer test needs two components and no coordinating object at all.
- **`IPricingProvider` derives totals rather than making them virtual**, so pricing arithmetic is
  testable against the interface without a trader existing.
- **`FInventoryItem` and `FInventoryEntry` are plain `USTRUCT`s with their logic inline** —
  `CanStackWith`, `CoversCell`, `GetFootprint`, `GetTotalWeight` need no `UObject` at all and are
  the fastest tests in the project.

The one place this doesn't hold is `smores` itself: `AStrategyPlayerController` is where selection,
input, window management and the trade transaction all meet, and none of it separates cleanly. That
is why the transaction test is a late slice rather than an early one, and the cost is a fair price
for the controller being where the variant-specific glue lives.

**No refactor is proposed here.** If a later session finds something genuinely untestable in a
feature module, that is worth recording as a signal about the design — but the rule is to write the
test around the code, not to reshape production code for the test's convenience. The one sanctioned
exception is a test-only subclass in the same module to reach a `protected` member.

## Implementation Order

Three slices, **one per clean session**. Every slice follows the same protocol, so it isn't
repeated per entry:

1. Read `testing.md`, this slice's entry, and the source files it names.
2. Write the tests. Every new test is a new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` class in a new or
   existing `.cpp`, which means **a cold Visual Studio build** (close the editor, build, reopen) —
   never Live Coding. A new file in a module is not something Live Coding picks up reliably.
3. Run them headless and confirm the count of tests run matches what was added — a test that fails
   to register doesn't fail, it silently doesn't appear. **Check the number, not just the colour.**
4. Commit the slice on its own, then move its shipped content from this file into `testing.md` and
   mark the slice `DONE` below.

### Why only three, and why none of them need Jim

An earlier draft of this roadmap cut the same work into eight slices. That was wrong, and the
reasoning is worth keeping because it applies to every roadmap in this project.

**There is no human in this work.** No slice needs a PIE pass, `unreal-mcp`, Blueprint wiring, or
an asset touched by hand — the entire point of the harness is that it decides pass/fail by
comparing two numbers. Nothing here is a thing Jim has to look at.

The three things that split the old eight were not reasons:

- **"It needs a cold build."** Cold builds are slow, not blocking. An agent can close the editor,
  build, reopen, and carry on inside one session.
- **"It's a different file, or a different module."** Once the first test file compiles, every file
  after it is the same pattern applied to different code. That is typing, not a decision.
- **"We should check the last one works first."** The check here is `Automation RunTests Smores`
  and reading a count. The agent does that itself, mid-session, and continues.

What remains is the one honest reason to split — **context budget**, since build errors and log
output are expensive — plus exactly one genuine judgment call, which is whether the expensive
controller test is worth attempting at all. Hence three.

### Slice 1 — The harness, and all of `SmoresItems`

The big one, and the one carrying most of the value in this document. It is large deliberately:
everything in it is the same shape, against components that need nothing but a world and an owner.

- **Build:** a test-support header (the RAII helper wrapping `FTestWorldWrapper` plus a spawned
  owner actor, and the in-memory `MakeTestItemDefinition` factory), then
  `Source/SmoresItems/Tests/InventoryGeometryTest.cpp`, `InventoryStackingTest.cpp`,
  `InventoryMoveTest.cpp`, `InventorySortTest.cpp` and `EquipmentComponentTest.cpp`.
- **Where the support header lives, decided here:** `SmoresCombat` does not depend on
  `SmoresItems`, so a helper placed in `SmoresItems` is unreachable from Slice 2's health tests.
  The world/owner helper therefore goes in **`SmoresCore`**, which every module already depends on
  and which is currently an empty proving module looking for exactly this kind of tenant. The
  item-definition factory is `SmoresItems`-specific and stays there.
- **The proving checkpoint — which is a checkpoint, not a session boundary.** Write the authority
  sanity check (`HasOwnerAuthority()` is true in a test world) plus the first geometry test
  *first*, cold build, run headless, then **deliberately break one assertion, re-run, and confirm
  it reports a failure**. A suite that has never gone red has not been shown to work. Do all of
  that inside this session, fix the assertion back, and keep writing. Everything unknown about the
  harness — the 5.5+ flag spellings, the `FTestWorldWrapper` API, whether a new `.cpp` registers at
  all — surfaces at this checkpoint. After it, the risk is gone.
- **Tests:** the authority check, then the "Placement and geometry", "Stacking", "counted-return
  family", "Move semantics", "Entries and lifecycle", "Weight", "Sort and repack", "Refusal
  reasons" and "Delegates" groups from the `UInventoryComponent` inventory above, then the whole
  `UEquipmentComponent` group.
- **Also produces:** the "Running the tests" and "When to add a test" sections of `testing.md`.
  Those are the deliverable as much as the code is — a suite nobody knows how to run is worth
  nothing, and a convention nobody records gets re-invented three different ways.
- **Touches:** new files only. No existing source, no `Build.cs`, no `.uproject`.
- **Done when:** `Automation RunTests Smores` runs headless and reports the expected count; the
  authority assertion is among them; the suite has been seen to go red on purpose; every
  counted-return case is asserted; the non-swap case asserts **both grids are unchanged** rather
  than only that the call returned false; the abandoned-sort case asserts the same about the grid
  it declined to repack; each `*WithReason` variant reports the right reason *and* agrees with its
  plain forwarder; and the all-or-nothing equip swap asserts three things together — false
  returned, original item still worn, grid untouched.
- **If context runs short:** stop after the sort and refusal groups and finish equipment in a fresh
  session. That is a budget decision taken on the day, not a change to the plan — don't re-cut the
  roadmap around it.

### Slice 2 — Economy, combat, and the content smoke tests

Everything outside `SmoresItems` that is cheap to reach. Three small subjects in one session.

- **Build:** `Source/SmoresEconomy/Tests/WalletComponentTest.cpp`,
  `Source/SmoresEconomy/Tests/PricingTest.cpp`,
  `Source/SmoresCombat/Tests/HealthComponentTest.cpp` and
  `Source/SmoresItems/Tests/ItemDefinitionAssetTest.cpp`.
- **Why these share a session:** they are small (`WalletComponent.cpp` is 97 lines,
  `TraderComponent.cpp` 60, `HealthComponent.cpp` 202), and each exercises a *different* part of
  the harness that Slice 1 never touched — `BeginPlayInTestWorld()` for `StartingGold`,
  `TickTestWorld()` for the recovery timer, and editor context plus the asset registry for the
  smoke tests. That makes this one coherent "prove the harness does the rest of its job" session
  rather than three sessions that each prove one thing.
- **Tests:** the `SmoresEconomy` group, the `UHealthComponent` group including the timer cases, and
  the "Editor smoke tests" group.
- **Touches:** new files, **plus one `Build.cs` change** — enumerating `UItemDefinition` assets
  needs `AssetRegistry`, which `SmoresItems` does not currently depend on. Add it to
  `PrivateDependencyModuleNames`. This is the one place this roadmap's "new files only" claim does
  not hold, and it was previously missed.
- **Prove the asset rules against an in-memory definition, not a broken asset.** An earlier draft
  had this slice prove itself by adding a deliberately malformed `UItemDefinition` to `Content/`
  and then removing it — that is hand work in the editor, it dirties the content tree, and it
  demonstrates nothing that the same assertion run against a `NewObject<UItemDefinition>()` with an
  empty `ItemId` doesn't demonstrate for free. Keep the asset-registry sweep pointed at real
  content; prove the *rule* in memory.
- **Editor context note:** these smoke tests need `EditorContext` in their flags and will not run
  in a headless *game* target. The run command in `testing.md` uses `UnrealEditor-Cmd`, so it
  covers them; a packaged-build runner would not.
- **Done when:** the insufficient-funds case asserts the balance is unchanged; the margin invariant
  (buy > sell) is asserted rather than two hardcoded numbers; the kill-cancels-recovery case ticks
  past `DownedDurationSeconds` and asserts the state is still `Dead`; and a failing asset sweep
  names the offending asset rather than just failing.

### Slice 3 — The trade transaction, and a run script (optional)

The only slice with a real decision in it, which is why it stays separate rather than folding into
Slice 2.

- **Start it only once Slices 1 and 2 have shipped and proven useful.** That judgment is Jim's, and
  it is the single genuine human gate in this roadmap — not a verification step, a decision about
  whether an expensive test is worth its setup cost.
- **Build:** `Source/smores/Tests/TradeTransactionTest.cpp`, standing up enough of a controller,
  player state and pawn to reach `TryTradeItem`; plus, if wanted, a small `RunTests.ps1` at the
  project root wrapping the headless command with a non-zero exit on failure.
- **Tests:** the all-or-nothing ordering — a purchase the player can't fully afford is refused with
  nothing moved and nothing debited; a purchase truncated by a destination stack cap debits for
  what actually moved and not for what was asked.
- **Touches:** new files only, but the setup is substantial and may reveal that
  `AStrategyPlayerController` needs a seam. **If it does, stop and record it here rather than
  reshaping the controller inside a testing slice** — that is a design change wearing a test's
  clothes, and it deserves its own session.
- **Done when:** both ordering cases are asserted, or the slice is abandoned with a written reason.
  Abandoning it is an acceptable outcome; the ordering is already documented and the manual
  `SmoresBuyItem` exec exercises the same path. The `RunTests.ps1` wrapper is convenience rather
  than coverage and rides along here only because it has nowhere better to go — skip it if the full
  command line isn't annoying anyone yet.

## Resolved Design Decisions

Recorded so future sessions don't reopen them:

- **Tests live inside the feature modules** (`Source/<Module>/Tests/`), guarded by
  `#if WITH_DEV_AUTOMATION_TESTS`. Chosen because it needs no new module, no `.uproject` entry, no
  `Build.cs` change and no new target — `Core` and `Engine` are already dependencies everywhere, and
  the guard compiles the whole thing out of Shipping. It is also Epic's own layout. (**Rejected:** a
  dedicated `SmoresTests` module — it needs an entry in `.uproject`, a `Build.cs` naming every
  module it tests, and an editor-only type to keep it out of packaged builds, in exchange for a
  separation the `#if` already provides; worse, its dependency edges point *up* at every feature
  module at once, which is the one thing `unreal-module-organization.md`'s dependency-direction rule
  exists to prevent. Revisit only if test compile time becomes a real cost. **Rejected:** Low-Level
  Tests / Catch2 — genuinely faster since they skip the editor entirely, but they need their own
  `*Tests.Target.cs`, the `bhaslowleveltests` flag and the `LowLevelTestsRunner` plumbing, and they
  are a poor fit for code that wants a `UWorld`, which is most of what's worth testing here.
  **Rejected:** Gauntlet — needs a build farm.)
- **The engine's automation framework, not a third-party one.** `IMPLEMENT_SIMPLE_AUTOMATION_TEST`
  ships with the engine, integrates with the Session Frontend window and the command line, and costs
  nothing to adopt. (**Rejected:** vendoring Catch2 or doctest directly — they don't know about
  `UObject` lifetime, garbage collection or worlds, which is where the bugs are.)
- **A test world with a spawned owner actor, via `FTestWorldWrapper`.** It is what makes an
  authority-gated mutator actually run. (**Rejected:** `NewObject` with no owner — every mutator
  silently no-ops and tests pass for the wrong reason, which is worse than having no tests.
  **Rejected:** extracting the grid arithmetic into free functions so it can be tested without a
  world — it splits working code for the tests' benefit and leaves the authority gate, which is the
  part multiplayer discipline cares about, untested.)
- **Item definitions are constructed in memory, never loaded from `Content/`.** A test that loads a
  real asset fails when a designer retunes that asset, and points at the wrong place when it does.
  (**Rejected:** a dedicated folder of test-only `.uasset` definitions — it is content to maintain,
  it can drift from what the tests assume, and it puts the test's inputs somewhere you have to open
  the editor to read.)
- **Pretty names are `Smores.<Module>.<Subject>.<Case>`.** The leading `Smores.` is a filter handle
  — it is what lets one command run the project's tests without the engine's thousands.
  (**Rejected:** naming by file or class, which gives no grouping and no filter prefix.)
- **`ProductFilter` for everything, not `SmokeFilter`.** Smoke tests run inside other people's build
  steps and are expected to be near-instantaneous; these are project tests that should run when
  asked. (**Rejected:** `SmokeFilter` for the fast struct-only tests — a split that buys a second or
  two and costs a rule nobody will remember.)
- **Shared test support lives in `SmoresCore`, item-specific support in `SmoresItems`.** The
  world/owner helper is wanted by three modules, and `SmoresCore` is the only one all of them
  already depend on. (**Rejected:** duplicating the helper per module, which is three copies of the
  thing most likely to need a fix when the engine's test API shifts.)
- **No test for the off-authority no-op path** until multiplayer is testable, because it needs a net
  driver to produce a non-authoritative actor. The gate is asserted positively instead — the mutator
  runs when it should.
- **Once the harness exists, tests are added alongside the work that creates the need**, per the
  protocol in `testing.md`, rather than in periodic catch-up passes. (**Rejected:** a standing
  "write tests for everything" habit — it produces a suite matched to the code as it was that day,
  with no habit behind it.) This is about *ongoing* work and does not conflict with the three
  build-out slices below, which are a one-time catch-up on code that already shipped untested.
- **The build-out is three slices, not eight.** Re-cut on 2026-09-16. The original eight split on
  module and file boundaries, which are not reasons — nothing in this roadmap needs a PIE pass,
  `unreal-mcp`, Blueprint wiring or a hand-touched asset, so there is no human step to slice
  around, and a cold build is slow rather than blocking. The remaining boundaries are context
  budget (Slices 1 and 2) and one genuine judgment call about whether an expensive test is worth
  its setup (Slice 3). (**Rejected:** one single slice — Slice 1 is already five files and the
  bulk of the assertions, and Slice 3's decision genuinely belongs to a different session.
  **Rejected:** keeping a separate slice per module — "same pattern, new code" is typing, and
  a session boundary is not free.)
- **A test-only subclass is the sanctioned way to reach a `protected` member** (`AttackRange`), not
  a test-only public accessor on the production class. (**Rejected:** widening access for tests — it
  makes the production API lie about its own encapsulation.)
- **Testing stops at the boundary of "a human has to look at it."** No widget, input-binding,
  animation, camera or AI-behavior tests, now or later. (**Rejected:** automating a PIE session
  through Gauntlet or functional-test actors — high setup cost, brittle, and it competes with the
  one thing that actually works for this project, which is Jim playing it.)
