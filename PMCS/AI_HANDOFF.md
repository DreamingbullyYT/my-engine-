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
- Platform: lifecycle/window/surface/input/filesystem contracts; concrete Android platform backend is still missing.
- World: generational ECS, component queries, hierarchy, Transform2D/Transform3D, Scene scheduler and versioned World/Scene archives.
- Input: keyboard/mouse/gamepad ActionMap.
- Assets: bounded async load/decode/register, CPU LRU cache, cancellation, request deduplication and dependency planner.
- Physics2D: body/collider simulation, broadphase, contacts, sensors, filters and raycasts; solver hardening remains.
- Render: backend-neutral contract, RenderDevice, frame validation and 2D frame builder. bgfx is a working CI proof for clear/2D sprites when runtime shader binaries are supplied; there is no 3D renderer yet.
- Runtime: EngineRuntime composes Scene, fixed-step loop, physics, input and render objects.
- DMod: ZIP package reader, manifest/parser/resolver, native package installer, NativeLibrary and NativeModManager. Native mods can be loaded, enabled, updated, disabled and unloaded; batch dependency order/rollback and deferred self-unload are tested.
- NativeModHostApiV1 provides raw object lookup, logging and deterministic interface enumeration. EngineRuntime exposes runtime, scene, world, Physics2D, action map, fixed clock, render builder, render device, NativeModManager and NativeHookRegistry.
- NativeHookRegistry supports Before/Around/Replace/After, descending priorities, stable order, repeated continuation calls, full replacement, owner deactivation/removal and safe deferred self-unload around dispatch. EngineRuntime already uses hooks for update, PhysicsWorld2D fixed steps, begin-render and render-frame. Scene per-system callbacks, World/ECS changes and asset lifecycle still need explicit integration.
- Native manager lifecycle, create-time hook activation state, EngineRuntime hook points and Android ARM64 cross-compilation passed in [run 45811326053](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45811326053), 4/4 jobs. Generic hook semantics passed in [run 45810170873](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45810170873). Earlier interface enumeration passed in [run 45807962851](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45807962851). Android is cross-compilation only.
- bgfx backend proof passed in [run 45790590738](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45790590738), 2/2 jobs.
- ChaiScript direct-AST evaluation is a proof only; no production script runtime is integrated.

## Next work
1. Build the concrete Android application/window/surface/filesystem adapter and validate it on a physical device.
2. Finish runtime shader packaging/loading and operational 2D rendering, then implement the 3D renderer and GPU resource pipeline.
3. Harden Physics2D and implement Physics3D.
4. Add audio, animation, runtime UI and asset import where needed.
5. Continue hooks at Scene individual system invocations, World/ECS mutation and asset lifecycle using public typed context structs. Then add resource/job/thread ownership and hot-reload safety. Do not add sandbox/capability restrictions.
6. Add publisher/signature trust metadata and test native .dmod install/load on Android ARM64.
7. Revisit script mods only as a separate optional path; native full-access mods do not depend on a script interpreter.

## Handoff requirements
- Update CURRENT_STATE.md, ARCHITECTURE.md, TECHNICAL_AUDIT.md and ROADMAP.md after implementation milestones.
- Update DECISIONS.md when the user explicitly changes or accepts a design direction.
- Never mark work VERIFIED without a reproducible run or device test.
- Record exact commit/run IDs, limitations and the next concrete action.
