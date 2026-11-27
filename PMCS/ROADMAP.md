# Roadmap

Milestones are marked complete only with reproducible evidence. Android cross-compilation is not physical-device validation.

## P0 — Requirements and technical proof
- [x] General-purpose C++ engine scope.
- [x] First-class 2D and 3D requirement.
- [x] GitHub Actions CI for Linux GCC/Clang and Android ARM64.
- [x] Linux GCC/Clang build and CTest baseline.
- [x] Android NDK core cross-build for arm64-v8a.
- [x] ChaiScript direct-AST proof with identity optimizer; this does not select a production script runtime.
- [x] C++20 is the engine-wide language baseline (CMake `cxx_std_20`, Android NDK and Linux CI); the oldest supported compiler-version matrix remains open.

## P1 — Foundation
- [x] Core library, version, logging, diagnostics, events and JobSystem.
- [x] Generational handles, ResourceRegistry and ResourceStore.
- [x] Application lifecycle contract.
- [x] Window/surface/input/filesystem contracts and virtual-path validation.
- [x] Rooted POSIX NativeFileSystem for Linux/Android with openat/O_NOFOLLOW traversal, bounded reads, atomic writes and root directory listing (CI run 45816961428).
- [x] AndroidX GameActivity 4.4.2 lifecycle/window/surface/key/pointer/gamepad adapter cross-compiled for ARM64 ([run 45820002663](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45820002663)).
- [x] Android GameActivity Gradle/manifest host and ARM64 APK workflow are integrated; [ ] physical-device validation is reserved for the user.
- [x] Android APK AssetManager read-only filesystem (cross-compiled in run 45820002663).
- [ ] Full IME/soft-keyboard text-input support.
- [ ] Expanded concurrency, lifetime and shutdown stress tests.

## P2 — World and core runtime
- [x] Scene/World lifecycle and deterministic system scheduler.
- [x] Generational ECS, component queries, hierarchy and 2D/3D transforms.
- [x] Versioned binary archive and explicit-codec World/Scene persistence.
- [x] Gameplay ActionMap for keyboard, mouse and gamepad.
- [x] Bounded async asset read/decode/register.
- [x] Asset dependency planner, CPU LRU cache, cooperative cancellation and in-flight request coordination.
- [x] Explicit async task lifetime scope, cooperative cancellation and shutdown wait for borrowed-service safety (run 45818494612).
- [x] Independent cancellation for individual subscribers of a deduplicated shared asset request, without cancelling shared work for other subscribers.
- [ ] GPU asset upload/residency integration.

## P3 — Rendering
- [x] Engine-owned backend-neutral rendering contract and RenderDevice.
- [x] Frame packet validation and deterministic 2D sprite frame builder.
- [x] bgfx Linux/Android backend compile proof and ESSL 300 / GLSL 330 shader compile proof ([run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738)).
- [x] Android ESSL 300 sprite/mesh shader compile, APK packaging and runtime loading path is implemented; fresh APK workflow verification is running.
- [x] Android 2D sprite/UI render submission path is implemented through bgfx and EngineRuntime; [ ] visual correctness on a physical GPU remains unverified.
- [x] bgfx 3D mesh/material buffers, mesh shaders, camera and draw submission path are implemented; [ ] physical GPU validation remains open.
- [ ] Shared GPU asset/resource lifetime and device-loss handling.
- [ ] Final renderer dependency decision after device testing.

## P4 — Simulation
- [x] Initial PhysicsWorld2D rigid-body, collision/filter/sensor/contact/raycast implementation.
- [x] Deterministic sweep-and-prune broadphase.
- [ ] Physics2D solver hardening: joints, CCD, sleeping, warm starting, multiple fixtures and more stable contact manifolds.
- [x] Physics3D collision geometry: sphere, AABB, OBB and OBB SAT queries ([run 45828037777](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45828037777)).
- [x] Physics3D body world, deterministic broadphase, integration, angular/linear contact solving, filters/sensors, contact events and ray/overlap queries.
- [ ] Animation and navigation integration.

