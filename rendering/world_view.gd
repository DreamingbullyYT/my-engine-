extends Control

## Side-on city view with a day/night sky, pixel buildings and walking citizens.
## Owns camera, touch input and drawing. It only reads colony state; every
## gameplay change (placing a building) is requested through signals.

signal camera_scale_changed(scale_name: String)
signal camera_transition_finished
signal building_tapped(building_id: String)
signal citizen_tapped(citizen_id: String)
signal empty_tapped
signal placement_changed(valid: bool)
signal placement_confirm_requested

const HomeWorldDefinition = preload("res://world/home_world_definition.gd")
const PlacementScript = preload("res://colony/placement.gd")
const PixelPainterScript = preload("res://rendering/city/pixel_painter.gd")
const BuildingArtScript = preload("res://rendering/city/building_art.gd")
const CitySceneryScript = preload("res://rendering/city/city_scenery.gd")
const CitizenWalkersScript = preload("res://rendering/city/citizen_walkers.gd")

const PLANET_RADIUS: float = HomeWorldDefinition.RADIUS
const CITY_CENTER_ANGLE := -PI * 0.5
const TILE := 12.0
const CITY_START_ZOOM := 3.0
const PLANET_OVERVIEW_ZOOM := 0.38
const LOCKED_MIN_ZOOM := 1.2
const UNLOCKED_MIN_ZOOM := 0.32
const MAX_ZOOM := 4.5
const PLANET_REVEAL_THRESHOLD := 0.45
const CAMERA_SMOOTHING := 7.0
const REVEAL_DURATION := 3.2
const GROUND_SCREEN_FRACTION := 0.66
const MAX_PAN_X := (PlacementScript.MAX_TILE_OFFSET + 1.0) * TILE
const DAY_PERIOD_SECONDS := 150.0
const START_CLOCK := 0.3
const ANIMATION_STEP := 1.0 / 30.0
const TAP_SLOP := 14.0
const TAP_MAX_MS := 700
const NIGHT_TINT := Color(0.02, 0.04, 0.17, 1.0)
const GHOST_OK := Color(0.55, 1.0, 0.6, 1.0)
const GHOST_BAD := Color(1.0, 0.45, 0.4, 1.0)
const SELECT_COLOR := Color(1.0, 0.85, 0.3, 1.0)

var _buildings: Array = []
var _citizens: Array = []
var _building_catalog: Dictionary = {}
var _rocket_exists: bool = false
var _rocket_position: Vector2 = Vector2(0.0, -PLANET_RADIUS)

var _launch_capability_unlocked: bool = false
var _camera_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _target_camera_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _camera_zoom: float = CITY_START_ZOOM
var _target_camera_zoom: float = CITY_START_ZOOM
var _transition_active: bool = false
var _transition_elapsed: float = 0.0
var _transition_start_zoom: float = CITY_START_ZOOM
var _transition_start_focus: Vector2 = Vector2(0.0, -PLANET_RADIUS)
var _touch_points: Dictionary = {}
var _last_pinch_distance: float = 0.0
var _mouse_pan_active: bool = false
var _last_scale_name: String = ""
var _last_rocket_in_flight: bool = false
var _follow_vehicle: bool = false

var _time: float = 0.0
var _clock: float = START_CLOCK
var _redraw_accumulator: float = 0.0
var _painter = PixelPainterScript.new()
var _walkers = CitizenWalkersScript.new()

var _press_active: bool = false
var _press_moved: bool = false
var _press_position: Vector2 = Vector2.ZERO
var _press_started_ms: int = 0

var _selected_building_id: String = ""
var _selected_citizen_id: String = ""
var _placement_type: String = ""
var _placement_width: float = 3.0
var _ghost_tile: float = NAN
var _frames_drawn: int = 0


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_STOP
	clip_contents = true
	set_process(true)
	queue_redraw()


# --- Data from the owning modules (read-only copies) -------------------------

