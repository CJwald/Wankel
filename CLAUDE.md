# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Wankel is a 3D game engine (C++20, OpenGL via glad/GLFW, ECS via EnTT) with a `Sandbox` example app that flies a ship around a scene with AABB/Sphere collision. Development happens from terminal (Vim), primarily on Linux, with Windows also supported via MSVC/Ninja.

## Build & run

```bash
git submodule update --init --recursive   # first time only

./scripts/build.sh      # configures + builds the Wankel static library (build/, Debug by default)
cd Sandbox
./build.sh              # builds the Sandbox executable against the Wankel lib
./run.sh                # runs it
```

`scripts/bc.sh` and `scripts/bwsr.sh` chain all three steps above. `BUILD_DIR`, `BUILD_TYPE` (Debug/Release), and `JOBS` env vars override the engine build's defaults.

Windows equivalents: `scripts\build.bat`, `Sandbox\build.bat`, `Sandbox\run.bat` (requires MSVC Build Tools 2022, Ninja, CMake).

There is no test suite, linter, or CI config in this repo — verifying a change means building and running the Sandbox app.

## Architecture

**Core** (`src/Wankel/Core/`): `Application` owns a `LayerStack` and a `Window`. `Application::Run()` is the main loop: poll input -> `Window::OnUpdate()` -> `Renderer::Clear()` -> iterate the `LayerStack` calling `Layer::OnUpdate()` -> ImGui pass. Client apps define `Wankel::CreateApplication()` and include `EntryPoint.h` to get `main()`. Events go through `Application::OnEvent` via `EventDispatcher`, propagating top-down through layers until handled. `Base.h` defines `Ref<T>`/`Scope<T>` (shared_ptr/unique_ptr aliases); `Log.h` provides `WK_CORE_*`/`WK_CLIENT_*` macros.

**ECS** (`src/Wankel/ECS/`): built directly on EnTT (`entt::registry`), not abstracted behind a custom API. `Entity` wraps `entt::entity` + registry pointer. `Scene` owns the registry plus a fixed, manually-wired set of system instances, ticked in hardcoded order from `Scene::OnUpdate(dt, camera)`: player controller -> physics -> transform -> kinematics -> procedural animation -> camera. There is no auto-registration/discovery of systems — adding one means wiring it into `Scene` explicitly. Components live under `ECS/Components/*.h`, aggregated via `Components.h`.

**Physics** (`src/Wankel/Physics/`): `PhysicsSystem::Update` (called from `Scene::OnUpdate`) does, in order: Movement->velocity integration, a separate position-integration pass over *all* `Transform`+`Rigidbody` entities (regardless of whether they have `Movement` — see "Fix Rigidbody without Movement never integrating position"), broad-phase via `SpatialHashGrid` rebuilt from `AABBCollider` entities, then narrow-phase through `CollisionDispatcher::ResolveCollision`, which type-checks collider combos and dispatches to `AABBvsAABB`/`SpherevsSphere`/`SpherevsAABB`. Manifold normals are kept consistently A->B by flipping sign for the Sphere(A)-vs-AABB(B) case. **In progress:** `Physics/Collision/ColliderType.h` (AABB/Sphere/Capsule/Mesh enum) is scaffolding for a planned unified collider dispatch to replace the current per-pair-type `if` chain in `CollisionDispatcher` — see README TODO "Collider system redesign."

**Renderer** (`src/Wankel/Renderer/`): OpenGL 3.3 core. Static `Renderer` class: `BeginScene(Camera&)`/`Submit(transform, Mesh, Shader*)`/`EndScene()`, plus `SubmitDebugLines` for `DebugDraw`. `Mesh`/`Shader`/`Buffer`/`VertexArray` are thin GL wrappers. `Camera` is a standalone view/projection object passed into both `Scene::OnUpdate` and `Renderer::BeginScene`.

**Platform** (`src/Wankel/Platform/`): `Linux/` and `Windows/` each implement `Window::Create()` and `Input.h`'s static polling API for their OS; `PlatformDetection.h` selects which side compiles via `WK_PLATFORM_*` macros (and the root `CMakeLists.txt` also filters `Platform/Linux|Windows` sources by `WIN32`/`UNIX`).

**Math** (`src/Wankel/Math/`): glm helpers, `Noise`, `Random`, `SecondOrderDynamics` (spring-like follow), `Spline3D`, `Polygonize` (marching-cubes triangulation table used by Terrain).

**Terrain** (`src/Wankel/Terrain/`): `MarchingCubes` + `VoxelDensityFeild` implement voxel iso-surface extraction — primitives only. Actual terrain/asteroid generation lives at the app level in `Sandbox/src/Terrain/`.

**Umbrella headers**: `src/Wankel.h` is the single public include for client apps (re-exports Core/Events/ECS/Renderer/ImGui/Math). `src/wkpch.h` is the precompiled header used by every engine `.cpp` — deliberately limited to STL + `PlatformDetection.h` + `Base.h` + `Log.h` to avoid PCH bloat; heavier engine headers are included directly where needed.

**Sandbox** (`Sandbox/src/`): `SandboxApp.cpp` subclasses `Application`, includes `EntryPoint.h`, and is the only place `CreateApplication()` is defined — it pushes a single `SandboxLayer`. `SandboxLayer` owns a `Scene`, a `Camera`, meshes/shaders, and app-specific systems (e.g. `PlayerInputSystem`); its `OnUpdate()` computes dt and calls `m_Scene.OnUpdate(dt, m_RenderCamera)`. `MeshLoader`/`PLYLoader` load `.ply` assets from `Sandbox/src/Assets/Mesh/`. `Debug/DebugOverlay.cpp` and `Debug/SecondOrderPreview.cpp` are ImGui-based dev tooling.

## Dependencies

Managed as git submodules under `external/` (spdlog, glm, glad, SDL3, glfw, EnTT, imgui). `GLM_ENABLE_EXPERIMENTAL` is defined engine-wide. Requires g++ 13+ (EnTT).
