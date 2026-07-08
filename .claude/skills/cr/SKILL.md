---
name: cr
description: Review every uncommitted file in the Wankel repo (staged, unstaged, and untracked) against this project's conventions, its stated goals, and its existing engine API — flags style drift, duplicated logic, reinvented utilities, and gameplay/engine code that bypasses an abstraction the engine already provides. Use for "/cr", "review my changes", "does this fit our conventions", "did I duplicate anything", "should this use the engine API". This is NOT a correctness/bug-hunt pass (use /code-review for that) — it's a fit-and-reuse pass.
---

# /cr — Convention, Goal & Reuse Review

Reviews the working tree's uncommitted changes for whether they belong in
this codebase the way it already is, not whether they're bug-free. Run
`/code-review` separately (or first) for correctness issues — don't
duplicate that work here.

## 1. Scope the diff

Gather everything uncommitted:
- `git status --short` to see staged, unstaged, and untracked files.
- `git diff` (unstaged) and `git diff --cached` (staged) for changed files.
- Read untracked files directly (skip anything gitignored — e.g. `*.pdf`,
  `build/`).

If there's nothing uncommitted, say so and stop — don't review the whole
repo, only what's about to be committed.

## 2. Conventions to check against

This repo has no separate style-guide doc — the convention *is* the
existing code. For each changed file:

- **Namespacing**: engine types belong in `namespace Wankel { }`. Flag new
  code that lives in the global namespace (the codebase already has a
  couple of known-bad precedents — `Time`, `PLYLoader`/`PLYMeshData` — don't
  treat those as license to add more).
- **Naming**: `PascalCase` for types, classes, structs, methods, and public
  members; `m_PascalCase` for private members (`m_Registry`, `m_Grid`,
  `m_CellSize`); `s_PascalCase` for static members. Local variables are
  `camelCase`.
- **Logging**: use `WK_CORE_*` / `WK_CLIENT_*` / `WK_SERVER_*` macros from
  `Log.h` (`WK_CORE_ERROR`, `WK_CORE_WARNING`, `WK_CORE_INFO`,
  `WK_CORE_TRACE`). Never `std::cout`/`std::cerr` in engine code under
  `src/Wankel/` — that was a real bug fixed in `Shader.cpp` already, don't
  reintroduce it. `Sandbox/` demo code logging to `std::cout` is
  established local precedent (e.g. `PLYLoader.h`) and lower priority to
  flag, but still worth a note if it's new code rather than existing style.
- **Resource ownership**: GL-handle-owning classes must not be copyable
  (see `Buffer.h`/`IndexBuffer.h`/`VertexArray.h`/`Shader.h` for the
  pattern: `= delete` on copy ctor/assignment). New classes wrapping a raw
  GL/OS handle should follow the same pattern. Prefer `Scope<T>`/`Ref<T>`
  (`CreateScope`/`CreateRef`, in `Base.h`) for new owning pointers in
  `Core`/`ECS`; the `Renderer` classes use bare `std::unique_ptr` instead —
  match whichever convention the immediate surrounding subsystem already
  uses rather than mixing both in one file.
- **File-local helpers in a `.cpp`**: wrap them in an anonymous namespace
  nested inside `namespace Wankel { }` (see `CollisionDispatcher.cpp` for
  the pattern) rather than giving them external linkage or leaving them
  in the global namespace. This matters here specifically — several
  helper names (e.g. `Idx`, `ToAABB`) would collide across translation
  units without it.
- **Include order** in `.cpp` files: `wkpch.h` first, then the file's own
  header, then other engine headers, then third-party/system headers.
- **Indentation**: match whichever the file *already* uses if editing an
  existing file (the codebase is inconsistent repo-wide — tabs in most of
  `Core`/`ECS`, spaces in newer `Physics` files — don't "fix" this in an
  unrelated change, just don't make a single file inconsistent with
  itself).

## 3. Goals to check against

Read `README.md` (FEATURES / TODO / BUGS sections) and, if present, the
review PDFs under `Documents/*.pdf` for the current architectural
direction and known gaps. Flag changes that:
- Quietly work around a listed TODO/BUG instead of addressing it or
  explicitly deferring it (e.g. adding a second hand-rolled mesh format
  instead of noting the existing "asset pipeline" TODO).
- Add something the roadmap already flags as needing a different shape
  (e.g. hardcoding another asset path string when "asset manager" is a
  known, named gap — that's fine for a quick Sandbox demo tweak, worth a
  note if it's meant to be a real engine feature).
- Move the engine further from, rather than toward, unifying something
  the codebase has already been burned by fragmenting (see below).

## 4. Duplication — this codebase's specific recurring failure mode

Both prior reviews of this repo found the *same* root cause repeatedly:
**multiple competing representations of one concept, left behind by a
partial refactor** — duplicate `MarchingCubes` classes in two files,
three different collider-type representations before the `ColliderShape`
unification, byte-identical Linux/Windows platform files. Treat this as
the single most important thing to catch:

- Before accepting a new function/class/struct, grep the engine
  (`src/Wankel/`, `Sandbox/src/`) for something that already does this or
  something close to it. A near-duplicate with a slightly different API
  is worse than no duplicate at all — it's exactly the pattern that's
  bitten this project before.
- If the change modifies one of two/three near-duplicate implementations
  without touching the others, flag it explicitly — that's how the
  fragmentation happened the first time.

## 5. Engine API reuse & extension

Check whether new code reinvents something the engine already exposes,
instead of using or extending it:

- Math constants/helpers → `Wankel::Math::` (`Clamp`, `Lerp`, `PI`, `TAU`,
  `Degrees`, `Radians`, `SmoothStep`) in `Math.h`, not local
  redeclarations (a real bug already fixed once in
  `SecondOrderDynamics.cpp`).
- New gameplay data → an ECS component in `ECS/Components/*.h`, processed
  by a system in `ECS/Systems/*`, following the existing
  `Scene::Registry().view<...>()` pattern — not inline state bolted onto
  `SandboxLayer` unless it's genuinely one-off demo scaffolding.
- New collider/narrow-phase behavior → extend the `ColliderType`/
  `ColliderShape`/`NarrowPhaseTable` dispatch in
  `Physics/Collision/CollisionDispatcher.cpp`, not a parallel one-off
  `if`-chain.
- New event/callback wiring → the `Event`/`EventDispatcher` pattern
  (`EVENT_CLASS_TYPE`/`EVENT_CLASS_CATEGORY`), not a hand-rolled
  `std::function` member unless there's a concrete reason the existing
  event system doesn't fit.
- New GPU resources → wrap them the way `Buffer`/`IndexBuffer`/
  `VertexArray`/`Shader` already do (RAII, no copy) rather than raw
  `glGen*`/`glDelete*` calls scattered in calling code.

Where a change genuinely needs a capability the engine doesn't have yet,
prefer *extending* the nearest existing abstraction over adding a
parallel one — note in the finding which existing system it should have
extended and why.

## 6. Report findings

Use the `ReportFindings` tool. One finding per issue, ranked most-important
first (duplication and API-bypass issues before pure style nits). For
`category`, use one of: `convention`, `duplication`, `api-reuse`,
`goal-alignment`. If nothing survived review, call it with an empty array
— don't manufacture nitpicks to have something to say.
