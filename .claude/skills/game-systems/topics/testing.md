# Automated Testing

## Purpose

This topic documents smores' **built** automated test suite: how to run it, what it covers, and
the standing rule for when a piece of work should add to it. The forward-looking plan — what
still needs building, in what order, and the decisions behind it — lives in
`Docs/roadmaps/testing-roadmap.md`.

> **Status: the harness is built and Slices 1 and 2 have shipped.** 92 tests run green, covering
> `SmoresItems`, `SmoresEconomy`, `UHealthComponent`, the content smoke tests, and (added by the
> HUD roadmap) the time-pace ladder, the target panel's action assembly and the activity log. Only
> Slice 3 of `Docs/roadmaps/testing-roadmap.md` remains. Everything below has been executed
> against this project rather than written in advance.

`Automation_smores.slnx` is **not** a test setup. It is a solution file that pulls in Epic's own
`UnrealBuildTool` and `EpicGames.*` C# projects, several of which are named `*.Tests`. None of
them test smores.

## What Is Covered

| System | Module | Covered | Slice |
|---|---|---|---|
| Test world holds authority | `SmoresCore` | ✅ 1 test | 1 |
| Inventory grid geometry and placement | `SmoresItems` | ✅ 8 tests | 1 |
| Inventory stacking and counted returns | `SmoresItems` | ✅ 7 tests | 1 |
| Inventory moves and transfers | `SmoresItems` | ✅ 8 tests | 1 |
| Inventory entries, grid resize, weight, delegate | `SmoresItems` | ✅ 8 tests | 1 |
| Inventory sort repack (determinism, all-or-nothing) | `SmoresItems` | ✅ 8 tests | 1 |
| Refusal reasons (`*WithReason` vs. their forwarders) | `SmoresItems` | ✅ in sort + equipment | 1 |
| Equipment slots and the all-or-nothing swap | `SmoresItems` | ✅ 6 tests | 1 |
| Wallet balance, refusals and broadcasts | `SmoresEconomy` | ✅ 8 tests | 2 |
| Pricing (markup, markdown, totals, margin) | `SmoresEconomy` | ✅ 6 tests | 2 |
| Health state machine (Alive/Downed/Dead) and its timer | `SmoresCombat` | ✅ 9 tests | 2 |
| `UItemDefinition` assets under `Content/` | `SmoresItems` | ✅ 3 tests | 2 |
| Both maps still load | `smores` | ✅ 1 test | 2 |
| Time-pace ladder (tiers, dilation, stepping, clamping, authority) | `SmoresCore` | ✅ 6 tests | HUD 2 |
| Target-panel action assembly (per target kind, reach, hostility) | `smores` | ✅ 7 tests | HUD 2 |
| Activity log ring buffer (eviction, shrink, filtering, broadcast, ids) | `SmoresCore` | ✅ 6 tests | HUD 3 |
| Attack range / out-of-range branch | `SmoresCombat` | — | unclaimed |
| Trade transaction ordering | `smores` | — | 3 |

**92 tests.** The last three rows were added by `Docs/roadmaps/hud-roadmap.md`, not by the
testing roadmap — a slice that ships numbers-and-state-machine code writes its own tests, whichever
roadmap it came from. Update this table as slices ship; it is the quick answer to "is this already
covered?"

## What Is Deliberately Not Covered

Automated tests here catch **silent numerical and state-machine regressions** — a function that
returns `true` having done only part of the job, a state that transitions when it shouldn't.
They do not, and will not, cover anything whose pass/fail is a human judgment.

**These stay in PIE, permanently:** drag-and-drop feel, the rotate-while-dragging gesture,
window layout and stacking order, camera pan/zoom tuning, animation and montage timing, EQS
destination quality, NPC behavior, and whether any number *feels* right. See
`Docs/roadmaps/testing-roadmap.md`'s "Explicitly Out of Scope" for the full list and the reasoning.

The console `exec` commands remain the manual counterpart and are not replaced by any of this —
`SmoresDumpInventory`, `SmoresAddItem`, `SmoresDropItem`, `SmoresDumpEquipment`, `SmoresAddGold`,
`SmoresBuyItem`, `SmoresKillNPC` and the rest. They exercise the real code paths from inside a
running game, which is something no test here does.