func set_world(
	buildings: Array,
	citizens: Array,
	rocket_exists: bool,
	rocket_in_flight: bool,
	rocket_on_surface: bool,
	rocket_position: Vector2
) -> void:
	_buildings = buildings.duplicate(true)
	_citizens = citizens.duplicate(true)
	_rocket_exists = rocket_exists
	_rocket_position = rocket_position
	_walkers.sync_citizens(_citizens, _buildings, _widths(), PLANET_RADIUS)

	if rocket_in_flight and not _last_rocket_in_flight:
		_follow_vehicle = true
		_target_camera_zoom = minf(_target_camera_zoom, PLANET_OVERVIEW_ZOOM)
	if rocket_in_flight and _follow_vehicle:
		_transition_active = false
		_target_camera_focus = rocket_position
	elif _last_rocket_in_flight and rocket_on_surface:
		focus_city()

	_last_rocket_in_flight = rocket_in_flight
	if not _placement_type.is_empty() and not is_nan(_ghost_tile):
		placement_changed.emit(is_ghost_valid())
	queue_redraw()


## Per-type presentation facts (width_tiles, worker_slots). Read-only.
func set_building_catalog(catalog: Dictionary) -> void:
	_building_catalog = catalog
	_walkers.sync_citizens(_citizens, _buildings, _widths(), PLANET_RADIUS)
	queue_redraw()


func set_launch_capability_unlocked(unlocked: bool, animate_reveal: bool = false) -> void:
	if not unlocked:
		_launch_capability_unlocked = false
		_transition_active = false
		_camera_focus = Vector2(0.0, -PLANET_RADIUS)
		_target_camera_focus = _camera_focus
		_camera_zoom = CITY_START_ZOOM
		_target_camera_zoom = CITY_START_ZOOM
		queue_redraw()
		return

	if _launch_capability_unlocked:
		return

	_launch_capability_unlocked = true
	if animate_reveal:
		_start_home_planet_reveal()
	else:
		_camera_focus = Vector2.ZERO
		_target_camera_focus = Vector2.ZERO
		_camera_zoom = PLANET_OVERVIEW_ZOOM
		_target_camera_zoom = PLANET_OVERVIEW_ZOOM
	queue_redraw()


func reveal_home_planet() -> void:
	set_launch_capability_unlocked(true, true)


func is_planetary_zoom_unlocked() -> bool:
	return _launch_capability_unlocked


func is_full_planet_visible() -> bool:
	return _launch_capability_unlocked and _camera_zoom <= PLANET_REVEAL_THRESHOLD


func get_camera_zoom() -> float:
	return _camera_zoom


func get_target_camera_zoom() -> float:
	return _target_camera_zoom


func get_target_camera_focus() -> Vector2:
	return _target_camera_focus


func is_camera_transition_active() -> bool:
	return _transition_active


func focus_city() -> void:
	_follow_vehicle = false
	_transition_active = false
	_target_camera_focus = Vector2(0.0, -PLANET_RADIUS)
	_target_camera_zoom = CITY_START_ZOOM
	_clamp_camera_focus()
	queue_redraw()


func get_camera_scale_name() -> String:
	if not _launch_capability_unlocked or _camera_zoom > 1.55:
		return "CITY"
	if _camera_zoom > PLANET_REVEAL_THRESHOLD:
		return "LOCAL SURFACE"
	return "HOME PLANET"


## Visual clock only: 0 = midnight, 0.5 = noon. It does not touch simulation time.
func set_time_of_day(clock: float) -> void:
	_clock = fposmod(clock, 1.0)
	queue_redraw()


func get_time_of_day() -> float:
	return _clock


## Number of completed draw passes. Smoke tests use it to prove drawing code ran.
func get_frames_drawn() -> int:
	return _frames_drawn


# --- Selection and placement --------------------------------------------------

func select_building(building_id: String) -> void:
	_selected_building_id = building_id
	_selected_citizen_id = ""
	queue_redraw()


func select_citizen(citizen_id: String) -> void:
	_selected_citizen_id = citizen_id
	_selected_building_id = ""
	queue_redraw()


