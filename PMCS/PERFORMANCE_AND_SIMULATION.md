# Performance, Simulation, Saves, and Android Lifecycle

Status: planned design. Android ARM64 is the first-class target; GitHub Actions builds/tests and the user validates on a physical device.

## Simulation tiers
Use a consistent authoritative state model with variable detail:
1. Active/local: detailed citizen, building, vehicle, contact, and physics simulation where the player interacts.
2. Nearby/background: reduced-frequency simulation with relevant interactions retained.
3. Distant: deterministic aggregate colony/economy simulation and orbital propagation for remote craft.
4. Strategic/fleet aggregate: grouped simulation for entities too remote or numerous for individual high-frequency updates.

Tier changes must preserve mass, fuel, cargo, population, inventories, time, orbital state, and causal outcomes. No tier may create resources, erase costs, or grant guaranteed success. Promote entities to higher fidelity when approached; demote when safe. Record enough state for reproducible save/load and debugging.

## Time
The game clock pauses when the application is closed by default. Time warp is a player-controlled in-game mechanic and advances all relevant systems consistently. Pause/resume and backgrounding must not silently consume days of colony supplies or destroy settlements. A future opt-in offline progression mode would require explicit design and tests.

## Performance budgets
Define budgets for active citizens, buildings, vehicles, bodies, mods, scripts, physics contacts, draw calls, memory, storage, and simulation time per frame. Profile on target Android ARM64 hardware. Use batching, spatial partitioning, object pooling, event-driven updates, and lower update frequencies where appropriate. Do not choose final numeric limits without profiling evidence.

## Android lifecycle
Handle pause, resume, process death, activity recreation, context loss, GPU resource restoration, audio focus, touch input, storage permissions, and file-picker imports. Persist safely and recover from interrupted saves. Rendering must restore after app switching; a successful build is not proof of device correctness.

## Save system
Use versioned, deterministic save data with schema migrations, atomic writes, backups, corruption detection, and clear recovery behavior. Save world seed, celestial states, inventories, citizen/colony state, craft part graph, orbits, station modules, mod IDs/versions, and relevant mod-owned data. Missing or incompatible mods must not silently corrupt saves.

## CI and device validation
GitHub Actions performs formatting, static checks, unit/integration tests, Android ARM64 build, artifact size/nonzero checks, and upload of logs/reports/APKs. Device tests include launch/render, touch, pause/resume, context restoration, long-running simulation, save/load, .dmod runtime loading, and performance/memory profiling. CI success and device success are separate evidence.


## Adaptive hybrid rendering and visual LOD — D-016

The authoritative world model is independent of its rendering caches. Keep stable logical sector coordinates and building/citizen state; allow only the visual cache regions to change size dynamically. Use distance-aware hybrid building rendering:
- Close range: render buildings individually for selection, inspection, and full detail.
- Medium range: use cached/baked static building visuals while drawing dynamic components separately.
- Far range: combine multiple sectors into larger cache regions where this reduces rendering overhead.
- Off-screen: do not render or animate visual elements that cannot contribute to the frame.

Mark only affected cache regions dirty after construction, demolition, upgrades, damage, or mod changes. Batch rapid edits; retain the previous valid cache until its replacement is ready. Avoid cache thrashing with reuse, memory budgets, eviction, stable logical partitions, and hysteresis at LOD boundaries. Prepare cache data asynchronously only where safe; perform GPU resource operations on the correct Godot rendering thread. Keep selection/hit-testing based on logical building geometry and IDs, not baked texture pixels. Keep animated/dynamic layers (lights, doors, machinery, smoke, particles, damage and similar effects) separate as required. Modded assets may declare static/dynamic parts or opt out of baking. Provide a safe fallback when baking is unsupported or fails.

Building visual detail adapts gradually: full animation nearby, reduced animation/effects at medium range, minimal or no animation at far range, and no off-screen visual animation. Preserve animation continuity when detail returns. Automatic adaptation is the default behavior, with player-selectable quality/performance preferences. The system may consider zoom, visible workload, measured frame time, and reliable device resource/thermal signals when available. Selecting a distant building temporarily raises that building's visual detail only. None of these visual changes may alter gameplay simulation, production, economy, physics, or time.

Citizens are persistent logical individuals, while their visual representation is pixel-scale and distance-aware. Near the camera, show individual moving/animated pixels. As they become smaller, reduce visual movement/animation cadence and detail. Below a readability/visibility threshold, stop drawing individual sprites. Do not delete citizens or erase/merge their identity, population, job, skills, needs, health, or save data. Continue simulation according to the active/nearby/distant simulation tiers above; visual disappearance is not population disappearance.

No numeric sector dimensions, zoom thresholds, update rates, cache sizes, or device budgets are fixed yet. Establish them through profiling on target Android ARM64 hardware. Measure CPU/GPU frame time, draw calls, memory/VRAM, cache rebuild latency, battery, and thermal behavior with representative building counts, citizen populations, zoom levels, and modded content. See D-016 in DECISIONS.md.


## Hybrid excavatable terrain and underground colonies — D-017

Terrain visuals follow a simple, readable side-view style over the planet's radial surface. Vegetation is minimal. Keep visual terrain/cache data separate from authoritative terrain geometry, collision, excavation topology, resource deposits, and building placement.

Excavation is local. A dig/mining/build action marks only the affected terrain region and dependent collision/render data dirty; unaffected regions and their valid caches remain intact. Rebuild local visual and collision data safely, preserving the previous valid visual result until replacements are ready where practical. Do not rebuild an entire planet for a local tunnel or chamber edit.

Underground colonies use a hybrid representation:
- Tunnels and chambers created for player use are persistent logical spaces with stable coordinates and connectivity, and are traversable gameplay areas.
- Render and simulate nearby/relevant underground spaces at the fidelity required for movement, construction, selection, and interaction.
- Simplify or aggregate distant, hidden, or irrelevant underground details; do not require per-rock or full-resolution voxel simulation everywhere.
- Preserve authoritative resources, structures, access routes, and colony state when visual detail is reduced.
- Keep terrain collision and navigation consistent with excavation changes; cache textures must never be the source of truth for movement or hit-testing.

Mining deposits and extracted resources remain gameplay data with real costs, quantities, and logistics. Surface/planetary excavation and asteroid mining are related resource activities but distinct world systems.

Initial excavation rules are intentionally simple (D-018): ordinary tunnels and chambers remain stable without cave-in or structural-strength simulation; there are no required support beams, pillars, reinforcement structures, or automatically generated support decorations; and all terrain follows the same basic excavation rule without complex material-specific hardness simulation. Material/deposit differences still determine what resources may be found and extracted. These exclusions reduce CPU work and implementation complexity.

Measure local rebuild latency, collision/navigation update cost, memory, save size, and frame time on Android ARM64. Do not set numeric terrain chunk sizes, excavation resolution, or underground LOD thresholds before profiling. See D-017 and D-018 in DECISIONS.md.
