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

- [x] **Vertex format + lighting (normals done; UVs/textures/materials still open).**
      Added `Normal` to `Vertex` (appended after `Color`, not inserted, so the existing
      2-element aggregate-init call sites like `Geometry::CubeVertices` keep compiling via
      its default member initializer) and a basic Blinn-Phong directional-light shader path.
      **Still open:** UVs, a `Texture` class, and material abstraction — none of that exists
      yet, so nothing can be textured. `cube.h`'s hardcoded background box also wasn't
      touched (still 8 shared vertices with no real per-face normals, so it lights as flat
      "up-facing" under the new shader) since it's out of scope for this pass.
      Engine changes: `Mesh.h`/`.cpp` (layout + `CreateMirrored` now also flips `Normal`),
      `Shader::SetMat3` added, `Renderer::LightSettings`/`SetLight()` (mirrors the existing
      `FogSettings`/`SetFog()` pattern) with the normal matrix computed once per `Submit()`
      call (CPU-side, not recomputed per-vertex in the shader). `Sandbox/src/shaders/cube.vert`/
      `cube.frag` updated for the new attribute + lighting math; `SandboxLayer` got a mirrored
      "Lighting" ImGui panel next to the existing "Fog" one. Verified with a standalone test
      confirming computed/authored normals are unit-length for both loaders below, plus a
      live Sandbox run with no shader link errors.
