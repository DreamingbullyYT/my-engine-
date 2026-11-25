# Architecture

**Status:** Initial C++ core, logger, diagnostics, event dispatcher and job system are CI-verified. ChaiScript direct AST evaluation proof passes on Linux; Android compatibility and runtime safety remain open.

## High-level modules
- **Platform:** desktop and Android lifecycle, windows/surfaces, input, files, threads and OS integration.
- **Core:** memory/handles, jobs, events, logging, diagnostics, serialization and resource identity.
- **World:** scenes, entities, components, transforms and world lifecycle.
- **Rendering:** engine-owned API; 2D and 3D render paths; cameras, materials, shaders, lighting, particles and post-processing.
- **Simulation:** 2D physics, 3D physics, animation and navigation through replaceable adapters.
- **Runtime services:** input actions, audio, UI, networking and localization as optional capabilities.
- **DMod:** package validation, dependency resolver, load order, source interpreter, bindings, hooks, lifecycle, permissions and hot reload.
- **Game project:** game-specific systems, content, assets, UI and scripts.
- **Tools/editor:** editor and debugging tools consume engine APIs but are not required by runtime.

## Core contracts implemented
- Engine identity/version API.
- Logger instance with severity filtering and replaceable sink. Sink configuration is synchronized; user callback is invoked outside the lock to avoid callback-induced deadlock.
- Structured diagnostic value carrying severity, stable code, message and source location string.
- Typed event dispatcher with move-only RAII connections, descending priority and stable registration order, snapshot dispatch, callback invocation outside locks, re-entrancy and per-listener exception isolation. Dispatch returns invoked/failed counts.
- Job system with a configurable worker pool, futures, task exception propagation, `wait_idle()`, and shutdown that stops submissions, drains queued tasks and joins workers. Lifecycle operations are not safe to call concurrently or from a worker thread.
- No process-global singleton is imposed; callers own service instances and inject them where needed.

## Dependency direction
Platform adapters implement platform interfaces. Backends implement engine-owned rendering/physics/audio interfaces. Game systems depend on public engine APIs. DMods depend on versioned DMod APIs and granted capabilities. Core must not depend on a particular game, renderer or physics library.

## 2D/3D coexistence
2D and 3D may use specialized pipelines where semantics differ. Share resource identity, asset management, scheduling, diagnostics and other services where appropriate. A project may use 2D, 3D or both. Avoid duplicating the entire engine.

## DMod package lifecycle
1. Discover package.
2. Validate archive, manifest, paths and declared API version.
3. Resolve dependencies and conflicts.
4. Determine deterministic load order.
5. Create mod context and granted capabilities.
6. Parse and interpret source.
7. Register hooks, events, components, systems and resources.
8. Enable mod and report diagnostics.
9. On unload/reload, unregister owned resources and isolate failures.

## Hook model
Plan for before/after/around/replace hooks, explicit priorities, deterministic ordering, recursion protection, error isolation and API-version checks. Do not promise arbitrary patching of every private C++ function. Deep access is provided through deliberate engine extension points.

## Source execution contract
The desired DMod runtime reads script source and evaluates its AST directly. A conventional VM that turns source into bytecode violates the strict requirement, even if it avoids native machine-code compilation. WASM binaries and native plugins are not zero-compilation source mods. A C++-like interpreted language is not actual C++.

## API stability
- Stable: documented contracts intended for broad mod compatibility.
- Advanced: deeper access with explicit compatibility warnings.
- Experimental: unstable internal-facing hooks and APIs.
Never make every private implementation symbol a permanent public contract.

## Build and CI baseline
- CMake is the project build system.
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Linux CI builds and runs tests with GCC and Clang.
- `Threads::Threads` is linked through the core target for portable threading support.
- Android CI cross-compiles with the NDK for `arm64-v8a` (ARMv8-A).
- An Android cross-build is not equivalent to running on a physical device.
- C++20 is currently used by the initial scaffold as a provisional baseline; final standard/compiler support remains an open decision.

## Proposed repository layout
```
/CMakeLists.txt
/cmake/
/engine/core/
/engine/platform/
/engine/world/
/engine/render/
/engine/simulation/
/engine/runtime/
/engine/dmod/
/tools/
/tests/
/third_party/
/examples/
/PMCS/
```

## Build direction
Keep dependency versions pinned and build options reproducible. Do not lock renderer, ECS, physics or scripting dependencies before comparative research and CI proof.