func clear_selection() -> void:
	_selected_building_id = ""
	_selected_citizen_id = ""
	queue_redraw()


func begin_placement(building_type: String) -> void:
	clear_selection()
	_placement_type = building_type
	_placement_width = float(_widths().get(building_type, 3))
	var target_tile := _camera_focus.x / TILE
	_ghost_tile = PlacementScript.nearest_free_center(target_tile, _placement_width, _intervals())
	placement_changed.emit(is_ghost_valid())
	queue_redraw()


func end_placement() -> void:
	_placement_type = ""
	_ghost_tile = NAN
	queue_redraw()


func is_placing() -> bool:
	return not _placement_type.is_empty()


func is_ghost_valid() -> bool:
	if _placement_type.is_empty() or is_nan(_ghost_tile):
		return false
	return PlacementScript.is_free(_ghost_tile, _placement_width, _intervals())


func get_ghost_angle() -> float:
	if is_nan(_ghost_tile):
		return NAN
	return PlacementScript.tile_to_angle(_ghost_tile)


# --- Per-frame update ---------------------------------------------------------

func _process(delta: float) -> void:
	var previous_zoom := _camera_zoom
	var previous_focus := _camera_focus

	if _transition_active:
		_transition_elapsed += delta
		var amount := clampf(_transition_elapsed / REVEAL_DURATION, 0.0, 1.0)
		var eased := amount * amount * (3.0 - 2.0 * amount)
		_camera_zoom = lerpf(_transition_start_zoom, _target_camera_zoom, eased)
		_camera_focus = _transition_start_focus.lerp(_target_camera_focus, eased)
		if amount >= 1.0:
			_transition_active = false
			camera_transition_finished.emit()
	else:
		var blend := 1.0 - exp(-CAMERA_SMOOTHING * delta)
		_camera_zoom = lerpf(_camera_zoom, _target_camera_zoom, blend)
		_camera_focus = _camera_focus.lerp(_target_camera_focus, blend)

	_time += delta
	_clock = fposmod(_clock + delta / DAY_PERIOD_SECONDS, 1.0)
	_walkers.update(delta, CitySceneryScript.night_amount(_clock))

	_redraw_accumulator += delta
	var camera_moved := not is_equal_approx(previous_zoom, _camera_zoom) or previous_focus.distance_to(_camera_focus) > 0.01
	if camera_moved or _redraw_accumulator >= ANIMATION_STEP:
		_redraw_accumulator = 0.0
		queue_redraw()

	var current_scale_name := get_camera_scale_name()
	if current_scale_name != _last_scale_name:
		_last_scale_name = current_scale_name
		camera_scale_changed.emit(current_scale_name)


func _start_home_planet_reveal() -> void:
	if _transition_active:
		return
	_transition_start_zoom = _camera_zoom
	_transition_start_focus = _camera_focus
	_target_camera_zoom = PLANET_OVERVIEW_ZOOM
	_target_camera_focus = Vector2.ZERO
	_transition_elapsed = 0.0
	_transition_active = true


# --- Input --------------------------------------------------------------------

func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		_handle_touch(event)
		accept_event()
		return

	if event is InputEventScreenDrag:
		_handle_drag(event)
		accept_event()
		return

	# Android also delivers an emulated mouse copy of every touch. The touch
	# events above already handled it, so ignore the copy to avoid double pans.
	if event is InputEventMouse and event.device == InputEvent.DEVICE_ID_EMULATION:
		return

	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			_mouse_pan_active = event.pressed
			if event.pressed:
				_begin_press(event.position)
			elif _press_is_tap(event.position):
				_handle_tap(event.position)
			_press_active = false
			accept_event()
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_apply_zoom(1.12)
			accept_event()
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_apply_zoom(0.89)
			accept_event()
		return

	if event is InputEventMouseMotion and _mouse_pan_active:
		if _press_active and _press_position.distance_to(event.position) > TAP_SLOP:
			_press_moved = true
		_pan_by(event.relative)
		accept_event()


