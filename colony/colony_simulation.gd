extends Node

signal changed
signal message_changed(message: String)

const DAY_LENGTH_SECONDS := 8.0
const MAX_RESOURCE_STOCK := 999
const CITY_CENTER_ANGLE := -PI * 0.5
const BUILDING_ANGLE_SPACING := 0.06

const BUILDING_TYPES: Dictionary = {
	"habitat": {
		"label": "Habitat",
		"cost": {"metal": 18},
		"housing": 20,
		"worker_slots": 0,
		"job": "Resident"
	},
	"solar": {
		"label": "Solar Array",
		"cost": {"metal": 14},
		"housing": 0,
		"worker_slots": 1,
		"job": "Technician"
	},
	"farm": {
		"label": "Farm",
		"cost": {"metal": 16},
		"housing": 0,
		"worker_slots": 3,
		"job": "Farmer"
	},
	"mine": {
		"label": "Mine",
		"cost": {"metal": 20},
		"housing": 0,
		"worker_slots": 4,
		"job": "Miner"
	}
}

const FIRST_NAMES: PackedStringArray = [
	"Ada", "Mira", "Noah", "Leo", "Iris", "Eli", "Nia", "Milo", "Sana", "Theo",
	"Rhea", "Arin", "Lina", "Kai", "Ema", "Oren", "Zoe", "Ivo", "Maya", "Luca"
]

var day: int = 1
var resources: Dictionary = {}
var citizens: Array = []
var buildings: Array = []
var last_message: String = "Settlement ready."

var _day_timer: Timer


func _ready() -> void:
	start_new_game()
	_day_timer = Timer.new()
	_day_timer.wait_time = DAY_LENGTH_SECONDS
	_day_timer.one_shot = false
	_day_timer.timeout.connect(advance_day)
	add_child(_day_timer)
	_day_timer.start()


func start_new_game() -> void:
	day = 1
	resources = {
		"metal": 60,
		"food": 100,
		"energy": 80
	}
	citizens.clear()
	buildings = [
		{"id": "habitat-1", "type": "habitat", "angle": CITY_CENTER_ANGLE - BUILDING_ANGLE_SPACING, "workers": 0},
		{"id": "solar-1", "type": "solar", "angle": CITY_CENTER_ANGLE, "workers": 0},
		{"id": "farm-1", "type": "farm", "angle": CITY_CENTER_ANGLE + BUILDING_ANGLE_SPACING, "workers": 0}
	]
	for index in range(FIRST_NAMES.size()):
		citizens.append({
			"id": "citizen-%02d" % (index + 1),
			"name": FIRST_NAMES[index],
			"job": "Unassigned",
			"happiness": 72,
			"health": 100
		})
	_assign_workers()
	_set_message("Settlement established. Build and sustain your city.")


func can_spend(cost: Dictionary) -> bool:
	for resource_name in cost:
		if int(resources.get(resource_name, 0)) < int(cost[resource_name]):
			return false
	return true


func try_spend(cost: Dictionary) -> bool:
	if not can_spend(cost):
		return false
	for resource_name in cost:
		resources[resource_name] = int(resources.get(resource_name, 0)) - int(cost[resource_name])
	changed.emit()
	return true


func can_build(building_type: String) -> bool:
	if not BUILDING_TYPES.has(building_type):
		return false
	var definition: Dictionary = BUILDING_TYPES[building_type]
	return can_spend(definition.get("cost", {}))


func build(building_type: String) -> bool:
	if not BUILDING_TYPES.has(building_type):
		_set_message("Unknown or unavailable city building.")
		return false
	var definition: Dictionary = BUILDING_TYPES[building_type]
	var cost: Dictionary = definition.get("cost", {})
	if not try_spend(cost):
		_set_message("Not enough metal for %s." % String(definition.get("label", building_type)))
		return false
	var building_id := "%s-%d" % [building_type, buildings.size() + 1]
	var angle := CITY_CENTER_ANGLE + (float(buildings.size()) - 1.0) * BUILDING_ANGLE_SPACING
	buildings.append({
		"id": building_id,
		"type": building_type,
		"angle": angle,
		"workers": 0
	})
	_assign_workers()
	_set_message("%s constructed." % String(definition.get("label", building_type)))
	changed.emit()
	return true


func has_building(building_type: String) -> bool:
	for building in buildings:
		if String(building.get("type", "")) == building_type:
			return true
	return false


func get_housing_capacity() -> int:
	var capacity := 0
	for building in buildings:
		var building_type := String(building.get("type", ""))
		if BUILDING_TYPES.has(building_type):
			capacity += int(BUILDING_TYPES[building_type].get("housing", 0))
	return capacity


func get_employed_count() -> int:
	var employed := 0
	for citizen in citizens:
		if String(citizen.get("job", "Unassigned")) != "Unassigned":
			employed += 1
	return employed


