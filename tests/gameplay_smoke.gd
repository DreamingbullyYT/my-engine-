extends SceneTree

const ColonySimulationScript = preload("res://colony/colony_simulation.gd")
const RocketFlightScript = preload("res://vehicles/rocket_flight.gd")

var failures: PackedStringArray = []


func _initialize() -> void:
	# Keep these domain tests outside the live SceneTree so gameplay timers do not
	# hold the headless process open if a check fails.
	var colony = ColonySimulationScript.new()
	colony.start_new_game()
	_check(colony.citizens.size() == 20, "New colony must create 20 citizens.")
	_check(colony.buildings.size() == 3, "Starting colony must contain its three starter buildings.")
	_check(colony.build("mine"), "Mine construction should succeed with starting resources.")

	var metal_before: int = int(colony.resources.get("metal", 0))
	colony.advance_day()
	_check(int(colony.resources.get("metal", 0)) > metal_before, "A staffed mine must produce metal.")
	_check(int(colony.resources.get("food", 0)) > 0, "The farm must keep the colony supplied.")
	_check(colony.get_housing_capacity() >= 20, "Starting housing must accommodate the starting population.")

	var rocket = RocketFlightScript.new()
	_check(rocket.construct(), "Rocket should be constructible.")
	_check(rocket.launch(), "Rocket should launch from the surface.")
	for _step in range(600):
		rocket.advance_simulation(0.05)
	_check(rocket.is_in_orbit(), "Guidance should insert the rocket into orbit.")
	_check(rocket.get_orbital_period_seconds() > 0.0, "Orbit must have a positive period.")
	_check(rocket.begin_return(), "An orbiting rocket should be able to start deorbit.")
	for _step in range(2000):
		if rocket.is_on_surface():
			break
		rocket.advance_simulation(0.05)
	_check(rocket.is_on_surface(), "The rocket should return to the planetary surface.")

	# Exercise serialization shape and domain restore without writing device data.
	var colony_copy = ColonySimulationScript.new()
	_check(colony_copy.restore_snapshot(colony.get_snapshot()), "Colony snapshot should restore.")
	_check(colony_copy.citizens.size() == 20, "Restored colony should preserve its citizens.")
	var rocket_copy = RocketFlightScript.new()
	_check(rocket_copy.restore_snapshot(rocket.get_snapshot()), "Rocket snapshot should restore.")
	_check(rocket_copy.is_on_surface(), "Restored rocket should remain landed.")

	colony.free()
	colony_copy.free()
	rocket.free()
	rocket_copy.free()

	if failures.is_empty():
		print("GAMEPLAY_SMOKE_OK: colony, production, save-data, launch, orbit, return")
		quit(0)
	else:
		push_error("GAMEPLAY_SMOKE_FAILED: %d check(s) failed." % failures.size())
		quit(1)


func _check(condition: bool, message: String) -> void:
	if condition:
		print("[PASS] %s" % message)
	else:
		failures.append(message)
		push_error("[FAIL] %s" % message)
