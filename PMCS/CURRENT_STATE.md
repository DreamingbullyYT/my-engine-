# Current State

**Last checked:** 2026-11-25  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project and `dream_core` static library.
- Public engine version/name API.
- Core logger with severity filtering and replaceable, thread-safe sink configuration.
- Structured diagnostic value type with severity, code, message and source.
- Typed event dispatcher with RAII subscriptions, priority ordering, re-entrant dispatch, callback execution outside locks and exception isolation.
- Job system with worker threads, futures, exception propagation, idle waiting and drain-on-shutdown semantics.
- Dependency-free tests for version, logging, diagnostics, event ordering/disconnection/failure isolation, and job execution/failure/draining.
- GitHub Actions workflow for Linux GCC/Clang build+CTest and Android NDK cross-build for `arm64-v8a`.

## Verified CI
- Run [45702432586](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45702432586) completed successfully.
- Linux GCC and Clang: configure, build and CTest passed.
- Android NDK: configure and core cross-build passed for `arm64-v8a` (ARMv8-A).
- ChaiScript direct AST proof: compiled and executed on Linux; the script returned 42 and no `Compiled` AST nodes were present.
- Event and job services are CI-verified.
- Android remains cross-compilation only; the ChaiScript proof has not yet been cross-compiled for Android.

## Build and test policy
- The user has no PC available. GitHub Actions is the primary build/test environment.
- Android target ABI is `arm64-v8a` (ARMv8-A); other ABIs are not currently required.
- C++20 is a provisional implementation baseline, not a final accepted compiler-support policy.

## Accepted direction
- General-purpose C++ game engine.
- 2D and 3D are first-class.
- PC development through CI; Android target is `arm64-v8a`.
- DMod dynamic source execution with strict no-compilation/no-bytecode requirement.

## Not yet implemented
- Platform abstraction and application lifecycle.
- Memory/handles/resource identity.
- Renderer and graphics backend.
- ECS/world/scene model.
- 2D/3D physics integration.
- DMod package loader, sandbox/resource controls and interpreter integration (ChaiScript AST proof passes on Linux; Android compatibility remains to be checked).
- Asset pipeline, audio, input, serialization.
- Editor and profiling.

## Immediate next actions
1. Cross-compile the pinned ChaiScript AST proof for Android ARM64.
2. Research ChaiScript execution limits, sandbox boundaries and lifecycle safety before any dependency adoption.
3. Continue concurrency stress/lifecycle tests for event and job services.
4. Keep dependency choices open until technical and licensing audits are complete.
