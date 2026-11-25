# Roadmap

All milestones begin incomplete. Mark tasks complete only with evidence.

## P0 — Requirements and technical proof
- [x] Confirm general-purpose engine scope.
- [x] Confirm 2D and 3D as first-class.
- [x] Confirm C++ as primary engine language.
- [x] Record strict DMod no-compilation requirement.
- [x] Configure CMake and GitHub Actions for Linux GCC/Clang and Android ARM64.
- [x] Verify Linux GCC build and CTest in Actions.
- [x] Verify Linux Clang build and CTest in Actions.
- [x] Verify Android ARM64 NDK cross-build in Actions.
- [ ] Select final supported C++ standard/compiler matrix.
- [ ] Prove direct source/AST interpretation for DMod (pinned ChaiScript proof is in CI; pending first result).

## P1 — Foundation
- [x] Initial core library and version API.
- [x] Core logging API with severity filter and configurable sink (verified in run 45700477183).
- [x] Structured diagnostic value type (verified in run 45700477183).
- [x] Typed event dispatcher with RAII subscriptions, priority order and exception isolation (verified in run 45702103975).
- [x] Worker job system with futures, idle wait and drain-on-shutdown (verified in run 45702103975).
- [ ] Platform abstraction and application lifecycle.
- [ ] Memory/handle/resource identity strategy.
- [ ] Expand event/job concurrency, lifetime and shutdown stress tests.
- [ ] Expand automated contract tests for desktop and Android build.

## P2 — World and core runtime
- [ ] Scene/world lifecycle.
- [ ] Entity/component model.
- [ ] Transform hierarchy.
- [ ] Serialization and save/load contracts.
- [ ] Resource and asset registry.

## P3 — Rendering
- [ ] Compare renderer candidates.
- [ ] Define engine-owned rendering API.
- [ ] 2D rendering and camera.
- [ ] 3D rendering, camera and materials.
- [ ] Shared asset/resource pipeline.
- [ ] Android surface/device validation.

## P4 — Simulation
- [ ] Compare 2D physics candidates.
- [ ] Compare 3D physics candidates.
- [ ] Implement adapters and deterministic test fixtures.
- [ ] Animation and navigation integration.

## P5 — DMod
- [ ] Choose interpreter only after source-level and Android proof (ChaiScript direct AST test pending).
- [ ] .dmod manifest and package validation.
- [ ] Dependency resolution and deterministic load order.
- [ ] C++ host bindings and mod contexts.
- [ ] Registries, events and hooks.
- [ ] Mod lifecycle, diagnostics and hot reload.
- [ ] Permission/capability model and resource limits.
- [ ] Desktop and Android ARM64 runtime tests.

## P6 — Runtime services and content
- [ ] Input actions.
- [ ] Audio.
- [ ] UI integration.
- [ ] Asset import pipeline.
- [ ] Networking/localization as justified.

## P7 — Tools
- [ ] Runtime diagnostics and profiler integration.
- [ ] Editor/tooling architecture.
- [ ] Touch-friendly Android workflows where applicable.

## P8 — Validation games
- [ ] Validate with the planned planet/colony game.
- [ ] Validate with a substantially different 3D game.
- [ ] Confirm neither project requires an engine fork.

## Out of scope until justified
- Native C++ plugins as the ordinary user mod format.
- Custom script language before existing interpreters are tested.
- Premature renderer/ECS/physics lock-in.
