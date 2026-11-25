# Project Memory — Dream Engine

## Identity
Dream Engine is a reusable, general-purpose game engine. It is not a custom engine for only the first planned game. The first planet/colony game is a validation project, not the engine's purpose.

## Accepted requirements
- **Primary implementation language: C++.** This is an explicit user decision. Do not use Rust as the engine language.
- First-class 2D and 3D support from the beginning. A later unrelated 3D game must not require an engine fork.
- Modular architecture with reusable systems and clear subsystem boundaries.
- Research and reuse/adapt suitable existing libraries and engine components. Do not rewrite everything needlessly, and do not adopt a complete engine unchanged without evaluating fit.
- GitHub Actions is the primary build/test environment because the user has no PC available.
- Android target ABI is `arm64-v8a` (ARMv8-A). Other ABIs are not currently required.
- Deeply extensible runtime modding through DMod packages.
- Ordinary mods must not require rebuilding the engine, game, or Android APK, and must not require native compilation on the user's device.
- User's strict interpretation of zero compilation also rejects runtime bytecode generation. Prefer direct source/AST interpretation; prove the actual runtime pipeline rather than relying on marketing claims.
- Mod APIs should reach meaningful engine/game internals through deliberate bindings, registries, events, hooks and replaceable interfaces.

## Scope boundaries
The engine provides generic rendering, resources, scenes/worlds, entities/components, input, audio, physics adapters, animation, jobs, serialization, diagnostics and runtime services as justified by research. Game-specific economy, planets, colonies, citizens, buildings and progression belong to game projects or their modules, not the engine core.

## DMod constraints
- Package format concept: .dmod; exact archive and manifest schema remain open.
- Runtime reads mod source and evaluates it dynamically.
- Desired strict model: parse source to an AST and interpret/evaluate it directly, with no native compilation and no generated VM bytecode.
- C++ engine APIs must be bindable to scripts.
- Support dependencies, deterministic load order, lifecycle, diagnostics, permissions/capabilities, events, hooks (before/after/around/replace), registries, mod state and hot reload.
- Native C++ plugins are optional developer extensions only; they are not ordinary DMods and cannot be required for user mods.
- A C++-like script is not actual C++ machine code. Never blur that distinction.

## Language policy
C++ is the engine language. Rust is not an alternative default and must not be introduced for engine subsystems without explicit user approval and a demonstrated technical reason. Third-party libraries may be written in other languages internally only where their integration model is acceptable and does not change the engine's language decision.

## Current status
The first CMake/core scaffold and CI workflow are now present. CI verification is pending. The repository's existing MIT license is not silently changed.

## Highest-risk question
Find or prove a C++-embeddable runtime that directly evaluates source/AST without generating bytecode, works on desktop and Android ARM64, exposes native C++ objects safely, and supports reload/diagnostics. This requirement affects architecture and must be tested before broad implementation.
