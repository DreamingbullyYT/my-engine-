# Rockets, Vehicles, and Flight Physics

Status: planned design. Reference direction: adapt the part-based construction and readable orbital-flight principles of Spaceflight Simulator; do not copy code, assets, names, UI, or exact part catalog.

## Vehicle construction
Spacecraft are built from attachable parts, not fixed templates. Each part exposes mass, dimensions, attachment nodes, structural strength, drag/aerodynamic properties, thermal limits, resource capacity, power draw/output, control authority, and functional capabilities. A craft is a connected assembly with validated attachment and resource graphs.

Part families include:
- Structural cores, adapters, nose cones, fairings, payload bays.
- Fuel tanks and pressurized resource tanks.
- Engines, RCS thrusters, reaction wheels, and control systems.
- Separators, decouplers, docking ports, launch clamps.
- Capsules, crew cabins, probes, batteries, solar panels, antennas.
- Landing legs, wheels, parachutes, heat shields, radiators, drills, cargo bays, robotic arms, and scientific payloads.
Mods can register new parts and attachment/capability types.

## Mass, propulsion, and staging
Vehicle mass is the sum of dry mass, propellant, payload, and crew/cargo. Engine thrust, specific impulse, throttle limits, gimbal, ignition/restart constraints, propellant type, and consumption determine acceleration and delta-v. TWR is derived from thrust and local gravity. Fuel flow must be physically accounted for; no free fuel or instantaneous refills except through actual compatible transfer systems.

Staging is an explicit ordered sequence. Separators detach connected assemblies; staging can trigger engines, decouplers, fairings, parachutes, or other configured actions. Stage plans must be inspectable and editable. Fuel crossfeed and tank priority are explicit connection/configuration rules.

## Flight model
Use a consistent world-state model with orbital propagation and local high-detail flight dynamics:
- Keplerian conic/orbit propagation for stable coast phases and distant objects.
- Numerical integration for powered flight, atmospheric flight, landing, docking, and other phases where forces/control interactions matter.
- Continuous state handoff: position, velocity, orientation, angular velocity, mass, fuel, and thermal state must be preserved across model changes. No teleportation or free trajectory corrections.
- Player and NPC pilots use the same vehicle model, environment, fuel limits, and maneuver constraints.
- Precision and simulation-step requirements are defined per flight regime and tested against reference scenarios.

## Navigation and mission planning
Provide map/orbit view, current orbit parameters, target selection, maneuver nodes, projected trajectories, encounter/transfer planning, delta-v budget, TWR, fuel estimates, and time warp. Time warp is limited or disabled during unsafe powered/atmospheric/docking phases. Maneuver plans are predictions, not guaranteed outcomes; actual execution uses the same physics and can diverge.

## Atmosphere, heating, and aerodynamics
Atmospheric density and composition vary by altitude and body. Drag, lift, stability, heating, and pressure loads affect flight. Heat shields and thermal limits matter during re-entry. Parachutes require valid atmospheric density and deployment conditions. Wings/control surfaces require airflow. Each body has its own gravity, atmosphere, rotation, terrain, and landing conditions.

## Docking, landing, and surface operations
Docking requires compatible ports, relative-position/velocity control, alignment, and structural compatibility. Fuel and cargo transfer require connected compatible ports and actual inventory. Landing depends on terrain, gravity, descent speed, landing gear, and environmental conditions. Rovers and surface vehicles use terrain contact and traction. A landed craft can deploy drills, cargo systems, science payloads, and outpost modules.

## Failure and recovery
Parts can overheat, exceed pressure/structural limits, run out of propellant, lose power, lose control, collide, or fail due to environment and damage. Failure severity should be configurable by difficulty/realism settings. Recovery, rescue, crew survival, debris, and mission aborts are supported where appropriate. Avoid random failure unrelated to visible causes.

## NPC and remote piloting
NPC flight planning uses craft capabilities and the same physics. Mission feasibility reports explain missing delta-v, propellant, control authority, communication, docking hardware, landing capability, or crew support. Player takeover is always possible; the plan is recalculated from current state.

## Accessibility and controls
Provide touch-friendly construction, part selection, staging, throttle, attitude control, map navigation, and maneuver editing. Offer configurable assistance/realism settings without changing the underlying conservation/resource rules. Beginner-facing flight should offer recommended craft, suggested maneuvers, and assisted controls; advanced telemetry and manual controls remain accessible immediately on demand. See MOBILE_UX.md.
