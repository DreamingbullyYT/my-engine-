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
- Keep the engine reusable; game-specific systems belong in game projects.
- 2D and 3D are both first-class.
- The user has no PC available: use GitHub Actions for builds and tests.
- Android ABI target: arm64-v8a (ARMv8-A).
- Ordinary DMods must not require native compilation, engine/game/APK rebuild, or generated runtime bytecode.
- Prove interpreter behavior from source and tests; do not make unsupported claims.
- Preserve the root MIT license; record third-party notices separately.
- Update PMCS whenever implementation state or verified decisions change.

## Current implementation
- dream_core: version API, logging, diagnostics, event dispatcher and worker job system.
- dream_platform: application lifecycle, window/surface/input contracts and virtual filesystem path validation.
- dream_dmod: manifest model, validation, dependency resolver, TOML parser and read-only in-memory ZIP package reader with local-header validation.
- Pinned dependencies: miniz 3.1.2 at 77d0dce8627735138c51770d1799a1ef48f2117d; toml++ 3.5.1 at efd80e03356af65c7d4e61c56ffb326b34f8d0f4.
- Application lifecycle passed Linux GCC/Clang and Android ARM64 cross-build in [run 45708669112](https://github.com/DreamingbullyYT/my-engine-/actions/runs/45708669112).
- Latest window/surface/input/filesystem contracts are awaiting CI; don't mark them verified until success.
- Android results are cross-compilation, not physical-device execution.
- C++20 remains a provisional implementation baseline.

## Next work
1. Verify latest platform contracts in CI and fix actual failures.
2. Add concrete platform backends only after interface review; don't choose SDL or another dependency prematurely.
3. Continue resource identity/lifetime foundations and renderer API research.
4. Define DMod package signature/trust and format-version policy; continue interpreter security audit.

## Handoff requirements
Update CURRENT_STATE.md, ARCHITECTURE.md, TECHNICAL_AUDIT.md and ROADMAP.md as work progresses. Update DECISIONS.md only for explicitly accepted decisions. Never label work VERIFIED without reproducible evidence.
