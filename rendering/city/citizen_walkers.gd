extends RefCounted

## Presentation-only pixel people. Each persistent citizen record gets a small
## walker that goes to its workplace by day, home by night, and wanders when
## unemployed. Walkers never change simulation data: identity, home and job come
## from the colony records, and a hidden walker is still fully simulated.

const CITY_CENTER_ANGLE := -PI * 0.5
const WALK_SPEED := 15.0
const NIGHT_THRESHOLD := 0.55
const ARRIVE_DISTANCE := 1.5
const HIT_HALF_WIDTH := 6.0
const HIT_HEIGHT := 16.0

const SHIRTS: Array = [
	Color(0.85, 0.3, 0.28), Color(0.3, 0.55, 0.85), Color(0.95, 0.75, 0.25), Color(0.35, 0.72, 0.4),
	Color(0.7, 0.4, 0.8), Color(0.92, 0.55, 0.25), Color(0.9, 0.9, 0.92), Color(0.25, 0.65, 0.68)
]
const HAIRS: Array = [
	Color(0.15, 0.1, 0.08), Color(0.4, 0.25, 0.12), Color(0.82, 0.62, 0.25), Color(0.6, 0.18, 0.12), Color(0.55, 0.55, 0.58)
]
const SKINS: Array = [
	Color(0.96, 0.8, 0.65), Color(0.85, 0.65, 0.48), Color(0.66, 0.46, 0.32), Color(0.45, 0.3, 0.21), Color(0.93, 0.74, 0.58)
]
const PANTS := Color(0.2, 0.24, 0.34)

var _walkers: Dictionary = {}
var _building_x: Dictionary = {}
var _bounds: Vector2 = Vector2(-120.0, 120.0)
var _rng: RandomNumberGenerator = RandomNumberGenerator.new()


func _init() -> void:
	_rng.seed = 7331


## Rebuilds the lookup tables from the latest read-only colony state.
func sync_citizens(citizens: Array, buildings: Array, widths: Dictionary, radius: float) -> void:
	_building_x.clear()
	var low := 0.0
	var high := 0.0
	for building in buildings:
		var width_units := float(widths.get(String(building.get("type", "")), 3)) * 12.0
		var center := (float(building.get("angle", CITY_CENTER_ANGLE)) - CITY_CENTER_ANGLE) * radius
		_building_x[String(building.get("id", ""))] = center
		low = minf(low, center - width_units * 0.5)
		high = maxf(high, center + width_units * 0.5)
	_bounds = Vector2(low - 36.0, high + 36.0)

	var seen: Dictionary = {}
	for citizen in citizens:
		var id := String(citizen.get("id", ""))
		if id.is_empty():
			continue
		seen[id] = true
		var walker: Dictionary
		if _walkers.has(id):
			walker = _walkers[id]
		else:
			walker = _spawn(id, String(citizen.get("home_id", "")))
			_walkers[id] = walker
		walker["home_id"] = String(citizen.get("home_id", ""))
		walker["work_id"] = String(citizen.get("job_building_id", ""))
	for id in _walkers.keys():
		if not seen.has(id):
			_walkers.erase(id)


func update(delta: float, night: float) -> void:
	var is_night := night > NIGHT_THRESHOLD
	for id in _walkers:
		_update_walker(_walkers[id], delta, is_night)


func walker_count() -> int:
	return _walkers.size()


func visible_count() -> int:
	var count := 0
	for id in _walkers:
		if not bool(_walkers[id]["inside"]):
			count += 1
	return count


## Citizen id whose figure is under a point in city space, or "".
func citizen_at(world_x: float, height: float) -> String:
	if height < -2.0 or height > HIT_HEIGHT:
		return ""
	var best := ""
	var best_distance := HIT_HALF_WIDTH
	for id in _walkers:
		var walker: Dictionary = _walkers[id]
		if bool(walker["inside"]):
			continue
		var distance := absf(float(walker["x"]) - world_x)
		if distance <= best_distance:
			best_distance = distance
			best = String(id)
	return best


func position_of(citizen_id: String) -> float:
	if _walkers.has(citizen_id):
		return float(_walkers[citizen_id]["x"])
	return NAN


func draw(p, left_x: float, right_x: float, selected_id: String) -> void:
	for id in _walkers:
		var walker: Dictionary = _walkers[id]
		if bool(walker["inside"]):
			continue
		var x := float(walker["x"])
		if x < left_x - 8.0 or x > right_x + 8.0:
			continue
		_draw_figure(p, walker, String(id) == selected_id)