## Running the Tests

### From the editor

**Tools → Session Frontend → Automation tab.** Type `Smores` in the filter box, tick the tests
you want, and press **Start Tests**. Results appear in the same panel; a failed assertion links
to the file and line.

This is the fastest loop while writing a test, because the editor is already loaded.

### Headless, from the command line

This is the one to use when verifying work, and the one an agent should use. It boots the
project without a window, runs everything under the `Smores` prefix, and quits.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\dev\smores\smores.uproject" "-ExecCmds=Automation RunTests Smores" "-testexit=Automation Test Queue Empty" -unattended -nopause -nosplash -nullrhi -log -ReportExportPath="C:\dev\smores\Saved\Automation"
```

Notes on the pieces that matter:

- **`UnrealEditor-Cmd.exe`, not `UnrealEditor.exe`** — the `-Cmd` variant is the console build
  and is what makes `-unattended` behave.
- **Quote whole arguments that contain spaces**, as above (`"-ExecCmds=Automation RunTests
  Smores"`). PowerShell splits on the space otherwise and the editor silently receives a
  truncated command, then sits there having run nothing.
- **`-testexit="Automation Test Queue Empty"`** is what makes the process quit when the run
  finishes. Without it the editor stays open forever and the command never returns.
- **`-nullrhi`** skips rendering. Safe for everything in the roadmap; drop it if a future test
  ever needs a real render target.
- **`-ReportExportPath`** writes a JSON report. Optional, but it is the only durable record of a
  run.
- Expect **30–60 seconds of project load** before the first test runs. That is the cost of the
  editor-based harness and is not a sign anything is wrong.
- Running this while the editor is open on the same project works, but the two contend over the
  derived-data cache and the run is slower. Closing the editor first is the cheaper path.

### Useful variations

| Command | Does |
|---|---|
| `Automation List` | Prints every registered test name — use it to confirm a new test registered at all |
| `Automation RunTests Smores.Items` | Runs one module's subtree |
| `Automation RunTests Smores.Combat.Health` | Runs one subject |
| `Automation RunTests Blueprint` | Everything with "Blueprint" in its path (442 tests, mostly engine and toolset) — **run this after any C++ rename or removed `UFUNCTION`/`UPROPERTY`**, it is the cheapest way to catch a Blueprint broken by it. Note: `Project.Blueprints` is named in a lot of UE documentation but matches **no registered test in this 5.8 build** — it produces no report at all, which reads as a pass if you only check the exit code. Verified 2026-09-16 |

### Reading the result — the one rule that matters

**Check the number of tests run, not just the colour.** A test that fails to register does not
fail — it silently does not appear, and the run comes back green having executed nothing. After
adding tests, confirm the reported count went up by the number you added. `Automation List` is
the direct check when it doesn't.

### After adding or changing a test file

A new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` is a new C++ class, and a new `.cpp` is a new file in
the module. **Both require a cold build** — close the editor, build, reopen. Live Coding is
unreliable for new types generally (see CLAUDE.md) and does not reliably pick up new files at all.

Building from the command line, which is what an agent should do:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" smoresEditor Win64 Development -Project="C:\dev\smores\smores.uproject" -WaitMutex
```

#### The incremental-build trap, which cost a session once and will again

**Jim's machine dual-boots Windows and Ubuntu, and Windows starts up four hours fast until he
syncs it against an NTP server.** That sync can land in the middle of a session, and when it does
the system clock jumps *backwards* four hours.

Anything built before the jump then carries a timestamp four hours ahead of anything edited after
it. UnrealBuildTool compares source against object, finds the object newer, and correctly reports
**"Target is up to date"** having compiled nothing. The tests then run against stale object code
and report whatever the previous version did.

This is silent. Nothing errors; the build says `Result: Succeeded` in about a second. It is **not**
a bug in UnrealBuildTool, UBA or Unreal — the timestamps really do say what UBT reads them as
saying, and `-NoUBA` is not a fix for it. Don't go looking for a build-system setting; there isn't
one to find.

