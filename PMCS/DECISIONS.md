# Accepted Decisions

Change a decision only through a new entry explaining the reason, migration impact, and compatibility consequences.

## D-001 — Official unmodified Godot
Status: ACCEPTED. Use official Godot 4.7.x; do not maintain a custom source fork by default. Keep Dream code in a separate GDExtension. Reconsider source patches only if a required capability cannot be achieved through public extension points and alternatives are documented.

## D-002 — Separate Dream Runtime
Status: ACCEPTED. Dream runtime/API code lives outside Godot's source tree to keep upstream clean and upgrades manageable.

## D-003 — Android ARM64 first
Status: ACCEPTED. Primary ABI is arm64-v8a. CI must produce Android ARM64 artifacts before Android support is claimed.

## D-004 — GitHub Actions as primary build environment
Status: ACCEPTED. The user has no PC development environment. CI must be reproducible, produce useful logs/artifacts, and fail loudly. Physical-device validation is performed by the user.

## D-005 — Script-first .dmod (SUPERSEDED by D-010)
Historical decision: ordinary mods were planned to use Lua, JSON, scenes, and assets. Superseded by D-010 before runtime implementation. No Lua runtime is required by the accepted design.

## D-006 — NeoForge-like extensibility, not compatibility
Status: ACCEPTED. Aim for registries, events, hooks, lifecycle, dependencies, networking, and data-driven content. Do not claim compatibility with NeoForge/Minecraft Java mods.

## D-007 — Native mods are trusted code
Status: ACCEPTED. Native code can crash, corrupt memory, access files, or compromise the process. Do not promise safety or sandboxing without a tested threat model.

## D-008 — Small modular changes
Status: ACCEPTED. Prefer separate modules and focused patches. Avoid invasive upstream changes.

## D-009 — PMCS is mandatory
Status: ACCEPTED. Update project memory in the same change as meaningful architecture, API, status, or workflow changes.

## D-010 — GDScript is the primary .dmod language
Status: ACCEPTED — 2026-11-27. Ordinary .dmod mods use Godot GDScript as their primary language, alongside JSON, scenes, and assets. This resolves the conflict with the earlier Lua-first PMCS proposal and aligns PMCS with the game's MOD_API.md and PLAN.md.

Rationale:
- GDScript is native to Godot and integrates with its objects, scenes, signals, resources, and APIs.
- It avoids maintaining and binding a second scripting runtime for ordinary mods.
- It supports deep game-specific integration while retaining a documented stable Mod API.
- C++ remains available through Dream Runtime/GDExtension for native integration and performance-critical code.

Consequences:
- Remove Lua as a planned runtime dependency; do not build a Lua VM for standard mods.
- Use .dmod packages containing mod.json, GDScript, JSON, scenes, and assets.
- Mods are trusted code, not sandboxed. The manager warns and requires explicit activation.
- Direct access to game internals is permitted by design but is not a stable compatibility guarantee.
- Historical Lua experiments and CodeGraphContext Lua smoke fixtures remain historical only.
- Runtime script loading/execution must be verified on exported Android ARM64 before claiming support.


## D-011 — Gradual mayor autonomy based on player oversight
Status: ACCEPTED — 2026-11-27.
The mayor begins mainly as an adviser, recommending improvements and warning about problems. As the player spends more in-game time away from a colony or gives it less direct attention, the mayor progressively assumes more routine management. This is a gradual response to reduced oversight, not a mandatory mayor appointment or a simple permanent autonomy toggle.

The mayor must obey actual resource, workforce, infrastructure, production, and logistics constraints. The player can return and intervene. Reports should explain what the mayor recommended, what they did, and why. A colony can still fail or be destroyed; future enemies and other severe threats must remain capable of causing loss. The mayor is not an invulnerability system.

## D-012 — Modular orbital settlements and mobile colony ships
Status: ACCEPTED — 2026-11-27.
The space-settlement design includes small functional satellites/probes, large modular orbital stations, and very large mobile colony ships/megaships. Stations support docking and player-installed modules. A mobile colony ship can function as a planetless settlement if equipped with sufficient life support, power, food production, storage, population facilities, and other required modules. Asteroid bases, orbital habitats, industrial platforms, and shipyards may extend this family.

Rationale: space infrastructure should support more than observation satellites and should allow civilization to exist and expand without requiring every settlement to be founded on a planet.

Consequences:
- Keep satellites, stations, and mobile colony ships as distinct entity classes or capability profiles, not one decorative satellite type.
- Make stations and colony ships modular and extensible through the game host and mod API.
- Their survival and operation depend on real installed systems, resources, and logistics; no free power, supplies, or guaranteed safety.
- These are design requirements only; no implementation is implied.


## D-013 — Autonomous mobile-colony fleets and permission-gated planetary expansion
Status: ACCEPTED — 2026-11-27.

