# Decisions

## ADR-0001 — General-purpose engine
**Status:** ACCEPTED  
**Decision:** Dream Engine is reusable and not tied to the first game.  
**Consequence:** Game-specific systems remain outside engine core.

## ADR-0002 — First-class 2D and 3D
**Status:** ACCEPTED  
**Decision:** Both 2D and 3D are supported from the beginning.  
**Consequence:** Shared services with specialized render/physics paths where appropriate; no 3D fork later.

## ADR-0003 — Reuse suitable components
**Status:** ACCEPTED  
**Decision:** Research and adapt proven engines/libraries/subsystems rather than rewriting everything or adopting one complete engine unchanged.  
**Consequence:** Every dependency requires technical and license audit.

## ADR-0004 — C++ is the engine language
**Status:** ACCEPTED  
**Decision:** Implement Dream Engine in C++. Rust is not the engine language and is not a default choice for subsystems.  
**Reason:** Explicit user instruction.  
**Consequence:** Core architecture, build tooling and platform integration are C++-first.

## ADR-0005 — DMod is a core requirement
**Status:** ACCEPTED  
**Decision:** Provide runtime-loaded .dmod packages with deep engine/game extension capabilities.  
**Consequence:** Mod API and runtime are foundational architecture concerns.

## ADR-0006 — No compilation on the user's device
**Status:** ACCEPTED WITH SCOPE UPDATE (2026-11-26)  
**Decision:** Installing a .dmod must not compile code on the user's device or require rebuilding the engine, game or APK. Prebuilt native libraries supplied by mod authors are allowed and are the full-access mod path. The old restriction against accepting any precompiled native mod artifact is superseded.  
**Consequence:** Native packages must include a library built for each target platform/ABI. A separate script lane may retain a source/AST/no-runtime-bytecode requirement if it is implemented.

## ADR-0007 — Deep mod access
**Status:** ACCEPTED  
**Decision:** Mods need deep access to engine/game internals through live host objects, registries, events, hooks and replaceable services. For native mods, API tiers describe compatibility only and are not security boundaries.  
**Consequence:** Expand raw native interfaces and build hook/replace/mixin infrastructure; do not pretend a restricted wrapper is equivalent to full access.

## ADR-0008 — PC development, Android target
**Status:** ACCEPTED  
**Decision:** PC is the primary development/test environment; Android is an important deployment target.  
**Consequence:** Validate both desktop and Android ARM64 early.

## ADR-0009 — No dependency lock-in before proof
**Status:** ACCEPTED  
**Decision:** Do not finalize renderer, ECS, physics or scripting dependencies before comparative research and prototypes.  
**Consequence:** Maintain a technical audit and record exact versions/licenses.

## ADR-0010 — CI-first development
**Status:** ACCEPTED  
**Decision:** GitHub Actions is the primary build and test environment because the user has no PC available.  
**Consequence:** Every engine milestone must have automated build/test evidence; avoid workflows that require a local desktop.

## ADR-0011 — Android ABI
**Status:** ACCEPTED  
**Decision:** The initial Android target is `arm64-v8a` (ARMv8-A).  
**Consequence:** Configure and validate this ABI first; do not spend CI time on other ABIs without a requirement.

## Open decisions
- Final C++ standard and compiler support matrix (the initial scaffold uses C++20 provisionally)
- CMake details and dependency manager
- Renderer/backend
- ECS/world model
- 2D and 3D physics libraries
- DMod interpreter/runtime and script syntax
- Asset formats and pipeline
- Audio, networking and editor technology
- Minimum Android API and device requirements