**The nastier variant, seen in Slice 2: the sync can land *inside* a build.** The build then
compiles plenty of files, prints `Compile [x64]` lines for every source you edited, reports
`Result: Succeeded` — and still links one module against **stale generated reflection code**,
because UHT compared a header stamped before the jump against generated output stamped after it and
decided nothing had changed. The C++ compiles and links fine, because the edits were inline in the
header; what's missing is the reflection data. A `UFUNCTION` added that session simply isn't
registered, so an `AddDynamic` binding to it silently never fires.

The tell is that the module's own `Compile`/`Link` lines are *absent* from an otherwise busy build,
and the check is to look for the new name in the built DLL:

```powershell
Select-String -Path "C:\dev\smores\Binaries\Win64\UnrealEditor-SmoresCore.dll" -Pattern "MyNewFunction" -Encoding Ascii
```

UHT writes reflected names into the binary as plain ASCII, so a name that isn't there wasn't built.
The fix is the bigger hammer below — delete that module's `Intermediate` folders (both the
`Development\<Module>` objects and the `UnrealEditor\Inc\<Module>` generated headers) plus
`Makefile.bin`, then rebuild.

Three habits defuse all of it:

- **Check that the build actually compiled something.** If the output has no `Compile [x64] <file>`
  lines and you just edited a file, it did not build your change. A one-second build is the tell,
  and it is the only reliable signal.
- **After changing a `UCLASS`/`UFUNCTION`/`UPROPERTY`, check the name landed in the DLL**, as
  above. A green build is not evidence that reflection data was regenerated.
- **When a file is skipped, delete its `.obj`** from
  `Intermediate\Build\Win64\x64\UnrealEditor\Development\<Module>\` and build again. Deleting
  `Intermediate\Build\Win64\x64\smoresEditor\Development\Makefile.bin` is the bigger hammer, needed
  when UBT has not noticed *new* files in an existing folder — the same skew makes the cached
  makefile look newer than the directory it should be rescanning.

Jim manages the clock himself and has asked that it be left alone. The point of this section is
only that an agent should recognise the symptom rather than spend a build cycle misdiagnosing it.

## When to Add a Test

The standing rule for any session — human or agent — finishing a piece of work.

### Add one when the code has one of these shapes

1. **A function that returns a count, or a bool that means "did it work."** Assert the count,
   and assert the *partial* case specifically. This project has shipped the same bug in this
   shape three times (`AddItem`, then `MoveItem`, then the purchase built on it) — a `true`
   return that meant "some of it landed." `Docs/roadmaps/inventory-roadmap.md` puts it as a standing warning:
   *assume any other "did it work?" bool in this system is hiding a quantity until checked.* A
   test is how you check.
2. **An all-or-nothing operation.** Assert the failure path **mutates nothing** — not merely
   that it returned false. The equip swap with no room for the displaced item, a purchase the
   wallet can't cover: both are correct only if the world is byte-identical afterwards.
3. **A state machine transition**, especially one with a timer behind it. The
   `Docs/roadmaps/inventory-roadmap.md` Slice 7 trap — a
   `Kill()` on an already-Downed unit leaving a recovery timer in flight, so the corpse stands
   back up seconds later — is exactly this shape, and is silent, delayed, and reproducible only
   by waiting.
4. **Arithmetic on money, weight, quantity, or grid cells.** Cheap to assert, and the category
   where a wrong answer is invisible rather than crashy.
5. **A rule a comment calls deliberate.** `CanStackWith` ignoring `Condition`, `CanEquipItem`
   gating on slot type and nothing else, a weight capacity of 0 meaning unlimited. Asserting
   these means a future session that wants to change the rule has to *delete a test saying not
   to*, which is the point at which it re-reads the reasoning.
6. **A bug you just fixed.** Write the failing case first, watch it fail, then fix it. A
   regression test written after the fix has never been shown to catch anything.

### Don't add one for

- Anything needing a skeletal mesh, skeleton, anim instance or montage — `UCombatComponent`'s
  attack swing, every animation-driven behavior.
- Widget layout, drag payloads, focus, window stacking.
- Input bindings and asset wiring — `input-and-keybinds.md` and the `mcp-workflow` skill's
  verification habits own those.
- Camera behavior, AI, EQS results.
- Anything where deciding pass/fail means looking at the screen.

When in doubt: if you would verify it by reading two numbers, test it. If you would verify it by
playing, don't.

### The workflow

1. Write the test **in the same commit as the code it covers**. A test in a follow-up commit is
   a test that doesn't get written.
2. Cold build.
3. Run headless. Confirm the count went up.
4. Add a line to the "What Is Covered" table above if the work opened a new subject.
5. If the work made something *harder* to test — a rule that moved onto a class needing a
   controller, say — say so in `Docs/roadmaps/testing-roadmap.md` rather than reshaping the code to suit a
   test. That is a design signal worth its own session.

## Writing a Test

### Where the file goes

`Source/<Module>/Tests/<Subject>Test.cpp`, in the same module as the code under test. No
`Build.cs` change and no `.uproject` change is needed — `Core` and `Engine` are already
dependencies of every module, and UnrealBuildTool picks up new files under a module folder
automatically.

Wrap the whole file in `#if WITH_DEV_AUTOMATION_TESTS` / `#endif`. That macro is `0` in Shipping
and Test targets, so nothing written this way reaches a packaged build.

