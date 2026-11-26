# Technical Audit

**Status:** Core services, World/ECS, async assets, Physics2D, backend-neutral rendering, EngineRuntime and native in-process DMod loading are CI-verified. Native mods intentionally have no sandbox or capability enforcement. ChaiScript remains a proof-only candidate for a separate script lane.

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
- `IFileSystem` implementations must check the file size against `max_bytes` before allocating the output buffer and report `FileError::TooLarge` when exceeded. NativeFileSystem now uses fstat before allocating and enforces the limit again while reading to handle concurrent file growth. Its rooted openat/O_NOFOLLOW traversal prevents symlink traversal through both parent components and final file opens. Writes use same-directory temporary files and atomic rename; list operations are sorted, omit symlink/special entries and support the root via an empty path. Its behavior/security tests passed Linux GCC/Clang and the source compiled in Android arm64-v8a CI (run 45816961428). The configured root path itself remains trusted setup input; this is a POSIX backend, not a complete Android window/lifecycle/input adapter.
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
- `AssetLoadScope` adds deterministic task lifetime management for callers that borrow JobSystem/filesystem/store/cache. Each `acquire_token()` creates a shared lease; move the token into `load_async` or `load_async_cached`. The async load callable releases the lease on every exit path. `shutdown_and_wait()` closes new admissions, signals cancellation and waits for active leases to reach zero. The test blocks a filesystem read, begins shutdown, verifies shutdown cannot return while the read is blocked, releases the read, and confirms the cancelled asset is not registered. All four Linux GCC, Linux Clang, Android ARM64 cross-compile and DMod AST proof jobs passed in [run 45818494612](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45818494612).
- The scope does not own the borrowed services or interrupt active I/O/decoder code. Those services must outlive the scope; declare the scope after them so it is destroyed first. Do not call blocking shutdown from one of its own tasks. A token copy retains its shared lease until all copies are released.
- The coordinator does not own JobSystem, filesystem, ResourceStore or AssetCache. Those borrowed services must outlive the task. Explicit service shutdown/lifetime policy remains open.

## World, ECS and system scheduler
- `World` owns generational entity handles and component pools keyed internally by `std::type_index`. Each component pool checks entity generation, so index reuse does not expose a stale component.
- Access is callback-scoped under the World shared/exclusive mutex. Callbacks must not re-enter the same World; retaining component references after callback return is invalid.
- Hierarchy data is stored in the World rather than only as a user component, allowing cycle validation, reparenting and entity-destruction cleanup under one lock. Parent destruction detaches direct children. Ancestor walks reject stale links and cycle attempts.
- Transform composition uses column-major matrices with column vectors. This preserves shear from rotated children beneath non-uniformly scaled parents. `Transform3D` expects normalized quaternions; normalization/validation of each arbitrary component value is not automatically enforced.
- Scheduler ordering is phase, descending priority, then registration order. A snapshot provides deterministic add/remove behavior during ticks. Exceptions are converted to per-system failures. Recursive/concurrent update calls are rejected via atomic guard.
- Linux GCC/Clang tests and Android ARM64 cross-compilation passed in [run 45772233089](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45772233089).

## Binary archive and World/Scene persistence
- Binary primitives are explicitly little-endian. DENG archive header carries a major/minor version; reader accepts only the same major and a file minor not newer than the supported minor. Size limits are configurable.
- UTF-8 is validated, including overlong sequences, surrogates, truncated sequences and out-of-range code points. Booleans accept only encoded 0/1. Error states are sticky.
- World persistence uses a codec registry with stable project-defined 64-bit type IDs and explicit component versions. C++ `type_index` and implementation-defined RTTI names are not serialized.
- World save emits entities deterministically by live handle index and writes archive-local parent IDs. Load allocates new runtime handles; old handles are intentionally not stable across restore.
- Components without registered codecs cause save failure to prevent silent data loss. Unknown saved component IDs are rejected by default; explicit option can skip them. Component versions newer than registered codec are rejected; backwards migration must be implemented in codec decoder.
- Loading is staged into a temporary World; partial worlds are never returned. Entity references inside user-defined components are not automatically remapped; codecs must define a stable representation and resolve references explicitly.
- World save currently enumerates/accesses components in multiple locked operations. Caller must ensure no concurrent structural/component mutations while snapshotting; an atomic whole-world snapshot/transaction is not yet implemented.
- Scene archives wrap scene name + World archive. System callbacks are omitted because they are code/runtime objects and must be registered by game code after load.
- All four CI jobs passed binary archive, World archive and Scene archive in [run 45772640872](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45772640872), [run 45773296150](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45773296150) and [run 45773898548](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45773898548).

