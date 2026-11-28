# Gameplay and Colony Simulation

Status: accepted gameplay design target; most systems are not yet implemented. Under D-024, this city/colony design is the active implementation phase and must pass PMCS/CITY_PHASE.md's completion gate before playable space work begins. Reference direction: adapt broad colony-management principles of The Final Earth 2; no code, art, names, UI, or exact balance values are copied.

## Core loop
Build and maintain a settlement → extract and process resources → house and support citizens → assign workers → research technologies → unlock buildings and equipment → construct spacecraft → explore and bring back resources/knowledge → expand and sustain more settlements. Space activity must continually benefit inhabited colonies; it must not replace colony management as the central game.

## Citizens
Citizens are persistent simulated individuals, not only a population counter. Each citizen can have identity, home, job, skills/experience, needs, health, happiness, traits, and current activity. The simulation should support:
- Arrival/immigration and migration between settlements.
- Birth, aging, illness, injury, and death where the chosen population model supports them.
- Job assignment based on available positions, skills, commute/access, and player or mayor priorities.
- Unemployed, unassigned, sick, absent, or overworked citizens as visible states.
- Following/inspecting individual citizens and understanding why they are happy, unhappy, productive, or idle.
Population growth must be supported by housing, food, services, life support, and sustainable jobs; population is never free capacity.

## Needs, services, happiness, and health
Citizens need suitable housing, food, water, safety, health care, sanitation, education, recreation/social life, and other needs appropriate to the settlement and technology. Each need has measurable supply, access, capacity, and quality. Shortages create visible consequences rather than hidden arbitrary penalties.

Happiness is a composite outcome of living conditions, needs, health, work, commute, safety, policies, and social/recreational opportunities. It influences productivity, retention, migration, and social stability. Festivals and temporary boosts may help but cannot permanently replace essential services. Exact weights and curves are balance data, not hard-coded assumptions; expose them to configuration/mods and show explanatory breakdowns to the player.

Health includes medical capacity, sanitation, disease/epidemic possibilities, injury and environmental hazards. Medical buildings require workers, supplies, energy, and capacity. Health systems should be extensible and should not require a detailed medical simulation for every citizen every frame.

## Jobs, buildings, and production
Buildings declare footprint/placement constraints, construction cost/time, worker roles and capacity, inputs, outputs, storage, energy/water requirements, service radius/access, upgrade levels, and operating conditions. Production is limited by workers, input availability, output storage, power, maintenance/state, and logistics. A building without required inputs or workers cannot produce at full rate.

Provide:
- Manual worker assignment and priorities, plus automatic assignment policies.
- Production/consumption graphs, stock history, bottleneck explanations, and projected shortages.
- Building upgrades that improve defined properties at real costs and possible trade-offs.
- Construction, repair, maintenance, damage, decommissioning, and demolition.
- Local storage and transport links; resources do not teleport between disconnected buildings/colonies.
- Vertical/radial building placement on the spherical world, including the project's established stacking and underground concepts.

## Resources and economy
Resources have definitions, units, mass/volume where relevant, storage rules, sources, sinks, processing recipes, and transport requirements. Separate raw materials, refined materials, consumables, components, science/knowledge, and credits. Recipes specify inputs, outputs, workers, energy, time, by-products, and unlock requirements.

Trade uses cargo capacity, routes, travel time, risk, supply/demand and changing prices. Market prices may respond to scarcity and delivered volume but must remain understandable and exploitable only through intended gameplay. Contracts and missions create optional demand/rewards; they do not replace the core economy.

## Research and progression
Research unlocks buildings, production chains, spacecraft parts, tools, habitat systems, automation capabilities, and new exploration options. Use a visible prerequisite graph with costs, time, and required facilities/workforce. Science may come from laboratories, observations, missions, samples, and discoveries. Multiple specializations and alternative routes should support different colony strategies. Avoid one mandatory linear tech path.

## Policies, factions, events, and scenarios
Colony administration can enact policies with measurable benefits, costs, trade-offs, and affected groups. Factions/interest groups can represent citizens with different priorities; they should be optional to the depth of the simulation and extensible by mods. Events can present choices and consequences, including immigration, labor, taxation, trade, safety, environmental events, and diplomacy. Scenarios/story objectives coexist with open-ended sandbox/free-play. The game must remain playable without a mandatory story campaign.

## Planetary colony and world building
A colony is a settlement on a spherical body. Surface placement follows radial coordinates and local terrain/environment. Planetary environments affect construction, energy, agriculture, health, resource extraction, and survival. Buildings and population must be constrained by local gravity, temperature, atmosphere, radiation, day/night, and available land/underground volume.

