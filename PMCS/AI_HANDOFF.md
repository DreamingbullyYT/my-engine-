# AI Handoff

## Project
Dream Engine — general-purpose C++ engine, first-class 2D/3D, Android arm64-v8a, GitHub Actions-first testing, DMod packages with an unrestricted in-process native mod path.

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
- 2D and 3D are both first-class.
- The user has no PC available. All builds/tests must run in GitHub Actions.
- Android target ABI is arm64-v8a (ARMv8-A).
- Keep game-specific systems/content outside engine core.
- Native DMods are full-access in-process code: no sandbox and no capability enforcement. Do not add a permissions layer or pretend the host API is an isolation boundary.
- Native mod packages ship prebuilt target-ABI libraries; do not compile on the user's device or require APK/engine/game rebuild.
- Only native packages the user trusts should be installed. Native code can crash or corrupt the game process and use the app's OS privileges.
- Do not call Android cross-compilation physical-device testing.
- Preserve the root MIT license and record third-party notices.

## Current implementation
- Core: version, logger, diagnostics, typed events, JobSystem, handles, ResourceRegistry/ResourceStore and FixedStepClock.
- Platform: lifecycle/window/surface/input/filesystem contracts; rooted POSIX NativeFileSystem, AndroidAssetFileSystem and AndroidX GameActivity 4.4.2 adapter are implemented and CI-verified for ARM64. Host APK/Gradle/manifest integration and physical-device validation are still missing.
- World: generational ECS, component queries, hierarchy, Transform2D/Transform3D, Scene scheduler and versioned World/Scene archives.
- Input: keyboard/mouse/gamepad ActionMap.
- Assets: bounded async load/decode/register, CPU LRU cache, cancellation, request deduplication and dependency planner.
- Physics2D: body/collider simulation, broadphase, contacts, sensors, filters and raycasts; solver hardening remains. Physics3D currently has AABB/sphere/OBB collision geometry and SAT queries, but not a body world or solver.
- Render: backend-neutral contract, RenderDevice, frame validation and 2D frame builder. bgfx is a working CI proof for clear/2D sprites when runtime shader binaries are supplied; there is no 3D renderer yet.
- Runtime: EngineRuntime composes Scene, fixed-step loop, physics, input and render objects.
- DMod: ZIP package reader, manifest/parser/resolver, native package installer, NativeLibrary and NativeModManager. Native mods can be loaded, enabled, updated, disabled and unloaded; batch dependency order/rollback and deferred self-unload are tested.
- NativeModHostApiV1 provides raw object lookup, logging and deterministic interface enumeration. EngineRuntime exposes runtime, scene, world, Physics2D, action map, fixed clock, render builder, render device, NativeModManager and NativeHookRegistry.
- NativeHookRegistry supports Before/Around/Replace/After, descending priorities, stable order, repeated continuation calls, full replacement, owner deactivation/removal and safe deferred self-unload around dispatch. EngineRuntime already uses hooks for update, PhysicsWorld2D fixed steps, begin-render and render-frame. Scene per-system callbacks are integrated; World/ECS changes and asset lifecycle still need explicit integration.
- Native manager lifecycle, create-time hook activation state, EngineRuntime hook points and Android ARM64 cross-compilation passed in [run 45811326053](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811326053), 4/4 jobs. Generic hook semantics passed in [run 45810170873](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45810170873). Earlier interface enumeration passed in [run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851). Android is cross-compilation only.
- bgfx backend proof passed in [run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738), 2/2 jobs.
- Android GameActivity adapter and all engine tests passed Linux GCC, Linux Clang, Android ARM64 and DMod AST jobs in [run 45820002663](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45820002663). It compiled against official GameActivity 4.4.2 Prefab headers; it has not been linked into an APK or run on a phone.
- ChaiScript direct-AST evaluation is a proof only; no production script runtime is integrated.

## Next work — finish the engine base before advanced modding
1. Integrate the GameActivity adapter into a real Gradle host app and validate on a physical device; full IME/soft-keyboard support remains open. AndroidAssetFileSystem is already implemented.
2. Finish runtime shader packaging/loading and operational 2D rendering, then implement the 3D renderer and GPU resource pipeline.
3. Harden Physics2D; extend Physics3D geometry into a body world, broadphase, integration and contact solver.
4. Add audio, animation, runtime UI and asset import where needed.
5. Add broad concurrency/lifetime stress tests, performance profiling and two representative validation games.
6. Only after the base is complete, resume deep DMod work: World/ECS and asset hooks, owned resource/job/thread cleanup, hot reload, publisher trust/signatures and native .dmod device tests. Do not add sandbox/capability restrictions.
7. Revisit script mods only as a separate optional path; native full-access mods do not depend on a script interpreter.

## Handoff requirements
- Update CURRENT_STATE.md, ARCHITECTURE.md, TECHNICAL_AUDIT.md and ROADMAP.md after implementation milestones.
- Update DECISIONS.md when the user explicitly changes or accepts a design direction.
- Never mark work VERIFIED without a reproducible run or device test.
- Record exact commit/run IDs, limitations and the next concrete action.
## Live handoff — 2026-11-26