func _draw_figure(p, walker: Dictionary, selected: bool) -> void:
	var x := float(walker["x"])
	p.set_origin_x(x - 2.0)
	var moving := bool(walker["moving"])
	var step := int(floor(float(walker["phase"]) * 7.0)) % 2 == 0
	var facing := float(walker["dir"])
	var shirt: Color = walker["shirt"]
	var hair: Color = walker["hair"]
	var skin: Color = walker["skin"]

	var back_leg := 3.0
	var front_leg := 3.0
	if moving:
		if step:
			back_leg = 2.0
		else:
			front_leg = 2.0
	p.rect(0.5, 0.0, 1.2, back_leg, PANTS)
	p.rect(2.3, 0.0, 1.2, front_leg, PANTS)
	p.rect(0.0, 3.0, 4.0, 4.0, shirt)
	p.rect(0.0, 3.0, 4.0, 1.0, shirt.darkened(0.2))
	p.rect(0.5, 7.0, 3.0, 2.6, skin)
	p.rect(0.5, 9.0, 3.0, 1.3, hair)
	if facing >= 0.0:
		p.rect(2.7, 8.0, 0.8, 0.8, Color(0.1, 0.1, 0.12))
	else:
		p.rect(0.5, 8.0, 0.8, 0.8, Color(0.1, 0.1, 0.12))
	if selected:
		var bob := floorf(sin(float(walker["phase"]) * 6.0) * 1.0)
		p.rect(0.0, 13.0 + bob, 4.0, 1.0, Color(1.0, 0.85, 0.3))
		p.rect(1.0, 12.0 + bob, 2.0, 1.0, Color(1.0, 0.85, 0.3))


func _spawn(id: String, home_id: String) -> Dictionary:
	var salt := absi(id.hash())
	var start_x := _rng.randf_range(_bounds.x, _bounds.y)
	if _building_x.has(home_id):
		start_x = clampf(float(_building_x[home_id]) + _rng.randf_range(-40.0, 40.0), _bounds.x, _bounds.y)
	return {
		"x": start_x,
		"dir": 1.0 if salt % 2 == 0 else -1.0,
		"inside": false,
		"moving": false,
		"mode": "wander",
		"stage": "go",
		"target": NAN,
		"timer": _rng.randf_range(0.5, 3.0),
		"phase": _rng.randf_range(0.0, 10.0),
		"speed": 0.85 + float(salt % 5) * 0.08,
		"home_id": home_id,
		"work_id": "",
		"shirt": SHIRTS[salt % SHIRTS.size()],
		"hair": HAIRS[(salt / 7) % HAIRS.size()],
		"skin": SKINS[(salt / 13) % SKINS.size()]
	}


func _update_walker(walker: Dictionary, delta: float, is_night: bool) -> void:
	walker["phase"] = float(walker["phase"]) + delta
	walker["moving"] = false
	var desired := _desired_mode(walker, is_night)
	if desired != String(walker["mode"]):
		walker["mode"] = desired
		walker["stage"] = "go"
		walker["target"] = NAN
		walker["inside"] = false
	match String(walker["mode"]):
		"home":
			_go_inside_at(walker, float(_building_x[String(walker["home_id"])]))
		"work":
			_work_cycle(walker, delta)
		_:
			_wander(walker, delta, float(walker["x"]), 80.0)


func _desired_mode(walker: Dictionary, is_night: bool) -> String:
	if is_night and _building_x.has(String(walker["home_id"])):
		return "home"
	if not is_night and _building_x.has(String(walker["work_id"])):
		return "work"
	return "wander"


func _go_inside_at(walker: Dictionary, target_x: float) -> void:
	if bool(walker["inside"]):
		return
	if _walk_toward(walker, target_x, 0.0):
		walker["inside"] = true


func _work_cycle(walker: Dictionary, delta: float) -> void:
	var work_x := float(_building_x[String(walker["work_id"])])
	match String(walker["stage"]):
		"go":
			if _walk_toward(walker, work_x, delta):
				walker["inside"] = true
				walker["stage"] = "in"
				walker["timer"] = _rng.randf_range(8.0, 16.0)
		"in":
			walker["timer"] = float(walker["timer"]) - delta
			if float(walker["timer"]) <= 0.0:
				walker["inside"] = false
				walker["stage"] = "out"
				walker["timer"] = _rng.randf_range(3.0, 6.0)
				walker["target"] = NAN
		"out":
			_wander(walker, delta, work_x, 36.0)
			walker["timer"] = float(walker["timer"]) - delta
			if float(walker["timer"]) <= 0.0:
				walker["stage"] = "go"


func _wander(walker: Dictionary, delta: float, center_x: float, reach: float) -> void:
	if float(walker["timer"]) > 0.0 and is_nan(float(walker["target"])):
		walker["timer"] = float(walker["timer"]) - delta
		if float(walker["timer"]) > 0.0:
			return
	if is_nan(float(walker["target"])):
		var goal := clampf(center_x + _rng.randf_range(-reach, reach), _bounds.x, _bounds.y)
		walker["target"] = goal
	if _walk_toward(walker, float(walker["target"]), delta):
		walker["target"] = NAN
		walker["timer"] = _rng.randf_range(1.0, 3.5)


## Moves toward target_x. Returns true on arrival. A zero delta only tests arrival.
func _walk_toward(walker: Dictionary, target_x: float, delta: float) -> bool:
	var offset := target_x - float(walker["x"])
	if absf(offset) <= ARRIVE_DISTANCE:
		return true
	if delta <= 0.0:
		delta = 1.0 / 30.0
	var step := WALK_SPEED * float(walker["speed"]) * delta
	var direction := signf(offset)
	walker["x"] = float(walker["x"]) + direction * minf(step, absf(offset))
	walker["dir"] = direction
	walker["moving"] = true
	return absf(target_x - float(walker["x"])) <= ARRIVE_DISTANCE
