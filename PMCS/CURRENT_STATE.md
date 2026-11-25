# Current State

**Last checked:** 2026-11-25  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project and `dream_core` static library.
- Public engine version/name API.
- Core logger with severity filtering and replaceable, thread-safe sink configuration.
- Structured diagnostic value type with severity, code, message and source.
- Dependency-free tests for version, logging filters/records and diagnostic classification.
- GitHub Actions workflow for Linux GCC/Clang build+CTest and Android NDK cross-build for `arm64-v8a`.

## Verified CI
- Run [45698607959](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45698607959) completed successfully.
- Linux GCC: configure, build and CTest passed.
- Linux Clang: configure, build and CTest passed.
- Android NDK: configure and cross-build passed for `arm64-v8a` (ARMv8-A).
- This is cross-compilation evidence, not execution on a physical Android device.
- The latest logging/diagnostics additions are pending a new CI run; do not treat them as verified until that run passes.

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
- Event system and job system.
- Renderer and graphics backend.
- ECS/world/scene model.
- 2D/3D physics integration.
- DMod package loader and interpreter.
- Asset pipeline, audio, input, serialization.
- Editor and profiling.

## Immediate next actions
1. Verify the new logger/diagnostic changes with GCC, Clang and Android ARM64 in Actions.
2. Add core event and job contracts after CI is green.
3. Research DMod interpreter candidates at source level, beginning with ChaiScript parser/evaluator behavior.
4. Keep dependency choices open until technical and licensing audits are complete.