func get_average_happiness() -> int:
	if citizens.is_empty():
		return 0
	var total := 0
	for citizen in citizens:
		total += int(citizen.get("happiness", 0))
	return int(round(float(total) / float(citizens.size())))


func advance_day() -> void:
	var solar_generation := 0
	var power_demand := 0
	var potential_food := 0.0
	var potential_metal := 0.0

	for building in buildings:
		var building_type := String(building.get("type", ""))
		var workers := int(building.get("workers", 0))
		match building_type:
			"solar":
				if workers > 0:
					solar_generation += 24
			"farm":
				if workers > 0:
					power_demand += 4
					potential_food += 26.0 * minf(1.0, float(workers) / 3.0)
			"mine":
				if workers > 0:
					power_demand += 6
					potential_metal += 8.0 * minf(1.0, float(workers) / 4.0)

	var available_energy := int(resources.get("energy", 0)) + solar_generation
	var power_ratio := 1.0
	if power_demand > available_energy and power_demand > 0:
		power_ratio = float(available_energy) / float(power_demand)
	resources["energy"] = clampi(available_energy - power_demand, 0, MAX_RESOURCE_STOCK)

	var food_produced := int(floor(potential_food * power_ratio))
	var metal_produced := int(floor(potential_metal * power_ratio))
	resources["food"] = clampi(int(resources.get("food", 0)) + food_produced, 0, MAX_RESOURCE_STOCK)
	resources["metal"] = clampi(int(resources.get("metal", 0)) + metal_produced, 0, MAX_RESOURCE_STOCK)

	var food_required := citizens.size()
	var food_available := int(resources.get("food", 0))
	var fed := food_available >= food_required
	resources["food"] = max(0, food_available - food_required)

	for citizen in citizens:
		var happiness := int(citizen.get("happiness", 72))
		if fed:
			happiness = mini(100, happiness + 1)
		else:
			happiness = maxi(0, happiness - 4)
		citizen["happiness"] = happiness

	day += 1
	var food_note := "Food supply stable." if fed else "Food shortage! Farm capacity or power is insufficient."
	_set_message("Day %d: +%d metal, +%d food. %s" % [day, metal_produced, food_produced, food_note])
	changed.emit()


func get_population_summary() -> String:
	return "%d colonists / %d housing · %d assigned · %d%% average happiness" % [
		citizens.size(),
		get_housing_capacity(),
		get_employed_count(),
		get_average_happiness()
	]


func get_snapshot() -> Dictionary:
	return {
		"schema": 1,
		"day": day,
		"resources": resources.duplicate(true),
		"citizens": citizens.duplicate(true),
		"buildings": buildings.duplicate(true)
	}


func restore_snapshot(data: Dictionary) -> bool:
	if int(data.get("schema", 0)) != 1:
		return false
	if not (data.get("resources", null) is Dictionary):
		return false
	if not (data.get("citizens", null) is Array):
		return false
	if not (data.get("buildings", null) is Array):
		return false

	var incoming_resources: Dictionary = data["resources"]
	var incoming_citizens: Array = data["citizens"]
	var incoming_buildings: Array = data["buildings"]
	if incoming_citizens.is_empty():
		return false

	var safe_citizens: Array = []
	for citizen_data in incoming_citizens:
		if citizen_data is Dictionary:
			safe_citizens.append(citizen_data.duplicate(true))
	if safe_citizens.is_empty():
		return false

	var safe_buildings: Array = []
	for building_data in incoming_buildings:
		if building_data is Dictionary:
			safe_buildings.append(building_data.duplicate(true))

	day = maxi(1, int(data.get("day", 1)))
	resources = incoming_resources.duplicate(true)
	citizens = safe_citizens
	buildings = safe_buildings
	_assign_workers()
	_set_message("Saved settlement loaded.")
	changed.emit()
	return true


func _assign_workers() -> void:
	for citizen in citizens:
		citizen["job"] = "Unassigned"
	for building in buildings:
		building["workers"] = 0

	for building_index in range(buildings.size()):
		var building: Dictionary = buildings[building_index]
		var building_type := String(building.get("type", ""))
		if not BUILDING_TYPES.has(building_type):
			continue
		var definition: Dictionary = BUILDING_TYPES[building_type]
		var slots := int(definition.get("worker_slots", 0))
		var assigned := 0
		for _slot in range(slots):
			var next_citizen := -1
			for citizen_index in range(citizens.size()):
				var citizen: Dictionary = citizens[citizen_index]
				if String(citizen.get("job", "Unassigned")) == "Unassigned":
					next_citizen = citizen_index
					break
			if next_citizen < 0:
				break
			var chosen: Dictionary = citizens[next_citizen]
			chosen["job"] = String(definition.get("job", "General"))
			assigned += 1
		building["workers"] = assigned
		buildings[building_index] = building


func _set_message(message: String) -> void:
	last_message = message
	message_changed.emit(message)
