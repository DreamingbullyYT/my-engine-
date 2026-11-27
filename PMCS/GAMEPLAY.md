# Gameplay and Colony Simulation

Status: planned design. Reference direction: adapt the colony-management principles of The Final Earth 2; no code, art, names, UI, or exact balance values are copied.

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

The terrain supports local excavation and underground settlement through a hybrid model. Players can create traversable tunnels and chambers and establish underground colonies with suitable buildings and infrastructure. These are persistent gameplay spaces, not merely painted holes in a texture. Their exact construction rules, structural stability, support requirements, cave-in behavior, geology, and environmental hazards remain to be specified separately.

The engine does not need to simulate every rock fragment at full resolution. It may simplify distant, hidden, or irrelevant underground details while preserving authoritative tunnel connectivity, collision/navigation where needed, resources, structures, and colony state. Excavation and mining update only affected terrain regions and dependent data; they must not force a full-planet rebuild. Visual caches are never authoritative for terrain, resources, movement, or building placement.

Planetary surface/underground mining and asteroid mining both contribute to resource progression, but remain distinct systems with their own environment and interaction rules. All extraction must respect actual deposits, quantities, tools, infrastructure, workforce, energy, and transport capacity.
