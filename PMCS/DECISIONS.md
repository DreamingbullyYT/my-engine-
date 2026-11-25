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

## ADR-0006 — Strict zero compilation for ordinary mods
**Status:** ACCEPTED  
**Decision:** Ordinary mods must not require native compilation or generated runtime bytecode.  
**Consequence:** Prefer direct source/AST interpretation; prove the execution pipeline before choosing a runtime.

## ADR-0007 — Deep mod access
**Status:** ACCEPTED  
**Decision:** Mods need meaningful access to engine/game internals through host bindings, registries, events, hooks and replaceable services.  
**Consequence:** Design stable, advanced and experimental API tiers.

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
