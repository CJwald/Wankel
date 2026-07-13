# Wankel Engine — Code Review

Text Rendering, Mass-Aware Physics & Audio (TextRendering, MassPhysics, Audio branches)

Prepared 2026-07-12 | Repository: /home/cj/dev/Wankel | Branch: main @ 01469f0

---

## Executive Summary

This is a fresh code review of three features merged to `main` since the last formal
review (`Documents/WankelEngineReview2_GameReadiness.pdf`, commit `a5f736e`): bitmap-font
text rendering (`Font`/`Texture`/`Renderer::SubmitText`), mass-aware physics resolution
(`PhysicsSystem.cpp`), and basic audio playback via `miniaudio` (`src/Wankel/Audio/`). None
of these went through `/code-review` before merging.

Scope: `git diff 09a97a6..HEAD` — 6 commits, 23 files, ~824 insertions.

**Bottom line:** the mass-aware physics rewrite is a genuine improvement — it's not just
"more correct," one of the scenarios this review set out to flag as a possible regression
(the new single relative-velocity closing-check "missing" a case the old per-body check
caught) turned out, on a worked numerical example, to be the *old* code that was wrong. Two
real, fixable correctness bugs surfaced instead: a font-load failure crashes the whole app
instead of degrading gracefully, and an unguarded zero-size window can inject NaN into the
text rendering pipeline. The audio system works but is intentionally shaped for "basic
one-shot SFX only" — the next planned item (background music) will need real rework, not an
additive change, which is worth knowing before that work starts.

## Findings by Severity

| # | Severity | Summary |
|---|----------|---------|
| 1 | HIGH | `Font::Load` throws but callers treat it as nullable — missing font asset crashes the app |
| 2 | MEDIUM-HIGH | `Renderer::SubmitText`'s ortho projection has no zero-size guard — minimized window → NaN |
| 3 | MEDIUM | `AudioClip::CreateTone` has undefined behavior on negative/NaN duration |
| 4 | MEDIUM-LOW | `Texture`'s GL unpack-alignment state leak (global, never restored) |
| 5 | MEDIUM | Audio system's fixed voice pool won't extend cleanly to music/looping (next planned item) |
| 6 | LOW-MEDIUM | `Movement` system ignores `Rigidbody::Mass` — inconsistent with new mass-aware collisions |
| 7 | LOW | `Mass == 0` clamps to near-infinite inverse mass — latent, not currently triggered |
| 8 | LOW | Static body's `Velocity` isn't zeroed in the new relative-velocity computation — latent |

---

## 1. `Font::Load` throws; callers treat it as nullable — HIGH

**File:** `src/Wankel/Renderer/Font.cpp:22-55` (throw sites), `Sandbox/src/SandboxLayer.cpp:96,1027`