**Current main:** `54f6e8c39f599be919c669e6316b807cdf220654`. **Last fully passing engine CI:** [45859870368](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859870368) — GCC, Clang, ASan/UBSan, Android ARM64 and DMod AST proof all passed; 46/46 tests across each Linux test matrix.

### Verified current capabilities
- C++20 general-purpose engine foundation, World/ECS/system scheduler, fixed-step clock, platform contracts and Android GameActivity host shell.
- Physics2D world and Physics3D world (spheres/OBBs), filters, sensors, contact events and 3D ray/overlap queries.
- Backend-neutral frame builder/render device plus bgfx 2D sprites/textures and 3D meshes/materials. Shader binaries must currently be supplied to the backend explicitly.
- Retained UI runtime and indexed-2D rendering adapter; `EngineRuntime::enqueue_ui_draws` is the frame-loop integration point.
- Asset loader/cache/coordinator and bounded OBJ-to-DMES importer.
- Audio mixer, animation runtime and Android AAudio backend.
- Unrestricted native in-process DMod modules and automatic hooks at runtime, physics, scene, animation, importer invocation, UI draw and non-real-time audio mix boundaries.
- Two CTest headless sample games in `examples/`.

### Do not overstate
- The APK artifact is a GameActivity host smoke app, not a playable 2D/3D game. It does not initialize EngineRuntime or bgfx and does not package/load shader binaries.
- APK artifact page: [run 45859471974](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45859471974), artifact `15138770221` (built from `ee578327...`; Android host source remained unchanged after that). A newer APK needs to be built after runtime-host integration.
- No physical phone test has been done. User explicitly reserved that step for their phone.
- Headless examples test simulation and draw submission, not GPU pixels.
- Physics3D lacks CCD, joints, sleeping and multi-fixture bodies. OBJ is the only built-in model importer. UI text needs caller-supplied glyph provider.
- Native modules are not sandboxed and may crash/corrupt the process.

### Immediate next tasks
1. Verify/finish the Linux bgfx static-library/shader proof run; Android bgfx static-library proof already passed.
2. Build the actual GameActivity EngineRuntime host: map Android app lifecycle/window/input to EngineRuntime, load compiled shader assets and initialize bgfx on the native surface.
3. Run a fresh APK build and give the user that artifact for first phone smoke test.
4. Continue Physics3D CCD/joints and expand asset importers; keep CI tests and PMCS synchronized.

### Workflow discipline
- Fetch current main before editing; preserve concurrent changes.
- Use GitHub Actions for builds/tests (user has no PC).
- Record run IDs, commit hashes, and exact limitations. Cross-compilation is never physical-device validation.
- Native mods have no sandbox by explicit user choice; don't add sandbox/capability restrictions.


## Latest handoff — 2026-11-26, main `96dc09d43a8b4f244088a9469458d77c3e5bb483`

This section supersedes the older implementation snapshot and task order above.

### Verified
- [Dream Engine CI 45885498968](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885498968) passed all five jobs: GCC, Clang, ASan/UBSan, Android arm64-v8a (ARMv8-A) and DMod AST proof. All three Linux configurations passed 47/47 CTest cases.
- Asset request subscribers can cancel their own wait without stopping the shared in-flight result for other subscribers. Deferred futures do not spin forever.
- World ECS operations expose native hooks for entity creation/destruction, component emplace/remove/modify, multi-component mutation, mutable iteration and hierarchy changes. Hooks run outside the World lock. A continuation trampoline bug was fixed and is covered by ASan/UBSan.
- Android native host source creates EngineRuntime, loads APK GLES 3.0 sprite/mesh shader assets, initializes bgfx on GameActivity's surface, submits 2D/3D physics demo draws, routes retained UI touch input, and connects AudioMixer to AAudio.
- Android UI dimensions now use logical pixels; camera/framebuffer projection uses physical pixels. Simulation is paused when Android is suspended or the native surface is unavailable.
- Structured Android logs go to Logcat tag `DreamEngine` and to a rotating file under GameActivity external app files, with internal app files as fallback.

### APK status
[Android host APK workflow 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085) is building from this same commit. The job must reach the artifact upload step and report success before providing a download link. APK settings: package `dev.dreamengine.host`, minSdk 26, ABI `arm64-v8a`. Do not reuse the older shell-only APK artifact.

### Remaining tasks (do not mark complete)
- Full IME/soft-keyboard composition and state.
- Advanced Physics2D/3D: CCD, joints, sleeping/waking, multi-fixture bodies, robust multi-point contact manifolds; accelerate 3D ray/overlap queries.
- GPU asset residency/device-loss/resource-lifetime policy and device-level renderer validation.
- MTL, glTF/GLB, image/texture and audio importers beyond the bounded OBJ importer.
- Full font shaping/atlas, rounded-corner UI masks, animation blending/events/curves and audio device telemetry.
- Separate asset file-read/decode/register hook points.
- Safe mod-owned resource/job/thread tracking and hot reload; publisher signatures/trust policy. Preserve the user's explicit no-sandbox/no-capability-enforcement decision.
- Full profiler, editor/tooling and two real games on a real backend.

### Immediate next action
Wait for [APK workflow 45885499085](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45885499085) to finish. If successful, provide that ARM64 debug APK for the user's first phone test. The user, not CI, performs the physical-device validation. After that test, resume the remaining production engine work above; do not imply the complete PMCS backlog is finished.


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