### The template

```cpp
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InventoryComponent.h"
#include "Tests/SmoresItemTestFactory.h"
#include "Tests/SmoresTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSmoresInventoryStackCapTest,
    "Smores.Items.Inventory.StackCapClampsMerge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryStackCapTest::RunTest(const FString& Parameters)
{
    FSmoresTestWorld TestWorld;

    UInventoryComponent* Inventory = MakeTestInventory(TestWorld, 4, 4);

    if (!TestNotNull(TEXT("Inventory created"), Inventory))
    {
        return true;
    }

    UItemDefinition* Definition = MakeTestItemDefinition(TestWorld, FIntPoint(1, 1), /*MaxStack*/ 5);

    TestTrue(TEXT("The stack was placed"), Inventory->AddItemAt(MakeTestItem(Definition, 99), FIntPoint(0, 0), false));
    TestEqual(TEXT("...clamped to the effective cap"), Inventory->GetEntries()[0].Item.Quantity, 5);

    TestWorld.ForwardErrors(this);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

Note the include order: `Misc/AutomationTest.h` sits **above** the `#if`, everything else below
it. The harness headers are themselves compiled out in Shipping, so including them above the
guard would break a packaged build.

### What the harness gives you

Three support files, written in Slice 1. None of them needed a `Build.cs` or `.uproject` change.

| File | Holds |
|---|---|
| `Source/SmoresCore/Tests/SmoresTestWorld.h` | `FSmoresTestWorld` — the throwaway world, `SpawnOwner()`, `SpawnComponent<T>()`, `AddComponent<T>()`, `NewKeptObject<T>()`, `BeginPlay()`, `Tick()`, `TickFor()`, `ForwardErrors()` |
| `Source/SmoresCore/Tests/SmoresTestDelegateListener.h` | `USmoresTestDelegateListener` — counts broadcasts and records the payload |
| `Source/SmoresItems/Tests/SmoresItemTestFactory.h` | `MakeTestItemDefinition`, `MakeTestItem`, `MakeTestInventory`, `FInventorySnapshot`, and the small print helpers |

**The listener has three handlers, one per delegate shape.** Bind `OnChanged` to a delegate with
no parameters (`OnInventoryChanged`, `OnEquipmentChanged`, `OnDowned`, `OnRecovered`, `OnDied`),
`OnIntChanged` to one carrying an `int32` (`OnGoldChanged` — the value lands in `LastInt`), and
`OnActorChanged` to one carrying an actor (`OnDamaged` — the actor lands in `LastActor`). All
three share `CallCount`, and `Reset()` clears the count and both payloads. A delegate of any other
shape needs a fourth handler added there.

`LastActor` is a `TWeakObjectPtr` and deliberately **not** a `UPROPERTY`. A listener is kept alive
for the whole test, so holding the recorded actor strongly would keep alive the world it was
spawned into, and the teardown then fails with *"Previously active world not cleaned up by garbage
collection"* — a failure that points at the world rather than at the listener that caused it.
Compare with `LastActor.Get() == Expected`.

