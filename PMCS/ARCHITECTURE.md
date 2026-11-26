# Architecture

**Status:** Core services, application lifecycle, backend-neutral platform contracts, initial DMod package pipeline, typed generational handles, owning ResourceRegistry, keyed ResourceStore, asset dependency planning, bounded async loading and optional AssetCache integration are CI-verified.

## High-level modules
- Platform: desktop and Android lifecycle, windows/surfaces, input, files, threads and OS integration.
- Core: memory/handles, jobs, events, logging, diagnostics, serialization and resource identity.
- World: scenes, entities, components, transforms and world lifecycle.
- Rendering: engine-owned API with 2D and 3D render paths.
- Simulation: 2D/3D physics, animation and navigation through replaceable adapters.
- Runtime services: input actions, audio, UI, networking and localization as optional capabilities.
- DMod: package validation, dependency resolver, source interpreter, bindings, hooks, lifecycle, permissions and hot reload.
- Game project: game-specific systems, content, assets, UI and scripts.
- Tools/editor: editor and debugging tools are not required by runtime.

## Core contracts implemented
- Engine identity/version API.
- Logger with severity filtering and replaceable sink; callbacks run outside configuration locks.
- Structured diagnostic value.
- Typed event dispatcher with RAII connections, priority ordering, stable registration order, snapshot dispatch, re-entrancy and per-listener exception isolation.
- Job system with worker pool, futures, exception propagation, idle wait and drain-on-shutdown. Lifecycle operations are not safe to call concurrently or from a worker thread.
- No process-global singleton is imposed; callers own service instances.
- Typed generational handles use tag types to prevent cross-domain type mixing; a synchronized handle pool detects stale handles after release/reuse.
- ResourceRegistry<T, Tag> owns resources with unique_ptr, provides scoped callback access under a shared lock, rejects stale handles and safely invalidates identities before destruction. Callbacks must not re-enter the same registry; do not retain resource references outside the callback.
- ResourceStore<T, Tag> adds a case-sensitive string-key mapping over ResourceRegistry, with insert/find/erase and scoped access by key or handle. It is intentionally not a loader: file I/O, decoding, async scheduling, dependency tracking and GPU upload belong to higher-level asset systems. Store callbacks must not re-enter the same store.
- ResourceStore Linux GCC/Clang tests and Android ARM64 test cross-compilation passed in [run 45716324493](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45716324493). Android is compile-only, not physical-device execution.
- `dream_assets::load_async` composes JobSystem, bounded IFileSystem reads, a caller-provided decoder and ResourceStore. `load_async_cached` is an optional sibling path targeting AssetCache; it checks for an existing key before file I/O and takes a caller-provided decoded-size estimator. AssetCache is a CPU-side byte-budgeted LRU; it does not measure allocations or manage GPU residency. Borrowed services must outlive the task and callbacks run on a worker thread. Both paths passed all four CI jobs in [run 45768508344](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45768508344).
- ResourceRegistry callback-vs-destroy synchronization test passed with all four CI jobs in [run 45758612936](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45758612936).

## Platform contracts
- Application lifecycle owns initialization, frame polling/update and shutdown sequencing; stop requests are atomic and frame delta is clamped.
- Window contract uses opaque IDs, logical/pixel dimensions, DPI scale, focus/minimized state and lifecycle events.
- Surface contract separates a window from its native render surface. Native display/window pointers are borrowed and valid only until the surface is released or platform surface is destroyed.
- Input contract separates physical keyboard usage IDs (USB HID Keyboard/Keypad), Unicode text, mouse/touch/pen pointer identity, scroll and normalized gamepad axes.
- Filesystem contract uses UTF-8 relative virtual paths rooted by the backend, rejects traversal/absolute paths and invalid/nonportable names, and exposes read/write/existence/directory operations.
- These are interfaces only; no concrete SDL, Win32, X11, Wayland, Android or Apple backend has been selected or implemented.
- Window/surface/input/filesystem contracts and virtual-path tests passed Linux GCC, Linux Clang, DMod AST proof and Android ARM64 cross-build in [run 45710274229](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45710274229). Android is cross-compiled, not device-tested.
- The typed handle pool and owning ResourceRegistry are verified in [run 45715295194](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45715295194). Android is cross-compiled, not device-tested.

## Asset dependency planning
- `resolve_dependencies` accepts an in-memory set of asset definitions and returns deterministic dependency-first order using lexical tie-breaking.
- Required missing dependencies fail the plan; missing optional dependencies are reported but do not block it. Invalid/duplicate asset keys and duplicate dependency declarations are rejected; cycles return blocked asset keys and no partial load order.
- This is a pure planning layer: it does not load, own, cache, cancel or unload assets. All four CI jobs passed in [run 45766502045](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45766502045).

