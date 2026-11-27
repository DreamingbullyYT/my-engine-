# Automation, Mayors, Pilots, and Fleets

Status: planned design. This document consolidates D-011 and D-013 and the NPC pilot decisions.

## Colony mayors
A mayor begins as an adviser. With reduced player oversight, responsibility grows gradually from recommendations to routine operations and later to larger local projects, subject to player policies and real capabilities. This is not a fixed three-mode autonomy toggle. Every meaningful action has a reason, cost, result, and history entry. The player can inspect, override, and change permissions.

## NPC pilots
Support three compatible control arrangements:
- Crew pilot physically aboard a craft.
- Colonist pilot remotely controlling a craft over a functioning communication link.
- Autonomous control for uncrewed probes and vehicles.

Any craft can be piloted if its control interface and installed capabilities permit it; no universal mandatory autopilot part is required. NPCs can plan and execute launches, orbital insertion, transfers, corrections, rendezvous, docking, landing, and surface operations. They use the same physics, fuel, communications, vehicle capabilities, and risks as the player. They cannot teleport or bypass constraints. The player can take over at any time.

## Fleet management
Fleets group craft by role, location, mission, and logistics needs. Orders include exploration, transport, mining, construction, patrol/defense when available, rescue, resupply, and colonization. Orders are plans with prerequisites, not guaranteed success. The AI checks available craft, crew, fuel, cargo, propulsion, communications, target knowledge, and destination facilities.

## Cargo automation
Cargo routes define origin, destination, cargo manifest/priority, loading/unloading rules, capacity, schedule/conditions, and repeat behavior. A route can pause or fail due to insufficient stock, unavailable destination capacity, damaged craft, missing fuel, blocked docking, or communications issues. Cargo is physically carried and consumed/loaded/unloaded through inventories; it never teleports.

## Mobile-colony fleet autonomy
A mobile colony has its own mayor and can plan/build varied ship types only when ship designs, technology, shipyard capacity, workers, resources, energy, fuel, and time permit. Fleet composition reflects the colony's needs. No free ship or resource creation.

## Autonomous expansion
After roughly 5–6 explored star systems, the mobile-colony mayor may ask for explicit permission to autonomously found additional colonies. The player approves, inspects, changes, or revokes the policy. Only genuinely habitable worlds are ordinary automatic targets; special atmospheric/orbital habitats and suitable moons are engineered alternatives. Each expedition requires transport, population, life support, food, water, power, materials, communications, landing/habitat capability, and sustained logistics. New colonies receive their own mayor and remain vulnerable to real shortages, disasters, and isolation.

## Remote simulation and offline behavior
Nearby/active craft and colonies use detailed simulation. Distant entities use orbital propagation or aggregated economic simulation while preserving the same resources, costs, population, and failure rules. The game clock pauses when the app is closed by default. While the game is running, player-controlled time warp advances all relevant systems consistently. No mandatory daily login or punitive offline progression.

## AI explainability
Every autonomous plan should expose objective, assumptions, resource budget, risks, prerequisites, and outcome. Failures should report actionable causes. The AI may not invent resources, ignore physics, or silently change player permissions.
