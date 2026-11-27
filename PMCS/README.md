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
4. DECISIONS.md — accepted decisions.
5. MODDING_API.md — mod package and API requirements.
6. ROADMAP.md — implementation sequence and exit gates.
7. BUILD_AND_TEST.md — CI and validation policy.
8. AI_RULES.md — mandatory agent workflow.
9. CODE_MAP.md — actual code structure, currently empty/planned.

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
3. PMCS for intended design and accepted decisions.
4. Old chats and old commits as historical evidence only.

If source contradicts PMCS, report the conflict, inspect evidence, then update PMCS through an explicit decision. Do not silently choose.

## Update protocol
For every meaningful change:
1. Inspect branch, status, source, call sites, and tests.
2. Implement the smallest safe change and add tests.
3. Update CURRENT_STATE.md and affected design docs in the same change.
4. Record architecture changes in DECISIONS.md.
5. Record exact commit SHAs, CI run IDs, artifacts, and device evidence.
6. Mark unresolved items UNKNOWN/BLOCKED rather than guessing.

## File index
- CURRENT_STATE.md — exact current state.
- VISION.md — product goals.
- ARCHITECTURE.md — intended components and boundaries.
- DECISIONS.md — accepted decisions.
- MODDING_API.md — modding requirements.
- ROADMAP.md — milestones.
- BUILD_AND_TEST.md — reproducible validation.
- AI_RULES.md — instructions for coding agents.
- CODE_MAP.md — generated/evidence-backed source map.
- CODE_MEMORY.md — evaluation and adoption plan for persistent codebase indexing/memory.

## Change log
2026-11-27: Initial PMCS created after both source branches were intentionally emptied. Implementation status is not carried forward from historical experiments.
