# Architecture

**Status:** Core/runtime foundations, World/ECS, serialization, gameplay input, async assets, Physics2D, backend-neutral rendering, EngineRuntime and native in-process DMod loading are implemented and CI-verified. Rooted POSIX file access, AndroidX GameActivity lifecycle/window/surface/input and read-only APK AssetManager access compile for Android ARM64. Android APK packaging/device validation, IME composition, production 3D rendering/physics and several runtime services remain incomplete.

## High-level modules
- **Core:** time, jobs, events, logging, diagnostics, handles, resource ownership and serialization primitives.
- **Platform:** lifecycle, windows/surfaces, input, filesystem and OS integration. Rooted POSIX NativeFileSystem, AndroidAssetFileSystem and an AndroidX GameActivity backend are implemented; desktop native window/input adapters and Android host APK integration remain future work.
- **World:** Scene, World/ECS, entities, components, hierarchy, transforms and scheduler.
- **Assets:** bounded file loading, decoders, dependency planning, CPU cache and request coordination.
- **Rendering:** engine-owned 2D/3D frame and resource contracts with replaceable backends.
- **Simulation:** Physics2D is implemented initially; Physics3D, solver hardening, animation and navigation remain.
- **Runtime:** combines Scene, fixed-step updates, input actions, Physics2D and rendering.
- **DMod:** validated ZIP package, manifest/dependency handling, unrestricted in-process native modules and an ordered before/around/replace/after hook registry. World/ECS mutation and asset-lifecycle hook insertion, module-owned resource/job/thread lifetime tracking and a production script runtime remain future work.
- **Game project:** game-specific systems, content, assets, UI and gameplay code.
- **Tools/editor:** diagnostics, profiling and editor tools are not required for the initial runtime.

## Core contracts
- Engine identity/version API, logger, structured diagnostics, typed event dispatcher and worker JobSystem.
- No process-global singleton is imposed; callers own service instances.
- Typed generational handles, owning ResourceRegistry and keyed ResourceStore provide identity and lifetime management.
- ResourceStore is not an asset loader. I/O, decoding, dependency planning, cancellation, caching and GPU upload stay in higher-level systems.
- JobSystem lifecycle methods are not safe to call concurrently or from a worker thread. Callers must respect borrowed-service lifetimes.

## Platform contracts
- Application lifecycle defines initialization, update, stop requests and cleanup.
- Window contract uses opaque IDs, logical/pixel dimensions, DPI scale, focus/minimized state and lifecycle events.
- Surface contract separates a window from its native render surface. Native display/window pointers are borrowed and valid only until the surface is released or destroyed.
- Input contract separates physical keyboard usage IDs, Unicode text, mouse/touch/pen pointer identity, scrolling and normalized gamepad axes.
- Filesystem uses UTF-8 relative virtual paths and rejects traversal/absolute/unsafe paths. NativeFileSystem implements the contract on POSIX/Linux/Android using a pinned root directory descriptor, openat/O_NOFOLLOW traversal, bounded reads, temp-file + atomic-rename writes, sorted directory listings and empty-path root enumeration.
- NativeFileSystem is the rooted POSIX filesystem adapter. AndroidGameActivityBackend is a separate adapter in `engine/platform/android`; it attaches to a host-owned `android_app`, chains/restores app command callbacks and key/motion filters, consumes GameActivity input buffers, reports lifecycle/surface events, and leases `ANativeWindow` references. The Activity, Gradle/manifest setup and android_main remain owned by the embedding app. Desktop window/lifecycle/input backends (SDL, Win32, X11, Wayland, Apple) are not implemented.

## Android GameActivity adapter
- Uses AndroidX GameActivity 4.4.2 and its `android_app`/native-app-glue interfaces. The adapter does not define the process entry point or own the Activity.
- It converts known Android keys to USB HID usages, GameActivity pointer events to logical pixels, joystick axes and key events into the engine InputEvent contract; it enables the non-default joystick axes before reading them.
- `SurfaceCreated`/`SurfaceDestroyed` events delimit the render surface lifetime. The host must drain those events and release/recreate its RenderDevice surface as appropriate.
- The backend has passed ARM64 NDK compilation against the official 4.4.2 headers, not APK linking or physical-device execution. Host Gradle/manifest setup, AssetManager filesystem and full IME support remain integration work.

