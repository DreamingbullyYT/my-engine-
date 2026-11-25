# Technical Audit

**Status:** Research notes and candidates only. No dependency is approved or integrated.

## Engine references
- **Luanti:** study runtime modding, API exposure, package lifecycle and server/client boundaries.
- **O3DE:** study modular engine architecture, components, buses and extensibility.
- **Cocos2d-x:** study C++ mobile engine structure and scripting integration.
These are references for comparison, not code-copy permissions or automatic adoption decisions.

## Rendering candidates
- **bgfx:** cross-platform rendering abstraction; verify current API, backends, Android support, license and integration model.
- Compare Vulkan/OpenGL ES and other practical Android/desktop backends before selecting.

## Physics candidates
- **Jolt Physics:** 3D rigid-body physics candidate; verify Android ARM64, license, required features and adapter boundary.
- **Box2D:** 2D physics candidate; verify current release, API and license.
Do not expose third-party physics types throughout engine/game APIs.

## ECS candidates
- **EnTT** and **Flecs:** compare storage, queries, signals, reflection, serialization, scheduler integration, performance and Android build cost. ECS choice does not automatically solve world lifecycle or job scheduling.

## DMod runtime candidates — highest risk
### ChaiScript — promising, not yet approved
- Audited upstream `develop` commit: `f635b51e3f8e95868210edfc66fdbd8e475710f5`.
- License: BSD-3-Clause; upstream documents C++17 as its compiler baseline.
- Source path: `ChaiScript_Basic::do_eval()` calls `m_parser->parse(...)` and then evaluates the returned AST with `p->eval(...)`.
- Important caveat: upstream's default `Optimizer_Default` can replace AST nodes with `Compiled_AST_Node` objects holding `std::function` callables. These are not bytecode instructions, but they are optimized callable AST nodes and may not meet the strictest reading of zero compilation.
- The parser accepts an optimizer template parameter. A custom identity optimizer whose `optimize(node)` returns the node unchanged bypasses optimizer passes and leaves ordinary AST node implementations to evaluate themselves. An initial CI attempt showed that `Optimizer<>` itself is not instantiable in this upstream revision because its zero-argument constructors collide; the proof now uses the explicit identity optimizer. `ChaiScript_Basic` accepts an injected parser, so this configuration can be used without adopting the default optimizer.
- Added `tests/dmod/chaiscript_ast_proof.cpp` and a CI job pinned to the audited commit. The test checks that parsing and evaluating `var answer = 6 * 7; answer` yields 42 and that no `AST_Node_Type::Compiled` nodes exist when the empty optimizer is used.
- This proof is pending its first CI result. It does not yet prove Android NDK compatibility, memory limits, sandboxing, safe unload/hot reload or runtime performance. Do not integrate or approve ChaiScript until those checks and the user's zero-compilation interpretation are satisfied.

### Lua
Mature embedded scripting and C API. Conventional Lua parses source into VM instructions; it may avoid native compilation but does not meet the strict no-runtime-bytecode requirement. Consider only if user explicitly relaxes that requirement.

### AngelScript
C++-like language and host bindings, but documented module workflows involve script compilation/bytecode. Likely conflicts with strict no-bytecode requirement; do not select without source-level proof and user approval.

### Custom AST interpreter
Fallback only if existing runtimes fail. It could parse source into an AST and evaluate nodes directly, but creates substantial language, parser, type/runtime, diagnostics, security and tooling maintenance.

## Required DMod proof
For each candidate, record exact version/commit and inspect source:
- Does source parse to AST?
- Is AST interpreted directly?
- Is bytecode or another executable intermediate generated?
- Is native code generated?
- Can it bind C++ functions, objects and lifetimes safely?
- Can scripts reload and unload without stale references?
- Does it build and run on desktop and Android ARM64?
- What are binary size, memory, startup and execution costs?
- What security/resource limits exist?

## Additional subsystem research queue
- Audio: miniaudio, SDL audio, OpenAL Soft.
- Platform/window/input: SDL3 and native platform adapters.
- Internal tools UI: Dear ImGui; evaluate a separate touch-friendly runtime UI.
- Asset import: Assimp, cgltf, stb, KTX/Basis tooling.
- Navigation: Recast/Detour.
- Logging: spdlog.
- Profiling: Tracy.
- Serialization: nlohmann/json or another JSON parser plus a versioned save format.
- Networking: ENet or standalone Asio.

## License/provenance gate
Before adoption, pin repository, tag/commit and exact license files; inspect transitive dependencies and bundled assets; record local modifications; preserve required notices; confirm compatibility with the project's distribution plans. The repository currently contains an MIT LICENSE; do not silently change it.