`FSmoresTestWorld` is an RAII type: construct it on the stack, and the destructor tears the world
down and forces a collect. It is also an `FGCObject`, which is why `NewKeptObject<T>()` exists —
a `UItemDefinition` built in a test has no owner keeping it alive, and a collect part-way through
would pull it out from under the assertions.

**`FInventorySnapshot` is how "it mutated nothing" gets asserted.** Take one before the call,
compare after. It covers grid size, entry order, ids, anchors, rotation, quantity, stolen flag and
condition, and prints itself into the failure message. Asserting only that a call returned `false`
would pass even if the refusal had destroyed the item on the way out.

**Three things live in the header rather than an anonymous namespace per file, deliberately.** UE's
unity builds concatenate several `.cpp` files into one translation unit, so two test files each
defining `MakeTestInventory` in an anonymous namespace would collide the day adaptive unity decides
to put them together. Shared test helpers go in the header.

**`USmoresTestDelegateListener` is not wrapped in `WITH_DEV_AUTOMATION_TESTS`**, unlike everything
else. A dynamic delegate can only bind to a `UFUNCTION`, a `UFUNCTION` only exists on a `UCLASS`,
and UHT parses every header regardless of preprocessor conditions it does not know about. Guarding
it would generate reflection code for a class the compiler had been told to skip. It is an empty
`UObject` with one counter; the cost of it existing in a packaged build is a class registration.

### Conventions

- **Name it `Smores.<Module>.<Subject>.<Case>`.** The dots render as a tree in the Automation
  window, and the leading `Smores.` is the filter handle that separates this project's tests from
  the engine's several thousand.
- **`ProductFilter`, always.** Exactly one filter flag is mandatory — the macro carries a
  `static_assert` that rejects zero or two, and its error text is not obvious.
- **`EAutomationTestFlags_ApplicationContextMask` has an underscore.** `EAutomationTestFlags`
  became an `enum class` in UE 5.5, so the combined masks moved out of it and became free
  constants. Every pre-5.5 tutorial writes `EAutomationTestFlags::ApplicationContextMask`, which
  will not compile against 5.8.
- **`return true` means "the test ran," not "it passed."** Pass/fail comes from the `TestTrue` /
  `TestEqual` / `AddError` calls inside it. Returning false marks the test as unable to run at
  all.

### Components need an owner with authority

Nearly every mutator worth testing here is authority-gated — `UInventoryComponent`,
`UEquipmentComponent`, `UWalletComponent` and `UHealthComponent` all no-op silently
off-authority, by design and per `multiplayer-discipline.md`.

So a component under test must hang off an `AActor` spawned into a real (if throwaway) world. An
actor spawned into a world with no net driver holds `ROLE_Authority`, so `HasOwnerAuthority()` is
true and the mutator runs its real path.

**Do not create the component with `NewObject<T>(GetTransientPackage())` and no owner.** With no
owner, `HasOwnerAuthority()` is false, every mutator returns having done nothing, and a test
written as "call `RemoveEntry`, assert the grid is unchanged" **passes for entirely the wrong
reason**. An entire suite can be green while testing nothing at all. This is the single easiest
way to waste a session's work here.

`FSmoresTestWorld::SpawnComponent<T>()` does the whole thing in one line, and
`Smores.Core.TestWorld.SpawnedActorHasAuthority` asserts the assumption directly — so if this ever
stops being true it breaks in one obvious place rather than silently everywhere.

**The one sanctioned exception** is `Smores.Items.Inventory.SortWithoutAuthorityIsSilent`, which
creates an ownerless component *on purpose* to exercise the gate's refusing branch, and says so in
a comment. It is not a client — a real non-authority test needs a net driver and waits on
multiplayer — but it does prove the gate refuses rather than merely that it exists.

Two `FSmoresTestWorld` calls are opt-in and cost time, so use them only when needed: `BeginPlay()`
(for anything reading a `StartingGold` / `StartingStock` style property) and `Tick(DeltaSeconds)` /
`TickFor(Seconds)` (for anything on a timer). When ticking for a timer, **lower the duration
property in the test first** — ticking through `UHealthComponent`'s default 15-second recovery at
100fps is 1500 iterations for no benefit.

### A subsystem needs the outer its class demands, and getting it wrong fails green