## World, ECS and simulation
- World owns generational Entity handles and typed component pools. Structural operations use exclusive locking; scoped read/write callbacks use shared/exclusive access. Callbacks must not re-enter the same World or retain references after return.
- Parent links are World-owned; cycles are rejected and parent destruction detaches children.
- Transform2D and Transform3D use column-major matrices with column vectors. Transform composition preserves shear under rotated children and non-uniform parent scale.
- Scene owns a three-phase PreUpdate/Update/PostUpdate scheduler with descending priority and stable registration order. Add/remove behavior during ticks is deterministic; callback exceptions are reported per system.
- Serialization uses a versioned DENG little-endian archive and explicit stable component IDs/versions. World loading is staged. Runtime system callbacks are re-registered by game code after load.
- PhysicsWorld2D provides rigid bodies, circle/box colliders, forces/impulses, filtering, sensors, contact events, raycasts and a deterministic sweep-and-prune broadphase. Known gaps include joints, CCD, sleeping, warm-starting and multi-fixture bodies.
- Physics3D is not implemented.

## Assets
- AssetLoader composes JobSystem, bounded IFileSystem reads, caller-provided decoding and ResourceStore.
- Optional AssetCache is byte-budgeted CPU-side LRU; decoded size is caller-estimated and GPU residency is not managed.
- Cancellation is cooperative and cannot interrupt an active filesystem read or decoder.
- AssetLoadScope provides acquired task leases, cooperative cancellation on shutdown and a blocking wait until in-flight scoped load callables exit. It does not own the borrowed services; the scope must be destroyed before its JobSystem/filesystem/store/cache dependencies. It cannot interrupt active read/decode callbacks.
- AssetRequestCoordinator deduplicates pending requests by caller-provided ID. The first requester owns the factory/cancellation token; subscribers cannot independently cancel the shared operation.
- Dependency planning is deterministic and reports missing dependencies/cycles; execution and lifetime management remain separate.

## Rendering and EngineRuntime
- RenderFramePacket separates engine frame data from the graphics backend.
- validate_frame_packet checks dimensions, timing, camera/matrix values, vertices, indices and draw commands.
- IRenderBackend is the backend contract. RenderDevice owns a SurfaceId lease, refreshes surface metrics, forwards resize, validates frames and forwards texture create/destroy.
- RenderFrameBuilder builds indexed sprite quads and ordered 2D draw commands; it also represents 3D mesh draws at the contract level.
- bgfx is currently an integration proof, not a final locked dependency. The optional bgfx backend can clear and draw 2D sprites when compatible shader binaries are provided. CI compiles ESSL 300 and GLSL 330 shaders and Android ARM64 builds the backend. No 3D mesh/material renderer or physical Android GPU test exists yet.
- EngineRuntime composes Scene, fixed-step clock, ActionMap, PhysicsWorld2D, RenderFrameBuilder and RenderDevice. Native mods are updated from the fixed-step loop. Runtime update, Physics2D fixed steps, begin-render and render-frame operations are native hook points. ECS/physics body synchronization remains game/system-owned.