## ADR-0012 — Unrestricted in-process native mods
**Status:** ACCEPTED — 2026-11-26  
**Decision:** Native DMods execute inside the game/engine process with the application's process privileges. There is no sandbox and no capability enforcement for native modules. Native packages may ship prebuilt platform/ABI-specific shared libraries inside the .dmod container.  
**Reason:** Explicit user direction: mods must be able to reach deeply into the engine/game, without sandbox limits, similar to the deep modification model expected from Minecraft Java modding.  
**Consequences:**
- NativeModHost exposes raw live engine object pointers and discoverable interface IDs/versions.
- Newly implemented engine services must be made available to mods where deep access is needed; a hook/replace/mixin API is still required for structured internal behavior changes.
- The native runtime is not safe for untrusted code. A mod may corrupt state, access app-permitted OS resources, deadlock or crash the process. Install only trusted modules.
- Manifest capability declarations do not restrict native modules. Any future signatures/publisher trust policy verifies package origin/integrity, not runtime isolation.
- No native compilation happens on the user's device and installing a mod does not require an APK/engine/game rebuild.


## ADR-0013 — Native hook pipeline
**Status:** ACCEPTED AND IMPLEMENTED (2026-11-26)  
**Decision:** Native DMods can register named Before, Around, Replace and After hooks against engine/game-defined hook points. Hooks use descending explicit priority and deterministic registration order. Around/Replace callbacks may call the remaining chain repeatedly or skip it.  
**Consequences:**
- Hook callbacks receive a raw caller-owned context pointer; hook-point argument/result structs are defined by the engine/game and shared with mod authors.
- Hook registrations are owned by module ID, disabled with the module and removed before its shared library is unloaded.
- Dispatch is wrapped in NativeModManager's callback-depth guard, so self-unload is deferred until the current dispatch chain finishes.
- The generic registry is not a binary detour engine. Engine/game methods become hookable when code explicitly calls dispatch_hooks; integration across subsystems is incremental.
- This remains unrestricted native code in-process. No sandbox or capabilities are introduced.


## ADR-0014 — Runtime operation hook points
**Status:** ACCEPTED AND IMPLEMENTED (2026-11-26)  
**Decision:** EngineRuntime dispatches native hooks around runtime update, individual PhysicsWorld2D fixed steps, beginning a render frame and rendering a frame. Hook contexts expose mutable inputs and operation results.  
**Consequences:**
- Runtime update hooks can modify frame delta and the input-event span before the update.
- Physics2D hooks run once for each fixed simulation step and expose its report.
- Begin-render hooks can change clear color and clear flags before the frame packet is built; render-frame hooks observe or modify the result.
- The Scene scheduler's individual system invocation, World/ECS mutation and asset lifecycle still require their own explicit hook points.
- Hooks remain unrestricted in-process native callbacks; no sandbox or capability system is introduced.


## ADR-0015 — Adopt wgpu-native for the Android render backend
**Status:** ACCEPTED; Android clear/present backend IMPLEMENTED and CI-VERIFIED (2026-11-27). Physical-device rendering remains unverified.

**Decision:** Use the upstream `wgpu-native` C API behind Dream Engine's existing C++20 `IRenderBackend` contract as the active Android rendering backend. Pin the Android AArch64 release to v29.0.1.1 and verify the release archive SHA-256 in GitHub Actions. Keep the engine source and public render contracts in C++; do not port the Rust renderer from Rusty-survival. Keep bgfx optional for non-Android/legacy work, but disable it in the current Android host.

**Rationale:** wgpu-native gives the C++ host access to the WebGPU API while preserving the backend abstraction and Android GameActivity surface lifecycle. Rusty-survival is useful as a reference for renderer architecture and WGSL use, but its wgpu 30.0.1 renderer is too large and is not source to copy. The available Android package is v29.0.1.1 (wgpu-core 29.0.1), so the version difference must remain explicit.

**Consequences and limits:**
- Initial scope is native surface/device creation and clear/present only. 2D/3D/instancing stay disabled until real pipelines/resources exist.
- Vulkan and GL backends are enabled; Android Vulkan is the primary target and GL/GLES remains best-effort.
- The dependency carries MIT/Apache-2.0 notices; the pinned WebGPU native headers submodule carries BSD-3-Clause. Preserve notices in `third_party/`; do not change the repository root MIT license.
- Android CI proves cross-compilation and APK packaging, not physical GPU correctness. The user must verify visible output on a phone.