Found in the HUD roadmap's Slice 3, and the same shape as the authority trap above: the test
constructs its subject wrongly, and most of the suite passes anyway.

`USmoresActivityLog` is a `ULocalPlayerSubsystem`. Nothing its ring buffer does touches a local
player, so a test builds one directly rather than standing up a viewport and a game instance — but
the outer still has to satisfy the class's `ClassWithin`, and UE enforces that at construction.
The chain is **two links long**: `ULocalPlayerSubsystem` demands a `ULocalPlayer`, and
`ULocalPlayer` demands a `UEngine`. So:

```cpp
ULocalPlayer* OuterPlayer = NewObject<ULocalPlayer>(GEngine);
USmoresActivityLog* Log = NewObject<USmoresActivityLog>(OuterPlayer);
```

Neither object is initialised and neither is ever asked anything; they exist to be the right shape.

**The reason this is worth its own heading is how it reports.** A wrong outer raises a *handled
ensure* ("Object None of class ... was created in invalid Outer"), and a handled ensure fires **once
per call site per session**. Six tests shared one helper, so exactly one of them went red and the
other five passed on the identical mistake. Two habits follow:

- **A green run is not evidence the outer is right.** If one test in a group fails on an ensure
  raised from shared setup, assume the whole group is affected rather than the one that reported.
- **Check `ClassWithin` before hand-constructing any `UObject` a test doesn't spawn into a world.**
  `FSmoresTestWorld::SpawnComponent<T>()` exists so components never hit this; a subsystem, a
  `ULocalPlayer` or anything else with a declared `ClassWithin` is on its own.

Both objects go through `KeepAlive` for the usual reason — neither has an owner, and a collect
part-way through would pull them out from under the assertions.

### Timers need `BeginPlay()` before they will tick at all

Found the hard way in Slice 2, and silent enough to be worth its own heading.

`FTimerManager::Tick` returns immediately if it has already ticked on the current frame, and the
engine's test wrapper only advances the frame counter **once play has begun**. So ticking a world
that never called `BeginPlay()` advances every timer exactly once and then does nothing at all,
however long you tick for.

The damage is that it fails in the passing direction. `KillCancelsPendingRecovery` asserts a
killed unit is *still dead* after waiting out its recovery — with no `BeginPlay()` that test goes
green without ever having waited, because the recovery it was watching for could never have fired.
The test next to it, which asserts the timer *does* fire, is what exposed it.

Two habits, one of them now enforced:

- **Call `BeginPlay()` before ticking anything on a timer.** `TickFor()` returns `false` outright
  if play hasn't begun rather than reporting a wait that didn't happen, so **assert on its return
  value** — `TestTrue(TEXT("The world ticked"), TestWorld.TickFor(0.3f))`.
- **Write the pair.** A "the timer did not fire" test is only meaningful next to one proving the
  timer fires at all under the same setup.

### Expected log output

A test that deliberately drives code down a path that warns should say so:

```cpp
AddExpectedMessagePlain(TEXT("has no room for"), ELogVerbosity::Warning,
    EAutomationExpectedMessageFlags::Contains, /*Occurrences*/ 0);
```

`Occurrences = 0` means "one or more", so this is **stronger than suppression** — it asserts the
code actually warned, and the run stays green rather than yellow. Slice 1 uses it for the partial
add, the abandoned repack, and the entries a grid resize drops.

### Item definitions are built in memory

Tests construct their own `UItemDefinition` with `NewObject<UItemDefinition>()` and direct field
assignment. **Never load one out of `Content/`** — a test that loads a real asset is testing that
asset too, so a designer retuning a sword's weight breaks an unrelated inventory test and the
failure points at the wrong place. It also keeps the test's inputs visible next to the assertion
instead of inside a `.uasset`.

### The content smoke tests are the one exception, and run in editor context

`Smores.Content.*` deliberately does the opposite of everything above: it sweeps the real assets
under `Content/`. It is not asserting what any one asset contains — it asserts every
`UItemDefinition` is *well formed* (an `ItemId`, a `DisplayName`, a footprint inside the 1–16
clamps, a stack cap of at least 1) and that no two share an `ItemId`, plus that both maps still
load as packages. Those are rules about the content tree, not about an asset's tuning, so a
designer retuning a sword can't break them.

