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
13. Ordinary DMods must not require native compilation, engine rebuild, game rebuild or APK rebuild.
14. Strict zero compilation means no runtime bytecode generation either, unless the user explicitly relaxes this.
15. Test the interpreter's actual source pipeline; do not infer it from its name or syntax.
16. Prefer direct AST/source interpretation for the strict DMod requirement.
17. C++-like script syntax does not make scripts actual C++.
18. Native C++ plugins are optional trusted extensions, never the default mod format.
19. Expose engine internals through versioned bindings, registries, events, hooks and extension interfaces.
20. Separate stable, advanced and experimental mod APIs.
21. Include dependency resolution, deterministic load order, lifecycle, diagnostics and hot reload in DMod design.
22. Treat mod packages and assets as untrusted input; design permissions and resource limits.
23. Keep 2D and 3D render/physics implementations replaceable behind engine-owned interfaces.
24. Avoid duplicated systems and unnecessary abstraction layers.
25. Add tests for contracts, serialization, lifecycle, platform adapters and mod behavior.
26. PC tests must not be treated as proof of Android behavior.
27. Do not mark work complete without build/test evidence.
28. Update PMCS whenever requirements, decisions, architecture or verified state changes.
29. Keep accepted decisions separate from proposals and research candidates.
30. At every handoff, state exact next action, blockers and evidence.
