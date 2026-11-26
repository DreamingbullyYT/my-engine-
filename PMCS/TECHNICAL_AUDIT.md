# Technical Audit

**Status:** Core services, typed resource identity/ownership, keyed ResourceStore, asset dependency planning, bounded async asset loading, CPU-side AssetCache, cooperative cancellation and in-flight request coordination are CI-verified. ChaiScript remains a proof-only candidate; no script runtime has been approved for production use.

## Core resource identity
- Initial typed handle design is a tag-typed (index, generation) pair. Tag types prevent accidental interchange between domains such as textures and buffers at compile time.
- HandlePool<Tag> synchronizes allocation, release and liveness checks; releasing increments the generation so stale handles are rejected after slot reuse.
- The pool tracks identity only: it does not own resource objects, define destruction order, or provide cross-pool identity. A registry/resource owner must define those semantics before resources are stored behind handles.
- Generation is 32-bit and skips zero; after a full generation wrap, an extremely old stale handle could theoretically alias. Evaluate wider generations or pool identity if lifetime requirements demand it.
- Handle implementation and tests passed in [run 45714411879](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45714411879).

## Resource ownership registry
- Initial ResourceRegistry<T, Tag> owns each resource through unique_ptr and couples it to a typed generational handle.
- Creation constructs the resource before reserving an identity; allocation/storage failures release any reserved handle.
- Access is callback-scoped under a shared lock; destruction takes the exclusive lock, invalidates the handle, then destroys the object before the slot can be reused. The lock prevents registry operations from observing the intermediate state. Handle invalidation occurs before destruction so a free-list allocation failure leaves the resource intact.
- Callbacks must not re-enter the same registry. The API does not expose raw pointers or references beyond the callback contract.
- Registry implementation and tests passed in [run 45715803430](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45715803430), including construction failure and exactly-once destruction. Callback-vs-destroy synchronization stress coverage remains desirable.

## Keyed resource store
- ResourceStore<T, Tag> maps case-sensitive, non-empty string keys (maximum 4096 bytes) to typed generational handles.
- Duplicate keys and invalid keys are rejected; callers can find a handle, erase a key, or access a resource through a scoped callback by key or handle.
- Key-map insertion uses an inaccessible placeholder before constructing the resource, so a failed resource construction removes the placeholder and does not leave an orphaned resource.
- Store and registry locks are acquired in a consistent outer-to-inner order; store callbacks must not re-enter the same store. ResourceRegistry enforces callback return type void and holds its shared lock for the callback duration.
- The store does not interpret keys as filesystem paths and performs no I/O, decoding, async work, dependency tracking, cache eviction or GPU upload. Those are separate asset-pipeline responsibilities.
- Linux GCC/Clang unit tests and Android ARM64 cross-compilation of the tests passed in [run 45716324493](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45716324493). Android result is compile-only, not device execution.

