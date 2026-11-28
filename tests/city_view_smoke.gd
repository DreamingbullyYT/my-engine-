extends SceneTree

## Smoke test for city placement, the side-on city view, pixel citizens and the
## tap-to-build interface. Runs headless in CI. It drives the real main scene.

const PlacementScript = preload("res://colony/placement.gd")
const ColonySimulationScript = preload("res://colony/colony_simulation.gd")
const WalkersScript = preload("res://rendering/city/citizen_walkers.gd")
const SceneryScript = preload("res://rendering/city/city_scenery.gd")

var failures: PackedStringArray = []


func _initialize() -> void:
	_test_placement_geometry()
	_test_colony_placement()
	_test_walkers()
	await _test_scene()

	if failures.is_empty():
		print("CITY_VIEW_SMOKE_OK: placement footprints, tap-to-build, side-on view, pixel citizens, day/night")
		quit(0)
	else:
		push_error("CITY_VIEW_SMOKE_FAILED: %d check(s) failed." % failures.size())
		quit(1)


func _test_placement_geometry() -> void:
	var intervals: Array = [Vector2(-4.5, -1.5), Vector2(-1.5, 1.5)]
	_check(PlacementScript.is_free(3.0, 3.0, intervals), "A free spot beside buildings must be placeable.")
	_check(not PlacementScript.is_free(0.0, 3.0, intervals), "A spot overlapping a building must be rejected.")
	_check(PlacementScript.is_free(3.0, 3.0, [Vector2(1.5, 1.5)]), "Touching edges must not count as overlap.")
	_check(not PlacementScript.is_free(PlacementScript.MAX_TILE_OFFSET, 3.0, []), "Placing past the end of the strip must fail.")
	var snapped := PlacementScript.snap_center_tile(2.6, 3.0, intervals)
	_check(is_equal_approx(snapped, 3.0), "A ghost near a neighbour must snap flush against it.")
	var far := PlacementScript.snap_center_tile(20.2, 3.0, intervals)
	_check(is_equal_approx(far - 1.5, roundf(far - 1.5)), "A ghost away from neighbours must snap to the whole-tile grid.")
	var round_trip := PlacementScript.angle_to_tile(PlacementScript.tile_to_angle(7.25))
	_check(is_equal_approx(round_trip, 7.25), "Tile and angle conversion must round-trip.")
	var full: Array = [Vector2(-PlacementScript.MAX_TILE_OFFSET, PlacementScript.MAX_TILE_OFFSET)]
	_check(is_nan(PlacementScript.nearest_free_center(0.0, 3.0, full)), "A full strip must report no free centre.")
	_check(not is_nan(PlacementScript.nearest_free_center(0.0, 3.0, intervals)), "A strip with room must report a free centre.")


func _test_colony_placement() -> void:
	var colony = ColonySimulationScript.new()
	colony.start_new_game()
	colony.resources["metal"] = 500
	var count_before: int = colony.buildings.size()
	var occupied_angle: float = float(colony.buildings[1].get("angle", 0.0))
	_check(not colony.can_place("mine", occupied_angle), "Colony must reject placement on top of a building.")
	_check(not colony.build_at("mine", occupied_angle), "build_at must refuse an overlapping spot.")
	_check(colony.buildings.size() == count_before, "A refused placement must not add a building.")
	_check(int(colony.resources["metal"]) == 500, "A refused placement must not spend metal.")

	var free_angle := PlacementScript.tile_to_angle(12.0)
	_check(colony.can_place("mine", free_angle), "Colony must accept a free spot.")
	_check(colony.build_at("mine", free_angle), "build_at must place on a free spot.")
	_check(int(colony.resources["metal"]) == 500 - 20, "Placing must spend the building cost once.")
	_check(not colony.build_at("mine", free_angle), "The same spot must not accept a second building.")

	for _index in range(6):
		colony.build("solar")
	var intervals: Array = colony.get_building_intervals()
	var overlaps := 0
	for first in range(intervals.size()):
		for second in range(first + 1, intervals.size()):
			if intervals[first].x < intervals[second].y - 0.001 and intervals[first].y > intervals[second].x + 0.001:
				overlaps += 1
	_check(overlaps == 0, "Automatic placement must never overlap buildings.")
	var ids: Dictionary = {}
	for building in colony.buildings:
		ids[String(building["id"])] = true
	_check(ids.size() == colony.buildings.size(), "Every building must keep a unique id.")

	colony.resources["metal"] = 0
	var before: int = colony.buildings.size()
	_check(not colony.build_at("habitat", PlacementScript.tile_to_angle(-20.0)), "Placement must fail without enough metal.")
	_check(colony.buildings.size() == before, "An unaffordable placement must not add a building.")
	colony.free()


