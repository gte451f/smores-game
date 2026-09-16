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

**The project had no tests before Slice 1.** It now has 73. Note that
`Automation_smores.slnx` is still not a test setup - it is a solution file pulling in Epic's own
`UnrealBuildTool` and `EpicGames.*` C# projects, several of which are named `*.Tests`. None of them
test smores.

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

## The Harness - SHIPPED (Slice 1)

Built and documented in `testing.md` ("Writing a Test", "What the harness gives you"). In short:
tests live in `Source/<Module>/Tests/` behind `#if WITH_DEV_AUTOMATION_TESTS`, named
`Smores.<Module>.<Subject>.<Case>`; `FSmoresTestWorld` (in `SmoresCore`) supplies the throwaway
world and the authoritative owner actor; item definitions are built in memory by
`MakeTestItemDefinition`. No `Build.cs` or `.uproject` change was needed.

One thing the plan did not anticipate, now recorded in `testing.md`: Jim's machine dual-boots and
**Windows runs four hours fast until he syncs it**, so an NTP sync mid-session jumps the clock
backwards and leaves build outputs looking newer than freshly-edited sources. UnrealBuildTool then
correctly reports "up to date" and compiles nothing. Not a build-system bug and not fixable with a
flag - check the build output for `Compile [x64]` lines, and delete the stale `.obj` when it skips.

## Test Inventory by System

What follows is the catalogue of what is worth asserting, grouped by the module it lives in and
ordered roughly by value. Slices below draw from this list; it is not itself the running order.

### `SmoresItems` - SHIPPED (Slice 1)

`UInventoryComponent` (geometry, stacking, counted returns, moves, entries, weight, sort/repack,
refusal reasons, delegates) and `UEquipmentComponent` (slots, one-unit equip, the all-or-nothing
swap, refusal reasons) are covered by 45 tests. See `testing.md`'s "What Is Covered" table.

### `SmoresEconomy` - SHIPPED (Slice 2)

`UWalletComponent` (refusals that mutate nothing, the free/negative boundaries, the broadcast,
`StartingGold` at `BeginPlay`) and pricing (markup, markdown, the never-free rule, the margin
invariant, totals, negative quantities) are covered by 14 tests. See `testing.md`'s "What Is
Covered" table.

### `SmoresCombat` — `UHealthComponent` - SHIPPED (Slice 2)

The Alive/Downed/Dead machine, both timer cases (the kill that cancels a pending recovery, and the
recovery that fires), and the incapacitated queries are covered by 9 tests. The one lesson worth
carrying forward is in `testing.md` under "Timers need `BeginPlay()` before they will tick at all".

### `SmoresCombat` — `UCombatComponent` (partial) - UNCLAIMED

**No slice owns this.** Slice 2 shipped the rest of `SmoresCombat` and left it, because it is three
assertions behind a test-only subclass rather than part of the health group. It is small enough to
ride along with whatever next touches combat.

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

### Editor smoke tests - SHIPPED (Slice 2)

The asset sweep (well-formed definitions, unique `ItemId`s, the rule proved in memory) and the map
load are covered by 4 tests under `Smores.Content.*`. See `testing.md`'s "The content smoke tests
are the one exception, and run in editor context".

The engine already ships a Blueprint-compile smoke test that covers "did a C++ rename break a
Blueprint" — it needs no code here, only to be run. That is a run-instruction, not a slice, and
`testing.md`'s "Useful variations" table has the command.

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

Three slices, **one per clean session**. Slices 1 and 2 are done; only 3 remains. Every slice
follows the same protocol, so it isn't repeated per entry:

