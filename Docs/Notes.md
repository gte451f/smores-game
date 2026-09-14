# Notes

Personal reference for Unreal Engine and C++ technical knowledge. Not project logic — see
`.claude/skills/game-systems/` for how smores' systems actually work.

---

## Naming conventions

Unreal uses **two separate naming systems**, and they're easy to confuse because both are
prefixes.

**The underscore is the tell:**

- `UItemDefinition` — one letter, no underscore → a **C++ class**, lives in `Source/`
- `DA_Item_Apple` — letters then underscore → an **asset file**, lives in `Content/`

|  | C++ type prefixes | Asset name prefixes |
|---|---|---|
| Examples | `U` `A` `F` `I` `E` `T` `b` | `DA_` `BP_` `WBP_` `M_` `MI_` `IA_` `IMC_` `LVL_` |
| Applies to | Classes written in C++ and compiled | Files in the Content Browser (`.uasset`) |
| Shape | One letter, glued on: `FInventoryItem` | Letters + underscore: `DA_Item_Apple` |
| Enforced? | **Yes** — the build tool rejects a wrong prefix on an engine class | **No** — pure human convention; the engine doesn't care |
| Source | Epic's official C++ coding standard | Community convention |

### C++ type prefixes

| Prefix | Means | Examples |
|---|---|---|
| `U` | Engine-managed object, garbage collected — you hold *pointers* to it | `UInventoryComponent`, `UStaticMeshComponent` |
| `A` | An object that can be placed in the world (a subtype of `U`) | `AWorldItem`, `AActor`, `APlayerController` |
| `F` | Plain data — you *copy* it around | `FInventoryItem`, `FVector`, `FTransform` |
| `I` | Interface — a contract another class promises to implement | `IInventoryMoveHost` |
| `E` | A list of named options | `EEquipSlot`, `ESpawnActorCollisionHandlingMethod` |
| `T` | A container or wrapper that holds some other type | `TArray`, `TObjectPtr`, `TSubclassOf` |
| `b` | A true/false variable | `bRotated`, `bReplicates` |

**`U` vs `F` is the distinction that matters day to day:**

- A `U` thing is **owned by the engine**. It exists once, the garbage collector tracks it,
  and you pass around pointers to it. Two places referring to the same `U` object see the
  same object — change it in one place and the other sees the change.
- An `F` thing is **plain data you copy**. Assigning it makes a second, independent copy.
  Cheap to create and throw away, nothing tracks it.

Pick wrong and you get one of two bugs: copying something that was meant to be shared, or
paying garbage-collection overhead on something that should have been a cheap value.

### Asset name prefixes

| Prefix | Means | Example |
|---|---|---|
| `DA_` | Data Asset — a file of pure authored data, no logic | `DA_Item_Apple` |
| `BP_` | Blueprint class | `BP_WorldItem` |
| `WBP_` | Widget Blueprint (a UI panel) | `WBP_Inventory` |
| `M_` | Material | `M_ContainerColor` |
| `MI_` | Material Instance (a tweaked copy of a material) | `MI_WorldItem_Black` |
| `IA_` | Input Action (one bindable command) | `IA_Strategy_Inventory` |
| `IMC_` | Input Mapping Context (a set of key→action bindings) | `IMC_Strategy_Mouse` |
| `LVL_` | Level / map | `LVL_Strategy` |

Others you'll meet in Unreal generally: `SM_` static mesh, `SK_` skeletal mesh, `T_`
texture, `ABP_` animation blueprint, `NS_` Niagara system, `CR_` control rig.

### How the two systems relate

They describe different layers of the same thing. An asset's prefix roughly announces which
C++ class family it belongs to:

```
AWorldItem      ← C++ class,  Source/SmoresItems/WorldItem.h
BP_WorldItem    ← asset,      Content/..., its class is AWorldItem
```

So `DA_Item_Apple` is an **asset** (underscore prefix) whose **class** is `UItemDefinition`
(letter prefix). Both prefixes are doing their job at their own layer.

### Why "F"?

Nobody outside Epic knows for certain, and Epic's coding standard doesn't say — it just
states the rule. The commonly repeated explanation, attributed to Tim Sweeney from the
original 1998 Unreal codebase, is that **F stood for "Float"**: the earliest such structs
were floating-point math types (`FVector`, `FPlane`, `FMatrix`), and once the convention
existed it generalized to every plain struct.

Plausible folklore, not documented fact. Either way, the `F` implies nothing about floats —
it's a naming habit that outlived its reason.

---

## C++: `.h` vs `.cpp`

C++ splits a class across two files:

- **`.h`** (header) — what the class **has**. Field and function *declarations*: the names,
  types, and signatures, but not the bodies.
- **`.cpp`** (source) — what the class **does**. The function bodies.

**Why the split exists:** when one file needs to use a class from another, it `#include`s
the `.h`. The compiler then knows enough to type-check the calls without having to read or
recompile the implementation. Change a `.cpp` and only that file rebuilds; change a `.h` and
everything that includes it rebuilds too — which is why headers are kept lean.

**Practical rule of thumb:** if you want to know what something *is* or what fields it has,
open the `.h`. If you want to know how something *works*, open the `.cpp`.

In Unreal specifically, every field the editor shows in a details panel is declared in the
`.h`. A data-heavy class can have a nearly empty `.cpp` — `ItemDefinition.cpp` is nine lines
— because there's barely any behavior to implement.

---

## Unreal: two ways to store authored data

When you have a table of designer-authored values (item stats, enemy types, loot tables),
Unreal gives you two options:

| | Data Asset | Data Table |
|---|---|---|
| C++ base class | `UPrimaryDataAsset` / `UDataAsset` | `UDataTable` |
| Shape | **One file per entry** | **One file, one row per entry** |
| Asset prefix | `DA_` | `DT_` |
| Editing | Normal editor details panel, one at a time | Spreadsheet-style grid, or import from CSV |
| References to other assets | Natural — just point at them | Awkward — stored as text paths |
| Modding / patching | Easy to replace one entry | Have to replace the whole table |
| Bulk editing | Tedious — open each file | Easy — it's a spreadsheet |

Neither is "correct" — it's a trade. Many files with rich asset references favour Data
Assets; large flat numeric tables favour Data Tables.

**The general pattern behind both:** shared, immutable, authored-once data lives in an asset,
and the many *live* copies in the game hold only a pointer to it plus whatever actually
varies. That keeps per-copy memory small and means editing the asset changes every copy at
once.

Think of it as a **catalog**. There's one page per kind of thing, listing everything true of
all of them. Every live copy in the game is a **sticky note** saying *"I'm the thing on that
page, and here's what's different about me."* When the game needs a detail, it flips to the
page.

This project's items are one instance of it — `DA_Item_Apple` is the page; a carried
`FInventoryItem` is the sticky note, holding only quantity, condition and stolen-or-not.
Same pattern shows up all over Unreal: meshes, materials, particle systems and animations
are all shared assets with lightweight per-instance state pointing at them.

Note how the prefixes line up with the roles: the catalog page is a `U` (exists once,
pointed at), the sticky note is an `F` (cheap, copied). Reverse them and the scheme stops
working.
