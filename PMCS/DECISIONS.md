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