Four things about this group specifically:

- **The rule is proved in memory; the sweep is pointed at real content.**
  `MalformedDefinitionIsRejected` runs the same validator against a bare `NewObject<UItemDefinition>()`.
  Deliberately breaking a real asset to watch the sweep fail is hand work in the editor that
  proves nothing extra.
- **A failure names the offending asset**, by soft object path. A sweep that only says "something
  is malformed" costs whoever reads it a manual pass over the folder.
- **They need `EAutomationTestFlags::EditorContext`**, not `EAutomationTestFlags_ApplicationContextMask`,
  and they will not run in a headless *game* target. The `UnrealEditor-Cmd` command above covers
  them; a packaged-build runner would not.
- **`SmoresItems` depends on `AssetRegistry`** (a private dependency) solely for this file. It is
  the only `Build.cs` change any test has needed.

A sweep that finds no assets is green having looked at nothing, so both sweeps assert they found at
least one before checking anything — the same "check the number, not the colour" rule one level down.

## Known Gaps

- **`UCombatComponent` has no tests and belongs to no slice.** Most of it isn't reachable (see
  below), but three branches are: an attack on a target beyond `AttackRange`, an attack after
  `NotifyOwnerDowned`, and `ClearCurrentAttackTarget`. `AttackRange` is `protected`, so reaching it
  wants a test-only subclass in the same module rather than a new accessor. It is catalogued in
  `Docs/roadmaps/testing-roadmap.md` but no slice claimed it.
- **The off-authority path is untestable** until multiplayer is wired up. Asserting "this mutator
  no-ops on a client" needs an actor whose role is not `ROLE_Authority`, which needs a net driver.
  The gate is asserted positively instead — the mutator runs when it should.
- **`UCombatComponent`'s attack swing is untestable** — `PerformAttack` plays a montage on the
  owner's anim instance, which needs a skeletal mesh, a skeleton and authored montage assets. Only
  the out-of-range branch, which never reaches a montage, is reachable from a test.
- **`AStrategyPlayerController` is expensive to test.** Selection, input, window management and
  the trade transaction all meet there and none of it separates cleanly. That is the price of the
  controller being where variant-specific glue lives; `Docs/roadmaps/testing-roadmap.md` Slice 3 takes one
  scoped run at the transaction and is allowed to abandon it.
  - **`BuildTargetInfo` is the worked example of the way out**, and worth copying. It was written
    as a `public static` taking the selection as a parameter rather than reading `ControlledUnits`
    off the instance, so a test calls it with a hand-built array and no controller at all. Where a
    rule genuinely belongs on the controller, making it a static that takes its inputs explicitly
    costs nothing and is the difference between testable and not.
- **Abstract gameplay actors need a concrete stand-in to be spawned at all.**
  `AStrategyUnit`, `AStrategyPlayerUnit` and `AStrategyContainer` are all `UCLASS(abstract)` per
  the project's standing rule, and an abstract class cannot be spawned. `Source/smores/Tests/
  SmoresStrategyTestActors.h` holds one concrete subclass of each, unguarded by
  `WITH_DEV_AUTOMATION_TESTS` for the same reason `USmoresTestDelegateListener` is — UHT parses
  every header regardless. Each disables `AutoPossessAI`; a unit test has no navmesh and wants no
  AI controller. Add to that file rather than starting a second one.
- **Driving real AI entry points from a test costs more than it's worth.** `SetAggressive(true)`
  looked like the honest way to make an NPC hostile and is a trap in both directions: with no
  player pawn in the world it immediately stands the unit back down (`TryEngageNearestPlayerPawn`
  finds nothing to fight), and *with* one it starts a real fight — an attack montage needing a
  skeletal mesh, or an EQS move needing a navmesh and a query asset. `ATestStrategyNPC::
  MakeHostileForTest` sets the one piece of state the code under test reads. The general shape:
  when a setter has behaviour attached, a test of something downstream of its *state* should set
  the state, and say in a comment why it isn't using the setter.
- **No CI.** Every run is manual. `Docs/roadmaps/testing-roadmap.md` Slice 3 optionally wraps the command in a script;
  there
  is no build server and none is planned.