## P5 — DMod and unrestricted native mods
- [x] Manifest model/parser, validation and deterministic dependency resolver.
- [x] Bounded in-memory ZIP package reader with local-header, path, collision, size and CRC checks.
- [x] Native library open/symbol lookup.
- [x] Native .dmod package installation into app-private storage (POSIX implementation).
- [x] NativeModHostApiV1 with raw interface query and logging.
- [x] NativeModManager dependency order, batch rollback, enable/update/disable/unload lifecycle.
- [x] Deferred self-unload after native callbacks return.
- [x] EngineRuntime raw interface exposure for runtime, scene, world, Physics2D, input actions, fixed clock and render objects.
- [x] Deterministic host interface enumeration with ID/version reporting ([run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851)).
- [x] Explicit user decision: native mods run in-process with no sandbox and no capability enforcement.
- [x] EngineRuntime raw host exposure includes World/Scene, Physics2D/3D, animation, audio, UI, asset importer registry, input/action map, clock, render device/builder, mod manager and hook registry.
- [x] NativeHookRegistry with Before/Around/Replace/After stages, deterministic priority/registration order and repeatable call_next ([run 45810170873](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45810170873)).
- [x] Manager-owned callback enable/disable/removal and deferred self-unload until hook dispatch exits.
- [x] EngineRuntime hook points for runtime update, each PhysicsWorld2D step, begin-render-frame and render-frame, with mutable typed contexts ([run 45811326053](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811326053)).
- [x] Integrate per-system hooks in the Scene scheduler (dream.engine.scene.system; run 45811326053).
- [x] World/ECS entity, component, iteration and hierarchy mutation hooks are integrated into EngineRuntime; importer invocation has a typed hook. [ ] Separate file-read/decode/register lifecycle hooks remain open.
- [ ] Track module-owned resources/jobs/threads and make unload/hot reload safe; hook registrations are currently tracked and removed automatically.
- [ ] Native package signature/publisher trust policy; signatures verify origin/integrity, not runtime isolation.
- [ ] Native .dmod load/unload test on a real Android ARM64 device.
- [ ] Decide whether a separate script runtime adds enough value. ChaiScript is currently proof-only; it is not a production runtime.

## P6 — Runtime services and content
- [x] Initial device-independent keyboard/mouse/gamepad ActionMap.
- [x] Software AudioMixer and Android AAudio output backend are implemented and cross-compiled; hardware playback/latency/route behavior still needs device validation.
- [x] Animation runtime is implemented and scheduled by EngineRuntime; blending, event and curve extensions remain open.
- [x] Retained UI, pointer/focus activation, UI-to-render adapter and Android touch demo integration are implemented; text shaping/font atlas and rounded-corner masking remain open.
- [x] Bounded asset import pipeline and OBJ-to-DMES importer are implemented; MTL, glTF/GLB, image/texture and audio importers remain open.
- [ ] Networking/localization where justified.

## P7 — Tools
- [x] Structured logs, Android Logcat/file sinks and periodic runtime metrics are integrated; full CPU/GPU profiler remains open.
- [ ] Editor/tooling architecture.
- [x] Android demo includes a retained touch button and touch-to-physics interaction; broader game/editor tooling remains open.

## P8 — Validation games
- [ ] Validate with the planned planet/colony game.
- [ ] Validate with a substantially different 3D game.
- [ ] Confirm neither project requires an engine fork.

## Foundation-first execution order
The user chose to complete the engine foundation before expanding advanced DMod/modding features. Until the base is complete, keep DMod changes limited to correctness fixes and compatibility needed by engine work. Prioritize: (1) Android host integration, (2) operational 2D rendering and full 3D renderer/GPU resources, (3) Physics2D hardening and a complete Physics3D world/solver, (4) audio/animation/UI and asset import, (5) stress/performance/device validation. Then return to deep native mod APIs, World/ECS and asset hooks, owned-resource/job/thread cleanup, hot reload and Android package validation.

