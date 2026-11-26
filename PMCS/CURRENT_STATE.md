# Current State

**Last checked:** 2026-11-26  
**Repository:** DreamingbullyYT/my-engine-  
**Default branch:** main  
**Latest verified implementation commit:** eb96be230f5ce6ad53bf8703db939a449764b6ed

## Implemented

### Core and platform contracts
- CMake/C++20 project with dream_core, dream_platform, dream_dmod and interface targets for Assets, World, Serialization, Input, Render, Physics2D and Runtime.
- Core version API, logger, diagnostics, typed event dispatcher, worker JobSystem, fixed-step clock, generational handles, owning ResourceRegistry and keyed ResourceStore.
- Application lifecycle callbacks, frame-delta clamping, stop requests and failure cleanup.
- Backend-neutral window, native render-surface, input and virtual-filesystem contracts. Input covers USB HID keyboard/keypad, Unicode text, mouse/touch/pen and gamepad controls. Virtual paths reject traversal, absolute paths and unsafe/nonportable segments.
- NativeFileSystem is a concrete rooted POSIX IFileSystem implementation for Linux and Android. AndroidAssetFileSystem provides bounded read-only APK asset access. AndroidGameActivityBackend now attaches to a host-provided AndroidX GameActivity `android_app` and implements lifecycle/window/surface/input integration; it compiled against the official GameActivity 4.4.2 Prefab headers for ARM64 in CI.
- NativeFileSystem anchors access to an opened root directory descriptor, traverses path components with openat/O_NOFOLLOW, rejects traversal and symlinks, checks read limits before allocation, and writes through same-directory temporary files plus fsync/atomic rename.
- Directory listing is deterministic, filters symlink/special-file entries, and accepts an empty path to list the backend root. Read/list outputs remain unchanged on failure.
- AndroidGameActivityBackend lives in `engine/platform/android`. The app retains ownership of its Java/Kotlin Activity and `android_main`; the adapter attaches to the supplied `android_app`, chains/restores the previous app-command callback and restores prior key/motion filters on shutdown.
- It handles native-window create/destroy/resize, focus, pause/resume and low-memory commands; each acquired `SurfaceId` owns an `ANativeWindow_acquire` reference. Window events include surface-created/destroyed and app suspended/resumed/memory-low notifications.
- It reads GameActivity's swapped input buffers, clears both arrays after consuming them, maps known Android key codes to USB HID, converts pointer positions to logical pixels, enables joystick axes, and reports key/text/touch/mouse/gamepad events. IME composition and soft-keyboard control remain unimplemented.
- Host integration guide: [ANDROID_INTEGRATION.md](ANDROID_INTEGRATION.md). Android Gradle/manifest/Activity packaging and physical-device validation are not present in this engine repository; the CI result is a cross-compile only.

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

### 3D collision geometry
- New DreamEngine::Physics3D foundation includes finite-safe Vec3 operations, AABB3D, Sphere3D and orthonormal-axis OrientedBox3D bounds.
- Collision queries cover AABB-AABB, sphere-sphere, sphere-AABB and OBB-OBB using the 15-axis separating-axis test; manifolds provide a first-to-second normal, approximate contact point and penetration.
- Geometry tests cover separation, overlap, inside-sphere cases, rotated OBB validation and invalid shapes. This is geometry only: no 3D body world, integration, broadphase, contact solver, joints or CCD yet.

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
- Scene scheduler individual system invocations are dispatched through dream.engine.scene.system; World/ECS structural/component operations and asset lifecycle are not automatically dispatched yet.
- Native mods execute in-process with the application's process privileges. There is deliberately **no sandbox and no capability enforcement** for native modules. The archive validator is not a runtime security boundary. Only install native mods the user trusts.
- Native packages must ship a prebuilt library for the target platform/ABI. The engine does not compile mods on the user's device and does not require an APK/engine/game rebuild to install a package.
- The script interpreter is not a production runtime yet. ChaiScript direct-AST evaluation remains a proof-only candidate; do not describe script mods as implemented.

