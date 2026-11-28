# Dream Engine

Android-first game runtime and modding platform based on official, unmodified Godot 4.7.x, extended through a separate C++ GDExtension.

> **Implementation status:** after the 2026-11-27 source reset, the main branch contains the official Godot project foundation, a separate Dream Runtime GDExtension, GDScript mod-loading proofs, and an early colony prototype. Under D-024, the next target is a substantial city/colony game; playable space systems are deliberately deferred until the City Completion Gate in PMCS/CITY_PHASE.md is passed. See PMCS/CURRENT_STATE.md for verified status.

## AI / contributor starting point
1. Read PMCS/README.md.
2. Read PMCS/CURRENT_STATE.md.
3. Read PMCS/ARCHITECTURE.md and PMCS/DECISIONS.md.
4. Read PMCS/AI_RULES.md before editing.
5. Follow PMCS/ROADMAP.md.

## Direction
- Official Godot remains unmodified.
- Dream-specific runtime/API lives outside the Godot source tree.
- Android ARM64 is the primary target.
- GitHub Actions performs builds and automated tests; the user validates installable artifacts on a physical phone.
- Ordinary .dmod mods use GDScript, JSON, scenes, and assets. Lua is not a planned runtime dependency.
- Dream Runtime remains a separate C++ GDExtension for native integration/performance-sensitive facilities; official Godot source stays untouched.
- The accepted v1 destination remains one starting planet, its Moon, nearby asteroids, and a basic orbital station. Per D-024, finish the city/colony feature phase before starting playable space systems; interstellar systems and megaships remain later v2+ scope.
- Target NeoForge-like extensibility, not NeoForge binary compatibility.

PMCS is the version-controlled project memory. Update it with every meaningful architecture, status, API, or workflow change.