## Explicit non-goals
- Sandboxing or capability restrictions for native mods. Native modules are trusted in-process code by user decision.
- Compiling mods on the end user's device or requiring an APK/engine rebuild for mod installation.
- Claiming Android runtime correctness from cross-compilation alone.
## Current execution status — 2026-11-26

This live status supersedes older checkboxes above where they conflict. Evidence: [CI 45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368); current main `54f6e8c39f599be919c669e6316b807cdf220654`.

### Verified complete in this milestone
- [x] AndroidX GameActivity Gradle host builds an ARM64 debug APK and contains `libdream_engine_android_host.so`. This is a platform-host shell, not a playable engine app.
- [x] Runtime integration test suite and two executable headless 2D/3D sample games pass 46/46 under GCC, Clang and Clang ASan/UBSan.
- [x] Android ARM64 cross-compilation of the engine runtime, Android audio backend and sample-game source succeeds.
- [x] Physics3D raycast, raycast-all and sphere/OBB overlap queries.
- [x] bgfx CPU/GPU resource API implementation for sprites, textures, meshes and materials; renderer-specific shader compilation is a separate proof workflow.
- [x] Retained UI-to-2D-renderer adapter and EngineRuntime frame-order integration.
- [x] Bounded OBJ importer and versioned binary mesh decoding.
- [x] Native hooks now cover animation, importer invocation, UI drawing and regular (non-real-time) audio mixing in addition to existing runtime/physics/scene/render hooks.

### Remaining work, in priority order
1. **Android runtime host:** create EngineRuntime in the GameActivity native loop, wire real input/window metrics/lifecycle, initialize bgfx on the borrowed native surface, load renderer-compatible shaders and render a sample scene. Rebuild the APK after this integration.
2. **Phone validation:** user installs the first APK and reports launch/surface/lifecycle results. Do not mark device validation complete from Actions.
3. **Renderer completion:** automatic shader packaging/loading; validate GLES3 on-device; 2D texture/font/sprite path; 3D mesh/material pipeline on device; add material maps, samplers, depth/culling correctness, and GPU resource lifecycle/recreation tests.
4. **Physics:** add CCD, distance/revolute joints, sleeping/waking, multi-fixture bodies, robust OBB clipping/multi-point manifolds, query acceleration and deterministic stress tests.
5. **Assets:** robust concave OBJ triangulation, MTL, glTF/GLB, image/texture and audio importers, dependency graph and importer validation tests.
6. **Services:** built-in text shaping/font atlas or documented provider package; UI rounded-corner shader/clipping validation; connect audio mixer to AAudio output with device lifecycle and underrun telemetry; animation blending/events/curves.
7. **Native mod API:** add hook sites for ECS structural/component mutations and asset load/decode/register lifecycle; ABI version/capability metadata and crash/reload policy. No sandbox (user decision).
8. **Stability/performance:** concurrency stress/race tests, allocator and memory-budget tests, deterministic replay, CPU/GPU profiling, soak tests, Android lifecycle recovery and two sample games on a real backend.

Physical-device validation remains a user action and is explicitly not part of CI completion.


## Authoritative execution status — 2026-11-26 (supersedes older status sections)