## Native DMod runtime — no sandbox
- .dmod is a ZIP package with a root manifest. PackageReader validates archive structure, paths, local headers, collisions, sizes, compression ratios and CRC/decompression results before exposing bytes.
- For runtime="native", the manifest names a safe relative path to a prebuilt shared library. NativeModPackageInstaller writes it into an application-private root. POSIX installation uses directory file descriptors, openat, O_NOFOLLOW, restrictive permissions and atomic rename.
- NativeLibrary loads the installed binary with dlopen on POSIX or LoadLibraryW on Windows. The installer currently has no Windows implementation.
- NativeModManager validates descriptor/identity/version, loads dependency order, rolls back failed batches, controls enable/update/disable/unload and defers self-unload until native callbacks return.
- NativeModHostApiV1 exposes logging, query-by-ID and deterministic interface enumeration. It hands mods raw object pointers; it does not use proxies or a security sandbox.
- NativeHookRegistry exposes Before, Around, Replace and After callbacks. Hooks are sorted by descending priority and stable registration ID; After hooks unwind in reverse order. Around/Replace callbacks receive a call_next continuation that may be invoked multiple times or omitted to skip the original operation.
- Hook callback user_data is a caller-defined C++ argument/result context. The engine/game defines each hook point's context structure; modules must compile against matching headers and C++ ABI.
- NativeModManager tracks whether each owner is enabled, disables its hooks on disable, and removes all owned registrations before dlclose/FreeLibrary. Hook dispatch itself is wrapped in the manager's callback-depth guard, so self-unload waits until the whole hook chain exits. A failed/invalid dispatcher falls back to the original operation.
- EngineRuntime now dispatches hooks at runtime.update, physics2d.step, runtime.begin_render_frame and runtime.render_frame. Public contexts expose mutable operation arguments and result reports; native Before/After callbacks can modify/read those data while Around/Replace can repeat, wrap or bypass the original operation.
- Native module IDs are marked disabled before create(), so create-time hook registration is dormant until enable succeeds. Host/game-owned registrations without a manager owner state are active by default. Disable/unload deactivates owned hooks and removes them before the library is closed.
- The verified registry is not magic interception of every C++ method. Scene scheduler per-system callbacks, World/ECS mutation and asset lifecycle still need explicit integration calls.
- EngineRuntime currently registers dream.engine.runtime, dream.engine.scene, dream.engine.world, dream.engine.physics2d, dream.engine.input.actions, dream.engine.clock.fixed, dream.engine.render.frame_builder and dream.engine.render.device.
- Native code runs inside the game process with the app's OS privileges. There is no sandbox, no capability enforcement and no promise of crash/memory isolation. A ZIP path validator only validates package data; it does not make executable code safe.
- Native code can access any registered object and can also use its normal in-process/OS capabilities. Not every future engine subsystem or private implementation detail is automatically registered today. The roadmap includes broader engine service exposure and hook/replace/mixin points.
- Manifest.capabilities is parsed/validated metadata but is not an access-control boundary for native modules.
- Modules must be built and packaged per target ABI/platform. The user's device does not compile them, and loading a package does not rebuild the APK or engine.
- Native unload is not equivalent to safe thread termination. Mods must stop owned threads and unregister callbacks/resources before disable/destroy returns; host-side ownership tracking and full hot reload are not finished.

## Script mods
- runtime="script" remains a separate future lane.
- ChaiScript at the pinned proof commit demonstrated direct AST evaluation with an identity optimizer and no compiled AST nodes. This is a proof only; it is not integrated as the production script runtime.
- Do not describe script execution, memory quotas, sandboxing or reload as implemented.

## Mod API compatibility
- Stable, advanced and experimental are compatibility/documentation tiers, not security boundaries.
- Native ABI structs use a size/version contract. V1 extensions must be append-only and existing fields must retain their meaning.
- Because mods receive raw C++ object pointers, modules need compatible headers, architecture, toolchain/runtime ABI and engine API range. Raw pointer access is powerful but not a promise that every private C++ symbol has a stable cross-version ABI.
- Scene scheduler system invocations are now wired to dream.engine.scene.system, in addition to runtime update/render and Physics2D stepping. Remaining hook work is World/ECS structural/component operations and asset load/decode/register lifecycle, followed by resource/job/thread ownership tracking. Native hooks do not impose a sandbox.

## Dependency direction
- Platform adapters implement platform interfaces.
- Render/physics/audio backends implement engine-owned interfaces.
- Game systems depend on public engine APIs; game-specific systems do not enter engine core.
- Native mods can call registered engine objects directly. The host registry is discovery and compatibility infrastructure, not a security wall.
- Core must not depend on a particular game, platform windowing library or graphics backend.

## Verification policy
- Rooted NativeFileSystem tests and Android ARM64 compilation passed in [run 45816961428](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45816961428).
- Linux GCC/Clang builds run CTest.
- Android CI cross-compiles for arm64-v8a (ARMv8-A); it does not execute native modules on a real phone.
- Physics3D collision geometry and its test passed Linux GCC/Clang CTest and Android ARM64 compile in [run 45828037777](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45828037777).
- Current native interface enumeration is verified in [run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851).
- Renderer backend proof is verified in [run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738).
## Architecture update — 2026-11-26

The live engine now has these additional service boundaries and composition paths (supersedes earlier statements that these areas were not implemented):