- [x] **Real asset pipeline — glTF import done; textures/asset registry still open.** Added
      glTF/.glb import via `cgltf` (vendored as `external/cgltf`, a single-header library with
      no CMakeLists of its own — just added to Wankel's include path). Meshes that don't
      supply a NORMAL attribute (and all PLY meshes, which never carry one) fall back to a
      shared `ComputeSmoothNormals()` utility (area-weighted, averaged per-vertex normals from
      triangle winding). Per the game-readiness review's "how a real engine would work" ask,
      **`MeshLoader`/`PLYLoader` were moved from `Sandbox/src/` into the engine**
      (`src/Wankel/Assets/`) alongside the new `GltfLoader`, since asset import is generic
      engine infrastructure, not a Sandbox-specific concern — mirrors the existing
      Renderer/Mesh/Shader split (GL wrapping) vs. this new Assets split (file parsing).
      `PLYLoader` was also hardened while being moved: it now validates the PLY `format`
      header line and throws instead of silently returning empty data on any failure (missing
      file, non-ASCII format). `SandboxLayer`'s player head/leg now load the new
      `PlayerHead01.glb`/`PlayerLeg01.glb` for testing; ship/gun/box/enemy body/legs stay on
      `.ply` to prove both loaders coexist correctly in the same running scene. **Still
      open:** `stb_image` for textures and a minimal asset registry (handle/ID instead of
      hardcoded relative path strings in `SandboxLayer`'s constructor) — those need the
      Texture/material work above to actually be useful.
      **Axis-convention bug found & fixed post-merge:** the first version loaded glTF meshes
      90° yawed relative to PLY meshes. Blender's glTF exporter does the textbook Z-up→Y-up
      conversion (`gltf = (x, z, -y)`), but that's a *different* rotation than the one
      `PLYLoader` has always used (`engine = (-y, z, -x)`) — a fixed 90° discrepancy between
      the two conventions, not an asset-specific export mistake. Fixed by adding a
      `ToEngineSpace()` remap in `GltfLoader.cpp` (applied to both position and normal, after
      the node world-transform) so glTF assets target the same established convention as the
      20+ existing PLY assets, rather than changing `PLYLoader`'s formula or rotating source
      meshes in Blender (either of which would've had a much larger, riskier blast radius).
      **Two bugs found by `/code-review` and fixed:**
      - Every mesh-loader call threw `std::runtime_error` on failure (missing file, bad
        format, empty mesh) with no `try`/`catch` anywhere in the startup chain, so any
        missing/renamed asset crashed the whole app via an uncaught exception instead of the
        old graceful "log + render nothing" degrade. Fixed by wrapping `CreateApplication()`/
        `Run()` in `EntryPoint.h`'s `main()` in a top-level try/catch that logs via
        `WK_CORE_FATAL` and exits with a clean non-zero code. Verified live by temporarily
        renaming a build-output asset copy and confirming a clean exit (code 1) instead of a
        crash, then restoring it.
      - `GltfLoader`'s missing-normal fallback was scoped to the whole file via one shared
        `anyMissingNormals` flag — a single primitive lacking NORMAL data triggered
        `ComputeSmoothNormals` over the *entire* combined vertex buffer, silently overwriting
        correctly-authored normals from other primitives in the same file. Fixed by building
        each primitive into a local vertex/index buffer first, running the fallback (if
        needed) scoped to just that primitive, and only then appending it to the shared
        output — removing the cross-file flag entirely. Verified with a hand-built
        multi-primitive test `.gltf` (one primitive with an authored normal deliberately
        opposite its own geometric winding, one without) confirming the authored primitive's
        normal survives untouched while the other correctly falls back to a computed one.
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
- [x] **Mass-aware physics — `Rigidbody::Mass` wired in; `Force` deliberately left alone.**
      `PhysicsSystem.cpp`'s collision resolution now uses inverse-mass weighting throughout:
      - **Position solve**: penetration correction now splits proportional to each body's
        inverse mass (`invMassA/(invMassA+invMassB)` etc.) instead of a flat 50/50 — a
        strict generalization that reduces to the old "static side doesn't move, dynamic
        side takes it all" when one side has infinite mass (`invMass = 0`), and to the old
        flat 50/50 when both dynamic sides happen to have equal mass. Only differently-massed
        dynamic pairs actually change behavior.
      - **Velocity solve**: replaced the old "independently zero each body's own penetrating
        velocity component" scheme with the standard 2-body single-contact normal impulse
        formula (no friction, restitution hardcoded to 0 — fully inelastic, matching the
        engine's existing "objects stop dead on contact" feel). The old scheme wasn't
        actually momentum-conserving even for equal masses (both bodies fully stopped,
        losing momentum); the new one is exact — colliding bodies now end up moving
        *together* at the shared mass-weighted velocity, which is the physically correct
        outcome for restitution 0. Exact match to old behavior whenever one side is static.
      - `Mass` is also clamped away from `<=0` (`kMinMass = 0.0001f`) before inverting, so a
        misconfigured `Rigidbody::Mass` can't divide-by-zero and inject NaN into position or
        velocity — the same failure mode flagged (but not currently triggered) for the
        renderer's normal-matrix computation earlier.
      - **`Force` deliberately NOT wired up.** Nothing in the codebase ever sets
        `Rigidbody::Force` (still `PhysicsComponents.h:13`, "Not used, I think I want it
        eventually") — wiring a force-accumulator integration step for a field no caller
        ever writes would be unverifiable dead scaffolding. Revisit once something (gravity,
        thrust, explosions) actually needs to apply a force.
      Verified with a standalone test covering all four resolution shapes: equal-mass
      collision conserves momentum and ends with both bodies moving together (not both
      instantly stopped, which the old code did); a 100:1 mass collision conserves momentum
      and produces the correct inelastic shared velocity (500/101 ≈ 4.95, not the light body
      launching off faster - that only happens with restitution > 0); position correction
      split matches the inverse-mass ratio exactly (3:1 masses → 1:3 movement ratio); and a
      collider-only static wall (no `Rigidbody`) is still correctly treated as infinite mass.
      Also confirmed live in Sandbox — stable, no regressions.
      A per-body `Restitution` field (for bounciness) would be a natural, easy follow-on
      given the impulse formula already supports it — not added now since it wasn't asked
      for and the existing behavior (0 restitution) matches what the engine has always felt
      like.
- [x] **Audio — basic playback done via miniaudio.** Added `src/Wankel/Audio/`:
      `AudioClip` (owns a short in-memory mono PCM float buffer; `CreateTone()` procedurally
      generates a sine wave with a ~10ms fade-out envelope to avoid an audible click at the
      end) and `AudioSystem` (`Init`/`Shutdown`/`Play()` — a fixed 8-voice pool cycled
      round-robin with "voice stealing" when all voices are busy, the standard simple
      approach for non-spatialized one-shot SFX; each voice uses `ma_audio_buffer_ref` so
      concurrent/overlapping plays of the same clip don't fight over a shared read cursor,
      and each voice keeps its own `Ref<AudioClip>` alive for the duration of playback so a
      short-lived caller-side clip reference can't be freed out from under it mid-playback).
      Vendored `miniaudio` as `external/miniaudio` (single header, public domain/MIT-0,
      matches the `cgltf`/`stb` vendoring convention) — needs `-ldl -lpthread -lm` linked on
      Linux (added to `CMakeLists.txt`, backends themselves are loaded via `dlopen` at
      runtime so no other link-time dependency). `AudioSystem::Init()`/`Shutdown()` wired
      into `Application`'s constructor/destructor alongside `Renderer`/`InputSystem`.
      **Test wiring**: `SandboxLayer`'s existing left-click raycast (previously only
      teleported "Cube" entities on hit) now also plays a low tone (220Hz) on any click, or
      a higher tone (880Hz) instead when the click hits a block.
      Verified precisely (frequency, not just "doesn't crash"): a standalone test measures
      zero-crossings in the generated PCM data (52 crossings for the 220Hz/120ms clip vs.
      an expected ~52.8, 211 vs. ~211.2 for the 880Hz clip), confirms amplitude stays within
      the requested bound, confirms the fade-out actually reaches ~0 at the clip's end
      (avoiding a click/pop), and exercises `AudioSystem::Init`/`Play` (including forcing
      voice-stealing with 20 plays against 8 voices) /`Shutdown` without crashing. Also
      confirmed live in Sandbox — `AudioSystem: initialized (8 voices)` logs cleanly, no
      errors, stable run. Actually hearing the two distinct beeps on click/hit is still a
      manual check only you can do (no audio-capture tooling available here, same
      constraint as the screenshot situation for visuals).
      **Still open:** no spatialization/3D positioning (deliberately disabled for these
      one-shot UI-style beeps via `MA_SOUND_FLAG_NO_SPATIALIZATION`, but a future
      `AudioSource` component for positioned SFX would need it), no streaming/music/looping
      layer, no real sound-file loading yet (`AudioClip` only generates procedural tones —
      loading actual audio assets via miniaudio's own decoders is a natural, small follow-on
      once there's a real sound file to load, mirroring how `MeshLoader` dispatches by format).
- [x] **Minimal UI/text rendering — basic bitmap-font renderer done.** Added `Texture`
      (`src/Wankel/Renderer/Texture.{h,cpp}` — minimal single-channel/R8 GL wrapper, the
      engine's first texture support of any kind) and `Font` (`Font.{h,cpp}` — loads a .ttf,
      bakes ASCII 32-127 into a 512x512 atlas via `stb_truetype` at load time, vendored as
      `external/stb`). `Renderer::SubmitText()` mirrors the existing `SubmitDebugLines`
      pattern (its own dedicated VAO/VBO/shader, `text.vert`/`text.frag` under
      `src/Wankel/Renderer/shaders/`) — a screen-space orthographic overlay pass, depth-test
      disabled, drawn after `EndScene()`. `SandboxLayer` loads Orbitron (a sci-fi/futuristic
      display font, OFL-licensed, vendored at `Sandbox/src/Assets/Fonts/`) and renders
      "Wankel" top-right, re-measuring the text width every frame so it stays anchored to the
      corner across window resizes. Verified with a standalone test (headless GL context +
      `Font::Load`/`MeasureWidth`/`BuildQuads`) confirming width scales correctly with string
      length, quad/UV data is well-formed, and pen-advance math is internally consistent; also
      confirmed live in Sandbox (atlas bakes, no shader errors, stable run).
      **Bug found after initial "it builds and doesn't crash" pass — text was invisible.**
      The first version rendered nothing on screen despite loading/baking correctly. Root
      cause: the orthographic projection's Y-flip (screen Y-down -> NDC Y-up, needed so
      screen-space pixel coordinates map correctly) makes the text quads' triangle winding
      come out clockwise in the final rasterized image, so they were silently back-face
      culled under the engine's default `GL_CULL_FACE`/`GL_BACK` (enabled globally in
      `Renderer::Init()`) — `SubmitText` never disabled culling for its own draw call, unlike
      `SubmitDebugLines` which does (debug lines don't need it since `GL_LINES` isn't
      culled, but `SubmitText` uses `GL_TRIANGLES`, which is). Fixed by disabling
      `GL_CULL_FACE` around the text draw call. Verified by adding an offscreen-framebuffer
      pixel-readback test (headless GL context + FBO + `glReadPixels`) that reproduced the
      exact symptom with the fix reverted (0 non-black pixels) and confirmed the fix
      resolves it (866 visible pixels, glyph-shaped coverage) — a stronger check than "it
      builds and doesn't crash," which is what let this ship broken the first time.
      **Still open:** no general on-screen UI widgets (buttons/panels/layout) — this is text
      rendering only, not the "basic screen-space quad UI" half of this item. No word-wrap,
      no Unicode/dynamic glyph ranges (fixed ASCII atlas baked once at load), no batching
      across multiple `SubmitText` calls (one draw call each, fine at HUD-label scale, would
      need revisiting for large volumes of text).
      **Follow-on:** added a second HUD label, bottom-right, showing `Mode: FPS`/`Mode: FLIGHT`
      (light grey, reuses the same Orbitron font) — reads `PlayerController::Mode` directly
      each frame, so it switches on its own whenever `PlayerInputSystem` toggles look mode, no
      extra state tracking needed. Verified with an offscreen pixel-readback test confirming
      both mode strings render visible, right-aligned, and measure to distinctly different
      widths (i.e. actually different text, not a stale/cached value).

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

## Game-readiness gap checklist (context, not action items)

| Dimension | Status | Notes |
|---|---|---|
| Lighting/materials | Partial | Normals + basic Blinn-Phong directional light done; still no UVs, `Texture`/material system, or actual texturing |
| Asset pipeline | Partial | PLY + glTF/.glb (via `cgltf`) both work; still no OBJ/FBX/assimp, no `stb_image`/texture loader, no asset registry (paths still hardcoded) |
| Scene persistence | Absent | No save/load. Not a full-scene-dump problem here (world is procedural) — needed as a narrow seed + player-state save; see Phase 2 data-persistence breakdown |
| Audio | Basic | `miniaudio`-backed one-shot SFX playback (`AudioSystem::Play`, 8-voice pool) done; still no spatialization, streaming, music/looping, or real sound-file loading (procedural tones only) |
| UI/HUD | Partial | Basic bitmap-font text rendering done (`Font`/`Renderer::SubmitText`); still no general widgets/panels/layout, still relies on Dear ImGui for all debug panels |
| Testing/CI | Absent | No Catch2/gtest/doctest, no `.github/workflows` |
| Build/packaging | Dev-build only | No `install()`/CPack target; asset paths relative to `bin/` cwd |
| Threading | Absent | Fully single-threaded, no job system |
| Physics fidelity | Basic | Mass-weighted position/velocity resolution (proper momentum conservation) now done; still no inertia/rotation, no restitution/friction (hardcoded inelastic), no Force integration |
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
