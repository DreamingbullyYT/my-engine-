# World view / rendering module

Status: **CITY-FIRST CAMERA FOUNDATION IMPLEMENTED IN SOURCE; CI AND DEVICE VALIDATION PENDING.**
Decision references: D-016, D-017–D-019, D-022, D-023.
Entry point: `rendering/world_view.gd`.

## Ownership
Presentation only. It displays the colony's radial surface and reads building/citizen/rocket state supplied by the owning modules. It owns camera position/zoom, panning, touch gestures, visual LOD, and drawing. It does **not** own world/body definitions, citizen/building identity, rocket physics, colony economy, unlock progression, save data, or exploration outcomes.

## Entry and integration
- `rendering/world_view.gd`: one full-screen Control used by `ui/screens/game_screen.gd`.
- `ui/screens/game_screen.gd`: persistent main game screen, HUD/build controls, diagnostic overlay, and adapter that passes read-only presentation values from ColonySimulation and RocketFlight.
- `world/home_world_definition.gd`: shared starting-body geometry/gravity prototype values consumed by renderer and flight simulation.
- `vehicles/rocket_flight.gd`: provides simple `has_rocket()`, `is_flight_active()`, and `is_on_surface()` facts to the presentation adapter.

## Camera behavior
1. New-game camera starts close to the colony on the radial surface. It does not show the full planet.
2. Before rocket-launch capability is unlocked, zoom-out is clamped to city/local-region scale.
3. In the current prototype, constructing the launch pad emits the rocket-capability unlock; GameScreen tells the same WorldView to animate from the city to a home-planet overview sized to fit the viewport.
4. The player can pinch zoom and drag to pan. A City button recentres to the same colony.
5. On rocket flight, the camera follows the craft; a user pan/pinch cancels automatic following. On landing after a flight, the camera recentres on the same city.
6. Diagnostics are shown as an overlay on GameScreen; they do not replace the game scene/session.

## Visual LOD
- Buildings are drawn as individual primitive shapes near the city and gradually aggregate to small markers with distance.
- Citizens are individual small primitive figures near the city and fade out when below useful visual scale; their simulation records are untouched.
- The city surface/planet disc transition and sky elements blend rather than swapping to a different scene.
- All current visuals are code-drawn primitive placeholders. No final art is included. User art remains limited to 32×32 px per image file.

## Invariants and dependencies
- Read radial building angles and world/rocket positions; never mutate them.
- Use the shared HomeWorldDefinition; do not duplicate planet radius/gravity.
- Renderer receives simple rocket presentation facts instead of duplicating the RocketFlight state enum.
- Camera zoom, transitions, LOD, or off-screen drawing never change authoritative simulation or game time.
- Do not add a monolithic world/game manager. Keep responsibilities as in MODULES.md.

## Known limitations / next steps
- The current starting-body definition contains only the home world. There is no celestial-body catalogue; therefore the additional nearest-planet/system-target phase is not yet implemented. Do not generate a fake body. Add authored v1 body records before implementing target selection.
- City economy and rocket ascent/orbit/return exist, but there is no local exploration scan/mission outcome or returned resource/science reward yet. This is required to complete the continuous city↔space gameplay loop.
- Touch selection/build placement on the world, detailed building footprints/streets, real sprites, reduced-motion settings, accessible pan/zoom controls, and rendering/performance profiling remain future work.
- Headless smoke tests verify states and wiring, not visual layout. A physical Android test is required.

Last source revision: this PMCS update; CI validation pending.