## Mayor and remote colony simulation
Use D-011: adviser first, gradually increasing delegated responsibility based on player attention/absence. Mayors coordinate existing systems; they do not bypass resource, workforce, production, transport, or survival rules. All actions are logged and reversible where practical. Remote colonies use deterministic aggregated simulation with the same economic constraints as active colonies. Offline real-time progression is paused by default; no punitive catch-up while the app is closed. Time warp and accelerated simulation are player-controlled while the game is running.

## UI and player feedback
Every critical system needs readable status, capacity, input/output, worker, power, storage, and shortage information. Show production graphs and citizen-level inspection. Use actionable alerts and summarized reports rather than constant notification spam. Touch-first UI must support selecting, building, assigning workers, inspecting, and navigating large colonies. Keep the default view simple and reveal detailed production graphs, citizen controls, and technical values on demand. Automatic worker assignment and recommended priorities should be available without removing manual control. See MOBILE_UX.md for the full mobile interaction proposal.

## Modding
All core content and rules should be data-driven and extensible through the Dream Mod API. Mods can add citizen traits/jobs/needs, buildings, resources, recipes, policies, research, events, factions, and entire systems. See MODDING_API.md.


## Citizen visual representation and distance LOD — D-016

Citizens are persistent simulated individuals, but their on-screen representation is deliberately simple: pixel-scale people. Their visual movement and animation are reduced progressively as the camera moves away. At close range, individual pixel people can move and animate; at medium/far range, visual movement/animation updates become less frequent and details are simplified; when an individual is too small to be meaningfully visible, its sprite is no longer drawn. This does not remove the citizen from the simulation or save: identity, population membership, job, skills, needs, health, happiness, and other authoritative state remain intact. Off-screen citizens likewise stop consuming visual animation work, while their simulation continues at the appropriate simulation tier. See PERFORMANCE_AND_SIMULATION.md and D-016.


## Terrain, minimal vegetation, mining, and underground colonies — D-017

Planetary terrain uses a simple, readable side-view presentation guided by Spaceflight Simulator's broad design principles, adapted to Dream Engine's spherical/radial worlds. Do not copy reference-game assets, code, names, or distinctive UI. Vegetation is intentionally sparse and visually restrained, guided by The Final Earth 2; gameplay-relevant plants must still be represented as real resources/entities with simulation state.

The terrain supports local excavation and underground settlement through a hybrid model. Players can create persistent, traversable tunnels and chambers and establish underground colonies with suitable buildings and infrastructure. These are real gameplay spaces, not merely painted holes in a texture. Nearby/relevant underground spaces receive the fidelity needed for movement, construction, selection, and interaction; distant, hidden, or irrelevant details may be simplified. Excavation updates only affected terrain regions and dependent data, never requiring a full-planet rebuild. Visual caches are not authoritative for terrain, resources, movement, or building placement.

The initial excavation model is deliberately lightweight (D-018):
- Ordinary tunnels and chambers are stable by default. There is no routine cave-in, rock-pressure, or structural-strength simulation.
- Underground spaces require no structural supports. The initial game has no support beams, pillars, reinforcement structures, or automatically generated support decorations.
- All terrain follows the same basic excavation rule. There is no complex material-specific hardness or digging-resistance simulation.
- Terrain/material types may still differ in their resource deposits and the resources available through mining; simplifying excavation does not make resource geology identical.

Planetary surface/underground mining and asteroid mining both contribute to resource progression, but remain distinct systems with their own environment and interaction rules. Extraction respects actual deposits, quantities, tools, infrastructure, workforce, energy, and transport capacity.


## Earlier colony-to-orbit prototype — 2026-11-28 (space portion dormant under D-024)

The earlier prototype is a small foundation, not the complete v1 design above. Under D-024, the rocket/launch portion is now hidden and dormant while the city phase is developed. The city-only player experience must not expose space capabilities until the City Completion Gate.

