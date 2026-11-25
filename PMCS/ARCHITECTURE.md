# Architecture

**Status:** Planning baseline; no implementation verified.

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
The desired DMod runtime reads script source and interprets its AST directly. A conventional VM that turns source into bytecode violates the strict requirement, even if it avoids native machine-code compilation. WASM binaries and native plugins are not zero-compilation source mods. A C++-like interpreted language is not actual C++.

## API stability
- Stable: documented contracts intended for broad mod compatibility.
- Advanced: deeper access with explicit compatibility warnings.
- Experimental: unstable internal-facing hooks and APIs.
Never make every private implementation symbol a permanent public contract.

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
Use a C++-first build. CMake is the current recommended direction, subject to a minimal desktop and Android NDK proof. Keep dependency versions pinned and build options reproducible.
