# Architecture

**Status:** Core/runtime foundations, World/ECS, serialization, gameplay input, async assets, Physics2D, backend-neutral rendering, EngineRuntime and native in-process DMod loading are implemented and CI-verified. Android platform integration, production 3D rendering/physics and several runtime services remain incomplete.

## High-level modules
- **Core:** time, jobs, events, logging, diagnostics, handles, resource ownership and serialization primitives.
- **Platform:** lifecycle, windows/surfaces, input, filesystem and OS integration. Platform contracts exist; concrete Android/desktop backends remain future work.
- **World:** Scene, World/ECS, entities, components, hierarchy, transforms and scheduler.
- **Assets:** bounded file loading, decoders, dependency planning, CPU cache and request coordination.
- **Rendering:** engine-owned 2D/3D frame and resource contracts with replaceable backends.
- **Simulation:** Physics2D is implemented initially; Physics3D, solver hardening, animation and navigation remain.
- **Runtime:** combines Scene, fixed-step updates, input actions, Physics2D and rendering.
- **DMod:** validated ZIP package, manifest/dependency handling and an unrestricted in-process native runtime. A production script runtime and hook system remain future work.
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
- Filesystem uses UTF-8 relative virtual paths and rejects traversal/absolute/unsafe paths.
- These are interfaces only. No concrete SDL, Android NativeActivity/GameActivity, Win32, X11, Wayland or Apple backend has been completed.

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
- AssetRequestCoordinator deduplicates pending requests by caller-provided ID. The first requester owns the factory/cancellation token; subscribers cannot independently cancel the shared operation.
- Dependency planning is deterministic and reports missing dependencies/cycles; execution and lifetime management remain separate.

## Rendering and EngineRuntime
- RenderFramePacket separates engine frame data from the graphics backend.
- validate_frame_packet checks dimensions, timing, camera/matrix values, vertices, indices and draw commands.
- IRenderBackend is the backend contract. RenderDevice owns a SurfaceId lease, refreshes surface metrics, forwards resize, validates frames and forwards texture create/destroy.
- RenderFrameBuilder builds indexed sprite quads and ordered 2D draw commands; it also represents 3D mesh draws at the contract level.
- bgfx is currently an integration proof, not a final locked dependency. The optional bgfx backend can clear and draw 2D sprites when compatible shader binaries are provided. CI compiles ESSL 300 and GLSL 330 shaders and Android ARM64 builds the backend. No 3D mesh/material renderer or physical Android GPU test exists yet.
- EngineRuntime composes Scene, fixed-step clock, ActionMap, PhysicsWorld2D, RenderFrameBuilder and RenderDevice. Native mods are updated from the fixed-step loop. ECS/physics body synchronization remains game/system-owned.

## Native DMod runtime — no sandbox
- .dmod is a ZIP package with a root manifest. PackageReader validates archive structure, paths, local headers, collisions, sizes, compression ratios and CRC/decompression results before exposing bytes.
- For runtime="native", the manifest names a safe relative path to a prebuilt shared library. NativeModPackageInstaller writes it into an application-private root. POSIX installation uses directory file descriptors, openat, O_NOFOLLOW, restrictive permissions and atomic rename.
- NativeLibrary loads the installed binary with dlopen on POSIX or LoadLibraryW on Windows. The installer currently has no Windows implementation.
- NativeModManager validates descriptor/identity/version, loads dependency order, rolls back failed batches, controls enable/update/disable/unload and defers self-unload until native callbacks return.
- NativeModHostApiV1 exposes logging, query-by-ID and deterministic interface enumeration. It hands mods raw object pointers; it does not use proxies or a security sandbox.
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
- A future hook API should support deterministic registration, before/after/around/replace behavior, explicit priority, ownership tracking, recursion protection, error diagnostics and safe unregistration. Hooks must not impose a sandbox on native mods.

## Dependency direction
- Platform adapters implement platform interfaces.
- Render/physics/audio backends implement engine-owned interfaces.
- Game systems depend on public engine APIs; game-specific systems do not enter engine core.
- Native mods can call registered engine objects directly. The host registry is discovery and compatibility infrastructure, not a security wall.
- Core must not depend on a particular game, platform windowing library or graphics backend.

## Verification policy
- Linux GCC/Clang builds run CTest.
- Android CI cross-compiles for arm64-v8a (ARMv8-A); it does not execute native modules on a real phone.
- Current native interface enumeration is verified in [run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851).
- Renderer backend proof is verified in [run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738).
