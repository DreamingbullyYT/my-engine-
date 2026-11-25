# Technical Audit

**Status:** Core dependencies for DMod package parsing and the first read-only ZIP package reader are CI-verified. ChaiScript remains a proof-only candidate; no script runtime has been approved for production use.

## Engine references
- Luanti: study runtime modding, API exposure, package lifecycle and server/client boundaries.
- O3DE: study modular engine architecture, components, buses and extensibility.
- Cocos2d-x: study C++ mobile engine structure and scripting integration.
These are references, not code-copy permissions or automatic adoption decisions.

## Rendering candidates
- bgfx: cross-platform rendering abstraction; verify API, backends, Android support, license and integration.
- Compare Vulkan/OpenGL ES and practical Android/desktop backends before selecting.

## Physics candidates
- Jolt Physics: 3D rigid-body candidate; verify Android ARM64, license, features and adapter boundary.
- Box2D: 2D physics candidate; verify current release, API and license.
Do not expose third-party physics types throughout engine/game APIs.

## ECS candidates
- EnTT and Flecs: compare storage, queries, signals, reflection, serialization, scheduler integration, performance and Android build cost.

## DMod package dependencies

### miniz — pinned for ZIP archive access
- Upstream: richgel999/miniz.
- Version: 3.1.2.
- Immutable commit: 77d0dce8627735138c51770d1799a1ef48f2117d.
- Upstream LICENSE file identifies the project as MIT licensed.
- C implementation; ZIP APIs support reading an archive from memory and incremental file extraction. No external runtime dependency is needed for the selected build.
- The project CMake compiles miniz sources as a static library with stdio disabled.
- Dream Engine reads ZIP packages from caller-owned memory and never extracts archive paths to disk.
- Run [45708233794](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708233794) passed GCC, Clang and Android ARM64 cross-build plus Linux package-reader tests.
- Current protections: archive/per-file/total decompressed-size caps, compression-ratio cap, entry-count cap, path normalization, UTF-8 validation, traversal/backslash/drive rejection, case-collision rejection, Unix special-file and Windows reparse-point rejection, encryption and unsupported-method rejection, root manifest validation, full local-header/central-directory validation and CRC/decompression error reporting.
- Adversarial tests now cover exact duplicate paths, Unix symlink metadata, unsupported compression methods, malformed local headers and CRC/decompression failure.
- Remaining work: signature/trust policy, package-format versioning and further fuzzing.

### toml++ — pinned for manifest parsing
- Upstream: marzer/tomlplusplus.
- Version: 3.5.1.
- Immutable commit: efd80e03356af65c7d4e61c56ffb326b34f8d0f4.
- MIT license; header-only C++17 parser with ARM64 support.
- Integrated as an interface target; manifest parser caps input at 64 KiB and validates the expected fields.
- CI run [45706951573](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45706951573) passed GCC, Clang and Android ARM64 cross-build.

## DMod runtime candidates — highest risk

### ChaiScript — promising, not approved
- Audited upstream commit: f635b51e3f8e95868210edfc66fdbd8e475710f5.
- License: BSD-3-Clause; upstream documents C++17 baseline.
- ChaiScript_Basic::do_eval() parses source then evaluates returned AST nodes.
- Default Optimizer_Default can replace AST nodes with Compiled_AST_Node callable wrappers. These are not bytecode, but violate the strictest no-transformation interpretation.
- The pinned proof injects an identity optimizer and verifies no AST_Node_Type::Compiled nodes.
- Proof uses an AST evaluation step budget and explicit no-module/no-external-script/no-IO configuration.
- Run [45704291464](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45704291464) passed Linux proof and Android ARM64 cross-compilation.
- This does not prove physical Android execution, memory quotas, safe native host bindings, unload safety or complete sandboxing. Step limits are not memory limits; exposed C++ host functions can block or mutate engine state. Do not integrate until these risks are addressed.

### Lua
Conventional Lua parses source into VM instructions. It does not meet the strict no-runtime-bytecode requirement unless the user relaxes that requirement.

### AngelScript
Its module workflows involve script compilation/bytecode and likely conflict with strict no-bytecode requirements. Do not select without source-level proof and approval.

### Custom AST interpreter
Fallback only if existing runtimes fail. It requires substantial language, parser, runtime, diagnostics, security and tooling maintenance.

## Required DMod proof
For each interpreter, inspect exact source/version:
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
Before adoption, pin repository, tag/commit and exact license files; inspect transitive dependencies and bundled assets; record modifications; preserve notices; confirm compatibility with distribution plans. The repository currently contains an MIT LICENSE; do not silently change it.
