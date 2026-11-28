# Colony module

Status: **INITIAL CITY SIMULATION FOUNDATION IMPLEMENTED; D-024 CITY FEATURE SET IN PROGRESS.**
Decision references: D-011, D-014–D-019, D-022, D-024.
Entry point: `colony/colony_simulation.gd`.

## Ownership

The colony domain owns authoritative settlement state and rules: citizens, buildings, worker assignments, resources, day progression, production/consumption, housing capacity, happiness, research, policies, and governance as those systems are implemented.

It does not own screen layout, visual rendering, camera transforms, rocket physics, or celestial-body definitions. UI must call colony use-cases and read domain state; rendering receives read-only presentation values.

## Current implementation

- `ColonySimulation` is registered as an autoload in `project.godot`.
- Starting state: 20 named citizen records, habitat/solar/farm starter buildings, metal/food/energy stocks, day counter.
- Buildable city baseline: habitat, solar array, farm, and mine.
- Workers are automatically assigned to available building slots in building order.
- A simple day tick calculates solar generation, farm food, mine metal, energy demand, food consumption, and a basic happiness adjustment for food availability.
- `get_snapshot()` / `restore_snapshot()` preserve the current colony prototype state.

## City-phase boundary

- D-024 makes city/colony management the active development phase. Space launch pads, rocket construction, launch, orbit, Moon/asteroid activity, and the planet reveal are not available in the city UI.
- Vehicle and space-related source is not owned by this module and must not be added here.
- The current simulation is only a prototype: citizen records lack full needs, age, traits, skills, routines, homes, health/service coverage, and detailed activity; jobs are only simple string assignments.
- Building definitions are currently a small embedded dictionary. They do not yet model formal footprints, placement constraints, build time, inputs/outputs, storage, maintenance, access, service radii, upgrades, repair, or demolition.
- Resource handling is currently global stock values, not spatial storage or logistics. The simple day tick is not a full production scheduler.
- Research, policies, events, factions, mayor behavior, migration, population growth, and underground topology are not implemented.

## Dependencies and callers

- `ui/screens/game_screen.gd` reads citizens, buildings, stock levels, day, messages, and summary values; it must not calculate colony rules.
- `rendering/world_view.gd` receives building/citizen arrays through a presentation adapter and must not mutate them.
- `saves/save_service.gd` stores colony snapshots as part of the game save.

## Invariants

- One authoritative citizen/building/resource state; no copies owned by UI or renderer.
- Simulation advances only while the app is running; app background/closure does not create offline consumption under D-014.
- Production and population changes must obey actual workers, inputs, power, capacity, and logistics as their systems are introduced.
- Happiness and health must be explainable from their contributing needs/services; no arbitrary invisible penalties.
- All persistent entities require stable IDs and versioned save representation.
- Colony code must remain independent of vehicle/rocket physics. Future space systems consume explicit colony inventory/research contracts.

## Tests

- `tests/gameplay_smoke.gd`: city-only headless smoke test for starting population, starter housing/jobs, mine construction/production, food, hidden planet zoom, and snapshot restoration.
- CI runs it with official Godot before native build and Android export.
- Physical Android tests must separately validate city rendering, touch controls, pause/resume, performance, and persistence.

## Next work

Follow `PMCS/CITY_PHASE.md` in order: stable citizen state and need system; worker/job priorities; data-driven buildings/services; inventories/recipes/logistics; population growth; research; policies/events/mayor; surface/underground construction; analytics/tutorial/localization; robust saves/mod integration; profiling and device validation.

Last source revision: pending first city-only CI pass.