- **Main commit:** `96dc09d43a8b4f244088a9469458d77c3e5bb483`.
- **Verified engine CI:** [run 45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968) passed 5/5 jobs: Linux GCC, Linux Clang, Linux Clang ASan+UBSan, Android arm64-v8a (ARMv8-A), and DMod/ChaiScript AST proof. The Linux jobs each passed **47/47 CTest cases**.
- **APK build:** [run 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085) is building the fresh ARM64 APK from this same commit. Do not treat an artifact as available until the workflow has completed and uploaded it.
- **Android host implementation:** the GameActivity native loop now owns an EngineRuntime, loads four GLES 3.0 shader binaries from APK assets, initializes/recreates bgfx on the native surface, updates a combined 2D/3D physics demo, routes touch/UI input, and connects AudioMixer to AAudio. The APK workflow compiles and packages the shader binaries and checks the ARM64 .so plus shader assets.
- **Android lifecycle fixes:** simulation and UI input are not advanced while the app is suspended or the render surface is unavailable; UI layout uses logical pixels to match the GameActivity pointer coordinate convention; GPU resources are uploaded again after surface recreation.
- **Logs:** Android Logcat tag is `DreamEngine`. The app writes `logs/engine.log` under GameActivity `externalDataPath`, falling back to `internalDataPath`; the file sink creates the directory and rotates at 8 MiB with three backups. Runtime metrics are periodically recorded.
- **New asset coordination:** each subscriber can cancel its own wait/result without cancelling a shared in-flight request for other subscribers. Deferred futures are handled without an infinite wait loop.
- **New native mod hooks:** World entity create/destroy, component emplace/remove/modify, multi-component modify, mutable iteration, and parent set/clear operations dispatch typed native hooks outside the World lock. EngineRuntime passes the actual WorldMutationContext through the continuation trampoline; CI exercises this under ASan/UBSan.
- **Still open before production completeness:** full IME/soft-keyboard composition; complete GPU asset residency/device-loss policy; advanced Physics2D joints/CCD/sleeping/multi-fixture and stable manifolds; Physics3D CCD/joints/sleeping/multi-fixture/robust clipping and accelerated queries; extended MTL/glTF/GLB/image/audio import; full font shaping/atlas and rounded-corner masks; animation blends/events/curves; asset read/decode/register hooks; safe ownership tracking for mod resources/jobs/threads and hot reload; publisher trust/signatures; a full profiler/editor; and real game/device validation. These are not being marked complete merely because the APK host builds.
- **Device testing:** physical-device execution, rendered-pixel validation, AAudio playback verification and native .dmod load/unload on a real phone remain user-run steps. Android CI is cross-compilation/package verification only.


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


## Concurrency status — 2026-11-26

The native mod manager now serializes its public state and callback-lifetime operations, supports same-thread callback re-entry, and has a multithreaded dispatch regression test plus a targeted ThreadSanitizer CI job. The broader stability work remains open: test the full engine for cross-subsystem races, verify TSan in Actions, and validate mod-owned worker-thread shutdown/resource lifetime. A green Android cross-build does not replace execution on a physical phone.


## P3 renderer migration status — 2026-11-27

- [x] Select `wgpu-native` as the Android host backend while preserving the engine-owned `IRenderBackend` contract.
- [x] Pin and SHA-256 verify the official Android AArch64 `wgpu-native v29.0.1.1` release in GitHub Actions.
- [x] Compile/link/package the C++ Android host plus `libwgpu_native.so` for `arm64-v8a`; APK workflow run 91 passed.
- [x] Create the Android native-window surface, request adapter/device/queue, configure the surface and submit clear/present frames at the code/build level.
- [ ] Physical-device proof that the magenta clear frame is visible, including surface destroy/recreate and app pause/resume.
- [ ] Build WGSL 2D pipeline: colored/textured quads, camera/projection, blend modes, batching, bind groups, texture upload and GPU resource lifetime.
- [ ] Build WGSL 3D pipeline: vertex/index buffers, transforms, camera, depth texture/testing, materials, texture/sampler bindings and draw ordering.
- [ ] Complete GPU asset residency, device-loss recovery and robust native resource teardown; test on multiple Android GPU vendors.
- [ ] Rebuild the previous 2D/UI/3D host demo on wgpu after the above capabilities exist. The active wgpu diagnostic host is not feature-equivalent to the prior bgfx demo.

The bgfx backend and its existing 2D/3D code remain optional engine-side implementation/reference paths. The current Android app explicitly disables bgfx and links `DreamEngine::RenderWgpuNative`; do not silently restore bgfx as the Android renderer. The native dependency choice is settled for this Android milestone, but future desktop backends and cross-platform packaging remain open to separate evidence.