A mobile colony ship/megaship has its own mayor and can develop as a mobile settlement. Its administration may design, select, and construct any ship types for which it has the required designs, technology, shipyard capability, workforce, materials, components, energy, fuel, and time. Its fleet choices are driven by its needs and plans; this is not free or instantaneous ship creation.

After the civilization has explored roughly 5–6 star systems, a mobile-colony mayor may ask the player for permission to begin autonomous expansion to additional planets. The threshold is a progression/balance target, not a reason to bypass discovery, travel, or logistics. The player must explicitly approve this expansion policy; the mayor does not silently colonize new worlds before permission.

Automatic planetary colonization is restricted to worlds assessed as genuinely habitable for the colony's current biology/technology and available infrastructure. The assessment must account for surface or atmospheric conditions, temperature, pressure, atmosphere, gravity, radiation, water, resources, and other relevant hazards. The Sun is never a colonization target; Mercury is not an ordinary habitable colony target; Venus's surface and gas-giant interiors are not ordinary habitable targets. Specialized floating habitats in suitable atmospheric layers (for example, Venus's upper clouds), orbital habitats, and settlements on suitable moons are separate engineered settlement cases, not proof that the planet itself is ordinarily habitable.

The mayor must verify that an expedition can be supplied and sustained: population, transport capacity, ships, life support, food, water, power, construction materials, communications/logistics, and a viable landing or habitat plan. Colonies founded this way receive their own local governance and remain subject to the same economic, physical, and survival constraints as player-founded colonies. The player can inspect reports and history, change or revoke permissions, and intervene.

This decision extends D-011 and D-012. It is a design requirement only; no implementation is implied.


## D-014 — Reference-guided gameplay and flight design
Status: ACCEPTED — 2026-11-27.

Use The Final Earth 2 as a design reference for colony simulation (individual citizens, worker assignment, production/consumption, housing and services, happiness, research, upgrades, policies, and open-ended settlement growth) and Spaceflight Simulator as a design reference for part-based spacecraft construction and readable orbital flight (part properties, mass, propulsion/fuel accounting, staging, map/orbit planning, atmospheric flight, docking, landing, and payload operations). Adapt the underlying gameplay principles to Dream Engine's spherical worlds, multi-colony economy, modular space settlements, NPC automation, and deep modding API. Do not copy source code, assets, names, distinctive UI, or exact balance values. The detailed project contracts live in GAMEPLAY.md and ROCKETS_AND_PHYSICS.md, with supporting systems in UNIVERSE.md, AUTOMATION.md, SPACE_INFRASTRUCTURE.md, and PERFORMANCE_AND_SIMULATION.md.

The game clock pauses when the app is closed by default; time warp is player-controlled while the game is running. This avoids punitive offline progression and follows the active-play focus of the reference games. This is a project design baseline, not a claim about identical implementation in either reference title.

## D-015 — Guided first-launch tutorial followed by free sandbox
Status: ACCEPTED — 2026-11-27.

The default first-launch experience begins with a guided tutorial and then transitions into an open, unrestricted sandbox. The tutorial introduces essential systems progressively through practical objectives; it is not a mandatory campaign and does not impose a permanent sequence of objectives after completion.

The tutorial must be skippable and replayable later. Skipping it must not remove access to content or advanced mechanics. Advanced controls and systems remain available when the player chooses to explore them; onboarding may explain them progressively but must not lock them behind arbitrary player levels.

Rationale: provide a welcoming learning path for mobile players without compromising the game's open-ended sandbox identity.

Consequences:
- The new-game/first-launch flow should make the guided tutorial the default path.
- After the tutorial, the player enters the same free sandbox experience available to players who skip it.
- Tutorial progress and completion should be saved per game/profile as appropriate, without making tutorial completion a prerequisite for sandbox access.
- This is a design decision only; no tutorial implementation is implied.


## D-016 — Adaptive hybrid rendering and visual LOD for colonies
Status: ACCEPTED — 2026-11-27.

Buildings use a hybrid, distance-aware rendering system. The world keeps stable logical sectors and coordinates; rendering cache regions may change size dynamically according to zoom, visible density, and measured device constraints. At close range, buildings are rendered individually. At medium and far range, static building visuals may be baked into sector caches, with multiple sectors combined into larger caches where useful. Dynamic components (for example lights, doors, machinery, smoke, particles, damage indicators, and other animated effects) remain separate where needed. Selection and interaction always use authoritative building data/geometry, never pixels from a baked texture.

Cache updates are localized: changes mark affected regions dirty, rapid edits are batched, and only affected cache regions are rebuilt. A previous valid cache remains visible until its replacement is ready. Cache generation and GPU resource upload must respect Godot's thread-safety requirements; background work may prepare data, while GPU operations occur on the appropriate rendering thread. Cache memory is budgeted and evictable; multiple visual LODs and smooth transitions avoid popping/flicker. Exact sector sizes, zoom thresholds, cache dimensions, and numeric budgets remain profiling-driven, not fixed by this decision.

Building animation and visual detail adapt gradually with distance and performance. Close buildings can show full animation and effects; medium-distance buildings use reduced animation/effects; distant buildings use minimal or no animation; off-screen buildings do not spend resources on visual animation. Animation state/time must remain coherent when detail is reduced and later restored. The player can choose an automatic mode or manually select quality/performance preferences. Automatic adaptation may use zoom, visible workload, frame-time/performance signals, and reliable device thermal/resource signals when available. Selection temporarily promotes the selected distant building's visual detail without promoting the whole planet. These visual settings must never change building functionality, production, economy, physics, or simulation time.

Citizens remain persistent logical individuals in the simulation. Their visual representation is pixel-scale and uses distance-aware visual LOD: near the camera they may be drawn as individual moving/animated pixels; with increasing distance, movement/animation update cadence and visual detail are reduced; once individuals are too small to be meaningfully distinguished, their sprites are no longer drawn. This is a rendering decision only: citizens are not deleted, merged destructively, or removed from population, jobs, needs, health, skills, identity, or save data merely because they are not visible. Their simulation continues at the appropriate active/nearby/distant simulation tier defined by the performance specification.

Rationale: preserve a lively, inspectable colony while reducing draw calls, animation work, texture rebuild cost, GPU memory pressure, and battery use on Android. Keep rendering caches strictly separate from authoritative gameplay state.

Consequences and validation:
- Keep source building assets/resources, logical building records, and baked visual caches separate.
- Keep dynamic overlays and interaction/selection data independent of baked static textures.
- Prevent cache thrashing with batching, stable logical partitioning, cache reuse, eviction, and hysteresis around LOD transitions.
- Provide safe fallback to individual/simple rendering if cache generation fails or a modded asset cannot be baked.
- Modded buildings must be able to declare static and dynamic visual components or opt out of baking when required; their gameplay behavior remains independent of rendering LOD.
- Profile CPU, GPU, draw calls, frame time, memory/VRAM, rebuild latency, battery, and thermal behavior on target Android ARM64 devices before setting numeric thresholds or claiming performance targets.
- This is a design decision only; no rendering, citizen simulation, or gameplay implementation is implied.


## D-017 — Hybrid excavatable terrain and underground colonies
Status: ACCEPTED — 2026-11-27.

Planetary terrain uses a simple, readable side-view visual style guided by the design principles of Spaceflight Simulator; do not copy its assets, code, names, or distinctive presentation. Planet surfaces retain their spherical/radial world geometry. Vegetation is intentionally minimal, guided by the restrained colony-world presentation of The Final Earth 2. Vegetation is primarily a visual accent; any plant that affects gameplay must exist as authoritative simulation data rather than only as decoration.

Terrain supports a hybrid approach to mining and underground settlement. Excavated tunnels and chambers are real, traversable gameplay spaces in areas where the player digs and builds. The implementation may use simplified or abstracted representations for distant, hidden, or otherwise irrelevant underground details instead of simulating every rock fragment or voxel at full fidelity. The player can establish underground colonies and place appropriate facilities in excavated spaces, subject to terrain, structural, environmental, access, resource, power, and life-support constraints defined by later gameplay specifications.

Terrain modification is local, not a requirement to deform or rebuild an entire planet. Excavation, mining, and other terrain edits invalidate only affected regions and their dependent visual/physical data. The authoritative terrain, excavation topology, resources, building placement, and collision/gameplay state must remain separate from visual caches. Surface mining may expose or consume deposits; asteroid mining remains a distinct space-mining activity and does not imply that all asteroid surfaces use the planetary underground-colony system.

Rationale:
- Preserve the readable, low-detail planetary presentation and mobile performance.
- Enable meaningful underground colonies and physical passageways without requiring globally deformable high-resolution terrain.
- Keep the system extensible for future mining tools, underground structures, and modded terrain interactions.

Consequences and validation:
- Represent excavated passages/chambers as persistent logical world data with stable coordinates and connectivity; do not treat a baked texture as the authoritative tunnel.
- Update terrain visuals and collision locally after excavation, while retaining unaffected valid caches.
- Use distance/visibility-aware simplification for underground rendering and simulation, without losing persistent colony, resource, or structural state.
- Keep structural stability, cave-ins, support materials, detailed geology, and exact excavation tools as open gameplay specifications; this decision does not silently settle their rules.
- Profile excavation updates, collision changes, underground rendering, memory, and save/load on Android ARM64 before choosing numeric limits.
- This is a design decision only; no terrain engine, mining, or underground-colony implementation is implied.
