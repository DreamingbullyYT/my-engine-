# Space Expansion — Mobile Colony Fleets and Habitable-World Colonization

Status: accepted design requirements (D-013), not implemented.

## Mobile-colony fleet autonomy
A mobile colony ship/megaship has its own mayor and operates as a governed settlement. The mayor can plan, select, and request construction of varied ship types according to the colony's needs: transport, exploration, mining, cargo, construction, or other unlocked roles.

Ship construction is constrained by actual capability:
- Available ship designs and technology unlocks.
- A suitable shipyard and installed production modules.
- Skilled workforce and available population.
- Raw materials, components, energy, fuel, and construction time.
- Logistics for moving resources and supporting the fleet.

The mayor does not create ships, materials, fuel, or workforce for free. It cannot build a design the colony cannot manufacture.

## Permission-gated autonomous colonization
After roughly 5–6 star systems have been explored, a mobile-colony mayor may ask the player for permission to begin autonomously establishing new colonies. This is a progression/balance target and should be configurable, not a universal hard-coded constant.

- The player must explicitly approve the policy before autonomous planetary colonization begins.
- The player can inspect expansion plans and history, alter or revoke permission, and take control at any time.
- The mayor can search discovered systems and propose viable candidates; it cannot colonize undiscovered worlds or bypass travel, construction, or logistics.
- Every new colony receives its own local governance and remains subject to real population, resource, infrastructure, and survival constraints.

## Habitability and settlement classification
Automatic planetary colonization is restricted to worlds that are genuinely habitable for the civilization's current biology and technology. Evaluation should consider temperature, pressure, atmospheric composition, gravity, liquid water, radiation, resources, and environmental hazards. It must report why a world is accepted, rejected, or requires special infrastructure.

Ordinary planetary colonization exclusions:
- **The Sun:** never a planetary colony target.
- **Mercury:** not an ordinary habitable-world target.
- **Venus surface:** not an ordinary habitable-world target.
- **Gas-giant interiors (for example Jupiter):** not ordinary planetary surfaces and not ordinary colony targets.

These exclusions do not forbid specialized engineering:
- Floating atmospheric habitats in suitable layers, such as Venus's upper clouds.
- Orbital stations and habitats around planets or stars, subject to their own environmental and supply requirements.
- Settlements on suitable natural moons, evaluated as separate worlds.
- Other specialized habitats only when their required technology and modules exist.

A gas giant itself must not be mislabeled as a normal solid-surface colony simply because it has moons or orbital infrastructure.

## Expedition feasibility
Before dispatch, evaluate:
- Population and colonist capacity.
- Transport ships and cargo capacity.
- Life support, food, water, power, and storage.
- Construction/landing systems or a suitable habitat plan.
- Fuel, propulsion, communications, and supply-chain continuity.
- Required technology, workforce, materials, and time.

Failed or under-supplied expeditions can fail. A founded colony can later decline or be lost. Mayors reduce micromanagement; they do not grant immunity.

## Modding and host integration
Expose extensible host interfaces for ship designs, shipyard capabilities, habitability profiles, settlement environments, expansion policies, expedition planning, and colony founding. Mods may add ship types and specialized habitat classes, but must expose their capability requirements and environmental constraints.
