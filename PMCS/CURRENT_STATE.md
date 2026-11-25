# Current State

**Last checked:** 2026-11-25  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project with separate dream_core and dream_dmod static libraries.
- Core version API, logger, diagnostics, typed event dispatcher and worker job system.
- DMod manifest data model: ID, name, semantic version, engine API range, entry script, dependencies and requested capabilities.
- Manifest validation for IDs, metadata bounds, safe relative entry paths, traversal, duplicate dependencies/capabilities and version ranges.
- Deterministic dependency resolver with required/optional dependencies, version checks, duplicate package detection, missing dependency errors, cycle detection and stable topological ordering.
- Tests for core services and DMod manifest/resolver contracts.
- GitHub Actions for Linux GCC/Clang, Android ARM64 cross-build and ChaiScript AST proof.

## Verified CI
- Run [45705412340](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45705412340) completed successfully for commit `21362dfbb0af28125857c78fe074b72c82b2049d`.
- Linux GCC: configure, build and CTest passed.
- Linux Clang: configure, build and CTest passed.
- Android NDK: engine core and DMod cross-build passed for arm64-v8a (ARMv8-A).
- ChaiScript direct AST proof passed on Linux and cross-compiled for Android ARM64. The proof evaluates a script to 42, confirms no Compiled AST nodes, and checks the AST step-budget interruption path.
- Android proof is cross-compilation, not physical-device execution.

## Build and test policy
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Android target ABI is arm64-v8a (ARMv8-A).
- C++20 is a provisional implementation baseline, not a final compiler-support decision.

## Accepted direction
- General-purpose C++ engine; 2D and 3D are first-class.
- DMod source/AST execution only: no native compilation and no generated runtime bytecode.
- Native C++ plugins are not the ordinary DMod format.

## Not yet implemented
- .dmod archive reader, secure archive limits, manifest text parser and signature/trust policy.
- Interpreter integration, safe host bindings, per-mod context and enforced capabilities.
- Runtime memory/time quotas and robust sandbox isolation.
- Hooks/registries, lifecycle, unload/reload and diagnostics integration.
- Platform abstraction, application lifecycle, handles/resources, renderer, ECS/world, physics, asset pipeline, audio, input, serialization, editor and profiling.

## Immediate next actions
1. Define and test the .dmod package contract and manifest text format.
2. Audit candidate archive/parser libraries for license, Android ARM64 support and security controls before adoption.
3. Continue ChaiScript runtime, lifecycle and security audit before adopting it.
4. Implement DMod context/capability enforcement after package validation contracts stabilize.