## Verified CI
- [Run 45828037777](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45828037777) passed all four jobs for commit eb96be230f5ce6ad53bf8703db939a449764b6ed: Linux GCC/Clang build plus CTest, Android arm64-v8a cross-compilation (including the new Physics3D geometry test), and DMod AST proof.
- [Run 45820002663](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45820002663) passed all four jobs for commit 4a783c57511ddad2db153c83eff597a435c01e10: Linux GCC/Clang, Android arm64-v8a and DMod AST proof. Android downloaded GameActivity 4.4.2 and compiled `game_activity_backend.cpp` against its official Prefab headers.
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
1. Android host Gradle/manifest/Activity integration and physical-device validation; GameActivity adapter and read-only APK AssetManager filesystem are implemented and cross-compiled.
2. Finish the operational 2D renderer path, runtime shader packaging/loading and GPU resource lifecycle; then implement 3D rendering, meshes, materials, lighting and GPU asset residency.
3. Harden Physics2D and implement 3D physics.
4. Audio, animation, UI/runtime services and asset import pipeline.
5. Add World/ECS mutation and asset lifecycle hooks; add ownership tracking for mod resources/jobs/threads and safe hot reload. Scene per-system scheduler hooks are already integrated.
6. Define package trust/signature policy and test native .dmod loading on real Android ARM64. Signatures may establish publisher/integrity; they do not sandbox code.
7. Decide whether a separate script runtime is worth implementing. It is not a prerequisite for the native full-access path.

## Build and test policy
- GitHub Actions is the primary build/test environment.
- Linux GCC and Clang builds execute CTest.
- Android uses the NDK toolchain for arm64-v8a (ARMv8-A); cross-compilation must never be reported as physical-device execution.
- The user chose to finish the engine foundation before expanding advanced DMod/modding features; keep modding maintenance-only until platform, rendering, simulation and runtime-service foundations are complete.
- C++20 remains the current implementation baseline; final compiler-support policy is still open.
## Authoritative status update — 2026-11-26

This section supersedes earlier status statements in this file wherever they conflict.

- **Current main commit:** `54f6e8c39f599be919c669e6316b807cdf220654`.
- **Verified CI:** [Dream Engine CI run 45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368) passed all five jobs: Linux GCC, Linux Clang, Linux Clang ASan+UBSan, Android arm64-v8a (ARMv8-A), and the DMod/ChaiScript direct-AST proof.
- **Tests:** 46/46 CTest cases passed under GCC, Clang, and Clang ASan+UBSan. This includes the executable 2D platformer and 3D physics/render integration samples.
- **Android compile coverage:** engine runtime, AAudio integration and both sample-game translation units cross-compiled for Android arm64-v8a. This is not device execution.
- **APK:** the GameActivity host APK was assembled successfully in [Android APK run 45859471974](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859471974); artifact `dream-engine-android-host-arm64-debug`, artifact ID `15138770221`, SHA-256 `345306a9572ae92f5b88052c4be06ae9835235a8b5ca2dd6a81fbf15614261e6`. The artifact was built from `ee57832717d141e8408f9e6887d402a9ef371ca8`; the Android host sources did not change in the subsequent runtime-hook commit. Do not describe this artifact as containing the latest runtime implementation until a fresh APK build is verified.

### Newly implemented and CI-verified

- Physics3D dynamic/static/kinematic sphere and oriented-box world with broadphase, angular/linear solving, filters, sensor/contact lifecycle, transactional integration, nearest/all raycasts, and sphere/OBB overlap queries.
- bgfx backend resource path for 2D textures/sprites and 3D mesh/material resources, with separate 3D and 2D passes. Runtime use requires supplying compatible offline shader binaries before initialization.
- Retained UI draw adapter that converts panels, images and provided glyph quads into indexed 2D sprite draws; `EngineRuntime::enqueue_ui_draws` enforces frame ordering.
- Bounded OBJ importer with negative indices, fan triangulation, vertex deduplication, generated normals, versioned DMES output and decoder. This is not a full interchange-format importer.
- Native hook call sites for runtime update, Physics2D, Physics3D, scene systems, begin/render frame, animation update, asset importer invocation, retained UI draw and ordinary-thread audio mix. Hooks are owner/lifecycle-aware and support Before/Around/Replace/After ordering.
- Native mods receive live service interfaces for runtime, scene/world, physics worlds, animation, audio, UI, assets/importers, input/action maps, clock, renderer, mods and hooks.
- The AAudio callback/context publication is atomic and configuration reads synchronize with stream lifecycle operations. Arbitrary native mod callbacks are intentionally not dispatched from the real-time audio callback.
- Two headless sample games are run by CTest: `examples/2d_platformer` and `examples/3d_physics_lab`.

### Important boundaries still open