func _handle_touch(event: InputEventScreenTouch) -> void:
	if event.pressed:
		_touch_points[event.index] = event.position
		if _touch_points.size() == 1:
			_begin_press(event.position)
		else:
			_press_active = false
		if _touch_points.size() == 2:
			var points: Array = _touch_points.values()
			_last_pinch_distance = points[0].distance_to(points[1])
		return
	var was_single := _touch_points.size() == 1
	_touch_points.erase(event.index)
	_last_pinch_distance = 0.0
	if was_single and _press_is_tap(event.position):
		_handle_tap(event.position)
	_press_active = false


func _handle_drag(event: InputEventScreenDrag) -> void:
	if not _touch_points.has(event.index):
		return
	if _touch_points.size() >= 2:
		_press_active = false
		_follow_vehicle = false
		var points: Array = _touch_points.keys()
		var other_id: int = int(points[0]) if int(points[1]) == event.index else int(points[1])
		var other_position: Vector2 = _touch_points[other_id]
		var new_distance: float = event.position.distance_to(other_position)
		if _last_pinch_distance > 0.0 and new_distance > 0.0:
			_apply_zoom(new_distance / _last_pinch_distance)
		_last_pinch_distance = new_distance
		_touch_points[event.index] = event.position
		return
	_touch_points[event.index] = event.position
	if _press_active and _press_position.distance_to(event.position) > TAP_SLOP:
		_press_moved = true
	_pan_by(event.relative)


func _pan_by(relative: Vector2) -> void:
	_follow_vehicle = false
	_transition_active = false
	_target_camera_focus -= relative / maxf(_camera_zoom, 0.01)
	_clamp_camera_focus()


func _begin_press(position_value: Vector2) -> void:
	_press_active = true
	_press_moved = false
	_press_position = position_value
	_press_started_ms = Time.get_ticks_msec()


func _press_is_tap(release_position: Vector2) -> bool:
	if not _press_active or _press_moved:
		return false
	if _press_position.distance_to(release_position) > TAP_SLOP:
		return false
	return Time.get_ticks_msec() - _press_started_ms <= TAP_MAX_MS


func _handle_tap(screen_position: Vector2) -> void:
	if _launch_capability_unlocked and _planet_mix() > 0.05:
		return
	var world := _screen_to_world(screen_position)
	var city_x := world.x
	var height := -PLANET_RADIUS - world.y

	if is_placing():
		var tapped_tile := city_x / TILE
		var ghost_left := _ghost_tile - _placement_width * 0.5
		var on_ghost := not is_nan(_ghost_tile) and tapped_tile >= ghost_left and tapped_tile <= ghost_left + _placement_width
		if on_ghost and is_ghost_valid():
			placement_confirm_requested.emit()
			return
		_ghost_tile = PlacementScript.snap_center_tile(tapped_tile, _placement_width, _intervals())
		placement_changed.emit(is_ghost_valid())
		queue_redraw()
		return

	var citizen_id: String = _walkers.citizen_at(city_x, height)
	if not citizen_id.is_empty():
		select_citizen(citizen_id)
		citizen_tapped.emit(citizen_id)
		return

	var building_id := _building_at(city_x, height)
	if not building_id.is_empty():
		select_building(building_id)
		building_tapped.emit(building_id)
		return

	clear_selection()
	empty_tapped.emit()


func _building_at(city_x: float, height: float) -> String:
	var widths := _widths()
	for index in range(_buildings.size() - 1, -1, -1):
		var building: Dictionary = _buildings[index]
		var building_type := String(building.get("type", ""))
		var interval: Vector2 = PlacementScript.interval_for(
			float(building.get("angle", CITY_CENTER_ANGLE)), float(widths.get(building_type, 3)))
		if city_x >= interval.x * TILE and city_x <= interval.y * TILE:
			if height >= -4.0 and height <= BuildingArtScript.height_of(building_type) + 10.0:
				return String(building.get("id", ""))
	return ""


