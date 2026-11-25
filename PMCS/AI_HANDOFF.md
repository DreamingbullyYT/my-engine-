# AI Handoff

## Project
Dream Engine — general-purpose C++ engine, first-class 2D/3D, Android ARM64 target, dynamic DMod runtime.

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
- The user has no PC available: use GitHub Actions for builds and tests.
- Android ABI target: `arm64-v8a` (ARMv8-A); do not add other ABIs unless requested.
- Reuse appropriate libraries after source/license/platform audit.
- Ordinary DMods must not require native compilation, engine/game/APK rebuild, or generated runtime bytecode.
- Prove interpreter behavior from source and tests; do not make unsupported claims.

## Current implementation
- Root CMake project and a small `dream_core` static library exist.
- A core version API and dependency-free smoke test exist.
- CI workflow has Linux GCC/Clang test jobs and Android NDK `arm64-v8a` cross-build.
- The initial workflow has not yet been confirmed green. Treat all build/test status as pending until Actions evidence is inspected.
- C++20 is currently used provisionally; the final standard/compiler matrix remains open.

## Next work
1. Inspect the first GitHub Actions run; fix failures before expanding the engine.
2. Add focused core tests and services in small, independently testable modules.
3. Prototype candidate DMod interpreters and inspect parser/evaluator internals.
4. Build a .dmod proof that loads source, binds a C++ host function, invokes a callback, reports errors and reloads safely.
5. Decide dependencies only after measured evidence.

## Handoff requirements
Update CURRENT_STATE.md with actual files, builds and tests. Update DECISIONS.md only when a choice is explicitly made. Update TECHNICAL_AUDIT.md with exact versions, source evidence and licensing. Never label work VERIFIED without reproducible evidence.
