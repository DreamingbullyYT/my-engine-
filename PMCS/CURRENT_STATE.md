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
- Run [45700477183](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45700477183) completed successfully after the logger/diagnostics and independent-test-target changes.
- Linux GCC: configure, build and CTest passed.
- Linux Clang: configure, build and CTest passed.
- Android NDK: configure and cross-build passed for `arm64-v8a` (ARMv8-A).
- This is cross-compilation evidence, not execution on a physical Android device.
- Event/job implementation has been pushed and is awaiting validation in the current CI run; do not mark it verified until the run passes.

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
- DMod package loader and interpreter.
- Asset pipeline, audio, input, serialization.
- Editor and profiling.

## Immediate next actions
1. Verify event and job services with GCC, Clang and Android ARM64 in Actions.
2. Review concurrency edge cases and add stress/lifecycle tests where useful.
3. Research DMod interpreter candidates at source level, beginning with ChaiScript parser/evaluator behavior.
4. Keep dependency choices open until technical and licensing audits are complete.
