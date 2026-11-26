# Roadmap

Milestones are marked complete only with reproducible evidence. Android cross-compilation is not physical-device validation.

## P0 — Requirements and technical proof
- [x] General-purpose C++ engine scope.
- [x] First-class 2D and 3D requirement.
- [x] GitHub Actions CI for Linux GCC/Clang and Android ARM64.
- [x] Linux GCC/Clang build and CTest baseline.
- [x] Android NDK core cross-build for arm64-v8a.
- [x] ChaiScript direct-AST proof with identity optimizer; this does not select a production script runtime.
- [ ] Final C++ standard/compiler-support policy.

## P1 — Foundation
- [x] Core library, version, logging, diagnostics, events and JobSystem.
- [x] Generational handles, ResourceRegistry and ResourceStore.
- [x] Application lifecycle contract.
- [x] Window/surface/input/filesystem contracts and virtual-path validation.
- [x] Rooted POSIX NativeFileSystem for Linux/Android with openat/O_NOFOLLOW traversal, bounded reads, atomic writes and root directory listing (CI run 45816961428).
- [x] AndroidX GameActivity 4.4.2 lifecycle/window/surface/key/pointer/gamepad adapter cross-compiled for ARM64 ([run 45820002663](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45820002663)).
- [ ] Android host APK/Gradle/manifest integration and physical-device validation.
- [ ] Android APK AssetManager read-only filesystem and full IME/soft-keyboard text-input support.
- [ ] Expanded concurrency, lifetime and shutdown stress tests.

## P2 — World and core runtime
- [x] Scene/World lifecycle and deterministic system scheduler.
- [x] Generational ECS, component queries, hierarchy and 2D/3D transforms.
- [x] Versioned binary archive and explicit-codec World/Scene persistence.
- [x] Gameplay ActionMap for keyboard, mouse and gamepad.
- [x] Bounded async asset read/decode/register.
- [x] Asset dependency planner, CPU LRU cache, cooperative cancellation and in-flight request coordination.
- [x] Explicit async task lifetime scope, cooperative cancellation and shutdown wait for borrowed-service safety (run 45818494612).
- [ ] Independent cancellation for individual subscribers of a deduplicated shared asset request.
- [ ] GPU asset upload/residency integration.

## P3 — Rendering
- [x] Engine-owned backend-neutral rendering contract and RenderDevice.
- [x] Frame packet validation and deterministic 2D sprite frame builder.
- [x] bgfx Linux/Android backend compile proof and ESSL 300 / GLSL 330 shader compile proof ([run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738)).
- [ ] Runtime shader asset packaging/loading for Android and Linux.
- [ ] Complete operational 2D renderer and physical-device validation.
- [ ] 3D renderer, mesh buffers, materials, lighting and camera path.
- [ ] Shared GPU asset/resource lifetime and device-loss handling.
- [ ] Final renderer dependency decision after device testing.

## P4 — Simulation
- [x] Initial PhysicsWorld2D rigid-body, collision/filter/sensor/contact/raycast implementation.
- [x] Deterministic sweep-and-prune broadphase.
- [ ] Physics2D solver hardening: joints, CCD, sleeping, warm starting, multiple fixtures and more stable contact manifolds.
- [ ] Physics3D implementation and adapter.
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
- [ ] Expand raw host exposure as additional engine systems are implemented.
- [x] NativeHookRegistry with Before/Around/Replace/After stages, deterministic priority/registration order and repeatable call_next ([run 45810170873](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45810170873)).
- [x] Manager-owned callback enable/disable/removal and deferred self-unload until hook dispatch exits.
- [x] EngineRuntime hook points for runtime update, each PhysicsWorld2D step, begin-render-frame and render-frame, with mutable typed contexts ([run 45811326053](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811326053)).
- [ ] Integrate per-system hooks in Scene scheduler, World/ECS mutation hooks and asset lifecycle hooks.
- [ ] Track module-owned resources/jobs/threads and make unload/hot reload safe; hook registrations are currently tracked and removed automatically.
- [ ] Native package signature/publisher trust policy; signatures verify origin/integrity, not runtime isolation.
- [ ] Native .dmod load/unload test on a real Android ARM64 device.
- [ ] Decide whether a separate script runtime adds enough value. ChaiScript is currently proof-only; it is not a production runtime.

## P6 — Runtime services and content
- [x] Initial device-independent keyboard/mouse/gamepad ActionMap.
- [ ] Audio backend and mixer.
- [ ] Animation runtime.
- [ ] Runtime UI integration.
- [ ] Asset import pipeline.
- [ ] Networking/localization where justified.

## P7 — Tools
- [ ] Runtime diagnostics/profiler integration.
- [ ] Editor/tooling architecture.
- [ ] Touch-friendly Android workflows.

## P8 — Validation games
- [ ] Validate with the planned planet/colony game.
- [ ] Validate with a substantially different 3D game.
- [ ] Confirm neither project requires an engine fork.

## Explicit non-goals
- Sandboxing or capability restrictions for native mods. Native modules are trusted in-process code by user decision.
- Compiling mods on the end user's device or requiring an APK/engine rebuild for mod installation.
- Claiming Android runtime correctness from cross-compilation alone.
