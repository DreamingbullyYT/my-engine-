extends SceneTree

const ColonySimulationScript = preload("res://colony/colony_simulation.gd")
const RocketFlightScript = preload("res://vehicles/rocket_flight.gd")


func _initialize() -> void:
	var colony = ColonySimulationScript.new()
	root.add_child(colony)
	assert(colony.citizens.size() == 20, "New colony must create 20 citizens.")
	assert(colony.buildings.size() == 3, "Starting colony must contain its three starter buildings.")
	assert(colony.build("mine"), "Mine construction should succeed with starting resources.")
	var metal_before: int = int(colony.resources.get("metal", 0))
	colony.advance_day()
	assert(int(colony.resources.get("metal", 0)) > metal_before, "A staffed mine must produce metal.")
	assert(int(colony.resources.get("food", 0)) > 0, "The farm must keep the colony supplied.")

	var rocket = RocketFlightScript.new()
	root.add_child(rocket)
	assert(rocket.construct(), "Rocket should be constructible.")
	assert(rocket.launch(), "Rocket should launch from the surface.")
	for _step in range(600):
		rocket.advance_simulation(0.05)
	assert(rocket.is_in_orbit(), "Guidance should insert the rocket into orbit.")
	assert(rocket.get_orbital_period_seconds() > 0.0, "Orbit must have a positive period.")
	assert(rocket.begin_return(), "An orbiting rocket should be able to start deorbit.")
	for _step in range(2000):
		if rocket.is_on_surface():
			break
		rocket.advance_simulation(0.05)
	assert(rocket.is_on_surface(), "The rocket should return to the planetary surface.")

	print("GAMEPLAY_SMOKE_OK: colony, production, launch, orbit, return")
	quit(0)