func _apply_zoom(factor: float) -> void:
	_transition_active = false
	var minimum_zoom := UNLOCKED_MIN_ZOOM if _launch_capability_unlocked else LOCKED_MIN_ZOOM
	_target_camera_zoom = clampf(_target_camera_zoom * factor, minimum_zoom, MAX_ZOOM)
	_clamp_camera_focus()
	queue_redraw()


func _clamp_camera_focus() -> void:
	if not _launch_capability_unlocked:
		_target_camera_focus = Vector2(
			clampf(_target_camera_focus.x, -MAX_PAN_X, MAX_PAN_X), -PLANET_RADIUS)
	else:
		var maximum_planet_pan := PLANET_RADIUS * 1.7
		if _target_camera_focus.length() > maximum_planet_pan:
			_target_camera_focus = _target_camera_focus.normalized() * maximum_planet_pan


# --- Geometry helpers ---------------------------------------------------------

func _planet_mix() -> float:
	if not _launch_capability_unlocked:
		return 0.0
	return clampf(inverse_lerp(1.35, PLANET_REVEAL_THRESHOLD, _camera_zoom), 0.0, 1.0)


func _screen_anchor(planet_mix: float) -> Vector2:
	return Vector2(size.x * 0.5, lerpf(size.y * GROUND_SCREEN_FRACTION, size.y * 0.5, planet_mix))


func _screen_to_world(screen_position: Vector2) -> Vector2:
	var anchor := _screen_anchor(_planet_mix())
	return _camera_focus + (screen_position - anchor) / maxf(_camera_zoom, 0.01)


func _widths() -> Dictionary:
	var widths: Dictionary = {}
	for building_type in _building_catalog:
		widths[building_type] = int(_building_catalog[building_type].get("width_tiles", 3))
	return widths


func _intervals() -> Array:
	var widths := _widths()
	var intervals: Array = []
	for building in _buildings:
		var width := float(widths.get(String(building.get("type", "")), 3))
		intervals.append(PlacementScript.interval_for(float(building.get("angle", CITY_CENTER_ANGLE)), width))
	return intervals


# --- Drawing ------------------------------------------------------------------

func _draw() -> void:
	var planet_mix := _planet_mix()
	var flat_mix := 1.0 - planet_mix
	var night := CitySceneryScript.night_amount(_clock) * flat_mix
	var anchor := _screen_anchor(planet_mix)
	var transform_position := anchor - _camera_focus * _camera_zoom
	var horizon_y := transform_position.y - PLANET_RADIUS * _camera_zoom

	_draw_backdrop(planet_mix, horizon_y)

	draw_set_transform(transform_position, 0.0, Vector2.ONE * _camera_zoom)
	var left_x := _camera_focus.x - anchor.x / _camera_zoom - 24.0
	var right_x := _camera_focus.x + (size.x - anchor.x) / _camera_zoom + 24.0
	left_x = maxf(left_x, -1000.0)
	right_x = minf(right_x, 1000.0)

	_painter.begin_world(self, PLANET_RADIUS, flat_mix)
	_painter.night = night
	_painter.time = _time
	var intervals := _intervals()

	_draw_home_planet(planet_mix)
	CitySceneryScript.draw_ground(_painter, left_x, right_x, intervals)
	_draw_buildings(left_x, right_x)
	_draw_ghost()
	if flat_mix > 0.5:
		_walkers.draw(_painter, left_x, right_x, _selected_citizen_id)
	_draw_rocket_marker()

	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	if night > 0.01:
		draw_rect(Rect2(Vector2.ZERO, size), Color(NIGHT_TINT.r, NIGHT_TINT.g, NIGHT_TINT.b, 0.46 * night))
	draw_set_transform(transform_position, 0.0, Vector2.ONE * _camera_zoom)
	_painter.flush_lights()
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	_frames_drawn += 1


