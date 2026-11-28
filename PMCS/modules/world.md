# World module — starting body definition

Status: **INITIAL SOURCE FOUNDATION; full catalogue and generation NOT IMPLEMENTED.**
Decision references: D-017, D-019, D-020, D-023.
Source: `world/home_world_definition.gd`.

## Ownership
The world module owns celestial-body identity and physical/geometric metadata. It must not own rendering pixels, colony economy, vehicle propulsion, or UI state.

The current source is only one starting-body definition. It is not yet a star-system catalogue, procedural generator, discovery system, or save model.

## Current entry point
- `world/home_world_definition.gd`: stable body ID plus normalized prototype radius and gravitational parameter.
- Current values are **simulation units**, not real-world Earth/planet measurements. They exist only to keep the first city/radial-surface and rocket-flight prototype using one consistent body definition.

## Consumers
- `rendering/world_view.gd` reads the radius to position the city surface and fit the home-world overview.
- `vehicles/rocket_flight.gd` reads the same radius and gravitational parameter for its temporary radial ascent/orbit model.

Do not duplicate these values in consumers. Replace the prototype definition with data-driven body records when the authored celestial catalogue is implemented.

## Invariants
- A planet/body has one authoritative definition. Renderer and vehicle physics consume it; they do not maintain independent radius or gravity constants.
- Camera focus/zoom is presentation-only; it is not a body coordinate or a change to the body's physical position.
- Do not invent a decorative target planet. D-023's nearest-target camera stage requires real body records from the future catalogue; v1 fallback may target an actual Moon/asteroid/destination where available.
- D-020 limits first playable content to the local system: starting planet, Moon, nearby asteroids, and a basic orbital station. Interstellar systems remain later work.

## Tests and limitations
- `tests/gameplay_smoke.gd` checks that rendering and rocket scripts consume the same home-world radius.
- No catalogue, orbit hierarchy, physical mass model, rotation, atmosphere, terrain, body discovery, or deterministic procedural generation exists yet.
- No real-world units or accuracy claim is made by the prototype constants.

Last source revision: this PMCS update; CI validation pending.
