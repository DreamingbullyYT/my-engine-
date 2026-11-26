# Dream Engine

Dream Engine is a general-purpose **C++ game engine** for **2D and 3D** games, developed on PC with Android as a target platform.

## Start here (users and AI assistants)

1. Read this README first.
2. Read [PMCS/README.md](PMCS/README.md) for the project documentation map.
3. Read [PMCS/AI_HANDOFF.md](PMCS/AI_HANDOFF.md) before making changes.
4. Check [PMCS/CURRENT_STATE.md](PMCS/CURRENT_STATE.md) to see what actually exists and what is verified.

## Current foundations

The repository currently contains tested core/platform services, an initial ECS and scene scheduler, 2D/3D transform math, versioned binary and World/Scene save-load, an asset pipeline, DMod package parsing, and gameplay input action mapping. The renderer, physics, concrete native platform backends, production DMod runtime and physical-device validation are still in development. See [PMCS/CURRENT_STATE.md](PMCS/CURRENT_STATE.md) for verified scope and CI evidence.

## Important project rules

- C++ is the engine language. Do not switch the engine to Rust.
- Keep the engine reusable; game-specific features belong in game projects.
- Treat both 2D and 3D as first-class.
- Ordinary DMods must load dynamically without native compilation, engine/game/APK rebuilds, or generated runtime bytecode.
- Research and test dependencies before adopting them. Do not claim untested work is complete.
- Update the PMCS whenever project decisions, implementation state, or verified results change.

**When in doubt:** follow the accepted decisions in PMCS/DECISIONS.md and the rules in PMCS/RULES.md.