- The player starts with a close view of the city on the curved radial planetary surface. The full planet stays hidden for the entire D-024 city phase. The eventual D-023 reveal remains reserved for real rocket capability in the later space phase; there is no separate city/planet scene.
- Five building types are implemented: habitat (housing), solar array (energy generation), farm (food production), mine (metal production), and launch pad (rocket access).
- A new colony starts with 20 named citizen records. Each record has a stable local ID, current job, health, and happiness. Worker assignment is automatic and intentionally simple in this slice.
- The day loop consumes food, produces food/metal, generates and consumes energy, and updates citizen happiness when food is insufficient. Offline progression remains paused; no catch-up simulation occurs while the app is closed.
- The previous prototype included a simplified rocket path, but it is not exposed as playable UI during D-024. Its source is dormant until the later space phase.
- The current city-phase save stores colony resources, citizens, buildings, and day in a local JSON save slot; the city phase persists only the state it actively owns.
- This earlier slice does not yet include the broader city feature set in CITY_PHASE.md: detailed needs/routines, services, research, policies/events, logistics, population growth, excavation, and related systems. Space is also explicitly deferred until the city gate. The flight model is a deliberately simplified first gameplay implementation, not a complete orbital simulator.

## Art budget and temporary visuals — user decision, 2026-11-28

The user will create all final textures and sprites. **32×32 pixels is the maximum size for any individual final sprite/texture.** Temporary visuals must follow the same maximum and stay extremely basic. Use flat colours and primitive pixel shapes that are straightforward to replace. Do not spend development effort on detailed textures, polished illustrations, or large texture atlases. The current city-first presentation uses code-drawn primitive placeholders rather than external texture assets. It follows D-023; the rejected planet-first renderer is not the main scene.


## One continuous city-to-space gameplay loop — D-023

The game is presented as one persistent world, not as separate city, planet, and space modes. The same world session, colony, citizens, buildings, resources, vehicles, and simulation remain active while the camera changes scale.

### Starting experience: city first
- Start zoomed close to the settlement. Buildings, citizens, paths/streets, and local surface details occupy the view.
- Do not show the full planet/globe in the initial scene or default camera framing.
- Until rocket launch capability is genuinely unlocked, camera limits keep the view at city/local-region scale.
- Keep the spherical/radial world model authoritative underneath this close-up view; the camera presents the local surface, it does not turn the world into a separate flat level.

### Rocket milestone: reveal the world
When the player unlocks the real ability to build and launch rockets, run a one-time automatic camera reveal:
1. Pull back from detailed buildings and citizens to the wider settlement.
2. Continue through the local curved planetary surface to reveal the whole home planet.
3. Continue to the local-system view and guide focus to the nearest actual planet in the celestial-body catalogue. If the v1 system has no additional planet, use the nearest real Moon/asteroid/destination available in the v1 world data rather than inventing a decorative body.

The transition is a camera animation only. It does not change the simulation, teleport the craft, grant technology/resources, or load a different gameplay scene. It must be possible to zoom back into the same city and see the same buildings and citizens.

### Connected progression loop
City survival/economy → construction/research → rocket capability → spacecraft/launch → local-space exploration and infrastructure → returned resources/data or supported settlement growth → city economy and new capabilities. All later v1 systems (Moon, nearby asteroids, orbital station, research, spacecraft, cargo, and colony expansion) must connect to this same persistent loop and use the owning domain modules from MODULES.md. They must not be implemented as unrelated sample screens or independent demos.

The city remains the player's home and the centre of progression. Space activity should create useful consequences for the same colony; it does not replace colony management.

### Presentation and simulation invariants
- One world session and one persistent main world scene; use overlays for contextual UI rather than switching between separate city/planet/space scenes.
- Zoom/pan/follow-camera changes never create or destroy gameplay entities and never change authoritative simulation values.
- Building/citizen visual LOD follows D-016 while all identities, jobs, needs, production, and save data remain persistent.
- Use the radial/polar planetary surface and persistent underground topology from D-017–D-019. The local city is a close-up of this surface, not a separate disconnected map.
- Keep the game clock paused while the app is closed, per D-014. A camera transition must not advance hidden/offline time.


### D-023 first camera implementation — source status pending CI

The current prototype connects the city and the home-world/rocket scale in one `GameScreen` session:
- The home-world radius and gravitational parameter are read from `world/home_world_definition.gd` by both `rendering/world_view.gd` and `vehicles/rocket_flight.gd`.
- Building the launch pad is the current prototype's unlock event for rocket construction/launch capability and triggers the one-time camera reveal.
- The camera can follow the same rocket during powered ascent, orbit, and return. A user pan/pinch stops automatic follow; landing recentres the camera on the same city.
- The `City` control recentres the camera without resetting colony or vehicle state.
- The current starting-world definition is only a normalized prototype definition. The separate celestial-body catalogue, nearest additional target selection, local exploration/scanning, and returning resource/science rewards remain unimplemented. These are required before claiming that the complete city↔space gameplay loop is finished.
