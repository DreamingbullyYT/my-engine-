extends SceneTree

const ColonySimulationScript = preload("res://colony/colony_simulation.gd")
const CitizenModelScript = preload("res://colony/citizens/citizen_model.gd")
const WorldViewScript = preload("res://rendering/world_view.gd")

var failures: PackedStringArray = []


func _initialize() -> void:
	# Keep domain Nodes outside the live SceneTree so the timer does not keep
	# headless CI alive after a failed assertion.
	var colony = ColonySimulationScript.new()
	colony.start_new_game()

	_check(colony.citizens.size() == 20, "A new settlement must create 20 persistent citizens.")
	_check(colony.buildings.size() == 4, "The starting settlement must include housing, power, food, and water infrastructure.")
	_check(colony.get_housing_capacity() >= 20, "Starting housing must accommodate the initial population.")
	_check(colony.get_housed_count() == 20, "Every starting citizen must have an assigned home.")
	_check(colony.has_building("water_pump"), "The city phase must start with an operational water pump.")
	_check(int(colony.resources.get("water", 0)) >= 20, "The starting settlement must have an initial water reserve.")
	_check(not colony.has_building("launchpad"), "The city phase must not start with a space launch facility.")
	_check(not colony.can_build("launchpad"), "Space construction must remain unavailable during the city phase.")

	var first_citizen: Dictionary = colony.citizens[0]
	_check(first_citizen.has("age_years"), "Citizens must preserve an age value.")
	_check(first_citizen.has("home_id") and not String(first_citizen["home_id"]).is_empty(), "Citizens must preserve a home assignment.")
	_check(first_citizen.has("skills") and first_citizen["skills"] is Dictionary, "Citizens must carry a skill profile.")
	_check(first_citizen.has("traits") and first_citizen["traits"] is Array, "Citizens must carry an extensible trait list.")
	_check(first_citizen.has("needs") and first_citizen["needs"] is Dictionary, "Citizens must carry individual needs.")
	_check(first_citizen.has("activity") and first_citizen.has("status"), "Citizens must expose activity and health status.")

	_check(colony.get_employed_count() > 0, "Starter buildings must receive available workers.")
	var employed_citizen: Dictionary = {}
	for citizen in colony.citizens:
		if String(citizen.get("job", "Unassigned")) != "Unassigned":
			employed_citizen = citizen
			break
	_check(not employed_citizen.is_empty() and not String(employed_citizen.get("job_building_id", "")).is_empty(), "An employed citizen must identify their workplace.")

	_check(colony.build("mine"), "Mine construction should succeed with starting resources.")
	var metal_before: int = int(colony.resources.get("metal", 0))
	var water_before: int = int(colony.resources.get("water", 0))
	colony.advance_day()
	_check(int(colony.resources.get("metal", 0)) > metal_before, "A staffed mine must produce metal.")
	_check(int(colony.resources.get("food", 0)) > 0, "The farm must keep the colony supplied.")
	_check(int(colony.resources.get("water", 0)) > water_before, "The staffed water pump must replace more water than the city consumes.")
	_check(colony.day == 2, "A single simulation step must advance one in-game day.")
	_check(float(first_citizen["needs"].get("food", 0.0)) > 90.0, "Adequate food supply must satisfy the food need.")
	_check(float(first_citizen["needs"].get("water", 0.0)) > 90.0, "Adequate water supply must satisfy the water need.")

	var citizen_model = CitizenModelScript.new()
	var breakdown: Dictionary = citizen_model.get_happiness_breakdown(first_citizen)
	_check(breakdown.has("score") and breakdown.has("contributions"), "Happiness must expose an explanatory contribution breakdown.")
	var inspection: Dictionary = colony.get_citizen_inspection(String(first_citizen.get("id", "")))
	_check(inspection.has("happiness_breakdown"), "The colony must expose inspectable citizen happiness details.")

	for building in colony.buildings:
		if String(building.get("type", "")) == "water_pump":
			building["workers"] = 0
	colony.resources["water"] = 0
	var thirst_before := float(first_citizen["needs"].get("water", 100.0))
	colony.advance_day()
	_check(float(first_citizen["needs"].get("water", 100.0)) < thirst_before, "Water shortage must reduce each citizen's water satisfaction.")
	for _day in range(5):
		colony.advance_day()
	_check(float(first_citizen.get("health", 100.0)) < 100.0, "Prolonged essential-need shortages must damage health.")

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
	_check(int(colony_copy.resources.get("water", 0)) == int(colony.resources.get("water", 0)), "Restored settlement must preserve water stock.")
	_check(colony_copy.citizens[0].has("needs"), "Restored citizens must preserve their individual needs.")

	var legacy_colony = ColonySimulationScript.new()
	var legacy_save := {
		"schema": 1,
		"day": 7,
		"resources": {"metal": 25, "food": 40, "energy": 20, "fuel": 15},
		"citizens": [{"id": "legacy-citizen", "name": "Legacy", "job": "Unassigned", "health": 100, "happiness": 65}],
		"buildings": [
			{"id": "habitat-1", "type": "habitat", "angle": -1.5, "workers": 0},
			{"id": "launchpad-old", "type": "launchpad", "angle": -1.4, "workers": 0}
		]
	}
	_check(legacy_colony.restore_snapshot(legacy_save), "Version-1 colony saves must migrate.")
	_check(not legacy_colony.has_building("launchpad"), "Legacy migration must not restore the dormant launch pad.")
	_check(not legacy_colony.resources.has("fuel"), "Legacy migration must discard dormant space fuel.")
	_check(int(legacy_colony.resources.get("water", 0)) > 0, "Legacy migration must seed a safe water reserve.")
	_check(legacy_colony.citizens[0].has("skills") and legacy_colony.citizens[0].has("needs"), "Legacy citizens must receive the new citizen data model.")

	colony.free()
	colony_copy.free()
	legacy_colony.free()
	world_view.free()
	citizen_model.free()

	if failures.is_empty():
		print("CITY_SMOKE_OK: citizen profiles, individual needs, housing, water service, jobs/skills, shortages, save migration, city-only camera")
		quit(0)
	else:
		push_error("CITY_SMOKE_FAILED: %d check(s) failed." % failures.size())


func _check(condition: bool, message: String) -> void:
	if condition:
		print("[PASS] %s" % message)
	else:
		failures.append(message)
		push_error("[FAIL] %s" % message)