func _test_walkers() -> void:
	var walkers = WalkersScript.new()
	var buildings: Array = [
		{"id": "habitat-1", "type": "habitat", "angle": PlacementScript.tile_to_angle(-3.0)},
		{"id": "farm-1", "type": "farm", "angle": PlacementScript.tile_to_angle(3.0)}
	]
	var citizens: Array = []
	for index in range(6):
		citizens.append({
			"id": "c%d" % index, "home_id": "habitat-1",
			"job_building_id": "farm-1" if index < 3 else ""
		})
	var widths := {"habitat": 3, "farm": 3}
	walkers.sync_citizens(citizens, buildings, widths, 600.0)
	_check(walkers.walker_count() == 6, "Every citizen record must get a walker.")

	for _step in range(400):
		walkers.update(0.25, 1.0)
	_check(walkers.visible_count() == 0, "At night every housed citizen must be indoors.")

	for _step in range(80):
		walkers.update(0.25, 0.0)
	_check(walkers.visible_count() > 0, "By day citizens must come out and move about.")

	var target_id := ""
	for id in walkers._walkers:
		if not bool(walkers._walkers[id]["inside"]):
			target_id = String(id)
			break
	if target_id.is_empty():
		_check(false, "A visible citizen must exist to test tapping.")
	else:
		var x: float = walkers.position_of(target_id)
		_check(walkers.citizen_at(x, 5.0) != "", "Tapping a citizen's figure must find a citizen.")
		_check(walkers.citizen_at(x, 90.0) == "", "Tapping far above the ground must not find a citizen.")

	var removed: Array = citizens.slice(0, 2)
	walkers.sync_citizens(removed, buildings, widths, 600.0)
	_check(walkers.walker_count() == 2, "Walkers must disappear with their citizen records.")

	_check(SceneryScript.night_amount(0.0) > 0.9, "Midnight must be dark.")
	_check(SceneryScript.night_amount(0.5) < 0.1, "Noon must be bright.")


func _test_scene() -> void:
	var scene: Control = load("res://app/main.tscn").instantiate()
	root.add_child(scene)
	for _frame in range(6):
		await process_frame
	var view = scene._world_view
	_check(view.get_frames_drawn() > 0, "The city view must complete at least one draw pass.")
	_check(not view.is_full_planet_visible(), "The city phase must keep the planet hidden.")

	view.set_time_of_day(0.05)
	var drawn_before: int = view.get_frames_drawn()
	for _frame in range(8):
		await process_frame
	_check(view.get_frames_drawn() > drawn_before, "The view must keep drawing at night.")
	view.set_time_of_day(0.5)

	var colony = root.get_node("ColonySimulation")
	colony.start_new_game()
	colony.resources["metal"] = 100
	await process_frame
	var buildings_before: int = colony.buildings.size()

	scene._on_card_pressed("solar")
	_check(view.is_placing(), "Pressing a card must start placement.")
	_check(view.is_ghost_valid(), "The first ghost must start on a free spot.")
	_check(scene._placement_row.visible and not scene._menu_row.visible, "Placement must swap the bottom bar.")

	var anchor: Vector2 = view._screen_anchor(0.0)
	var zoom: float = view.get_camera_zoom()
	var focus: Vector2 = view._camera_focus
	var free_world_x := 14.0 * 12.0
	var free_screen := anchor + (Vector2(free_world_x, -600.0 - 3.0) - focus) * zoom
	view._handle_tap(free_screen)
	_check(view.is_ghost_valid(), "Tapping free ground must keep the ghost valid.")
	var ghost_angle: float = view.get_ghost_angle()
	_check(absf(PlacementScript.angle_to_tile(ghost_angle) - 14.0) < 2.5, "The ghost must follow the tapped ground.")

	var blocked_screen := anchor + (Vector2(0.0, -600.0 - 3.0) - focus) * zoom
	view._handle_tap(blocked_screen)
	_check(not view.is_ghost_valid(), "Tapping a building must show an invalid ghost.")
	_check(scene._confirm_button.disabled, "Build must be disabled on a blocked spot.")
	scene._confirm_placement()
	_check(colony.buildings.size() == buildings_before, "Confirming a blocked spot must not build.")

	view._handle_tap(free_screen)
	var metal_before: int = int(colony.resources["metal"])
	scene._confirm_placement()
	_check(colony.buildings.size() == buildings_before + 1, "Confirming a free spot must build.")
	_check(int(colony.resources["metal"]) < metal_before, "Building from the screen must spend metal.")
	_check(not view.is_placing(), "Placement must end after building.")
	_check(scene._menu_row.visible and not scene._placement_row.visible, "The build menu must return after building.")

	scene._on_card_pressed("mine")
	_check(view.is_placing(), "A second card press must start a new placement.")
	scene._on_card_pressed("mine")
	_check(not view.is_placing(), "Pressing the active card again must cancel.")

	colony.resources["metal"] = 0
	await process_frame
	scene._on_card_pressed("mine")
	_check(not view.is_placing(), "An unaffordable card must not start placement.")
	colony.resources["metal"] = 100

	var tall_building_screen := anchor + (Vector2(0.0, -600.0 - 30.0) - focus) * zoom
	view._handle_tap(tall_building_screen)
	_check(scene._info_panel.visible, "Tapping a building must open the info panel.")
	_check(scene._info_title.text == "Solar Array", "The info panel must name the tapped building.")
	view._handle_tap(anchor + (Vector2(1000.0, -600.0 - 60.0) - focus) * zoom)
	_check(not scene._info_panel.visible, "Tapping empty sky must close the info panel.")

	scene.queue_free()
	await process_frame


func _check(condition: bool, message: String) -> void:
	if condition:
		print("[PASS] %s" % message)
	else:
		failures.append(message)
		push_error("[FAIL] %s" % message)
