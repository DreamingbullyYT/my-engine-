# City / colony phase — settlement-first implementation contract

Status: **ACCEPTED PRIORITY; feature implementation is incomplete.**
Decision: D-024.
Reference: The Final Earth 2, used only for broad colony-management design inspiration under D-014.

## Purpose

Build the settlement/city game as a substantial, satisfying game on its own before implementing playable spaceflight, orbital infrastructure, or interplanetary gameplay. The target is an original game with roughly the breadth of the major city/colony-management systems associated with The Final Earth 2, adapted to Dream Engine's spherical world, Android-first controls, modular architecture, and existing PMCS decisions.

This is not a request to recreate The Final Earth 2 exactly. Do not copy its source, assets, names, distinctive UI, story, exact building roster, or exact balance values. Design original content around the same broad types of player decisions and systemic depth.

## Scope rule

- The current phase is **city/colony only**. The player starts zoomed into the settlement and remains in the city/local surface view.
- Do not expose active rockets, rocket construction, launch, orbit, Moon/asteroid operations, stations, star maps, or the city-to-planet reveal as playable features during this phase.
- Existing vehicle/world code may remain as a dormant technical seam when it does not complicate or leak into the city game. It must not advance, consume resources, or appear as an unfinished gameplay button.
- Do not build temporary separate "city demo" and "space demo" scenes. The eventual product remains one persistent world/session under D-023; the city is the first fully playable layer of that same world.
- D-020's local-system v1 scope is **not deleted**. Its space content is sequenced after the City Completion Gate, not started in parallel.
- Keep the internals modular under D-022. "One game loop" is a player-facing continuity rule, not permission for a monolithic manager.
- Implement one focused feature/vertical slice at a time. Every slice must consult this document and the owning module note, add tests, and update PMCS.

## City feature coverage target

The City Completion Gate requires functional, integrated systems across the following areas. These are broad capability requirements, not demands to reproduce reference-game content or numbers.

### 1. World, building placement, and urban growth
- A readable close-up settlement on the project's radial/spherical surface, with touch-friendly pan/zoom and stable entity selection.
- Building definitions with stable IDs, footprints, placement rules, orientation, construction costs/time, build states, operating states, and upgrades.
- Roads, paths, access, service reach, and meaningful connection rules where required by a building or production chain.
- Expansion into additional surface sectors and later underground spaces, using D-017–D-019. Underground topology is persistent and traversable; ordinary tunnels need no support structures and use the accepted simple excavation rules.
- Demolition, relocation where supported, repair, damage, maintenance, and construction cancellation with clear costs/refunds.

### 2. Individual citizens and population
- Persistent individual citizen identity, home, job, skills/experience, traits, needs, health, happiness, activity, and relevant history.
- Visible employment states: employed, unemployed, unassigned, sick/absent, and overworked where those states apply.
- Citizen movement/routines and inspectable reasons for their current status, using performance-aware simulation and rendering LOD.
- Sustainable population change through immigration/migration and, where selected population rules support it, birth, aging, illness, injury, and death.
- Population growth is constrained by real housing, food, water, services, jobs, and environmental support.

### 3. Needs, services, health, and happiness
- Needs have measurable demand, supply, access, capacity, and quality. Start with essential needs and expand into social/recreational/comfort needs progressively.
- Housing, food, water, sanitation, safety, health care, education, recreation/social life, and later technology-appropriate services have real capacity and operating requirements.
- Happiness is calculated from explainable contributing factors; it affects productivity, retention, immigration, and social stability.
- Health and hazards must have consequences. Medical services require workforce, supplies, energy, and capacity; no hidden arbitrary rescue.
- Building service coverage/access is distinct from a building merely existing somewhere in the city.

### 4. Jobs, buildings, and production chains
- Manual assignment and priorities plus optional automatic assignment. The player can inspect and override assignments.
- Buildings have worker roles/capacity, required inputs, outputs, energy/water demand, storage, maintenance, service radius, quality, and operating conditions as applicable.
- Recipes and multi-step chains distinguish raw materials, refined materials, components, consumables, knowledge/science, and currency.
- Production is limited by actual workers, inputs, power, storage, maintenance, access, and logistics. No free or teleported resources.
- Construction, upgrades, repairs, and operations consume real resources and time. Upgrades have understandable costs and trade-offs.

### 5. Economy, inventory, and logistics
- Data-driven resource definitions, units, storage capacities, sources, sinks, and processing recipes.
- Local storage and transport/access rules. Connected buildings/colonies move resources through explicit logistics; disconnected inventories do not silently share stocks.
- Stock history, production/consumption graphs, bottleneck explanations, and shortage forecasts.
- Trade, market demand/prices, contracts, and missions are optional extensions that build on the same cargo, capacity, time, and risk constraints.

### 6. Research and progression
- Research facilities, workforce, costs, time, and prerequisites are real requirements.
- A visible prerequisite graph supports multiple specializations and alternative routes rather than one mandatory linear tech tree.
- Research unlocks buildings, production recipes, services, policies, quality upgrades, automation and, later, space capabilities.
- The colony must remain fun and usable in open-ended sandbox play without a mandatory story campaign.

