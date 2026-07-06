---
name: run
description: Use this skill to build and/or launch the Wankel engine's Sandbox app — when the user asks to "build", "run", "launch the sandbox", "run the app", "test my change", "see it working", or wants to verify a change to the engine actually works. Covers the full build-engine -> build-sandbox -> run pipeline and the GDB debug variant.
---

# Build & Run Wankel Sandbox

The only way to exercise the Wankel engine is through `Sandbox`, the example
app (flies a ship around a scene with AABB/Sphere collision). There is no
test suite — verifying an engine change means building and running Sandbox.

## Full pipeline

```bash
./scripts/build.sh   # configure + build the Wankel static library -> build/bin
cd Sandbox
./build.sh           # configure + build the Sandbox executable -> Sandbox/build/bin/Sandbox
./run.sh             # launch it (opens a GLFW/OpenGL window, needs a display)
```

`scripts/bc.sh` / `scripts/bwsr.sh` (repo root) and `Sandbox/br.sh` (inside
`Sandbox/`) chain build+run in one shot.

- Only touched engine code under `src/Wankel/`? Re-run `./scripts/build.sh`
  then `cd Sandbox && ./build.sh && ./run.sh`.
- Only touched `Sandbox/src/`? `./build.sh && ./run.sh` from inside `Sandbox/`
  is enough — no need to rebuild the engine.
- `run.sh` exits immediately with "Sandbox binary not found!" if the binary
  is missing — that means a build step was skipped or failed silently.

## Env overrides

`BUILD_DIR` (default `<dir>/build`), `BUILD_TYPE` (`Debug`/`Release`,
default `Debug`), `JOBS` (default `nproc`) — same var names for both the
engine and Sandbox builds.

## Debugging a crash

```bash
cd Sandbox
./runGDB.sh   # same binary-existence check as run.sh, then launches under gdb
```

## What to look for

This is a windowed GUI app (GLFW window, OpenGL 3.3 core) — it needs a live
`DISPLAY`/Wayland session, it won't run headless. A clean build + the window
opening without an immediate crash/exit is the main automated signal;
actually judging rendering/gameplay correctness needs a human looking at the
window (or a screenshot) since there's no automated visual test harness.
