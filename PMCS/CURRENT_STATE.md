# Current State

**Last checked:** 2026-11-26  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main  
**Latest verified implementation commit:** 0f6401be20366ce65f38eb684c11e7508b5eae67

## Implemented

### Core and platform contracts
- CMake/C++20 project with dream_core, dream_platform, dream_dmod and interface targets for Assets, World, Serialization, Input, Render, Physics2D and Runtime.
- Core version API, logger, diagnostics, typed event dispatcher, worker JobSystem, fixed-step clock, generational handles, owning ResourceRegistry and keyed ResourceStore.
- Application lifecycle callbacks, frame-delta clamping, stop requests and failure cleanup.
- Backend-neutral window, native render-surface, input and virtual-filesystem contracts. Input covers USB HID keyboard/keypad, Unicode text, mouse/touch/pen and gamepad controls. Virtual paths reject traversal, absolute paths and unsafe/nonportable segments.
- NativeFileSystem is now a concrete rooted POSIX IFileSystem implementation for Linux and Android. Concrete Android lifecycle/window/surface/input integration is still missing.
- NativeFileSystem anchors access to an opened root directory descriptor, traverses path components with openat/O_NOFOLLOW, rejects traversal and symlinks, checks read limits before allocation, and writes through same-directory temporary files plus fsync/atomic rename.
- Directory listing is deterministic, filters symlink/special-file entries, and accepts an empty path to list the backend root. Read/list outputs remain unchanged on failure.

### World, serialization and input
- World/ECS with generational entities, typed component pools, scoped access, multi-component queries, parent/child hierarchy with cycle rejection, Transform2D and Transform3D matrices.
- Scene-owned PreUpdate/Update/PostUpdate scheduler with deterministic priority/order and safe add/remove behavior during ticks.
- Versioned little-endian DENG binary archive and explicit stable-ID component codecs. World restoration is staged; Scene archives preserve scene name and World, while runtime callbacks are re-registered by game code.
- ActionMap for keyboard, mouse and gamepad, with modifiers, analog deadzones, edge state, disconnect handling and global reset.

### Asset pipeline
- Bounded asynchronous asset read/decode/register path using JobSystem and IFileSystem.
- Optional byte-budgeted CPU AssetCache with LRU policy and caller-supplied size estimator.
- Cooperative cancellation checkpoints; active filesystem or decoder calls cannot be interrupted.
- `AssetLoadScope` supplies per-operation lifetime leases. Move a token from `scope.acquire_token()` into `load_async` or `load_async_cached`; `shutdown_and_wait()` closes admission, requests cancellation and waits until all task leases are released. A task guard releases the lease as the load callable exits, including cancellation/error returns.
- Scope does not own JobSystem, filesystem, ResourceStore, AssetCache or decoder dependencies. Declare it after those borrowed dependencies so it is destroyed first; do not call its blocking shutdown from one of its own worker tasks. Blocking reads/decoders must return before shutdown can complete.
- Deterministic asset dependency planner and in-flight request coordinator for matching request IDs. Coordinator shares pending work; it is not a result cache and still does not provide independent follower cancellation.

### 2D physics
- PhysicsWorld2D with static/kinematic/dynamic bodies, circle and oriented-box colliders, gravity, force/torque/impulse, damping, friction, restitution, category/mask/group filters and sensors.
- Deterministic sweep-and-prune broadphase; Begin/Stay/End contact events; raycast/raycast-all and point overlap queries.
- Known gaps: one collider per body, no joints, CCD, sleeping or persistent warm-start cache; OBB-OBB contact is approximate. Physics2D is not a complete production solver yet.

### Rendering and runtime
- Backend-neutral render contract, validated frame packets, RenderDevice surface ownership/resize handling and RGBA8 texture forwarding.
- RenderFrameBuilder creates indexed 2D sprite quads, sorts by layer/submission order and supports 3D mesh draw submissions at the contract level.
- Optional bgfx backend proof supports clear and 2D sprite rendering when compatible precompiled shader binaries are supplied. CI compiles ESSL 300 and GLSL 330 shaders and cross-builds the backend for Android ARM64. The current backend has no 3D mesh/material pipeline; bgfx is not yet a final production dependency decision.
- EngineRuntime coordinates Scene, fixed-step updates, PhysicsWorld2D, ActionMap, render frame building and RenderDevice. ECS-to-physics body synchronization is still game/system-owned.

