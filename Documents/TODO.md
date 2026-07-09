# Engine TODO / Recommendations

Source: `Documents/WankelEngineReview2_GameReadiness.pdf` (2026-07-06, commit `a5f736e`),
follow-up to `Documents/WankelEngineCodeReview.pdf` (same day, commit `3be338d`).
This file is the actionable, checkable distillation of both — update it in place as items
land instead of re-reading the PDFs or re-scanning the engine.

Also cross-reference `README.md`'s own `TODO:`/`BUGS:` sections — some items overlap
(sphere-vs-sphere, collider-without-Rigidbody are already listed there).

## Phase 1 — Correctness fixes (do before building anything on top)

- [x] Insert `SphereCollider` entities into the broad-phase grid (or build a second grid
      keyed on `SphereCollider`) — sphere-vs-sphere never collides today because the grid
      is only built from `Transform, AABBCollider` (`PhysicsSystem.cpp:67`). HIGH
      **Fixed:** `PhysicsSystem::Update` now also builds the grid from
      `Transform, SphereCollider` (`PhysicsSystem.cpp` build-grid section). Verified with a
      standalone test exercising `PhysicsSystem::Update` directly — two overlapping
      sphere-only dynamic entities are now pushed apart.
- [x] Guard `PhysicsSystem`'s pair resolution against colliders with no `Rigidbody`
      (treat as implicitly static) instead of the unconditional `registry.get<Rigidbody>(b)`
      that crashes/UB the first time a dynamic body queries a static collider-only entity
      (`PhysicsSystem.cpp:114`). HIGH
      **Fixed:** now uses `registry.try_get<Rigidbody>(b)` and treats a missing `Rigidbody`
      as implicitly static (no position movement, no velocity to cancel). Verified with a
      standalone test: a collider-only static "wall" entity with no `Rigidbody` no longer
      crashes when a dynamic body overlaps it, and resolution behaves as if it were static
      (dynamic body pushed out, penetrating velocity cancelled, wall itself unmoved).
- [x] Gate `InputSystem::PollControllers()` behind the success flag from `Init()` — it's
      called unconditionally every frame from `Application::Run()` even when `Init()`
      failed (`Application.cpp:47`). MEDIUM
      **Fixed:** `InputSystem` now tracks its own `s_Initialized` flag, set on successful
      `Init()` and cleared on `Shutdown()`; `PollControllers()` (and `Shutdown()`) return
      early when not initialized. Verified live — this sandbox's SDL init actually fails
      every run (no gamepad backend), and Sandbox now runs stably instead of hammering an
      uninitialized SDL subsystem every frame.
- [x] Remove or properly gate the dead ImGui docking/viewport code — it's guarded by
      `ImGuiConfigFlags_DockingEnable`/`ViewportsEnable`, which are Dear ImGui docking-branch
      enum values, not preprocessor macros, and don't exist in the vendored plain-master
      `external/imgui`. Dead code masquerading as a working feature
      (`ImGuiLayer.cpp:30-36,86-98`). HIGH
      **Fixed:** removed the dead `#ifdef`/viewport-render blocks (confirmed the vendored
      `external/imgui` declares neither symbol at all) and left a comment explaining
      docking/viewports need the docking-branch submodule, which wasn't swapped in as part
      of this fix.
- [x] Add a `Window::SetCursorMode()` API — cursor is unconditionally
      `GLFW_CURSOR_DISABLED` at window creation with no way to toggle it
      (`LinuxWindow.cpp`/`WindowsWindow.cpp` `Init()`). Blocks any non-FPS genre. MEDIUM
      **Fixed:** added `CursorMode{Normal, Hidden, Disabled}` + `Window::SetCursorMode()`/
      `GetCursorMode()`, implemented in both `LinuxWindow` and `WindowsWindow`. Also used it
      to replace two spots in `SandboxLayer.cpp` that were reaching past the `Window`
      abstraction straight into `glfwSetInputMode`/`GetNativeWindow()`.
