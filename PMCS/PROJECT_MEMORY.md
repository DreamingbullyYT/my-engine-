# Project Memory — Dream Engine

**Updated:** 2026-11-26

## Identity
Dream Engine is a reusable, general-purpose C++ game engine. The initial planet/colony game is a validation project, not the engine's purpose. A different 3D game must not require an engine fork.

## Accepted requirements
- C++ is the primary implementation language. Do not switch engine subsystems to Rust.
- 2D and 3D are both first-class from the start.
- Keep subsystems modular and reusable; game-specific rules/content remain in game projects.
- Research and reuse suitable libraries, but audit exact source, version, license, platform support and build cost.
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Initial Android ABI: arm64-v8a (ARMv8-A).
- Keep platform APIs behind engine-owned interfaces.

## DMod direction — updated 2026-11-26
- .dmod remains the package/container format and supports a manifest with runtime kind, version and dependency metadata.
- **The full-access mod path is native and in-process.** A runtime="native" package contains a prebuilt shared library for its target platform/ABI. The engine loads it into the game process.
- Native modules run with the application's process privileges. There is no sandbox, no capability enforcement and no security isolation between a native mod and the engine. A native mod can mutate engine/game memory and state, use resources available to the app, corrupt data or crash the process. Install only trusted native mods.
- Native modules receive raw pointers to live engine objects through NativeModHostApiV1. The host can query interfaces by ID and enumerate registered IDs/versions deterministically. This is a real object interface, not a sandboxed proxy.
- No compilation is performed on the user's device and installing a mod must not require rebuilding the APK, engine or game. Mod authors/distributors must provide the correct prebuilt binary for each target ABI/platform.
- Manifest capabilities may remain metadata for future script/runtime use, but they are not an access-control boundary for native modules.
- The current EngineRuntime exposes runtime, scene, world, PhysicsWorld2D, ActionMap, fixed clock, RenderFrameBuilder and RenderDevice. This is not yet the complete future engine API; newly added services and internal hook points must be exposed as the engine grows.
- A dedicated hook/replace/mixin system for deep engine/game behavior changes is still future work. Raw pointer access alone does not mean every private C++ method has a stable ABI.
- runtime="script" is a separate possible path. No production script interpreter has been selected or integrated. ChaiScript remains a direct-AST proof candidate, not an implemented mod runtime.

## Engine scope
The reusable engine provides generic core services, platform adapters, rendering, resources, scenes/worlds, entities/components, input, 2D/3D physics adapters, audio, animation, jobs, serialization and diagnostics. Economy, colonies, planets, citizens, buildings and progression belong to games or game modules.

## Engineering policy
- Do not claim a subsystem or runtime is complete without reproducible CI/device evidence.
- Android cross-build is not physical-device proof.
- Preserve the repository's MIT license and record third-party notices separately.
- Update PMCS after every accepted decision and verified implementation milestone.


## Native hook implementation — 2026-11-26
- NativeHookRegistry is part of the full-access native mod runtime. It supports Before/Around/Replace/After, priority ordering, repeated call_next and full replacement.
- It gives mods raw typed-by-convention context pointers defined by each engine/game hook point, not sandboxed proxies.
- Hooks are disabled with the owner mod and erased before library unload. Self-unload inside a hook is deferred through NativeModManager until the whole dispatch completes.
- CI run 45810170873 passed on Linux GCC, Linux Clang and Android arm64-v8a cross-compilation, plus the DMod AST proof.
- Automatic hook calls still need to be inserted at real EngineRuntime/Scene/physics/render/World/assets operations; generic dispatch alone does not intercept every C++ method.