## Initial asset loading and dependency planning
- `load_async` validates the virtual path and key before I/O, requests a bounded read (64 MiB default, configurable), decodes a byte span on a JobSystem worker and registers the decoded value in ResourceStore.
- The filesystem, ResourceStore and JobSystem are borrowed; all must outlive the submitted task. Decoder execution is on a worker thread, so shared decoder state must be synchronized by its owner. Futures propagate thrown exceptions.
- `IFileSystem` implementations must check the file size against `max_bytes` before allocating the output buffer and report `FileError::TooLarge` when exceeded. The current fake filesystem test follows this contract; audit future concrete backends against it.
- The asset dependency planner is a pure deterministic Kahn topological sort. It rejects invalid/duplicate keys and duplicate dependency declarations, reports missing required/optional dependencies, and returns no partial order on cycles. It does not execute loads or manage resource lifetimes/cache. All four CI jobs passed in [run 45766502045](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45766502045).
- Bounded async loader tests passed in [run 45765620129](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45765620129).
- AssetCache is a CPU-side byte-budgeted LRU over ResourceStore. Size is caller-estimated, not measured; eviction is based on least-recently-used access through cache APIs. Shrinking the budget evicts until within budget. Construction/insertion exception safety and cache behavior passed all four jobs in [run 45768090059](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45768090059).
- `load_async_cached` checks for an existing cache key before file I/O, then bounded-reads, decodes and inserts with the supplied size estimator. It returns a distinct CacheHit status and reports cache budget/duplicate-key rejection. Tests prove cache hits avoid additional filesystem reads. All four jobs passed in [run 45768508344](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45768508344).
- Both loader paths accept an optional `AssetCancellationToken`, created by `AssetCancellationSource`. Cancellation is cooperative: checkpoints occur before I/O/cache hits, after file reads, after decoding, and (cached path) after size estimation. It cannot interrupt an active backend read or decoder/estimator callback. If cancellation is observed before registration/insertion, the result is `AssetLoadStatus::Cancelled` and no new resource is registered.
- Cancellation tests passed Linux GCC, Linux Clang, DMod AST proof and Android ARM64 cross-compilation in [run 45769042275](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45769042275). Android verification is compile-only, not physical-device execution.
- `AssetRequestCoordinator<Result>` keeps pending shared futures keyed by caller-provided request ID. `load_async_coordinated` and `load_async_cached_coordinated` use it to deduplicate concurrent uncached or cached load requests. The factory runs only for the first caller while that ID remains pending; followers receive the same `shared_future` and result. The test deliberately blocks the first decoder, starts a second request, and verifies one read, one decode and one shared handle. All four CI jobs passed in [run 45770215802](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45770215802).
- Completed entries are lazily pruned on the next request or by explicit `prune_completed()`. The coordinator is not a result cache; callers retaining shared futures can still read results after pruning.
- Request IDs are caller-defined. They must namespace resource type, source/path, content version or other identity dimensions as appropriate; reusing one ID for unrelated resources will incorrectly share work.
- The first caller owns the underlying factory, decoder and cancellation token. Later callers' factories and cancellation tokens are ignored while a matching request is pending, so independent per-subscriber cancellation is not supported. Cancellation remains cooperative and cannot interrupt an active read/decoder.
- The coordinator does not own JobSystem, filesystem, ResourceStore or AssetCache. Those borrowed services must outlive the task. Explicit service shutdown/lifetime policy remains open.

## Engine references
- Luanti: study runtime modding, API exposure, package lifecycle and server/client boundaries.
- O3DE: study modular engine architecture, components, buses and extensibility.
- Cocos2d-x: study C++ mobile engine structure and scripting integration.
These are references, not code-copy permissions or automatic adoption decisions.

## Rendering candidates
- bgfx: cross-platform rendering abstraction; verify API, backends, Android support, license and integration.
- Compare Vulkan/OpenGL ES and practical Android/desktop backends before selecting.
Do not select a backend until comparative research and a minimal Android ARM64 integration proof are complete.

## Physics candidates
- Jolt Physics: 3D rigid-body candidate; verify Android ARM64, license, features and adapter boundary.
- Box2D: 2D physics candidate; verify current release, API and license.
Do not expose third-party physics types throughout engine/game APIs.

## ECS candidates
- EnTT and Flecs: compare storage, queries, signals, reflection, serialization, scheduler integration, performance and Android build cost.

## DMod package dependencies