## Gameplay ActionMap
- Action names are ASCII-safe identifiers. Bindings can target HID keyboard usage IDs, mouse buttons or a specific gamepad device ID plus control.
- Keyboard modifiers match exactly on key down; key-up clears a held binding even if modifier state changed before release.
- Digital and analog contributions are combined; action down is digital-held or analog magnitude >= 0.5. `begin_frame()` clears transient edges; analog state otherwise stays latched at last event.
- Axis deadzone is clamped through validation (0 <= deadzone < 0.95), then remaining range is rescaled. Binding scale supports inversion.
- Gamepad disconnect affects only gamepad bindings belonging to that device. `reset_all()` is intended for app focus loss/system input reset.
- Tests and Android ARM64 cross-compilation passed in [run 45774213362](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45774213362) and [run 45774483591](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45774483591).

## Engine references
- Luanti: study runtime modding, API exposure, package lifecycle and server/client boundaries.
- O3DE: study modular engine architecture, components, buses and extensibility.
- Cocos2d-x: study C++ mobile engine structure and scripting integration.
These are references, not code-copy permissions or automatic adoption decisions.

## Rendering candidates
- bgfx: cross-platform rendering abstraction; official overview documents Android, Linux X11/Wayland, Windows and Apple support, OpenGL ES 3, Vulkan, Metal and Direct3D backends; BSD-2-Clause license. Its upstream build guide documents `android-arm64-release` and Linux GCC release builds. Sources: [bgfx overview](https://bkaradzic.github.io/bgfx/overview.html) and [build guide](https://github.com/bkaradzic/bgfx/blob/master/docs/build.rst).
- Filament: Android-focused PBR renderer with native C++ API and OpenGL ES 3/Vulkan backends, but more opinionated toward 3D PBR and carries a broader tool/material pipeline. Source: [Google Filament](https://github.com/google/filament).
- SDL_GPU: supports Android but SDL's FAQ warns broad Android device support may require disabling Vulkan features; it has no OpenGL backend for SDL_GPU. It is not the first-choice sole backend until hardware constraints are tested. Source: [SDL3 GPU FAQ](https://wiki.libsdl.org/SDL3/FAQDevelopment).
- Current research shortlist: bgfx is the leading integration proof candidate for a general-purpose 2D/3D engine; this is not yet an approved dependency/backend. Compare build size, shader toolchain, surface integration, supported drivers, license notices and Android physical-device behavior before adoption.
Do not select a backend until a minimal Android ARM64 and Linux integration proof passes.

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



## AndroidX GameActivity platform adapter
- `AndroidGameActivityBackend` is an optional Android adapter in `engine/platform/android`; the embedding application supplies `android_app*` from its own `android_main`. The engine does not define a competing entry point or take ownership of the Activity.
- It installs a scoped `onAppCmd` thunk, chains the previous callback without overwriting `userData`, clears GameActivity key/motion filters while attached to receive all device classes, then restores the previous callback and filters at shutdown. It uses a registry keyed by `android_app*` instead of claiming `userData`.
- App commands are translated into resize, focus, surface created/destroyed, suspended/resumed, memory-low and close events. A surface lease owns an `ANativeWindow_acquire` reference. On `APP_CMD_TERM_WINDOW`, metrics for the old surface stop validating; the host must drain the surface-destroyed event and release/recreate renderer resources before rendering again.
- Input is consumed through GameActivity's swapped `android_input_buffer`; both key and motion arrays are cleared after processing. Known Android key codes map to USB HID usage IDs; unrecognized keys are omitted. Pointer positions are normalized from native-window pixels to the engine's logical-pixel convention. Joystick axes beyond X/Y are explicitly enabled; gamepad buttons and axis events retain device IDs. `unicodeChar` is best-effort key-to-text conversion, not full IME composition.
- The adapter compiles against official GameActivity 4.4.2 Prefab headers using the Android NDK arm64-v8a toolchain. All four jobs passed in [run 45820002663](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45820002663).
- This is not an APK or link/run proof: the repository still has no host Gradle app, manifest, Kotlin/Java Activity or physical-device test. The app integrator must add the Gradle dependency, enable Prefab, extend GameActivity, provide `android_main`, set `DREAM_BUILD_ANDROID_GAME_ACTIVITY_BACKEND=ON`, and drain surface lifecycle events before rendering. The exact setup is in `PMCS/ANDROID_INTEGRATION.md`.
- Remaining platform work: Android AssetManager-backed read-only IFileSystem, complete GameTextInput/IME and soft-keyboard control, APK integration and real device validation. Desktop native window/input adapters are not implemented.

## Native DMod runtime — implemented, intentionally unrestricted

- Native manifests use runtime="native" and a safe relative native_library path. The package contains a prebuilt shared library for the target platform/ABI.
- NativeLibrary uses dlopen(RTLD_NOW | RTLD_LOCAL) on POSIX and LoadLibraryW on Windows. Dynamic loading is in-process; it is not an isolated worker or sandbox.
- NativeModPackageInstaller installs the validated library beneath an existing application-private root. POSIX installation walks directories with openat and O_NOFOLLOW, writes a temporary file with restrictive permissions and atomically renames it into place. The non-POSIX installer path is currently not implemented.
- NativeModManager validates the module descriptor and manifest identity/version, supports dependency-ordered batch loads and rollback, enable/update/disable/unload and reverse-order shutdown. Self-unload from inside a native callback is deferred until the callback returns.
- NativeModHostApiV1 provides raw interface lookup, log forwarding and append-only interface enumeration. Enumeration returns IDs/versions in deterministic lexical order and calls visitors without holding the host registry lock.
- EngineRuntime currently exposes raw live objects for runtime, Scene, World, PhysicsWorld2D, ActionMap, FixedStepClock, RenderFrameBuilder and RenderDevice. The Runtime object itself exposes additional accessors for its managed systems.
- This is not a capability sandbox. Native code runs with the game's process privileges; it may mutate any object it obtains, use OS resources allowed to the app, corrupt memory/data, deadlock or terminate the process. Archive validation prevents malformed package paths; it does not make native code trustworthy.
- The manifest's capabilities field is parsed/validated but is not enforced for native modules. Do not add capability gates or claim isolation without a new explicit user decision.
- Raw C++ pointers are powerful but create compiler/ABI/lifetime obligations. Modules must target the correct CPU ABI and a compatible C++ runtime/header set. V1 host API extensions must be append-only while preserving existing fields.
- Native mod callbacks are expected to obey the declared noexcept ABI. The manager is not a general process fault-containment mechanism and does not make arbitrary mod-created threads safe to unload. Mods must stop their own threads and unregister callbacks/resources before disable/destroy returns.
- Native package loading and lifecycle plus interface enumeration passed Linux tests in [run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851). Android ARM64 jobs cross-compile the package installer/runtime sources; they do not execute a native mod on a phone.
- Remaining: full engine service exposure, hook/replace/mixin infrastructure, callback/resource/job/thread ownership tracking, hot reload, package publisher/signature trust and physical Android runtime validation.

## Rendering and physics implementation status

- Engine-owned render packet validation, RenderDevice surface lifetime/resize handling, RGBA8 texture forwarding and RenderFrameBuilder are implemented. The builder produces indexed 2D sprite quads and supports 3D draw submissions at the contract level.
- Optional bgfx backend proof compiles for Linux and Android ARM64. CI compiles ESSL 300 and GLSL 330 sprite shaders. The current backend draws 2D sprites only when compatible shader binaries are supplied; there is no 3D mesh/material renderer or physical device proof. Verified in [run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738).
- PhysicsWorld2D and deterministic sweep-and-prune broadphase have unit coverage. Known limitations include no joints, CCD, sleeping, persistent warm-starting or multiple fixtures per body. Physics3D is not implemented.


## Native hook registry — verified 2026-11-26

- Added NativeHookRegistry to the DMod runtime and exposed it through EngineRuntime as dream.engine.mods.hooks; the manager is also exposed as dream.engine.mods.manager.
- NativeHookStage supports Before, Around, Replace and After. Before runs before the original operation; Around wraps the remaining chain; Replace can omit call_next to skip the original operation; After runs in reverse priority/registration order after the chain returns.
- Priority order is descending; equal priorities preserve registration order. Around/Replace call_next can be invoked repeatedly to deliberately repeat the remaining chain or original operation.
- Hook callbacks receive NativeHookInvocationV1 with a raw user_data pointer. Each hook point must define and document its argument/result struct; the caller and module need matching header/compiler ABI.
- Invalid hook dispatch or snapshot allocation failure falls back to the original operation so the engine does not silently skip its operation.
- NativeModManager enables/disables hooks with the owning module, removes that module's registrations before unloading the library, and wraps dispatch in its callback-depth guard. If a callback requests self-unload, removal/library close is deferred until the full dispatch chain returns.
- Tests verify before/around/after sequencing, priority ordering, replace bypass, repeated call_next, invalid-ID fallback, and zero remaining hook registrations after self-unload.
- [Run 45810170873](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45810170873) passed all four jobs: Linux GCC, Linux Clang, Android arm64-v8a cross-compilation and DMod AST proof.
- Current limitation: the generic dispatcher does not automatically intercept arbitrary engine C++ methods. Runtime/game code must place dispatch_hooks at each operation to be extensible. Actual EngineRuntime/Scene/Physics2D/Render/World/Asset hook integration remains on the roadmap.


## EngineRuntime native hook integration — verified 2026-11-26

- Public hook point identifiers: dream.engine.runtime.update, dream.engine.physics2d.step, dream.engine.runtime.begin_render_frame and dream.engine.runtime.render_frame.
- RuntimeUpdateHookDataV1 carries EngineRuntime*, mutable frame delta, input-event span and RuntimeUpdateResult. Native Before callbacks can alter the delta or input span; After callbacks can inspect/mutate the returned result.
- RuntimePhysics2DStepHookDataV1 carries EngineRuntime*, per-step delta and PhysicsStepReport2D. Every fixed PhysicsWorld2D system execution dispatches the hook point.
- RuntimeBeginRenderFrameHookDataV1 carries clear color, clear-color/depth flags and result; Before callbacks can change the frame's clear state.
- RuntimeRenderFrameHookDataV1 provides the final render result for After callbacks.
- EngineRuntime test verifies update delta hook, one Physics2D hook per fixed step, clear-color mutation reaching the backend and render completion observation.
- Native mod IDs are inserted as disabled before create(), so hook callbacks registered in create() remain dormant until successful enable. Host/game hook owners are active by default unless explicitly disabled by NativeModManager.
- CI run 45811326053 passed Linux GCC, Linux Clang, Android arm64-v8a cross-compilation and DMod AST proof (4/4 jobs).
- Remaining integration is per-system Scene scheduler hooks, World/ECS structural/component operations and asset lifecycle; those do not automatically become hookable merely because the manager is exposed.


## Physics3D collision geometry foundation — verified 2026-11-26

- Added DreamEngine::Physics3D as a separate CMake interface target.
- Geometry API includes finite-safe Vec3 operations, AABB3D, Sphere3D and OrientedBox3D with validated orthonormal world axes.
- Collision queries implement AABB-AABB, sphere-sphere, sphere-AABB and OBB-OBB. OBB-OBB uses the complete 15 candidate SAT axes (three axes from each box and nine pairwise cross products), skipping degenerate cross axes.
- Contact manifold normal points from the first shape toward the second; moving the first shape opposite the normal resolves penetration. OBB contact point is a support-point midpoint approximation, not a clipped manifold.
- Tests cover overlap/separation, penetration/normal, sphere-inside-box exit selection, rotated OBB validity and malformed geometry.
- [Run 45828037777](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45828037777) passed Linux GCC and Clang build/CTest, Android arm64-v8a cross-compilation (including the new geometry test), and DMod AST proof.
- This is not a Physics3D simulation world. Broadphase, body handles/state, force integration, angular solver, contact persistence, joints, CCD and sleeping remain unimplemented.
- Scene scheduler per-system native hook dispatch is implemented via dream.engine.scene.system; remaining automatic hook sites are World/ECS structural/component operations and asset load/decode/register lifecycle.
## Current audit correction — 2026-11-26

Verified against main `54f6e8c39f599be919c669e6316b807cdf220654` and [CI run 45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368): all five jobs passed, with 46/46 CTest cases on GCC, Clang and Clang ASan/UBSan; Android ARM64 and DMod AST proof passed.

### Implemented since the older audit snapshot
- Physics3D broadphase/world/body handles, static/kinematic/dynamic integration, angular contact solver, filtering/sensors/contact transitions, raycast/raycast-all and sphere/OBB overlap queries. This corrects the older blanket statement that Physics3D world/body integration was absent.
- Renderer mesh/material resource contracts and bgfx GPU vertex/index/texture uploads plus 3D shader pipeline. Visual correctness on actual Android GPU remains unverified.
- Retained UI render adapter and EngineRuntime UI enqueue method.
- Built-in OBJ importer with bounded source/face/vertex/index limits, negative indices, fan triangulation, deduplication, generated normals, DMES serialization and decoder.
- Native hook sites for animation update, asset importer invocation, UI drawing and ordinary-thread audio mixing. This corrects older notes that only runtime/physics/scene hooks existed.
- AAudio callback/context atomic publication and synchronized stream configuration read.
- Two headless sample executables are now part of CTest and passed.

### Remaining technical risks / known limitations
- Android host APK currently launches a minimal GameActivity `Application` loop; it does not create EngineRuntime, bgfx, or game rendering. Actual window lifecycle and rendering must be validated on the user's phone.
- Separate bgfx workflow builds the Android static library and compiles 2D/3D ESSL300 plus desktop GLSL330 shaders. Shader binaries are not yet bundled in APK and loaded by an Android runtime.
- Physics3D query APIs currently scan body maps; broadphase-based query pruning is pending. No CCD, joints, sleeping/wake, multi-fixture support, persistent warm-start or robust multi-point OBB clipping.
- Physics2D still lacks CCD, joints, sleeping and multiple fixtures; OBB/OBB contact is approximate.
- OBJ n-gons use fan triangulation (not general concave ear clipping); no MTL/glTF/GLB/image/audio importers.
- UI text depends on `IUiGlyphProvider`; no built-in shaping/font rasterizer/atlas. Rounded-corner masks need a shader.
- AAudio has only CI cross-compilation; no device playback/underrun/route/lifecycle measurement.
- Runtime's direct importer entry calls the registered importer and hooks it; full AssetImportPipeline remains separately responsible for filesystem load, validation, dependency handling and hashing.
- Native mods are fully trusted in-process code: no sandbox, memory protection, crash isolation or ABI stability guarantee. A mod may corrupt/terminate the engine or access host process resources.
- Headless example backend validates API composition and draw submission, not pixels, GPU state or frame pacing.

### Verification pointers
- Engine: [45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368).
- Android host APK: [45859471974](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859471974), artifact ID `15138770221`; the host's Android sources are unchanged since that successful build, but rebuild after EngineRuntime integration.


## Current implementation audit — 2026-11-26

This update supersedes earlier statements in this file and in older PMCS status sections when they disagree. Source: main commit `96dc09d43a8b4f244088a9469458d77c3e5bb483`; all CI evidence below is from [run 45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968).

### CI evidence
- Linux/GCC: success, 47/47 CTest.
- Linux/Clang: success, 47/47 CTest.
- Linux/Clang ASan+UBSan: success, 47/47 CTest.
- Android arm64-v8a (ARMv8-A): cross-build success.
- DMod/ChaiScript direct AST proof: success.
- Android host APK [run 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085) is building from this same commit; shader compiler build was still in progress at the time this note was written.

### Correctness findings resolved
- **Asset subscription cancellation:** subscriber-level cancellation no longer cancels the shared pending factory/result for other subscribers; deferred futures are detected explicitly so `wait_for` cannot spin forever on `std::launch::deferred`.
- **World/ECS native hooks:** mutation contexts carry the original operation continuation and its stack-lifetime context. EngineRuntime's trampoline passes the real `WorldMutationContext` through NativeHookRegistry and invokes the original operation through the context. The original implementation treated the context pointer as a different stack struct, which produced a wild function-pointer jump; the correction is covered by the sanitizer run.
- **Android touch scaling:** retained UI layout uses logical pixel dimensions, matching GameActivity's pointer event coordinates; camera projection still uses physical framebuffer dimensions.
- **Android suspended/lost-surface updates:** the native host skips EngineRuntime updates while the renderer is unavailable or the app is suspended.
- **Android log access:** prefer GameActivity `externalDataPath`, fall back to `internalDataPath`; file sink creates parent directories and rotates the log at 8 MiB with three backups. Logcat tag: `DreamEngine`.

### Android render-host path now present
The GameActivity native loop constructs EngineRuntime, initializes/reinitializes bgfx on the supplied Android native surface, reads the four offline ESSL 300 shader binaries from APK assets, creates a combined 2D/3D physics demo, uploads a mesh/material, sends retained UI draws and routes touch input to the UI. AAudio is connected to AudioMixer, with output stopped on pause and restarted after resume when the renderer is ready. This is implemented in source; pixel correctness, audio playback and full lifecycle stability still require the user's physical-device test.

### Remaining technical risks
- Physics2D: no joints, CCD, sleeping, persistent warm-start cache or multiple fixtures; OBB/OBB manifold is approximate.
- Physics3D: no joints, CCD, sleeping/wake policy or multiple fixtures; OBB contact point is approximate; ray/overlap query APIs currently scan registered bodies rather than pruning through broadphase.
- Renderer/GPU: Android uses packaged GLES 3.0 shader binaries; device-specific depth/culling, filtering/samplers, device loss and GPU-resource ownership need physical validation and further tests. GPU asset residency integration is not complete.
- Assets: built-in model importer is OBJ only and uses fan triangulation for n-gons; no MTL/glTF/GLB/image/audio importers.
- UI/animation/audio: UI glyphs require a provider (the Android host uses a simple 5x7 bitmap provider); no shaping/font atlas or rounded-corner mask. Animation blending/events/curves remain limited. AAudio compile and wiring are not proof of actual speaker playback, route change or latency.
- Native mods: fully trusted in-process code, no sandbox, memory/crash isolation or stable cross-version ABI promise. Module-owned resource/job/thread accounting and safe hot reload remain unfinished. The new World hooks cover entity/component/hierarchy mutation; separate file-read/decode/register lifecycle hooks are still open.
- Platform: full IME composition and soft-keyboard state/control remain open. Android CI is cross-compilation only.

### Verification pointers
- Engine CI: [45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968).
- Current APK build: [45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085), pending artifact upload.


## Implementation update — 2026-11-26

This update is authoritative over older checklist/status text above where they conflict.

- **Physics broadphase queries:** 2D and 3D raycast/overlap queries use deterministic AABB candidate queries backed by cached X-interval indices. Updating/removing a proxy invalidates the index. Tests include 96 distant bodies and stable candidate order.
- **Continuous collision detection (limited opt-in):** `BodyDescription2D::bullet` enables analytic swept-circle time-of-impact against static circles; `BodyDescription3D::bullet` enables swept-sphere time-of-impact against static spheres. Other shapes and dynamic-vs-dynamic pairs still use discrete collision detection. Do not describe this as general CCD.
- **OBJ/MTL:** OBJ material-library parsing and concave polygon ear-clipping are implemented. Triangulation uses Newell normal projection and preserves face-corner UV/normal references; tests cover both polygon windings and signed area.
- **Asset lifecycle hooks:** asynchronous loader observers report read/decode/register and cache phases. The runtime queues owned events with sequence numbers and dispatches native hooks from the engine update thread, not worker threads. Queue capacity is bounded; overflow is logged.
- **Native world hooks:** ECS entity/component/hierarchy mutation hooks run outside the World lock, with the original operation continuation carried in the typed mutation context.
- **Android shader reproducibility:** the four ESSL 300 binaries in the APK are checked in with SHA-256. The Android workflow validates them directly; the separate pinned bgfx proof recompiles from source and compares the binaries byte-for-byte.
- **Android host:** GameActivity host, EngineRuntime, bgfx GLES 3 renderer, 2D/3D demo, retained touch UI, basic software-keyboard/IME bridge, AAudio output connection, pause/surface recreation handling and rotating logs are present. CI proves cross-compilation/package layout only, not physical-device rendering/audio/IME behavior.
- **Sleeping:** not yet implemented; no public sleep/wake state is exposed. It remains open rather than being marked complete.
- **Remaining production work:** general CCD for non-circular shapes and dynamic-vs-dynamic pairs; sleeping/waking; joints; multiple fixtures and robust contact manifolds; complete MTL texture handling and glTF/GLB/image/audio import; GPU asset residency/device-loss policy beyond renderer resource reinitialization; full IME composition/selection; full font shaping/atlas and UI clipping; animation blends/events/curves; native mod resource/job/thread ownership and hot reload; package signatures/trust; profiler/editor; two real-game integration tests; physical-device verification.