## Asset cache
- `AssetCache<T, Tag>` wraps ResourceStore with a byte-budgeted least-recently-used policy for decoded CPU assets. Callers provide estimated byte sizes; zero-byte assets are valid, and shrinking the budget evicts least-recently-used entries.
- Cache construction/insertion is exception-safe; cache callbacks execute under its mutex and must not re-enter the same cache. No pinning, automatic memory measurement, file I/O, decoding or GPU residency is provided.
- LRU cache behavior and exception safety passed all CI jobs in [run 45768090059](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45768090059).

## DMod contracts implemented and verified
- Manifest model includes package ID/name/version, engine API range, relative entry script, dependencies and requested capabilities.
- Validation rejects invalid IDs, unsafe paths (absolute paths, traversal, backslashes, drive-like paths and trailing separators), duplicate dependencies/capabilities and invalid version ranges.
- TOML parser accepts manifest format version 1, parses strict three-part numeric versions, dependency ranges and capabilities, and caps manifest text at 64 KiB.
- Deterministic resolver checks engine API compatibility, duplicate installed IDs, required dependency presence/version, optional dependencies and cycles.
- Read-only in-memory ZIP reader indexes package files without filesystem extraction, requires root manifest.toml, validates every local header against central-directory metadata, validates relative UTF-8 paths, rejects traversal/case-collisions/links/encryption/unsupported methods, applies configured archive/per-entry/total-size and compression-ratio limits, parses the manifest and checks that its entry script exists.
- File bytes are read into bounded memory buffers and CRC/decompression errors are returned to the caller.
- Linux GCC/Clang tests and Android ARM64 cross-build passed in [run 45708233794](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708233794).
- Package tests cover valid packages, traversal, duplicate/case-colliding paths, Unix symlink metadata, unsupported compression, malformed local headers, entry count and size/ratio limits, missing entry scripts, invalid archives and CRC/decompression failures.
- Package signature/trust, interpreter execution and capability enforcement remain future work.

## DMod package proposal
- .dmod is planned as a ZIP container with a root manifest.toml and relative paths for scripts/assets.
- The package reader is read-only and must not extract untrusted paths to the filesystem.
- The reader validates archive metadata, local headers, entry types, UTF-8 paths, duplicate/case-colliding paths, entry count, compressed/uncompressed sizes and compression ratios before exposing entries.
- Only stored and deflate entries are supported initially. Encryption, links, absolute paths and traversal are rejected.
- The current in-memory reader is a first implementation; package signature/trust and versioning remain pending.

## Dependency direction
Platform adapters implement platform interfaces. Backends implement engine-owned rendering/physics/audio interfaces. Game systems depend on public engine APIs. DMods depend on versioned DMod APIs and granted capabilities. Core must not depend on a particular game or backend.

## 2D/3D coexistence
2D and 3D may use specialized pipelines where semantics differ. Share resource identity, asset management, scheduling and diagnostics where appropriate. A project may use 2D, 3D or both.

## DMod package lifecycle
1. Discover package.
2. Validate archive, manifest, paths, identity and declared API version.
3. Resolve dependencies and conflicts.
4. Determine deterministic load order.
5. Create mod context and granted capabilities.
6. Parse and interpret source directly as AST (no bytecode generation).
7. Register hooks, events, components, systems and resources.
8. Enable mod and report diagnostics.
9. On unload/reload, unregister owned resources and isolate failures.

## Hook model
Plan for before/after/around/replace hooks, explicit priorities, deterministic ordering, recursion protection, error isolation and API-version checks. Do not promise arbitrary patching of every private C++ function.

## Source execution contract
DMod runtime reads script source and evaluates its AST directly. Conventional VM bytecode, native compilation, WASM modules and native plugins are not ordinary zero-compilation source mods.

## API stability
- Stable: documented contracts intended for broad mod compatibility.
- Advanced: deeper access with explicit compatibility warnings.
- Experimental: unstable internal-facing hooks and APIs.
Never make every private implementation symbol a permanent public contract.

## Build and CI baseline
- CMake is the project build system; GitHub Actions is the primary build/test environment.
- Linux CI builds and runs tests with GCC and Clang.
- Android CI cross-compiles with NDK for arm64-v8a (ARMv8-A); this is not physical-device runtime testing.
- C++20 is provisional; final compiler support remains open.

## Proposed repository layout
```
/CMakeLists.txt
/cmake/
/engine/core/
/engine/platform/
/engine/world/
/engine/render/
/engine/simulation/
/engine/runtime/
/engine/dmod/
/tools/
/tests/
/third_party/
/examples/
/PMCS/
```

## Build direction
Keep dependency versions pinned and builds reproducible. Do not lock renderer, ECS, physics or scripting dependencies before comparative research and CI proof.
