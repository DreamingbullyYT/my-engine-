# Current State

**Last checked:** 2026-11-26  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project with separate dream_core, dream_platform and dream_dmod static libraries.
- Core version API, logger, diagnostics, typed event dispatcher, worker job system and typed generational handle pool (verified in run 45714411879) and owning ResourceRegistry (awaiting CI).
- Platform application lifecycle with initialization/update/shutdown callbacks, stop requests, frame delta clamping and cleanup on failure.
- Backend-neutral window configuration/metrics/events and opaque window IDs.
- Native render-surface bridge with borrowed native handles and opaque surface IDs.
- Input event contract for USB HID keyboard usages, Unicode text, mouse/touch/pen pointers, scrolling and gamepad controls.
- Virtual filesystem contract with UTF-8 relative paths, traversal rejection and portable filename validation.
- DMod manifest model, validation, deterministic dependency resolver and tests.
- TOML manifest parser with format-version check, strict MAJOR.MINOR.PATCH parsing, dependency/capability parsing and a 64 KiB input cap.
- Read-only in-memory ZIP package reader. It does not extract package paths to disk, validates all local ZIP headers against the central directory, validates UTF-8 paths and case-collisions, rejects links/encryption/unsupported methods, enforces archive/per-entry/total-size and compression-ratio limits, reads the root manifest and checks the declared entry script exists.
- Pinned build dependencies: miniz 3.1.2 and toml++ 3.5.1, each fetched from an immutable upstream commit.
- GitHub Actions for Linux GCC/Clang, Android ARM64 cross-build and ChaiScript AST proof.

## Verified CI
- Run [45710274229](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45710274229) completed successfully for all four jobs: Linux GCC, Linux Clang, DMod AST proof and Android ARM64 cross-build.
- This run verifies the window/surface/input/filesystem contracts and their Linux tests; Android remains cross-compilation, not physical-device execution.
- Run [45708669112](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708669112) completed successfully for commit 0706b05976fd5cb03ffd3d2fa2d169ac099aa478.
- Linux GCC and Clang: configure, build and all CTest tests passed, including application lifecycle tests.
- Android NDK: engine core, DMod, miniz and application lifecycle code cross-build passed for arm64-v8a (ARMv8-A).
- ChaiScript direct AST proof passed on Linux and cross-compiled for Android ARM64.
- Android proof is cross-compilation, not physical-device execution.

## Pending verification
- Owning ResourceRegistry and its CTest coverage have been added after handle verification; wait for CI before marking registry work verified.

## Dependency pins
- miniz: richgel999/miniz, commit 77d0dce8627735138c51770d1799a1ef48f2117d, upstream LICENSE is MIT.
- toml++: marzer/tomlplusplus, commit efd80e03356af65c7d4e61c56ffb326b34f8d0f4, MIT.
- ChaiScript remains a proof-only candidate, not an adopted runtime.

## Build and test policy
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Android target ABI is arm64-v8a (ARMv8-A).
- C++20 is a provisional implementation baseline, not a final compiler-support decision.

## Accepted direction
- General-purpose C++ engine; 2D and 3D are first-class.
- DMod source/AST execution only: no native compilation and no generated runtime bytecode.
- Native C++ plugins are not the ordinary DMod format.
- Platform contracts must not require a particular windowing or graphics backend.

## Remaining DMod work
- Package signature/trust policy and explicit package-format versioning.
- Interpreter integration, safe host bindings, per-mod context and enforced capabilities.
- Runtime memory/time quotas and robust sandbox isolation.
- Hooks/registries, lifecycle, unload/reload and diagnostics integration.

## Other engine work not yet implemented
- Concrete desktop/Android platform backends and physical-device tests.
- Renderer, ECS/world, physics, asset pipeline, audio, serialization, editor and profiling. Handle identity has an initial implementation; resource ownership/registry remain unimplemented.

## Immediate next actions
1. Verify typed generational handles in Linux GCC/Clang and Android ARM64 CI.
2. Verify ResourceRegistry, then extend resource lifetime/concurrency tests.
3. Compare renderer candidates and define an engine-owned 2D/3D API without locking a backend prematurely.
4. Continue DMod signature/trust, format-versioning and interpreter security work.
