# PMCS — Persistent Memory and Context System

Project: Dream Engine
Repository: DreamingbullyYT/my-engine-
Snapshot: 2026-11-27
State: specification-only after intentional source-branch reset

This directory is the durable memory for humans and AI agents. It prevents repeated rediscovery, architecture drift, accidental reversals, and false implementation claims.

## Mandatory reading order
1. CURRENT_STATE.md — verified repository state.
2. VISION.md — goals and non-goals.
3. ARCHITECTURE.md — system boundaries and data flow.
4. DECISIONS.md — accepted decisions, including D-010 (GDScript mods), D-011 (gradual mayor autonomy), D-012 (modular orbital settlements/mobile colony ships), D-013 (mobile-colony fleet autonomy and permission-gated expansion), and D-014 (reference-guided colony and flight design).
5. MODDING_API.md — .dmod package and GDScript API requirements.
6. GAMEPLAY.md — citizens, colony economy, buildings, research, policies, and needs.
7. ROCKETS_AND_PHYSICS.md — part-based vehicles, propulsion, orbital and atmospheric flight.
8. UNIVERSE.md — stars, worlds, discovery, asteroids, and interstellar travel.
9. AUTOMATION.md — mayors, NPC pilots, fleets, cargo, and remote simulation.
10. SPACE_INFRASTRUCTURE.md — satellites, stations, megaships, and habitats.
11. SPACE_EXPANSION.md — mobile-colony fleet expansion and habitability rules.
12. PERFORMANCE_AND_SIMULATION.md — simulation tiers, saves, Android lifecycle, and budgets.
13. ROADMAP.md — implementation sequence and exit gates.
14. BUILD_AND_TEST.md — CI and validation policy.
15. AI_RULES.md — mandatory agent workflow.
16. CODE_MAP.md — generated/evidence-backed source map, currently no engine source.
17. CODE_MEMORY.md — CodeGraphContext indexing strategy and verified CI smoke-test results.

## Reference-guided design baseline
D-014 adapts colony-management principles from The Final Earth 2 and part-based orbital-flight principles from Spaceflight Simulator. It does not authorize copying code, art, names, distinctive UI, or exact balance values. See GAMEPLAY.md and ROCKETS_AND_PHYSICS.md.

## Current game-design clarifications
The colony remains central. A mayor begins as an adviser and progressively takes on routine management as player oversight decreases; colonies can still fail or be destroyed. Space settlements include functional satellites, large modular orbital stations, and mobile colony ships that can act as planetless colonies. Mobile-colony mayors can plan fleets and, after player permission at roughly 5–6 explored star systems, autonomously found colonies only on genuinely habitable worlds or through explicitly engineered habitat solutions. See D-011 through D-013.

## Core modding decision
Ordinary .dmod mods use GDScript, JSON, scenes, and assets. Lua is not part of the planned runtime. Mods are trusted code with direct Godot/game access by design; there is no script sandbox. See D-010 and MODDING_API.md.

## Evidence labels
- VERIFIED: confirmed in current source, successful CI, or user-confirmed device test. Cite path, commit, run, log, or artifact.
- PARTIAL: some code exists, but end-to-end behavior is not proven.
- PLANNED: desired design only.
- UNKNOWN: not inspected or evidence missing.
- BLOCKED: known issue prevents progress.

Never promote PLANNED or PARTIAL to VERIFIED based only on documentation, compilation, mocks, or historical chat context.

## Source-of-truth hierarchy
1. Current source code and executable tests for implemented behavior.
2. Current CI/device evidence for build and runtime claims.
3. PMCS for intended design, decisions, and status.
4. Old chats and old commits as historical evidence only.

If source contradicts PMCS, report the conflict, inspect evidence, then update PMCS through an explicit decision.

## Update protocol
For every meaningful change:
1. Inspect current branch, source, status, and tests.
2. Implement the smallest safe patch and add tests.
3. Update CURRENT_STATE.md and affected design docs in the same change.
4. Record architecture changes in DECISIONS.md.
5. Record exact commit SHAs, CI run IDs, artifacts, and device evidence.
6. Mark unresolved items UNKNOWN/BLOCKED rather than guessing.

## File index
- CURRENT_STATE.md — exact current state.
- VISION.md — product goals.
- ARCHITECTURE.md — intended components and data flow.
- DECISIONS.md — accepted decisions.
- MODDING_API.md — mod package and API requirements.
- ROADMAP.md — implementation milestones.
- BUILD_AND_TEST.md — reproducible CI policy.
- AI_RULES.md — instructions for coding agents.
- CODE_MAP.md — generated/evidence-backed source map.
- CODE_MEMORY.md — CodeGraphContext indexing strategy and verified CI smoke-test results.

## Change log
2026-11-27: Initial PMCS created after both source branches were intentionally emptied. Implementation status is not carried forward from historical chat context.
2026-11-27: D-010 accepted GDScript as the primary .dmod scripting language; PMCS architecture, API, roadmap, and status aligned. Documentation only; no runtime implementation.

2026-11-27: D-011 and D-012 record gradual mayor autonomy and modular orbital settlements/mobile colony ships. Documentation only; no gameplay implementation.
2026-11-27: D-013 records mobile-colony fleet autonomy and player-approved planetary expansion after roughly 5–6 explored star systems, restricted to genuinely habitable worlds or explicitly engineered habitat alternatives. Documentation only; no gameplay implementation.
2026-11-27: D-014 establishes reference-guided colony/flight design and creates detailed gameplay, rocket physics, universe, automation, infrastructure, and performance specifications. Documentation only; no gameplay implementation.
