# Roadmap

All milestones begin incomplete. Mark tasks complete only with evidence.

## P0 — Requirements and technical proof
- [x] Confirm general-purpose engine scope.
- [x] Confirm 2D and 3D as first-class.
- [x] Confirm C++ as primary engine language.
- [x] Record strict DMod no-compilation requirement.
- [x] Configure GitHub Actions for Linux GCC/Clang and Android ARM64.
- [x] Verify Linux GCC and Clang build/CTest.
- [x] Verify Android ARM64 NDK core cross-build.
- [ ] Select final supported C++ standard/compiler matrix.
- [x] Prove direct ChaiScript AST evaluation on Linux with identity optimizer.
- [x] Cross-compile the pinned ChaiScript AST proof for Android ARM64.

## P1 — Foundation
- [x] Initial core library and version API.
- [x] Core logging and structured diagnostics.
- [x] Typed event dispatcher with RAII subscriptions, priority order and exception isolation.
- [x] Worker job system with futures, idle wait and drain-on-shutdown.
- [x] Application lifecycle contract and tests (run 45708669112).
- [ ] Backend-neutral window and surface contracts; verify current CI.
- [ ] Portable keyboard, pointer/touch and gamepad input contract; verify current CI.
- [ ] Virtual filesystem contract and path validation; verify current CI.
- [ ] Concrete desktop/Android backend implementations.
- [ ] Memory/handle/resource identity strategy.
- [ ] Expand event/job concurrency, lifetime and shutdown stress tests.

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
- [x] In-memory manifest model and validation.
- [x] Required/optional dependency resolution and deterministic topological order.
- [x] TOML manifest parser, strict version fields and malformed/oversized input tests.
- [x] Pin miniz ZIP library and toml++ TOML parser to immutable upstream commits.
- [x] Initial read-only in-memory ZIP package reader with root manifest parsing, entry-script validation and bounded reads.
- [x] Validate ZIP local headers against central-directory metadata before indexing package entries.
- [x] Test archive/per-file/total decompressed-size limits, entry-count limits, compression-ratio limits, path traversal, case collisions, missing entry script, malformed local header and invalid ZIP data.
- [x] Test exact duplicate paths, Unix symlink rejection, unsupported compression methods and CRC/decompression error reporting (run 45708233794).
- [ ] Package signature/trust policy and explicit package-format versioning.
- [ ] Choose interpreter only after Android cross-compile and runtime safety audit.
- [ ] C++ host bindings, mod contexts and enforced capabilities.
- [ ] Registries, events and hooks.
- [ ] Mod lifecycle, diagnostics and hot reload.
- [ ] Runtime resource limits and sandbox review.
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
- Custom script language before existing runtimes are tested.
- Premature renderer/ECS/physics lock-in.
