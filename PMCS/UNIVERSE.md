# Universe, Worlds, and Exploration

Status: planned design.

## World model
The universe contains star systems, stars, planets, natural moons, asteroids, comets, and artificial settlements. The project uses spherical worlds viewed in 2D and a consistent radial surface coordinate model for colonies. Celestial bodies have mass, radius, rotation, gravity, orbit, terrain, and environmental data.

## Star systems and procedural content
Use a data-driven catalog that supports authored systems and procedural generation. Generation must be deterministic from a seed and produce physically coherent star/body relationships within game-scale constraints. Star properties influence habitable zones, illumination, solar power, radiation, and environmental evolution. System generation must be moddable and reproducible in saves.

## Planets and moons
Each body can define:
- Radius, mass, gravity, rotation period, axial tilt, orbit and parent body.
- Atmosphere composition, pressure, density profile, temperature and weather/environment hazards.
- Surface terrain, biomes, resources, water/ice, radiation and magnetic environment.
- Day/night illumination, solar flux, seasons where modeled, and local navigation/landing conditions.
- Habitability classification with explicit reasons and confidence/unknown data.
Planets and moons are independent settlement candidates; a gas giant's moons are evaluated separately from the gas giant.

## Discovery and scientific uncertainty
The player does not automatically know all body properties. Telescopes, probes, orbital surveys, landers, samples, and crewed expeditions reveal information progressively. Show confidence and unknown fields. A discovered planet is not automatically known to be habitable or fully mapped. Prospecting reveals resource composition and deposits according to instrument capability, orbit, location, and survey time.

## Asteroids and small bodies
Support ice-rich, silicate/rocky, and metallic asteroid classes, plus modded types. Their orbits, spin, shape/size, composition, resource deposits, and accessibility determine mining and anchoring operations. Mining consumes finite deposits unless a mod explicitly defines renewable behavior. Small-body operations require suitable anchors, drills, power, storage, and transport.

## Interplanetary and interstellar travel
Interplanetary travel uses orbital mechanics, transfer windows, propulsion, delta-v, and time. Interstellar travel is a distinct system: distances, travel time, propulsion technology, navigation, communication delay, fuel/energy, life support, and logistics must be modeled. Do not treat interstellar travel as an ordinary local orbital transfer. Exact propulsion technology and travel scale remain configurable progression/balance choices.

## Colonization and habitability
Habitability is relative to the colony's biology, technology, and installed infrastructure. Automatic colonization follows D-013: explicit player permission after roughly 5–6 explored star systems; only genuinely habitable candidate worlds are ordinary automatic targets. The Sun, Mercury, Venus's surface, and gas-giant interiors are excluded as ordinary planetary colony targets. Specialized floating habitats, orbital habitats, and suitable moons are distinct engineered settlement options.

## Markets and resource geography
Resource abundance and accessibility vary by body and deposit. Trade requires transport and time. Market prices can react to supply/demand, scarcity, distance, and risk. Mining, refining, storage, and cargo operations use the same resource accounting as colonies.

## Modding
Mods can add stars, body classes, planets, moons, asteroids, biomes, resource deposits, survey instruments, propulsion technologies, and generation rules.
