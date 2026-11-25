# Current State

**Last checked:** 2026-11-25  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project with separate dream_core and dream_dmod static libraries.
- Core version API, logger, diagnostics, typed event dispatcher and worker job system.
- DMod manifest model, validation, deterministic dependency resolver and tests.
- TOML manifest parser with format-version check, strict MAJOR.MINOR.PATCH parsing, dependency/capability parsing and a 64 KiB input cap.
- Read-only in-memory ZIP package reader. It does not extract package paths to disk, validates all local ZIP headers against the central directory, validates UTF-8 paths and case-collisions, rejects links/encryption/unsupported methods, enforces archive/per-entry/total-size and compression-ratio limits, reads the root manifest and checks the declared entry script exists.
- Pinned build dependencies: miniz 3.1.2 and toml++ 3.5.1, each fetched from an immutable upstream commit.
- GitHub Actions for Linux GCC/Clang, Android ARM64 cross-build and ChaiScript AST proof.

## Verified CI
- Run [45708233794](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708233794) completed successfully for commit b7afa2582ce0d217f23ae9f2fd05450613e98f97.
- Linux GCC and Clang: configure, build and all CTest tests passed, including package reader adversarial tests.
- Android NDK: engine core, DMod and miniz cross-build passed for arm64-v8a (ARMv8-A).
- ChaiScript direct AST proof passed on Linux and cross-compiled for Android ARM64.
- Android proof is cross-compilation, not physical-device execution.

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

## Remaining DMod work
- Package signature/trust policy and explicit package-format versioning.
- Interpreter integration, safe host bindings, per-mod context and enforced capabilities.
- Runtime memory/time quotas and robust sandbox isolation.
- Hooks/registries, lifecycle, unload/reload and diagnostics integration.

## Other engine work not yet implemented
- Platform abstraction, application lifecycle, handles/resources, renderer, ECS/world, physics, asset pipeline, audio, input, serialization, editor and profiling.

## Immediate next actions
1. Define signature/trust and package-format versioning policy.
2. Continue ChaiScript runtime, lifecycle and security audit before adopting it.
3. Proceed with engine platform/application and resource foundations without locking renderer/ECS/physics prematurely.
