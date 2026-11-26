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
