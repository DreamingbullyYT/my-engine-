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
- ResourceStore<T, Tag> maps case-sensitive string keys (non-empty, maximum 4096 bytes) to typed generational handles. Duplicate/invalid keys are rejected; find/erase and callback-scoped access by key or handle are supported. Linux tests and Android ARM64 cross-compilation passed in [run 45716324493](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45716324493).
- ResourceStore is not an asset loader; it remains a key-to-handle owner.
- `dream_assets::load_async` schedules bounded virtual filesystem reads on JobSystem (64 MiB default limit), decodes on a worker and registers in ResourceStore. `load_async_cached` is an optional sibling path that checks AssetCache before I/O and uses a caller-provided decoded-size estimator. Both paths accept optional cooperative cancellation tokens and report `AssetLoadStatus::Cancelled` at checkpoints. Cancellation cannot interrupt an active read/decoder/estimator callback. Verified across all four CI jobs in [run 45769042275](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45769042275). Borrowed services must outlive the task; decoder/estimator thread safety is caller-owned.
- `AssetRequestCoordinator<Result>` plus `load_async_coordinated` and `load_async_cached_coordinated` share one future/result for matching pending request IDs. Completed entries are lazily pruned; this is not a result cache. Request IDs must encode all relevant identity (type/source/version). The first caller owns the decoder and cancellation token; followers cannot independently cancel the shared operation. Verified all four CI jobs in [run 45770215802](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45770215802).
- Asset dependency planner is implemented in `engine/assets/include/dream/assets/dependency_graph.hpp`; deterministic ordering, missing dependency reporting and cycle detection passed all four CI jobs in [run 45766502045](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45766502045).
- `AssetCache<T, Tag>` provides CPU-side byte-budgeted LRU storage; cache tests and exception-safety checks passed all four CI jobs in [run 45768090059](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45768090059).
- dream_core: version API, logging, diagnostics, event dispatcher, worker job system, typed generational handle pool, ResourceRegistry and ResourceStore.
- dream_platform: application lifecycle, window/surface/input contracts and virtual filesystem path validation.
- `DreamEngine::World`: generational Entity handles, typed component pools, safe scoped queries, parent-child hierarchy with cycle checks, Transform2D/Transform3D and matrix composition; `Scene` owns World + three-phase SystemScheduler. Run [45772233089](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45772233089) passed 4/4 jobs.
- `DreamEngine::Serialization`: DENG versioned little-endian archive, UTF-8 validation and size bounds; explicit stable component codec IDs/versions; staged World restore; Scene name + World save/load. Runtime system callbacks are deliberately not serialized. Runs [45772640872](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45772640872), [45773296150](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45773296150), [45773898548](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45773898548) passed 4/4 jobs each.
- `DreamEngine::Input`: device-independent gameplay actions for HID keyboard/modifiers, mouse buttons and specific gamepad device buttons/axes; edge state, deadzones, disconnect release and global reset. Runs [45774213362](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45774213362) and [45774483591](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45774483591) passed 4/4 jobs each.
- dream_dmod: manifest model, validation, dependency resolver, TOML parser and read-only in-memory ZIP package reader with local-header validation.
- Pinned dependencies: miniz 3.1.2 at 77d0dce8627735138c51770d1799a1ef48f2117d; toml++ 3.5.1 at efd80e03356af65c7d4e61c56ffb326b34f8d0f4.
- Application lifecycle passed Linux GCC/Clang and Android ARM64 cross-build in [run 45708669112](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708669112).
- Window/surface/input/filesystem contracts passed all four CI jobs in [run 45710274229](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45710274229).
- Android results are cross-compilation, not physical-device execution.
- C++20 remains a provisional implementation baseline.

## Next work
1. Define explicit async service shutdown/lifetime behavior and independent subscriber cancellation semantics for coordinated requests.
2. Research renderer candidates and prove a minimal Android ARM64 backend integration before committing to one; create the engine-owned 2D/3D renderer and GPU-residency layer.
3. Build camera/view math and 2D/3D render pipelines after the backend-neutral design is tested.
4. Compare 2D/3D physics candidates and add deterministic adapter tests.
5. Continue DMod package trust/versioning and interpreter capability/sandbox audit.
6. Add physical Android runtime tests when a device runner is available.

## Handoff requirements
Update CURRENT_STATE.md, ARCHITECTURE.md, TECHNICAL_AUDIT.md and ROADMAP.md as work progresses. Update DECISIONS.md only for explicitly accepted decisions. Never label work VERIFIED without reproducible evidence.
