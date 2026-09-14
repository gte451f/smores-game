# Automated Testing

## Purpose

This topic documents smores' **built** automated test suite: how to run it, what it covers, and
the standing rule for when a piece of work should add to it. The forward-looking plan — what
still needs building, in what order, and the decisions behind it — lives in
`testing-roadmap.md`.

> **Status: the harness does not exist yet.** No test file has been written and the commands
> below will report zero tests until **Slice 1 of `testing-roadmap.md`** ships. The
> conventions, run commands and protocol are recorded here in advance deliberately, because
> they are Slice 1's deliverable as much as the code is — but nothing in this file has been
> executed against this project yet, and the first session to run it should expect to correct
> details here.

`Automation_smores.slnx` is **not** a test setup. It is a solution file that pulls in Epic's own
`UnrealBuildTool` and `EpicGames.*` C# projects, several of which are named `*.Tests`. None of
them test smores.

## What Is Covered

| System | Module | Covered | Slice |
|---|---|---|---|
| Inventory grid geometry and placement | `SmoresItems` | — | 1 |
| Inventory stacking, counted returns, moves | `SmoresItems` | — | 2 |
| Wallet and pricing | `SmoresEconomy` | — | 3 |
| Health state machine (Alive/Downed/Dead) | `SmoresCombat` | — | 4 |
| Equipment slots and the all-or-nothing swap | `SmoresItems` | — | 5 |
| Item definition assets and map loading | `SmoresItems` | — | 6 |
| Trade transaction ordering | `smores` | — | 7 |

Update this table as slices ship; it is the quick answer to "is this already covered?"

## What Is Deliberately Not Covered

Automated tests here catch **silent numerical and state-machine regressions** — a function that
returns `true` having done only part of the job, a state that transitions when it shouldn't.
They do not, and will not, cover anything whose pass/fail is a human judgment.

**These stay in PIE, permanently:** drag-and-drop feel, the rotate-while-dragging gesture,
window layout and stacking order, camera pan/zoom tuning, animation and montage timing, EQS
destination quality, NPC behavior, and whether any number *feels* right. See
`testing-roadmap.md`'s "Explicitly Out of Scope" for the full list and the reasoning.

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
| `Automation RunTests Project.Blueprints` | Engine's Blueprint-compile smoke test — **run this after any C++ rename**, it is the cheapest way to catch a Blueprint broken by a moved or renamed property |

### Reading the result — the one rule that matters

**Check the number of tests run, not just the colour.** A test that fails to register does not
fail — it silently does not appear, and the run comes back green having executed nothing. After
adding tests, confirm the reported count went up by the number you added. `Automation List` is
the direct check when it doesn't.

### After adding or changing a test file

A new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` is a new C++ class, and a new `.cpp` is a new file in
the module. **Both require a cold Visual Studio build** — close the editor, build, reopen. Live
Coding is unreliable for new types generally (see CLAUDE.md) and does not reliably pick up new
files at all.

## When to Add a Test

The standing rule for any session — human or agent — finishing a piece of work.

### Add one when the code has one of these shapes

1. **A function that returns a count, or a bool that means "did it work."** Assert the count,
   and assert the *partial* case specifically. This project has shipped the same bug in this
   shape three times (`AddItem`, then `MoveItem`, then the purchase built on it) — a `true`
   return that meant "some of it landed." `inventory-roadmap.md` puts it as a standing warning:
   *assume any other "did it work?" bool in this system is hiding a quantity until checked.* A
   test is how you check.
2. **An all-or-nothing operation.** Assert the failure path **mutates nothing** — not merely
   that it returned false. The equip swap with no room for the displaced item, a purchase the
   wallet can't cover: both are correct only if the world is byte-identical afterwards.
3. **A state machine transition**, especially one with a timer behind it. The Slice 7 trap — a
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
   controller, say — say so in `testing-roadmap.md` rather than reshaping the code to suit a
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
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSmoresInventoryStackCapTest,
    "Smores.Items.Inventory.StackCapClampsMerge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSmoresInventoryStackCapTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper WorldWrapper;
    WorldWrapper.CreateTestWorld(EWorldType::Game);
    UWorld* World = WorldWrapper.GetTestWorld();

    // ... spawn an owner actor, attach the component, assert ...

    WorldWrapper.ForwardErrorMessages(this);
    WorldWrapper.DestroyTestWorld(true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

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
off-authority, by design and per CLAUDE.md's multiplayer discipline.

So a component under test must hang off an `AActor` spawned into a real (if throwaway) world. An
actor spawned into a world with no net driver holds `ROLE_Authority`, so `HasOwnerAuthority()` is
true and the mutator runs its real path.

**Do not create the component with `NewObject<T>(GetTransientPackage())` and no owner.** With no
owner, `HasOwnerAuthority()` is false, every mutator returns having done nothing, and a test
written as "call `RemoveEntry`, assert the grid is unchanged" **passes for entirely the wrong
reason**. An entire suite can be green while testing nothing at all. This is the single easiest
way to waste a session's work here.

Two `FTestWorldWrapper` calls are opt-in and cost time, so use them only when needed:
`BeginPlayInTestWorld()` (for anything reading a `StartingGold` / `StartingStock` style property)
and `TickTestWorld(DeltaSeconds)` (for anything on a timer). When ticking for a timer, **lower
the duration property in the test first** — ticking through `UHealthComponent`'s default
15-second recovery at 100fps is 1500 iterations for no benefit.

### Item definitions are built in memory

Tests construct their own `UItemDefinition` with `NewObject<UItemDefinition>()` and direct field
assignment. **Never load one out of `Content/`** — a test that loads a real asset is testing that
asset too, so a designer retuning a sword's weight breaks an unrelated inventory test and the
failure points at the wrong place. It also keeps the test's inputs visible next to the assertion
instead of inside a `.uasset`.

## Known Gaps

- **Everything.** No test file exists yet; `testing-roadmap.md` Slice 1 is the first.
- **The off-authority path is untestable** until multiplayer is wired up. Asserting "this mutator
  no-ops on a client" needs an actor whose role is not `ROLE_Authority`, which needs a net driver.
  The gate is asserted positively instead — the mutator runs when it should.
- **`UCombatComponent`'s attack swing is untestable** — `PerformAttack` plays a montage on the
  owner's anim instance, which needs a skeletal mesh, a skeleton and authored montage assets. Only
  the out-of-range branch, which never reaches a montage, is reachable from a test.
- **`AStrategyPlayerController` is expensive to test.** Selection, input, window management and
  the trade transaction all meet there and none of it separates cleanly. That is the price of the
  controller being where variant-specific glue lives; `testing-roadmap.md` Slice 7 takes one
  scoped run at the transaction and is allowed to abandon it.
- **No CI.** Every run is manual. `testing-roadmap.md` Slice 8 wraps the command in a script; there
  is no build server and none is planned.