- [x] Clear `MoveInput`/`RollInput`/`LookDeltaX`/`LookDeltaY` when input focus is lost —
      currently frozen at last value while `!gameFocused`, and
      `PlayerControllerSystem::Update` keeps applying the stale input every tick
      (`Sandbox/src/Systems/PlayerInputSystem.cpp:19-23`). MEDIUM
      **Fixed:** `PlayerInputSystem::Update` now zeroes `MoveInput`/`RollInput`/
      `LookDeltaX`/`LookDeltaY`/`Boost` before the `!gameFocused` early-continue instead of
      leaving them at their last value.

Phase 1 is now fully cleared — remaining work is Phase 2/3 below.

## Phase 2 — Actual blockers to "a game" (priority order)

- [ ] **Vertex format + lighting.** Add normals + UVs to `Vertex` (`Mesh.h:13-16` is
      currently just `{Position, Color}`), add a basic Blinn-Phong/simple-PBR shader path,
      add a `Texture` class + material abstraction. Nothing downstream (real look, actual
      art) is possible without this — biggest single structural gap in the renderer.
- [ ] **Real asset pipeline.** Add glTF import (cgltf or tinygltf — much less work than the
      hand-rolled ASCII PLY parser), add `stb_image` for textures, introduce a minimal asset
      registry so meshes/textures/shaders are referenced by handle/ID instead of hardcoded
      relative path strings in `SandboxLayer`'s constructor.