- **Physics3D:** body world with sphere and oriented-box colliders, broadphase proxy maintenance, sequential impulse contact solving, friction/restitution, filters, sensors and Begin/Stay/End events. Query APIs expose normalized raycasts (nearest and sorted all hits), sphere overlap and oriented-box overlap. CCD, constraints, sleeping and multi-fixture bodies remain outside the current model.
- **Render:** backend-neutral `RenderFramePacket` and `RenderDevice`; bgfx backend tracks textures, static mesh vertex/index buffers and material descriptions. 3D and 2D render through ordered views (3D before UI/sprites). The selected renderer needs compatible shader binaries installed before backend initialization. No automatic APK asset packaging/renderer initialization yet.
- **UI:** retained UI still owns layout/input/focus; `UiRenderAdapter` converts draw-sink rectangles/images and externally provided glyph quads into `SpriteQuad2D` entries. UI is enqueued after 2D camera setup and before `render_frame`.
- **Assets:** `AssetImporterRegistry::find_for_path` selects importers by virtual-path extension. Runtime importer invocation has a native hook context; filesystem reads, output bounds/validation and hashing remain the responsibility of `AssetImportPipeline`. Built-in OBJ importer serializes typed/versioned DMES meshes.
- **Services:** AnimationRuntime is scheduled in fixed PreUpdate; AudioMixer has a hookable ordinary-thread mix entry. AAudio realtime callbacks use atomically published callbacks and bypass native hooks.
- **Native extension dispatch:** `NativeHookRegistry` supports Before/Around/Replace/After, stable priority and registration ordering, multiple continuation calls, module ownership, disable/unload cleanup and deferred unload while executing. Runtime hook points include update, Physics2D/3D, scene systems, render begin/render, animation update, asset importer invocation, UI draw and ordinary audio mix. Hooks are explicit call sites, not automatic interception of every C++ method.
- **Android:** AndroidX GameActivity app and native host build an ARM64 APK; the host currently validates platform Application/window/surface/input loop only. EngineRuntime and bgfx still need to be attached to that loop.

Native modules are trusted same-process code. They may mutate engine state directly and are not isolated from host memory or operating-system capabilities.


## Android wgpu-native backend — 2026-11-27

The Android host now binds the backend-neutral `RenderDevice` to `dream::render::WgpuNativeBackend` (`engine/render/wgpu_native/`). The implementation stays behind `IRenderBackend`; the active host does not expose WebGPU types to the rest of the engine.

1. GameActivity owns the app lifecycle and Android native window. `RenderDevice` acquires the native-surface lease, keeping the `ANativeWindow` valid while the backend uses it.
2. The backend creates `WGPUInstance`, chains `WGPUSurfaceSourceAndroidNativeWindow` into `WGPUSurfaceDescriptor`, creates the surface, and requests an adapter compatible with it.
3. It requests a device/queue, queries surface capabilities, chooses an sRGB surface format when available, and configures FIFO presentation.
4. The current frame path obtains the current surface texture, creates a view/encoder/render pass, clears the color attachment and presents the frame. Adapter/device callbacks are processed on the initialization thread with a bounded 15-second timeout; WebGPU validation/log output is forwarded to Android Logcat.
5. Surface destruction shuts down the wgpu objects before releasing the platform surface lease; re-creation reinitializes the RenderDevice/backend against the new native surface.

The Android CMake option is `DREAM_BUILD_WGPU_NATIVE_BACKEND` and consumes the verified package root through `WGPU_NATIVE_ROOT`. The app CMake enables this backend and disables bgfx. The bgfx engine module remains optional and may still be used by other targets; it is not the active Android renderer.

**Current capability boundary:** only clear/present is implemented. `supports_2d`, `supports_3d` and instancing remain false; no GPU texture, mesh or material resources are created; draw packets are rejected. The magenta clear frame is a diagnostic to validate surface presentation, not a game scene. Add the WGSL shader/pipeline and resource layers incrementally without leaking wgpu handles into engine-facing APIs.

## Active target architecture — Godot 4.7.2 migration

Godot 4.7.2-stable source is the active engine foundation. Godot should own the scene tree, resource system, rendering, physics, input, audio and Android platform/runtime integration where its facilities meet requirements. Dream-specific engine internals belong in small C++ modules/patches; game content belongs in Godot projects and resources. Do not maintain a second copy of a system that Godot already provides unless a documented requirement proves it necessary.

The prior `engine/` modules, AndroidX GameActivity adapter and wgpu-native host remain preserved reference code during migration. They are not yet deleted or represented as ported. Trusted in-process native mod support remains a requirement and needs an explicit GDExtension plus internal module/hook design.