`Font::Load` has exactly three exit paths: two `throw std::runtime_error` (missing file,
atlas doesn't fit) and one successful `return font`. It **never returns null.**

`SandboxLayer`'s constructor calls it unguarded — `m_TitleFont = Font::Load(...)` — with no
`try`/`catch` anywhere in the file. The HUD rendering code then gates all usage behind
`if (m_TitleFont)`, a pattern that only makes sense if `Font::Load` could return a
null/falsy `Ref<Font>` on failure. Since it can't, that guard is dead code.

**Failure scenario:** a missing or corrupted `Assets/Fonts/Orbitron-VariableFont_wght.ttf`
(fresh checkout with a broken submodule/asset, a renamed path, anything) throws during
`SandboxLayer`'s constructor. That's called from `SandboxApp::SandboxApp()` during
`Wankel::CreateApplication()`, inside `EntryPoint.h`'s top-level `try` block — so it's not
an uncaught `std::terminate`, but it *does* hard-exit the entire process (window already
created, GL context already up) before `Application::Run()` — and thus all mesh
rendering/physics — ever executes. The `if (m_TitleFont)` guard implies "skip the HUD text
and keep running" was the intended behavior; what actually happens is the whole app refuses
to start.

**Recommendation:** either wrap the `Font::Load` call in `SandboxLayer`'s constructor in a
`try`/`catch` that logs and leaves `m_TitleFont` null (matching what the `if` guard already
assumes), or remove the dead `if (m_TitleFont)` checks and let a missing font be a fatal
error like it already effectively is — pick one, the current code has one foot in each.

## 2. Zero-size window → NaN projection matrix in `SubmitText` — MEDIUM-HIGH

**File:** `src/Wankel/Renderer/Renderer.cpp:220`

```cpp
glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);
```

No check that `screenWidth`/`screenHeight` are non-zero. The vendored GLM's 4-argument
`ortho` (`external/glm/glm/ext/matrix_clip_space.inl`) unconditionally divides by
`(right - left)` and `(top - bottom)` — a 0-sized window produces `Inf`/`NaN` in the
resulting matrix.

**Reachability confirmed:** `SandboxLayer.cpp`'s HUD block calls `SubmitText` unconditionally
every frame with `screenWidth = window.GetWidth()`, `screenHeight = window.GetHeight()`,
sourced from `glfwGetFramebufferSize` with no clamping (`LinuxWindow.cpp`'s `OnUpdate`).
Minimizing/iconifying a window commonly reports a 0×0 framebuffer on Linux window managers.
Nothing in `Application::Run()` or `LinuxWindow::OnUpdate` skips rendering when
minimized — the render loop, and therefore this `SubmitText` call, proceeds unconditionally
regardless of window state.

**Recommendation:** guard `SubmitText` (or its caller) against `screenWidth == 0 ||
screenHeight == 0` and skip the draw for that frame.

## 3. `AudioClip::CreateTone` — undefined behavior on negative/NaN duration — MEDIUM

**File:** `src/Wankel/Audio/AudioClip.cpp:13`

```cpp
uint32_t frameCount = (uint32_t)(durationSeconds * (float)sampleRate);
```

Casting a negative or NaN float directly to `uint32_t` is undefined behavior per the C++
standard when the value isn't representable in the destination type. In practice this
typically wraps to a huge value, and the very next line (`m_Samples.resize(frameCount)`)
would then attempt a multi-gigabyte allocation — a crash/OOM instead of a graceful no-op.

Not currently triggered — the only call sites (`SandboxLayer.cpp:99-100`) pass hardcoded
positive literals (`0.12f`) — but `AudioClip::CreateTone` is a public static factory with no
documented precondition and no validation, so this is a real landmine for the next caller,
not just current-code-path safe.

**Recommendation:** clamp or early-return on `durationSeconds <= 0.0f` (and reject
non-finite values) before the cast.

## 4. `Texture` leaks global GL unpack-alignment state — MEDIUM-LOW

**File:** `src/Wankel/Renderer/Texture.cpp:20`

```cpp
glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
```

Set before uploading the font atlas, never restored to the GL default (4) afterward.
`GL_UNPACK_ALIGNMENT` is global context state, not scoped to the current texture bind — this
persists for every subsequent `glTexImage2D`/`glTexSubImage2D` call anywhere else in the
process for the rest of its lifetime.

Currently harmless by coincidence: the only other texture-upload code in the process is
Dear ImGui's own backend (`external/imgui/backends/imgui_impl_opengl3.cpp`), which happens
to *also* set alignment to 1 before its own uploads (and carries an identical `// FIXME:
Consider backing up and restoring` comment acknowledging the same gap upstream) — so the two
are compatible by luck, not by design. A future RGB texture loader with unpadded rows would
silently misinterpret its own upload if `Texture`'s constructor ran first and nothing reset
the state.

**Recommendation:** save the previous `GL_UNPACK_ALIGNMENT` value and restore it after the
upload, or at minimum leave a comment flagging the dependency the way imgui's own code does.

## 5. Audio system won't extend cleanly to music/looping — MEDIUM (design)

**File:** `src/Wankel/Audio/AudioSystem.{h,cpp}`, `src/Wankel/Audio/AudioClip.h`

`AudioSystem::Play()` returns `void` — no handle, no way to later stop/pause/fade a specific
sound. The fixed 8-voice pool has no priority/category concept; voice selection is
unconditional round-robin, so *any* sound (including a future looping music track) competes
equally with one-shot SFX and can be cut off by a burst of concurrent beeps. `AudioClip` is
hardcoded mono (`AudioSystem::Play`'s `ma_audio_buffer_ref_init` call passes a literal `1`
for channel count, not read from the clip) and assumes the entire clip is decoded into memory
up front (`std::vector<float>`) — no streaming.

`Documents/TODO.md`'s own Audio entry already names "no streaming/music/looping layer" as
the explicit next step, so this isn't speculative scope-creep criticism — it's a concrete
statement that the mechanism (unconditional voice-stealing, void-returning fire-and-forget,
hardcoded mono, fully-resident PCM) will need real rework, not an additive `Play(loop=true)`
parameter, to safely coexist with a protected/loopable music track.

**Recommendation:** when music work starts, budget for: a return handle from `Play()`, a
"protected/non-stealable" voice flag or a separate reserved voice for music, and either a
forced-mono-downmix policy or a `Channels` field threaded through `AudioClip` →
`AudioSystem::Play`.

## 6. `Movement` system doesn't read `Rigidbody::Mass` — LOW-MEDIUM (design)

**File:** `src/Wankel/Physics/Systems/PhysicsSystem.cpp:17-44` vs. `145-146`

Mass is now a first-class factor in *collision resolution* (this review's main feature), but
the player/AI-driven "Movement-driven velocity target-seeking" block earlier in the same
function computes acceleration/deceleration and sets `rb.Velocity` directly with no
reference to `rb.Mass` at all. Once anything gives an entity a non-default mass, it will
still accelerate/decelerate identically to a unit-mass body under player/AI control, and
only "feel" heavy when it collides with something.

This may be intentional — the block is explicitly commented as a kinematic
character-controller pattern, where mass-independent acceleration is a fairly conventional
choice — but it's worth confirming that's a deliberate design decision rather than an
oversight now that `Mass` has real, visible effects elsewhere in the same system.

## 7. `Mass == 0` clamps to near-infinite inverse mass — LOW (latent)

**File:** `src/Wankel/Physics/Systems/PhysicsSystem.cpp:143-146`

The old resolution code never read `Mass` at all, so `Mass == 0` was inert. The new code
computes `invMass = 1.0f / glm::max(mass, kMinMass)` with `kMinMass = 0.0001f` — a body with
`Mass == 0` gets `invMass ≈ 10000`, meaning it would absorb almost the entire position
correction and velocity impulse in any collision (get violently ejected) instead of behaving
like a normal body.

Verified latent, not live: `PhysicsComponents.h` defaults `Mass` to `1.0f`, and a repo-wide
grep found zero call sites anywhere that set `.Mass` to anything else. This is real
defensive-code risk for whenever gameplay code starts varying `Mass`, not a currently
reachable bug.

## 8. Static body's stale `Velocity` isn't zeroed in the new relative-velocity computation — LOW (latent)

**File:** `src/Wankel/Physics/Systems/PhysicsSystem.cpp:169-170`

```cpp
glm::vec3 velB = bIsStatic ? glm::vec3(0.0f) : rbbPtr->Velocity;
glm::vec3 relativeVelocity = velB - rba.Velocity;
```

`velB` is explicitly zeroed when `b` is static; `rba.Velocity` has no equivalent
`rba.IsStatic ? 0 : rba.Velocity` guard. If `a` were static with a stale nonzero `Velocity`,
it would corrupt the impulse magnitude applied to `b` (though `a` itself is still correctly
never mutated, since the velocity write-back is separately guarded by `if (!rba.IsStatic)`).

Verified inert today: `Velocity` defaults to `{0,0,0}`, the Movement- and
position-integration passes both skip static bodies entirely (never write `Velocity` for
them), every `IsStatic = true` entity in `SandboxLayer.cpp` has `Velocity` left untouched at
its default, and nothing in the codebase toggles `IsStatic` at runtime. A static body's
`Velocity` is always exactly zero by construction/convention right now — but the asymmetry
is a real landmine for a future kinematic/freeze-toggle feature.

**Recommendation:** add the missing guard for symmetry and to remove the implicit "callers
must never set Velocity before marking static" invariant this currently depends on.

---

## Investigated and Refuted (worth recording, not worth fixing)

Two candidates raised during the review's finder passes did not survive verification —
recorded here so they aren't re-investigated in a future pass:

- **"The new single relative-velocity closing-check misses a case the old per-body velocity
  check would have caught."** Constructed a worked numerical counterexample (body A
  retreating fast, body B creeping toward A's old position) that looked like a miss on
  paper. Checking the actual physics: the scenario is a pair whose separation is genuinely
  *growing* (net closing velocity was positive, meaning separating) — the old code's
  independent per-body check would have applied a spurious, momentum-inconsistent impulse to
  B in that exact scenario. The new formula is the standard textbook 2-body relative-velocity
  check (matches Box2D-style manifold resolution) and is provably identical to the old
  per-body check whenever exactly one side is static (per finding 8, always true today) — it
  only differs for two-dynamic-body pairs, and there it's the more correct formula.
- **"`SubmitText`'s vertex-truncation warning is inconsistent with the pre-existing debug-line
  pass, which lacks the same guard."** The debug-line pass (`Renderer.cpp`'s `EndScene()`)
  already has the identical `if (vertexCount > kMaxDebugVertices) { WK_CORE_WARNING(...);
  vertexCount = kMaxDebugVertices; }` guard, predating this diff. No asymmetry exists.

## Methodology

High-effort mode: 8 independent finder angles (line-by-line diff scan, removed-behavior
audit, cross-file call-site tracing, reuse, simplification, efficiency, altitude,
CLAUDE.md conventions), each surfacing up to 6 candidates, run as parallel subagents against
`git diff 09a97a6..HEAD`. Every candidate with a nameable failure scenario was then checked
by an independent 1-vote verifier subagent (given the diff, the relevant files, and the
specific claim) with an explicit CONFIRMED/PLAUSIBLE/REFUTED instruction and a bias toward
not dismissing realistic-but-currently-unexercised code paths. Two candidates were refuted
outright on verification (see above); the remaining 8 are reported here, ranked most severe
first, correctness bugs ahead of design/altitude findings per the review's own severity
ordering.
