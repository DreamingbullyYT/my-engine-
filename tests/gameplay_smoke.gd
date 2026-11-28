extends SceneTree

const ColonySimulationScript = preload("res://colony/colony_simulation.gd")
const WorldViewScript = preload("res://rendering/world_view.gd")

var failures: PackedStringArray = []


func _initialize() -> void:
	# Keep domain Nodes outside the live SceneTree so the timer does not keep
	# headless CI alive after a failed assertion.
	var colony = ColonySimulationScript.new()
	colony.start_new_game()

	_check(colony.citizens.size() == 20, "A new settlement must create 20 persistent citizens.")
	_check(colony.buildings.size() == 3, "The starting settlement must contain its three starter buildings.")
	_check(colony.get_housing_capacity() >= 20, "Starting housing must accommodate the initial population.")
	_check(not colony.has_building("launchpad"), "The city phase must not start with a space launch facility.")
	_check(not colony.can_build("launchpad"), "Space construction must remain unavailable during the city phase.")

	var employed_before: int = colony.get_employed_count()
	_check(employed_before > 0, "Starter buildings must receive available workers.")

	_check(colony.build("mine"), "Mine construction should succeed with starting resources.")
	var metal_before: int = int(colony.resources.get("metal", 0))
	colony.advance_day()
	_check(int(colony.resources.get("metal", 0)) > metal_before, "A staffed mine must produce metal.")
	_check(int(colony.resources.get("food", 0)) > 0, "The farm must keep the colony supplied.")
	_check(colony.day == 2, "A single simulation step must advance one in-game day.")

	var world_view = WorldViewScript.new()
	_check(not world_view.is_planetary_zoom_unlocked(), "The city phase must keep planetary zoom locked.")
	_check(not world_view.is_full_planet_visible(), "The full planet must remain hidden during city gameplay.")
	_check(is_equal_approx(world_view.get_camera_zoom(), 2.8), "A new game must start at the close city camera scale.")
	world_view.set_world(colony.buildings, colony.citizens, false, false, true, Vector2.ZERO)
	_check(is_equal_approx(world_view.get_target_camera_zoom(), 2.8), "City updates must not trigger an automatic planetary reveal.")
	world_view.focus_city()
	_check(is_equal_approx(world_view.get_target_camera_zoom(), 2.8), "City recenter must preserve the close city scale.")

	var colony_copy = ColonySimulationScript.new()
	_check(colony_copy.restore_snapshot(colony.get_snapshot()), "The colony snapshot must restore successfully.")
	_check(colony_copy.citizens.size() == 20, "Restored settlement must preserve all citizen records.")
	_check(colony_copy.buildings.size() == colony.buildings.size(), "Restored settlement must preserve its buildings.")
	_check(int(colony_copy.resources.get("metal", 0)) == int(colony.resources.get("metal", 0)), "Restored settlement must preserve its resource stocks.")

	colony.free()
	colony_copy.free()
	world_view.free()

	if failures.is_empty():
		print("CITY_SMOKE_OK: city-only start, housing, jobs, construction, production, locked space seam, snapshot restore")
		quit(0)
	else:
		push_error("CITY_SMOKE_FAILED: %d check(s) failed." % failures.size())
		quit(1)


func _check(condition: bool, message: String) -> void:
	if condition:
		print("[PASS] %s" % message)
	else:
		failures.append(message)
		push_error("[FAIL] %s" % message)
