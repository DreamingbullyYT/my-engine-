# AI Handoff

## Project
Dream Engine — general-purpose C++ engine, first-class 2D/3D, Android ARM64 target, dynamic DMod runtime.

## Read order
1. PROJECT_MEMORY.md
2. CURRENT_STATE.md
3. RULES.md
4. DECISIONS.md
5. ARCHITECTURE.md
6. TECHNICAL_AUDIT.md
7. ROADMAP.md

## Non-negotiable direction
- C++ is the engine language. Do not switch to Rust.
- Keep the engine reusable; game-specific systems belong in game projects.
- 2D and 3D are both first-class.
- The user has no PC available: use GitHub Actions for builds and tests.
- Android ABI target: arm64-v8a (ARMv8-A).
- Ordinary DMods must not require native compilation, engine/game/APK rebuild, or generated runtime bytecode.
- Prove interpreter behavior from source and tests; do not make unsupported claims.
- Preserve the root MIT license; record third-party notices separately.
- Update PMCS whenever implementation state or verified decisions change.

## Current implementation
- ResourceRegistry<T, Tag> owns resources through unique_ptr, provides callback-scoped access and rejects stale handles; tests passed in [run 45715803430](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45715803430).
- Initial ResourceStore<T, Tag> maps case-sensitive string keys to typed handles, supports find/erase and scoped access; implementation/tests are awaiting CI. It intentionally does not load or decode assets.
- dream_core: version API, logging, diagnostics, event dispatcher, worker job system, typed generational handle pool and owning ResourceRegistry (verified in run 45715295194).
- dream_platform: application lifecycle, window/surface/input contracts and virtual filesystem path validation.
- dream_dmod: manifest model, validation, dependency resolver, TOML parser and read-only in-memory ZIP package reader with local-header validation.
- Pinned dependencies: miniz 3.1.2 at 77d0dce8627735138c51770d1799a1ef48f2117d; toml++ 3.5.1 at efd80e03356af65c7d4e61c56ffb326b34f8d0f4.
- Application lifecycle passed Linux GCC/Clang and Android ARM64 cross-build in [run 45708669112](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708669112).
- Window/surface/input/filesystem contracts passed all four CI jobs in [run 45710274229](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45710274229).
- Android results are cross-compilation, not physical-device execution.
- C++20 remains a provisional implementation baseline.

## Next work
1. Verify ResourceStore and expand resource lifetime/concurrency tests.
2. Design a separate asset loading pipeline for file I/O, decoding, async scheduling and dependency tracking.
3. Research renderer candidates and define engine-owned 2D/3D rendering interfaces before selecting a backend.
4. Continue DMod package trust/versioning and interpreter security audit.

## Handoff requirements
Update CURRENT_STATE.md, ARCHITECTURE.md, TECHNICAL_AUDIT.md and ROADMAP.md as work progresses. Update DECISIONS.md only for explicitly accepted decisions. Never label work VERIFIED without reproducible evidence.