## ADR-0016 — Adopt Godot 4.7.2-stable as the engine foundation
**Status:** ACCEPTED — migration in progress (2026-11-27)

**Decision:** Use upstream Godot 4.7.2-stable source as the new foundation for the general-purpose Dream Engine. Pin tag `4.7.2-stable` to commit `ed1daf0bf001b61586d9930840f2f1394092c079` as ordinary tracked source files at `third_party/godot/`, not as a Git submodule.

**Reason:** Explicit user approval to use the Godot source rather than continue reimplementing mature engine foundations independently.

**Consequences:**
- Godot becomes the active runtime foundation; its scene/resource/input/physics/render/platform systems are preferred over parallel duplicates.
- C++ remains the default language for engine internals and native extensions; GDScript may be used for game/project scripts.
- Preserve the former `engine/` implementation and Android host as reference until replacements are built and tested.
- Build the Android source template for arm64-v8a through GitHub Actions. CI is not physical-device proof.
- Preserve Godot's MIT license and all third-party notices.
- The old wgpu-native Android backend is not the active renderer for the Godot runtime; keep it as legacy/reference code.

The previous ADR-0003 restriction against adopting a complete engine unchanged is superseded by this explicit user decision. Earlier C++-first and Android/CI requirements remain compatible.

## ADR-0017 — Preserve unrestricted native mod support during the Godot migration
**Status:** ACCEPTED REQUIREMENT; implementation pending

**Decision:** Retain the prior DMod requirement for trusted native in-process code with no sandbox/capability boundary. Evaluate GDExtension for supported public interfaces and a maintained Godot module/patch layer for deep internal hook/replace/mixin points.

**Consequences:**
- Do not claim GDExtension by itself grants access to every private Godot internal.
- Do not add a sandbox to the native mod path.
- Keep native mod authoring/building off the user's phone; distribute prebuilt ABI-specific libraries.
- Native mods can crash, corrupt engine/game state or use app-permitted OS resources. Only trusted mods should be loaded.
- Define ABI/version policy and real hook insertion points before marking the DMod migration implemented.

## ADR-0018 — Lua-first DMod packages with shared hook semantics
**Status:** ACCEPTED REQUIREMENT; implementation pending — 2026-11-27

**Decision:** Keep one `.dmod` package format with three runtime modes: `lua`, `native`, and `hybrid`. Lua-only mods are the normal/default authoring path and contain source scripts directly; they do not require a compiler, bytecode generation, a `.so`, on-device compilation or an APK rebuild. Native/hybrid packages may contain prebuilt platform/ABI-specific libraries for unrestricted deep C++ access.

**Decision:** Lua and native callbacks use the same owner-aware hook-point namespace and the same `Before`, `Around`, `Replace`, and `After` stages, with deterministic priority/registration ordering. The shared hook service must adapt Lua callbacks and native callbacks rather than creating incompatible parallel hook systems.

**Runtime direction:** Embed Lua 5.4.8 source into the built-in Dream Engine Godot C++ module, pinned to commit `6e22fedb74cf0c9b6656e9fce8b7331db847c605` (tag `v5.4.8`). Load Lua source from the validated package virtual filesystem. Provide typed, documented engine bindings. Lua callbacks run only at hook points whose thread/phase contract safely supports the Lua VM; arbitrary-thread synchronous hooks remain native-only unless a safe design is proven.

**Consequences:**
- A normal Lua-only `.dmod` is a single portable package and contains no shared library.
- Native code remains trusted in-process code with no sandbox/capability boundary; it is optional for advanced engine internals.
- The Lua runtime is not claimed to be a security boundary or a replacement for unrestricted native C++ access.
- Mod lifecycle owns script states, callback references and hook registrations. Disable/unload must remove callbacks and release Lua references before destroying the VM or unloading native libraries.
- Script exceptions must be contained and reported; they must never unwind through Godot/C++.
- Real engine call sites are required. A hook registry without actual main-loop/scene/physics/render/resource dispatch points is not considered complete.
- Exact Lua version pin, manifest parser extension, shared dispatch implementation and Godot hook insertion points require implementation and CI evidence.