### 7. Governance, policies, events, and social systems
- Policies have measurable benefits, costs, trade-offs, affected groups, and durations or conditions.
- Events offer understandable choices and causal consequences across immigration, labor, taxation, trade, safety, environment, and diplomacy.
- Factions/interest groups can add different priorities without becoming required complexity in the first tutorial.
- The mayor starts as an adviser and gains routine autonomy gradually as player oversight decreases, following D-011. Actions are logged, explainable, constrained by real state, and overridable; the mayor cannot guarantee survival.

### 8. Player experience and accessibility
- D-015 guided tutorial followed by the same unrestricted sandbox; tutorial is skippable and replayable.
- Main HUD prioritizes immediate survival/economy and actionable warnings. Inspectors, production graphs, citizen details, research, policies, and advanced values are available on demand.
- Touch-first placement, selection, assignment, batch actions, filters, search, undo/confirmation for destructive actions, scalable text, reduced motion, and Romanian/English localization follow MOBILE_UX.md and D-021.
- Temporary art remains primitive and replaceable. Every image texture/sprite remains at most 32×32 px under the user's existing art decision.

### 9. Persistence, mods, and technical quality
- Deterministic, versioned saves preserve world seed, citizens, buildings, terrain/underground topology, inventories, production queues, research, policies, events, and mayor history.
- Safe load/migration, atomic writes, backups, corruption recovery, and stable IDs are required before the City Completion Gate.
- Core definitions and rules expose data-driven integration points to the Dream Mod API. Mods can add citizen traits/jobs/needs, buildings, resources, recipes, policies, research, events, factions, and systems.
- Active/nearby/distant simulation tiers preserve population, inventory, production, causal outcomes, and save data; the app being closed does not consume colony supplies.
- Profile representative workloads on target Android ARM64. Do not invent numeric performance budgets without measurements.

## City Completion Gate — must pass before playable space work

Do not call the city phase complete merely because the opening city renders or a few buildings can be placed. The following must all be demonstrated in one persistent save/session:

1. Citizens have persistent identities, needs, jobs, health, happiness, inspectable statuses, and visible routines at the appropriate LOD.
2. Housing, food, water, energy, safety, health, sanitation, education, and recreation services function through real capacity/access/worker/operating rules. Any deliberately deferred service must be explicitly justified in this gate review.
3. Multiple buildings and multi-step production chains work with real inputs, outputs, workers, power, storage, construction time, upgrades, maintenance, and logistics.
4. The player can expand the city, place/select/inspect/build/upgrade/repair/demolish structures, and interact with surface/underground spaces according to D-017–D-019.
5. Population can develop sustainably; shortages and bad decisions have readable, causal consequences.
6. Research has a useful multi-branch progression. Policies and events create meaningful, reversible decisions. Mayor advice/delegation follows D-011.
7. Production/consumption history, bottlenecks, projected shortages, citizen inspection, and actionable reports are usable on the phone.
8. Tutorial and free sandbox both work, and the same colony can be saved, reloaded, resumed, and migrated safely.
9. Automated tests cover normal play, resource/worker/service shortages, failure cases, save/load, long simulation, and deterministic aggregate progression.
10. The player confirms the complete city experience is enjoyable and usable on the physical Android device; CI alone does not close this gate.

The phrase "roughly the breadth of The Final Earth 2" means the major city-management layers above are present and meaningfully interact. It does not mean exact feature, content-count, or visual parity.

## Space integration seam (design only during this phase)

Preserve clean future integration without implementing space gameplay now:
- Colony owns population, needs, jobs, inventories, production, research, policies, and governance.
- World owns celestial-body identity/coordinates; rendering consumes world and colony read models.
- Future exploration/vehicle/infrastructure modules request and deliver resources, science, unlocks, and new settlement locations through explicit domain contracts.
- Research unlocks may later declare space capability IDs, but the city does not import rocket physics or vehicle state.
- A future space settlement may reuse colony contracts where appropriate, but must be a distinct settlement context with its own local constraints and logistics.
- D-023's city → planet → nearest real target camera reveal becomes active only when space launch is implemented and genuinely unlocked after this phase. Until then, the player remains in the city/local view.
- Do not create empty future modules or speculative placeholder APIs. Add each contract when the first real integration slice needs it.

## Implementation order

Use small tested vertical slices in this dependency order:
1. City-only presentation and controls; stable IDs; world/building placement foundations.
2. Citizen state, needs, job assignment, skills, citizen inspection, and routines.
3. Building catalogue, construction/placement, services, capacities, operating states, and upgrades.
4. Resource definitions, inventories/storage, recipes, production chains, and logistics.
5. Population growth/migration and health/happiness consequences.
6. Research graph and unlock contracts.
7. Policies, events, factions/interest groups, and gradual mayor governance.
8. Surface expansion, excavation, underground topology, and local terrain updates.
9. Production analytics, alerts, tutorial/free-play, localization, save migration, modding integration, profiling, and device validation.
10. Review the City Completion Gate with the user. Only then activate the D-020 space phase and D-023 scale reveal.

Do not skip the gate by renaming a prototype or treating planned systems as implemented.
