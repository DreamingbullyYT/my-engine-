# AI Handoff

## Project
Dream Engine — general-purpose C++ engine, first-class 2D/3D, desktop development and Android target, dynamic DMod runtime.

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
- Do not turn the engine into a game-specific framework.
- 2D and 3D are both first-class.
- Reuse appropriate libraries after source/license/platform audit.
- Ordinary DMods must not require native compilation, engine/game/APK rebuild, or generated runtime bytecode.
- Prove interpreter behavior from source and tests; do not make unsupported claims.

## Current repository state
Only LICENSE exists outside PMCS. No code/build/test/dependency is verified.

## Next work
1. Establish C++ compiler and CMake baseline on desktop.
2. Add Android NDK/ARM64 cross-build baseline.
3. Prototype candidate DMod interpreters and inspect their parser/evaluator internals.
4. Build a tiny proof that loads source from a .dmod-like package, binds a C++ host function, invokes a callback, reports errors and reloads safely.
5. Decide dependencies only after measured evidence.

## Handoff requirements
Update CURRENT_STATE.md with actual files, builds and tests. Update DECISIONS.md only when a choice is explicitly made. Update TECHNICAL_AUDIT.md with exact versions, source evidence and licensing. Never label an untested feature VERIFIED.
