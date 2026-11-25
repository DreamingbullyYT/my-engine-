# Current State

**Last checked:** 2026-11-25  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented files (CI result pending)
- `CMakeLists.txt`: CMake project and `dream_core` static library.
- `engine/core/include/dream/core/version.hpp`: initial public version/name API.
- `engine/core/src/version.cpp`: version string implementation.
- `tests/core/version_tests.cpp`: dependency-free core smoke test.
- `.github/workflows/ci.yml`: Linux GCC/Clang build+CTest and Android NDK cross-build for `arm64-v8a`.

## Build and test policy
- The user has no PC available. GitHub Actions is the primary build/test environment; do not make local-PC steps a prerequisite.
- Android target ABI is `arm64-v8a` (ARMv8-A). Other Android ABIs are not currently required.
- Linux CI verifies desktop compilation and executable tests.
- Android CI currently verifies an NDK cross-compile of the engine core; it does not yet run on a physical Android device.
- Initial C++20 usage is a provisional implementation baseline, not a final accepted compiler-support policy.
- No CI result has been observed yet; the new workflow and code must not be called verified until Actions completes successfully.

## Accepted direction
- General-purpose C++ game engine
- 2D and 3D are first-class
- PC development through CI; Android target is `arm64-v8a`
- DMod dynamic source execution with strict no-compilation/no-bytecode requirement

## Not yet implemented or verified
- Successful Linux/Android CI run
- C++ standard/compiler support policy
- Platform abstraction and application lifecycle
- Logging, diagnostics, memory/handles, jobs and events
- Renderer and graphics backend
- ECS/world/scene model
- 2D/3D physics integration
- DMod package loader and interpreter
- Asset pipeline, audio, input, serialization
- Editor and profiling

## Immediate next actions
1. Inspect the first Actions run and fix all real build/test failures.
2. Add CI coverage for Android ARM64 without treating cross-compilation as device-runtime proof.
3. Research and prototype DMod source interpretation candidates, starting with source-level inspection.
4. Build the next reusable core services only after CI is green.
