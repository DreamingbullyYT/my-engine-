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
- [x] Backend-neutral window and surface contracts; Linux GCC/Clang tests and Android ARM64 cross-build passed in run 45710274229.
- [x] Portable keyboard, pointer/touch and gamepad input contract; Linux GCC/Clang tests and Android ARM64 cross-build passed in run 45710274229.
- [x] Virtual filesystem contract and path validation; Linux GCC/Clang tests and Android ARM64 cross-build passed in run 45710274229.
- [ ] Concrete desktop/Android backend implementations.
- [x] Typed generational handle pool and owning ResourceRegistry; verified in run 45715803430.
- [x] Initial typed ResourceStore key-to-handle layer; Linux tests and Android ARM64 cross-compilation verified in run 45716324493.
- [x] Add deterministic ResourceRegistry callback-vs-destroy synchronization test; all four CI jobs passed in run 45758612936.
- [ ] Expand event/job/resource concurrency, lifetime and shutdown stress tests.

## P2 — World and core runtime
- [ ] Scene/world lifecycle.
- [ ] Entity/component model.
- [ ] Transform hierarchy.
- [ ] Serialization and save/load contracts.
- [x] Initial typed ResourceStore key-to-handle layer with duplicate-key rejection and scoped access.
- [ ] Asset loading pipeline: file I/O, decoding, async scheduling, dependency tracking, caching policy and renderer upload (built above ResourceStore).

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