### DMod / native mods
- .dmod ZIP reader validates archive metadata, local headers, UTF-8 paths, duplicate/case collisions, entry limits, decompression limits and CRC/decompression errors. It reads packages in memory without extracting package-controlled paths.
- Manifest parser and deterministic dependency resolver support runtime="script" or runtime="native", version ranges and dependencies.
- Native shared-library loading through dlopen on POSIX and LoadLibraryW on Windows; native package installer materializes the declared library into an application-private directory. POSIX installation uses openat, O_NOFOLLOW and atomic rename for package-controlled path components.
- NativeModManager supports dependency-ordered batch loading, rollback on failure, enable/disable/update, reverse-order unload and deferred self-unload after a native callback returns.
- NativeModHostApiV1 exposes query-by-ID, logging and deterministic interface enumeration. Enumeration snapshots IDs/versions under lock and invokes the visitor outside the registry lock.
- EngineRuntime exposes raw live objects for dream.engine.runtime, dream.engine.scene, dream.engine.world, dream.engine.physics2d, dream.engine.input.actions, dream.engine.clock.fixed, dream.engine.render.frame_builder, dream.engine.render.device, dream.engine.mods.manager and dream.engine.mods.hooks.
- NativeHookRegistry supports Before, Around, Replace and After callbacks, descending priority with deterministic registration order, and reverse-order After unwinding. Around/Replace callbacks can invoke the remaining chain repeatedly or skip it; Replace can therefore bypass the original operation.
- Hook invocation carries caller-defined argument/result data through a raw user_data pointer. Hook points define their own C++ context layout; modules and the engine/game must use compatible headers and ABI.
- Manager tracks owner enable state and removes all callbacks owned by a mod before closing its shared library. Self-unload requested inside a hook is deferred until the entire dispatch chain returns. Invalid hook dispatch falls back to the original operation.
- Generic hook dispatch is implemented. EngineRuntime now calls it at four live operation points: runtime update, each PhysicsWorld2D fixed step, begin-render-frame, and render-frame. Public V1 context structs expose the runtime, mutable frame delta/input span, physics step report, clear color/flags and render result.
- Runtime Before hooks can modify frame delta and render clear state; After hooks can observe results. Each physics substep is independently hookable. EngineRuntime tests verify these concrete paths.
- Native module owners are inserted into the disabled registry state before plugin create(); hooks registered during create() remain inactive until enable succeeds. Hooks registered by host/game owners without an explicit manager state are active by default. Owner hooks are disabled and erased before unloading the module library.
- Scene scheduler individual system callbacks, World/ECS mutations and asset lifecycle are not automatically dispatched yet; those are next integration targets.
- Native mods execute in-process with the application's process privileges. There is deliberately **no sandbox and no capability enforcement** for native modules. The archive validator is not a runtime security boundary. Only install native mods the user trusts.
- Native packages must ship a prebuilt library for the target platform/ABI. The engine does not compile mods on the user's device and does not require an APK/engine/game rebuild to install a package.
- The script interpreter is not a production runtime yet. ChaiScript direct-AST evaluation remains a proof-only candidate; do not describe script mods as implemented.

## Verified CI
- [Run 45818494612](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45818494612) passed all four jobs for commit 0f6401be20366ce65f38eb684c11e7508b5eae67: Linux GCC/Clang, Android arm64-v8a and DMod AST proof. It verifies AssetLoadScope cancellation, waiting for a blocked read, no registration after cancellation, and lease release on task exit.
- [Run 45816961428](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45816961428) passed all four jobs for commit db4560523914556196a243a16642c7c3de9fbeed: Linux GCC/Clang, Android arm64-v8a (ARMv8-A), and DMod AST proof. It builds NativeFileSystem and runs its Linux security/behavior tests.
- [Run 45811326053](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811326053) completed successfully for all four jobs on commit e65ee68289c874297b5267599a997d1bb06f5901. It verifies native hook ownership (create-time hooks dormant until enable), EngineRuntime update/Physics2D/render hook points, lifecycle cleanup, Linux GCC/Clang, DMod AST proof and Android arm64-v8a cross-compilation.
- [Run 45811035311](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811035311) passed all four jobs for the initial EngineRuntime hook integration.
- [Run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851) completed successfully for native host interface enumeration and module lifecycle before hook integration.
- [Run 45805442161](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45805442161) completed successfully for all four jobs before the interface-enumeration extension.
- [Run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738) completed successfully for both bgfx backend proof jobs.
- Android CI is NDK cross-compilation for arm64-v8a (ARMv8-A), not execution on a physical Android device.

## Accepted direction
- General-purpose C++ game engine; 2D and 3D are both first-class.
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Android target ABI is arm64-v8a (ARMv8-A).
- Native DMods are full-access, in-process modules with no sandbox/capability fence. Compatibility tiers are not security boundaries.
- Native modules are distributed as prebuilt target-ABI libraries inside .dmod; no compilation occurs on the user's device.
- Keep package parsing/path validation, runtime trust and code isolation as three distinct concerns. Native code is trusted executable content, not sandboxed input.

## Remaining engine work
1. Concrete Android platform application/window/surface/filesystem implementation and physical-device validation.
2. Finish the operational 2D renderer path, runtime shader packaging/loading and GPU resource lifecycle; then implement 3D rendering, meshes, materials, lighting and GPU asset residency.
3. Harden Physics2D and implement 3D physics.
4. Audio, animation, UI/runtime services and asset import pipeline.
5. Continue native hook integration into per-system Scene scheduler callbacks, World/ECS mutations and asset lifecycle; add ownership tracking for mod resources/jobs/threads and safe hot reload.
6. Define package trust/signature policy and test native .dmod loading on real Android ARM64. Signatures may establish publisher/integrity; they do not sandbox code.
7. Decide whether a separate script runtime is worth implementing. It is not a prerequisite for the native full-access path.

## Build and test policy
- GitHub Actions is the primary build/test environment.
- Linux GCC and Clang builds execute CTest.
- Android uses the NDK toolchain for arm64-v8a (ARMv8-A); cross-compilation must never be reported as physical-device execution.
- C++20 remains the current implementation baseline; final compiler-support policy is still open.
