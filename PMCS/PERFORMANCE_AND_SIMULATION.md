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