func _draw_backdrop(planet_mix: float, horizon_y: float) -> void:
	var camera_shift := -_camera_focus.x * _camera_zoom
	if planet_mix < 0.999:
		CitySceneryScript.draw_sky(self, size, clampf(horizon_y, size.y * 0.2, size.y), _clock, _time, camera_shift)
	if planet_mix > 0.001:
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.015, 0.025, 0.05, planet_mix))
		for index in range(38):
			var angle := float(index) * 2.399
			var distance := 35.0 + float((index * 47) % 190)
			var point := size * 0.5 + Vector2.RIGHT.rotated(angle) * distance
			draw_rect(Rect2(point, Vector2(2.0, 2.0)), Color(0.84, 0.9, 0.96, 0.82 * planet_mix))


func _draw_home_planet(opacity: float) -> void:
	if opacity <= 0.001:
		return
	draw_circle(Vector2.ZERO, PLANET_RADIUS, Color(0.18, 0.31, 0.25, opacity))
	draw_arc(Vector2.ZERO, PLANET_RADIUS, 0.0, TAU, 144, Color(0.73, 0.81, 0.7, opacity), 4.0, true)


func _draw_buildings(left_x: float, right_x: float) -> void:
	var aggregation := 0.0
	if _launch_capability_unlocked:
		aggregation = clampf(inverse_lerp(1.3, 0.78, _camera_zoom), 0.0, 1.0)
	var widths := _widths()
	for building in _buildings:
		var building_type := String(building.get("type", ""))
		var width_tiles := float(widths.get(building_type, 3))
		var center_x := (float(building.get("angle", CITY_CENTER_ANGLE)) - CITY_CENTER_ANGLE) * PLANET_RADIUS
		var left := center_x - width_tiles * TILE * 0.5
		if left + width_tiles * TILE < left_x or left > right_x:
			continue
		if aggregation > 0.999:
			continue
		var slots := int(_building_catalog.get(building_type, {}).get("worker_slots", 0))
		var building_id := String(building.get("id", ""))
		_painter.set_origin_x(left)
		BuildingArtScript.paint(_painter, building_type, {
			"workers": int(building.get("workers", 0)),
			"slots": slots,
			"time": _time,
			"seed": absi(building_id.hash()) % 1000,
			"width": width_tiles * TILE
		})
		if building_id == _selected_building_id:
			_painter.outline(-2.0, -2.0, width_tiles * TILE + 4.0,
				BuildingArtScript.height_of(building_type) + 6.0, SELECT_COLOR, 1.0)


func _draw_ghost() -> void:
	if _placement_type.is_empty() or is_nan(_ghost_tile):
		return
	var valid := is_ghost_valid()
	var color := GHOST_OK if valid else GHOST_BAD
	var width_units := _placement_width * TILE
	var pulse := 0.6 + 0.12 * sin(_time * 5.0)
	_painter.set_origin_x((_ghost_tile - _placement_width * 0.5) * TILE)
	_painter.alpha = pulse
	_painter.tint = color
	BuildingArtScript.paint(_painter, _placement_type, {
		"workers": 1, "slots": 0, "time": _time, "seed": 3, "width": width_units
	})
	_painter.alpha = 1.0
	_painter.tint = Color(1.0, 1.0, 1.0, 1.0)
	_painter.rect(0.0, -2.0, width_units, 3.0, Color(color.r, color.g, color.b, 0.9))
	_painter.outline(-1.0, -1.0, width_units + 2.0,
		BuildingArtScript.height_of(_placement_type) + 4.0, color, 1.0)


func _draw_rocket_marker() -> void:
	if not _rocket_exists:
		return
	var radial := _rocket_position.normalized()
	var tangent := Vector2(-radial.y, radial.x)
	var position := _rocket_position
	var half_width := 5.0
	var tip := position + radial * 15.0
	var tail_left := position - radial * 10.0 - tangent * half_width
	var tail_right := position - radial * 10.0 + tangent * half_width
	draw_colored_polygon(PackedVector2Array([tip, tail_right, tail_left]), Color(0.91, 0.9, 0.81, 1.0))