### miniz — pinned for ZIP archive access
- Upstream: richgel999/miniz.
- Version: 3.1.2.
- Immutable commit: 77d0dce8627735138c51770d1799a1ef48f2117d.
- Upstream LICENSE file identifies the project as MIT licensed.
- C implementation; ZIP APIs support reading an archive from memory and incremental file extraction. No external runtime dependency is needed for the selected build.
- The project CMake compiles miniz sources as a static library with stdio disabled.
- Dream Engine reads ZIP packages from caller-owned memory and never extracts archive paths to disk.
- Run [45708233794](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708233794) passed GCC, Clang and Android ARM64 cross-build plus Linux package-reader tests.
- Current protections: archive/per-file/total decompressed-size caps, compression-ratio cap, entry-count cap, path normalization, UTF-8 validation, traversal/backslash/drive rejection, case-collision rejection, Unix special-file and Windows reparse-point rejection, encryption and unsupported-method rejection, root manifest validation, full local-header/central-directory validation and CRC/decompression error reporting.
- Adversarial tests now cover exact duplicate paths, Unix symlink metadata, unsupported compression methods, malformed local headers and CRC/decompression failure.
- Remaining work: signature/trust policy, package-format versioning and further fuzzing.

### toml++ — pinned for manifest parsing
- Upstream: marzer/tomlplusplus.
- Version: 3.5.1.
- Immutable commit: efd80e03356af65c7d4e61c56ffb326b34f8d0f4.
- MIT license; header-only C++17 parser with ARM64 support.
- Integrated as an interface target; manifest parser caps input at 64 KiB and validates the expected fields.
- CI run [45706951573](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45706951573) passed GCC, Clang and Android ARM64 cross-build.

## DMod runtime candidates — highest risk

### ChaiScript — promising, not approved
- Audited upstream commit: f635b51e3f8e95868210edfc66fdbd8e475710f5.
- License: BSD-3-Clause; upstream documents C++17 baseline.
- ChaiScript_Basic::do_eval() parses source then evaluates returned AST nodes.
- Default Optimizer_Default can replace AST nodes with Compiled_AST_Node callable wrappers. These are not bytecode, but violate the strictest no-transformation interpretation.
- The pinned proof injects an identity optimizer and verifies no AST_Node_Type::Compiled nodes.
- Proof uses an AST evaluation step budget and explicit no-module/no-external-script/no-IO configuration.
- Run [45716324493](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45716324493) passed the DMod AST proof and Android ARM64 cross-compilation.
- This does not prove physical Android execution, memory quotas, safe native host bindings, unload safety or complete sandboxing. Step limits are not memory limits; exposed C++ host functions can block or mutate engine state. Do not integrate until these risks are addressed.

### Lua
Conventional Lua parses source into VM instructions. It does not meet the strict no-runtime-bytecode requirement unless the user relaxes that requirement.

### AngelScript
Its module workflows involve script compilation/bytecode and likely conflict with strict no-bytecode requirements. Do not select without source-level proof and approval.

### Custom AST interpreter
Fallback only if existing runtimes fail. It requires substantial language, parser, runtime, diagnostics, security and tooling maintenance.

## Required DMod proof
For each interpreter, inspect exact source/version:
- Does source parse to AST?
- Is AST interpreted directly?
- Is bytecode or another executable intermediate generated?
- Is native code generated?
- Can it bind C++ functions, objects and lifetimes safely?
- Can scripts reload and unload without stale references?
- Does it build and run on desktop and Android ARM64?
- What are binary size, memory, startup and execution costs?
- What security/resource limits exist?

## Additional subsystem research queue
- Audio: miniaudio, SDL audio, OpenAL Soft.
- Platform/window/input: SDL3 and native platform adapters.
- Internal tools UI: Dear ImGui; evaluate a separate touch-friendly runtime UI.
- Asset import: Assimp, cgltf, stb, KTX/Basis tooling.
- Navigation: Recast/Detour.
- Logging: spdlog.
- Profiling: Tracy.
- Serialization: nlohmann/json or another JSON parser plus a versioned save format.
- Networking: ENet or standalone Asio.

## License/provenance gate
Before adoption, pin repository, tag/commit and exact license files; inspect transitive dependencies and bundled assets; record modifications; preserve notices; confirm compatibility with distribution plans. The repository currently contains an MIT LICENSE; do not silently change it.