1. Read `testing.md`, this slice's entry, and the source files it names.
2. Write the tests. Every new test is a new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` class in a new or
   existing `.cpp`, which means **a cold build** (close the editor, build, reopen) — never Live
   Coding. A new file in a module is not something Live Coding picks up reliably. Check the build
   output for `Compile [x64]` lines: after a system-clock correction UnrealBuildTool can report
   success having compiled nothing (see The Harness above).
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

### Slice 1 — The harness, and all of `SmoresItems` — **DONE** (2026-09-16)

Shipped. 46 tests, all green, run headless in about 0.3 seconds once the editor has loaded.

- **Built:** `SmoresCore/Tests/SmoresTestWorld.h` (the world/owner RAII helper, an `FGCObject` so
  test-built definitions survive a collect), `SmoresCore/Tests/SmoresTestDelegateListener.h`,
  `SmoresItems/Tests/SmoresItemTestFactory.h` (definition factory, `FInventorySnapshot`, shared
  helpers), plus `SmoresTestWorldTest.cpp`, `InventoryGeometryTest.cpp`,
  `InventoryStackingTest.cpp`, `InventoryMoveTest.cpp`, `InventoryEntriesTest.cpp`,
  `InventorySortTest.cpp` and `EquipmentComponentTest.cpp`.
- **Deviations from the plan, both small:** an extra file (`InventoryEntriesTest.cpp`) carries the
  entries/weight/delegate groups rather than crowding them into the stacking file; and the shared
  helpers live in the factory header rather than an anonymous namespace per file, because unity
  builds concatenate translation units and would collide on the duplicate names.
- **The proving checkpoint was done as written** and was worth it: the suite was deliberately
  broken, seen to report a real failure naming the test and the assertion, then repaired.
- **Everything in the Done-when list holds**: the counted-return cases assert the count rather than
  the bool, the non-swap and abandoned-sort cases assert both grids are byte-identical *and* that
  nothing was broadcast, each `*WithReason` variant is checked against its plain forwarder, and the
  all-or-nothing equip swap asserts all three things together.
- **What this cost that the plan did not predict:** the UBA UTC-timestamp trap (see The Harness
  above). It burned a build cycle and would have silently invalidated the whole run.

### Slice 2 — Economy, combat, and the content smoke tests — **DONE** (2026-09-16)

Shipped. 27 new tests, 73 in total, all green.

- **Built:** `SmoresEconomy/Tests/WalletComponentTest.cpp` (8) and `PricingTest.cpp` (6),
  `SmoresCombat/Tests/HealthComponentTest.cpp` (9),
  `SmoresItems/Tests/ItemDefinitionAssetTest.cpp` (3) and `smores/Tests/MapLoadTest.cpp` (1), plus
  the one planned `Build.cs` change (`AssetRegistry` on `SmoresItems`).
- **Deviations from the plan, both small:** the map check went to `Source/smores/Tests/MapLoadTest.cpp`
  rather than into the item-definition file, because the maps belong to the game module and not to
  `SmoresItems`; and the content group is named `Smores.Content.*` rather than `Smores.Items.*`,
  since it is about the content tree rather than about a module.
- **The harness needed two corrections**, both of which Slice 1 had never exercised and both now
  written up in `testing.md`:
  - **Timers don't tick before `BeginPlay()`.** `FTimerManager` skips a frame it has already
    ticked, and the engine's wrapper only advances the frame counter once play has begun, so a
    timer test without `BeginPlay()` waits for nothing — and `KillCancelsPendingRecovery` passes
    anyway, because what it asserts is that *nothing happened*. `TickFor()` now refuses and returns
    false when play hasn't begun, and the tests assert its return value.
  - **The delegate listener may not hold its payload strongly.** Recording the damage instigator in
    a `UPROPERTY` kept the test world alive through the listener and the teardown failed with
    "Previously active world not cleaned up by garbage collection". It is a `TWeakObjectPtr` now.
- **The proving checkpoint came for free.** Both corrections above arrived as real red failures
  naming the test and the assertion, so no deliberate breakage was needed to show the suite reports
  honestly.
- **Everything in the Done-when list holds**: every refusal case asserts the balance afterwards
  rather than only the returned bool, the margin is asserted as `buy > sell` rather than as two
  numbers, the kill-cancels-recovery case ticks a full second past a 0.2s recovery (and is paired
  with a test proving that timer does fire), and a failing asset sweep names the asset by soft
  object path.
- **What this cost that the plan did not predict:** the clock skew landed *mid-build* this time
  rather than between builds, leaving one module linked against stale generated reflection code
  while the build reported success. The tell was a `UFUNCTION` missing from the built DLL; the fix
  was deleting that module's `Intermediate` folders and `Makefile.bin` and rebuilding.

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
