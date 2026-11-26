# Current State

**Last checked:** 2026-11-26  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main

## Implemented
- CMake project with separate dream_core, dream_platform and dream_dmod static libraries.
- Core version API, logger, diagnostics, typed event dispatcher, worker job system, typed generational handle pool, owning ResourceRegistry and keyed ResourceStore.
- Platform application lifecycle with initialization/update/shutdown callbacks, stop requests, frame delta clamping and cleanup on failure.
- Backend-neutral window configuration/metrics/events and opaque window IDs.
- Native render-surface bridge with borrowed native handles and opaque surface IDs.
- Input event contract for USB HID keyboard usages, Unicode text, mouse/touch/pen pointers, scrolling and gamepad controls.
- Virtual filesystem contract with UTF-8 relative paths, traversal rejection and portable filename validation.
- DMod manifest model, validation, deterministic dependency resolver and tests.
- TOML manifest parser with format-version check, strict MAJOR.MINOR.PATCH parsing, dependency/capability parsing and a 64 KiB input cap.
- Read-only in-memory ZIP package reader. It does not extract package paths to disk, validates all local ZIP headers against the central directory, validates UTF-8 paths and case-collisions, rejects links/encryption/unsupported methods, enforces archive/per-entry/total-size and compression-ratio limits, reads the root manifest and checks the declared entry script exists.
- Pinned build dependencies: miniz 3.1.2 and toml++ 3.5.1, each fetched from an immutable upstream commit.
- GitHub Actions for Linux GCC/Clang, Android ARM64 cross-build, ChaiScript AST proof and Android cross-compilation of selected header-only tests.
- Initial header-only dream_assets module: asynchronous virtual-file read, caller-provided decoder, and ResourceStore registration. Implementation and tests are pending CI; it has no cache/dependency graph/GPU upload.

## Verified CI
- Run [45758612936](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45758612936) completed successfully for all four jobs. It includes deterministic ResourceRegistry callback-vs-destroy synchronization coverage.
- Run [45716324493](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45716324493) completed successfully for all four jobs and verifies ResourceStore Linux tests plus Android ARM64 test cross-compilation.
- Run [45715803430](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45715803430) completed successfully for ResourceRegistry and all four CI jobs.
- Run [45714411879](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45714411879) completed successfully for typed handles and all four CI jobs.
- Run [45710274229](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45710274229) completed successfully for window/surface/input/filesystem contracts and all four CI jobs.
- Run [45708669112](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708669112) completed successfully for application lifecycle and all four CI jobs.
- Android results are NDK cross-compilation for arm64-v8a (ARMv8-A), not execution on a physical device.

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
- Renderer, ECS/world, physics, asset dependency graph/cache policy/GPU upload, audio, serialization, editor and profiling. Initial async asset read/decode/register helper is implemented but pending CI; ResourceStore itself remains only a key-to-handle layer.

## Immediate next actions
1. Verify the initial async asset loader in Linux GCC/Clang tests and Android ARM64 cross-compilation; review borrowed-service lifetime and decoder thread-safety contracts.
2. Extend the asset layer with explicit cancellation/lifetime, dependency graph and cache policy only after the initial contract is verified.
3. Compare renderer candidates and define an engine-owned 2D/3D API without prematurely locking a backend.
4. Continue DMod package trust/signatures, capability enforcement and interpreter integration/security review.
