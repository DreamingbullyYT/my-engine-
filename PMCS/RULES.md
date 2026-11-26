# Engineering Rules

1. C++ is the primary and default implementation language for Dream Engine.
2. Do not introduce Rust for engine code or subsystems without explicit user approval.
3. Build a general-purpose engine, not a game-specific codebase.
4. 2D and 3D are first-class requirements, not future add-ons.
5. Keep modules cohesive and dependency direction explicit.
6. Prefer proven libraries when they fit; do not rewrite mature functionality without a reason.
7. Do not adopt a complete engine unchanged without an explicit evaluation.
8. Keep platform-specific code behind narrow interfaces.
9. Keep game-specific rules/content outside engine core.
10. Do not choose dependencies before checking exact version, license, maintenance, Android support, build cost and transitive dependencies.
11. Preserve third-party license notices and record provenance for reused code.
12. Do not copy code from a project merely because it is used as inspiration.
13. Installing a .dmod must not compile code on the user's device or require an engine/game/APK rebuild. Native mods may ship prebuilt target-platform/ABI libraries.
14. The no-generated-bytecode rule applies to any future source/script mod lane; it does not prohibit prebuilt native shared libraries.
15. Prove an interpreter's actual source pipeline before selecting it; do not infer behavior from its name or syntax.
16. A script runtime is optional and separate from the full-access native mod runtime.
17. C++-like script syntax does not make scripts actual C++.
18. Native C++ modules are a supported full-access mod format. They execute in-process with app privileges; no sandbox or capability enforcement is provided.
19. Expose live engine objects through versioned raw interfaces and add registries, events, hooks and replace/around extension points.
20. Stable, advanced and experimental API labels describe compatibility, not security boundaries.
21. Provide dependency resolution, deterministic load order, lifecycle, diagnostics and eventually safe hot reload. Native mods must stop their own threads and unregister callbacks/resources before unload.
22. Validate .dmod archive bytes/paths, but never confuse package validation with runtime isolation. Native mods are executable trusted code and may crash or corrupt the process; install only trusted modules.
23. Keep 2D and 3D render/physics implementations replaceable behind engine-owned interfaces.
24. Avoid duplicated systems and unnecessary abstraction layers.
25. Add tests for contracts, serialization, lifecycle, platform adapters and mod behavior.
26. PC tests must not be treated as proof of Android behavior.
27. Do not mark work complete without build/test evidence.
28. Update PMCS whenever requirements, decisions, architecture or verified state changes.
29. Keep accepted decisions separate from proposals and research candidates.
30. At every handoff, state exact next action, blockers and evidence.
