# Dream Engine

Dream Engine is a general-purpose **C++20 game engine** with first-class **2D and 3D** systems. Android arm64-v8a is the current device target; GitHub Actions is the primary build/test environment.

## Start here (users and AI assistants)

1. Read this README.
2. Read [PMCS/README.md](PMCS/README.md) for the documentation map.
3. Read [PMCS/AI_HANDOFF.md](PMCS/AI_HANDOFF.md) before changing code.
4. Read [PMCS/CURRENT_STATE.md](PMCS/CURRENT_STATE.md) for implementation and verification status.
5. Follow [PMCS/RULES.md](PMCS/RULES.md), [PMCS/DECISIONS.md](PMCS/DECISIONS.md) and [PMCS/ROADMAP.md](PMCS/ROADMAP.md).

## Current implementation

The engine includes a generational ECS and scene scheduler, 2D/3D physics worlds, backend-neutral rendering contracts and bgfx sprite/mesh/material paths, retained touch UI, animation, audio mixing, bounded asynchronous asset loading/cache/import, and a native in-process DMod runtime with unrestricted engine access and typed hooks.

The Android application is in `android-host/`. Its GameActivity native loop creates EngineRuntime, loads GLES 3.0 shader assets, initializes bgfx, renders a combined 2D/3D physics demo, routes touch input and connects AAudio. The host also writes logs to Android Logcat (`DreamEngine`) and a rotating `logs/engine.log` under app-specific GameActivity files, with an internal-storage fallback.

## Verification and download

- Latest engine CI: [run 45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968) — 5/5 jobs passed; GCC, Clang and ASan/UBSan each passed 47/47 tests; Android ARM64 cross-build and DMod AST proof passed.
- Fresh ARM64 Android APK: [run 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085). This workflow builds pinned bgfx shader tools, compiles and packages GLES 3.0 sprite/mesh shaders, assembles the debug APK, verifies its native ARM64 library and uploads the artifact when complete.

The APK is a first integrated demo host, not a finished production engine. Advanced physics, extended import formats, full font shaping, profiling/editor tooling, asset GPU residency, mod resource/job lifetime tracking, signatures and real-game validation remain in the roadmap. Physical-device testing is not inferred from CI; the first install/launch/lifecycle test is performed by the user.

## Project rules

- C++20 is the engine language; do not switch to Rust.
- Keep the engine reusable; game-specific systems belong in game projects.
- Treat both 2D and 3D as first-class.
- Ordinary native DMods are prebuilt, loaded dynamically and execute in-process with full access; **there is no sandbox or capability enforcement by explicit project decision**.
- Do not require recompiling native mods on the user's device or rebuilding the APK to install them.
- Research and test dependencies before adoption, and never mark work complete without reproducible evidence.
- Update PMCS after implementation or design changes.

**When in doubt:** follow the accepted decisions in PMCS and preserve the source-of-truth status in CURRENT_STATE.md.


## Implementation update — 2026-11-26

This update is authoritative over older checklist/status text above where they conflict.

- **Physics broadphase queries:** 2D and 3D raycast/overlap queries use deterministic AABB candidate queries backed by cached X-interval indices. Updating/removing a proxy invalidates the index. Tests include 96 distant bodies and stable candidate order.
- **Continuous collision detection (limited opt-in):** `BodyDescription2D::bullet` enables analytic swept-circle time-of-impact against static circles; `BodyDescription3D::bullet` enables swept-sphere time-of-impact against static spheres. Other shapes and dynamic-vs-dynamic pairs still use discrete collision detection. Do not describe this as general CCD.
- **OBJ/MTL:** OBJ material-library parsing and concave polygon ear-clipping are implemented. Triangulation uses Newell normal projection and preserves face-corner UV/normal references; tests cover both polygon windings and signed area.
- **Asset lifecycle hooks:** asynchronous loader observers report read/decode/register and cache phases. The runtime queues owned events with sequence numbers and dispatches native hooks from the engine update thread, not worker threads. Queue capacity is bounded; overflow is logged.
- **Native world hooks:** ECS entity/component/hierarchy mutation hooks run outside the World lock, with the original operation continuation carried in the typed mutation context.
- **Android shader reproducibility:** the four ESSL 300 binaries in the APK are checked in with SHA-256. The Android workflow validates them directly; the separate pinned bgfx proof recompiles from source and compares the binaries byte-for-byte.
- **Android host:** GameActivity host, EngineRuntime, bgfx GLES 3 renderer, 2D/3D demo, retained touch UI, basic software-keyboard/IME bridge, AAudio output connection, pause/surface recreation handling and rotating logs are present. CI proves cross-compilation/package layout only, not physical-device rendering/audio/IME behavior.
- **Sleeping:** not yet implemented; no public sleep/wake state is exposed. It remains open rather than being marked complete.
- **Remaining production work:** general CCD for non-circular shapes and dynamic-vs-dynamic pairs; sleeping/waking; joints; multiple fixtures and robust contact manifolds; complete MTL texture handling and glTF/GLB/image/audio import; GPU asset residency/device-loss policy beyond renderer resource reinitialization; full IME composition/selection; full font shaping/atlas and UI clipping; animation blends/events/curves; native mod resource/job/thread ownership and hot reload; package signatures/trust; profiler/editor; two real-game integration tests; physical-device verification.


## 2026-11-26 implementation note

Latest source now includes indexed 2D/3D broadphase queries, limited opt-in circle/sphere bullet CCD, OBJ concave ear clipping plus MTL parsing, queued asset lifecycle hooks, ECS mutation hooks and reproducible GLES shader binaries. The first Android host/APK path exists, but physical GPU/audio/IME verification and several production roadmap systems remain explicitly open; read [PMCS/ROADMAP.md](PMCS/ROADMAP.md) and [PMCS/CURRENT_STATE.md](PMCS/CURRENT_STATE.md) for the exact status.