- [ ] **Data persistence (layered, engine/game split).** A full "dump the whole scene"
      system is the wrong shape here — the world is procedurally generated, so there's no
      static level to serialize wholesale. Split into:
      - **Engine** (`src/Wankel/ECS/`): generic per-component JSON (de)serialize primitives
        — `to_json`/`from_json` per component type, dispatched through a registry (same
        spirit as `CollisionDispatcher`'s table-based dispatch for collider types), plus
        `Scene::SerializeEntity`/`DeserializeEntity`. Pure mechanism, reusable by any future
        game project regardless of whether it's procedural or hand-authored. Use
        nlohmann/json, not YAML — save data and generation params are machine-written/read,
        so YAML's human-editing ergonomics (comments, less punctuation) buy nothing, while
        its parser is heavier and has implicit-typing footguns (the "Norway problem"), and
        there's no clean single-header C++ YAML library the way there is for JSON.
      - **Game** (`Sandbox/src/`): world seed + generation config (tiny, one-time — the seed
        itself never "updates" as the world grows; per-tile content is derived via
        `hash(worldSeed, tileX, tileY, tileZ)`, the same approach Minecraft/No Man's Sky use,
        so save-file size doesn't grow with explored area), player state (position as tile
        coordinate + local offset, orientation, health/inventory once those components
        exist), and eventually a sparse `chunkCoord -> [edits]` delta map for persistent
        terrain mutation plus "already resolved" entity state (e.g. don't respawn a killed
        enemy on chunk reload). Builds on the existing "infinite-world chunk-wrap illusion"
        hack in `SandboxLayer.cpp` and `AsteroidGenerator`/`SplineCarver`.
      **Sequencing:** build the engine-level per-component serializer plus a minimal
      seed+player-state save file whenever a "continue" feature is wanted — cheap, can
      happen anytime. Defer the chunk-delta/mutation-state format; it's blocked on the
      Terrain/MarchingCubes pathway actually being implemented (see Phase 3) — designing
      that format now would be guessing at requirements that don't exist yet.
- [ ] **Mass-aware physics.** Wire `Rigidbody::Mass`/`Force` (declared but never read,
      `PhysicsComponents.h:13`, `PhysicsSystem.cpp:127-128`) into position/velocity
      resolution (mass-weighted split) before adding Capsule/Mesh narrow-phase — current
      flat 50/50 split will look wrong the moment two differently-sized dynamic objects touch.
- [ ] **Audio.** miniaudio is a strong pick (single header, permissive license, no dependency
      sprawl) — SDL3 is already linked but only its gamepad subsystem is used.
- [ ] **Minimal UI/text rendering.** A bitmap-font text renderer + basic screen-space quad UI
      is enough for a first HUD — doesn't need to compete with UGUI/UMG to be useful.

## Phase 3 — Scale & polish (once a game is actually in production)

- [ ] Generalize `PlayerControllerSystem`/`Movement` beyond the flight/FPS-hybrid demo
      controller (has its own `// TODO: is this needed` / `// hack, should be controlled
      from sandbox` comments), or explicitly document it as sandbox-only.
- [ ] Finish or delete the Terrain/MarchingCubes pathway — `MarchingCubes::Generate` is
      declared with **no `.cpp` implementing it anywhere** and zero call sites
      (`Terrain/MarchingCubes.h:22`). The whole voxel-terrain chain (`SplineCarver.h`,
      `AsteroidGenerator.h`, `SplineGenerator.h`) is unwired scaffolding — worse for a future
      contributor left half-finished than either finished or removed. HIGH
- [ ] Add a test harness (Catch2/doctest — Math/Physics narrow-phase functions are pure and
      easy to unit test) and a basic CI workflow that at least builds on push. Today a
      regression is only caught by manually building + running Sandbox.
- [ ] Add an `install()`/packaging target — shaders/assets are post-build-copied next to the
      executable, but asset paths in code are relative to the process's working directory,
      not the executable, so it only runs correctly from inside `bin/`.
- [ ] Decide and document whether Linux/Windows platform files are intentionally identical
      GLFW shims (fine, just say so) or should diverge with real OS-specific behavior later.

## Smaller / lower-severity open items (not yet fixed)

Core & Platform:
- [ ] GLFW framebuffer-size/close/mouse-button/scroll callbacks call `data.EventCallback(event)`
      with no null check (only the mouse-pos callback guards it) — an event firing before
      `Application::SetEventCallback` runs is an unhandled `std::bad_function_call`.
- [ ] `EntryPoint.h` has no try/catch around `CreateApplication()`/`Run()`, yet
      `Window::Init` throws `std::runtime_error` on GLFW/GLAD failure — unhandled crash
      with a raw exception message instead of a clean shutdown.
- [ ] `gladLoadGL` called twice back-to-back during `Init()` (`LinuxWindow.cpp:65,84` +
      Windows counterpart) — harmless but confusing/dead code.
- [ ] Dead shadowed mouse-tracking members duplicate the real `WindowData` fields in
      `LinuxWindow.h:32-34` (+ Windows), still marked `// TODO: Remove?`.
- [ ] `Core/Engine.h`/`Engine.cpp` are empty except a `// TODO: is this needed, Remove?`
      comment, yet are `#include`d from `Log.h`/`Application.h` — vestigial.
- [ ] `Core/Time.h`'s `Time` is declared in the global namespace, inconsistent with the
      rest of the engine's `Wankel::` convention.

ECS:
- [ ] `Scene::DestroyEntity` calls `m_Registry.destroy(handle)` with no
      `m_Registry.valid(handle)` guard — destroying an already-destroyed/stale `Entity`
      hits an entt assert in debug, UB in release.
- [ ] `CameraSystem.cpp:17-36` picks whichever `CameraComponent{Primary=true}` it iterates
      to first and `break`s — multiple primary cameras silently ignored, zero primary
      cameras leaves the render camera stale, no warning either way.
- [ ] `Entity.h` has no back-reference validity check between an `Entity` handle and its
      owning `Scene`/registry — nothing prevents using an `Entity` after its `Scene` is
      destroyed.

Renderer:
- [ ] `VertexArray::AddLayout()` is declared but never defined or called anywhere
      (`VertexArray.h:20`) — dead API surface, would be a link error if anyone tried it.
- [ ] `Shader.cpp:32-34,56-57` compile/link error logging uses a fixed 512-byte
      `char info[512]` buffer — long GLSL error logs get silently truncated.
- [ ] `Renderer::Submit` (`Renderer.cpp:110-129`) unconditionally pushes ~10 fog/time/camera
      uniforms into every shader regardless of whether that shader declares them — hard-couples
      "generic" mesh submission to one specific fog effect rather than a real
      material/uniform-buffer abstraction.

Physics:
- [ ] `SpatialHashGrid` cell size is a single hardcoded constant (1.0f,
      `SpatialHashGrid.h`/`PhysicsSystem.h:14`) with a fixed 3x3x3 neighbor query — any
      collider larger than one cell can miss overlaps with things more than one cell away.

Math, Terrain & Sandbox:
- [ ] `Math/Random.cpp`: `Random::Init()` is never called anywhere and `Float()`/`Int()`
      have zero call sites — dead module; if adopted without calling `Init(seed)` first, it
      silently uses mt19937's fixed default seed (identical sequence every run).
- [ ] `Math/Noise.cpp:82-111`: `PerlinNoise()` isn't real Perlin noise — gradient indices
      are derived by scaling the value-noise hash, not true lattice gradient vectors, and
      neither FBM variant normalizes summed amplitude. Low severity since Terrain is
      unwired, but worth fixing before finishing that feature.
- [ ] `Debug/SecondOrderPreview.cpp:29-36` duplicates the second-order-dynamics math from
      `SecondOrderDynamics.cpp` but omits the large-`dt` stability clamp added there — the
      ImGui preview can diverge from actual runtime animation behavior.
- [ ] `Sandbox/src/plate.h`, `triangle.h` — included by `SandboxLayer.cpp` but their
      vertex/index arrays are never referenced again — dead geometry headers.
- [ ] `Sandbox/src/PLYLoader.h` still assumes ASCII PLY with a fixed `x y z r g b a`
      property layout and never checks the `format` header line — a binary PLY or
      reordered-property file silently parses into garbage rather than erroring.

## Game-readiness gap checklist (context, not action items)

| Dimension | Status | Notes |
|---|---|---|
| Lighting/materials | Absent | No normals, no `Texture` class, no lighting terms in any shader |
| Asset pipeline | Partial | PLY only, hand-rolled parser, no glTF/OBJ/FBX/assimp, no texture loader |
| Scene persistence | Absent | No save/load. Not a full-scene-dump problem here (world is procedural) — needed as a narrow seed + player-state save; see Phase 2 data-persistence breakdown |
| Audio | Absent | No OpenAL/miniaudio/SDL_mixer/FMOD/Wwise; SDL3 only used for gamepad |
| UI/HUD | Absent beyond debug | Only Dear ImGui debug panels, no text rendering or shippable UI |
| Testing/CI | Absent | No Catch2/gtest/doctest, no `.github/workflows` |
| Build/packaging | Dev-build only | No `install()`/CPack target; asset paths relative to `bin/` cwd |
| Threading | Absent | Fully single-threaded, no job system |
| Physics fidelity | Basic | Overlap-resolution only, no mass/inertia/restitution/friction |
| Platform abstraction | Cosmetic | Linux/Windows backends are byte-identical GLFW code today |
| External foundation | Solid | EnTT, GLFW, glm, spdlog, Dear ImGui, SDL3 — sensible dependency set |

## Engine comparison (context)

Wankel's architecture is structurally close to TheCherno's "Hazel" engine tutorial series —
a reasonable teaching lineage, not a criticism. If the goal is "ship a specific game as
efficiently as possible," Godot or Unreal (both free, both give lighting/physics/audio/UI/
scene-tooling/asset-pipelines out of the box) will get there faster. If the goal is "build
and understand a custom engine, and the game is secondary," Wankel is a legitimate
foundation — just be clear-eyed that "incorporate it into a game" currently means writing a
lighting/material system, a real physics solver, an asset pipeline, audio, and UI first.