- The Android APK is currently a GameActivity platform-host smoke app: it creates a native window/surface, runs the platform application loop and consumes queued input. It does **not yet instantiate EngineRuntime, load the bgfx backend/shaders, or display the sample games**.
- Physical phone testing has not been performed. The user will perform the first install/launch/resume/pause test on their phone.
- bgfx shader binaries are compiled in a separate proof workflow, but are not yet automatically packaged into the APK or loaded by an Android EngineRuntime host. Linux bgfx proof completion must be verified separately; Android bgfx static-library build has passed.
- Physics3D still lacks joints, CCD, sleeping/wake policy, multiple fixtures per body and robust multi-point OBB contact manifolds. World overlap/ray queries currently scan registered bodies rather than querying a spatial tree.
- The retained UI has no built-in text shaper/font rasterizer/atlas; callers must provide `IUiGlyphProvider`. Rounded-corner masking needs a dedicated shader.
- Built-in asset import currently covers OBJ only. MTL, glTF/GLB, texture/image and audio importers are not present.
- Audio mixing and the Android AAudio output backend compile, but playback, latency, route changes and lifecycle recovery have not been tested on hardware.
- Native mods are intentionally unrestricted in-process native code. They can crash/corrupt the engine or access host resources. There is no sandbox or security isolation; ABI compatibility remains the host/mod developer's responsibility.


## Authoritative current state — 2026-11-26 (supersedes all earlier status notes)

**Main:** `96dc09d43a8b4f244088a9469458d77c3e5bb483`  
**Verified CI:** [Dream Engine CI 45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968) — 5/5 jobs passed; 47/47 CTest cases passed in each Linux configuration (GCC, Clang, Clang ASan/UBSan); Android ARM64 cross-build and DMod AST proof passed.

### Android host: current implementation
The old note above that Android only had a shell host is superseded. `android-host/` now contains an AndroidX GameActivity 4.4.2 application with application ID `dev.dreamengine.host`, minSdk 26 and `arm64-v8a` only. The native `android_main`:
- Creates `AndroidGameActivityBackend`, `EngineRuntime`, the bgfx renderer and an Android APK AssetManager filesystem.
- Loads GLES 3.0 sprite and mesh shader binaries from APK assets before renderer initialization.
- Creates a demo 2D dynamic player/ground and a 3D dynamic ball/floor, uploads a mesh and material, and submits both render paths.
- Builds a retained panel/button labelled `TAP TO JUMP`; Android pointer input reaches the UI and applies a 2D impulse.
- Connects the engine AudioMixer to Android AAudio output. Audio is stopped on suspend and restarted after resume.
- Handles native surface creation/destruction/recreation, recreates GPU resources after renderer reinitialization, and stops advancing simulation/UI while suspended or while the render surface is unavailable.
- Uses logical pixels for retained UI layout and physical pixels for framebuffer camera projection.
- Logs to Logcat (`DreamEngine`) and a rotating file at `logs/engine.log` under GameActivity `externalDataPath`, with `internalDataPath` fallback. Rotation is 8 MiB with three backups.

The Android APK workflow is [run 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085), building from this commit. It compiles the pinned bgfx shader compiler, generates ESSL 300 sprite/mesh binaries, assembles the Gradle debug APK, verifies `lib/arm64-v8a/libdream_engine_android_host.so` and shader assets, and uploads the installable artifact. At the time of this update the run is still in progress; no download artifact is claimed until it reports success.

### Newly verified asset and mod runtime work
- Asset request subscriptions have independent cancellation while preserving a shared pending request for other subscribers; deferred `shared_future` results are handled.
- World/ECS mutations now route through named native hook points for entity create/destroy, component emplace/remove/modify, multiple-component modify, mutable iteration and parent set/clear. Callbacks run outside the World lock. The integration test queries World from inside a hook and the sanitizer matrix passes.
- EngineRuntime exposes live service objects to trusted in-process native modules, including physics 2D/3D, animation, audio, retained UI, importer registry, ActionMap, fixed clock, render objects and hook registry. Native DMods intentionally remain unrestricted and unsandboxed by user decision.

### Remaining limitations
This is a first integrated Android demo host, not a feature-complete production engine. Physical GPU correctness is not proven until the user's phone run. Advanced Physics2D/3D features (CCD, joints, sleeping, multi-fixtures and robust contact manifolds), expanded import formats (MTL/glTF/GLB/images/audio), built-in font shaping/atlas and rounded UI masks, animation blend/event/curve systems, GPU asset residency/device-loss policy, explicit file-read/decode/register hooks, safe mod-owned job/resource/thread lifecycle and hot reload, publisher signatures/trust, full profiler/editor and real-game validation remain open. The user will perform the first phone test after the APK artifact is available.


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
