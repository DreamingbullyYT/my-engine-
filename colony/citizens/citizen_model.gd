extends RefCounted

const BALANCE_PATH := "res://colony/data/citizen_balance.json"

const DEFAULT_BALANCE: Dictionary = {
	"initial_needs": {
		"food": 100.0,
		"water": 100.0,
		"rest": 90.0,
		"housing": 100.0,
		"social": 100.0,
		"recreation": 100.0,
		"safety": 100.0
	},
	"daily_need_decay": {
		"food": 18.0,
		"water": 20.0,
		"housing": 15.0,
		"social": 0.5,
		"recreation": 0.5,
		"safety": 0.0
	},
	"rest_recovery_per_day": 12.0,
	"happiness_weights": {
		"food": 0.20,
		"water": 0.18,
		"rest": 0.12,
		"housing": 0.16,
		"social": 0.10,
		"recreation": 0.10,
		"safety": 0.06,
		"health": 0.08
	},
	"health_rules": {
		"critical_need_threshold": 20.0,
		"damage_per_day": 2.0,
		"recovery_need_threshold": 60.0,
		"recovery_per_day": 1.0
	},
	"skill_gain_per_work_day": 0.1
}

var _balance_data: Dictionary = {}


func _init() -> void:
	_balance_data = DEFAULT_BALANCE.duplicate(true)
	var file := FileAccess.open(BALANCE_PATH, FileAccess.READ)
	if file == null:
		push_warning("[CitizenModel] Balance file unavailable; using built-in defaults.")
		return

	var json := JSON.new()
	var parse_error := json.parse(file.get_as_text())
	file.close()
	if parse_error != OK or not (json.data is Dictionary):
		push_warning("[CitizenModel] Balance file invalid; using built-in defaults.")
		return
	_merge_balance(_balance_data, json.data)


func create_citizen(citizen_id: String, display_name: String, age_years: int, home_id: String = "") -> Dictionary:
	var initial_needs: Dictionary = _balance_data.get("initial_needs", {}).duplicate(true)
	return {
		"id": citizen_id,
		"name": display_name,
		"age_years": clampi(age_years, 1, 120),
		"home_id": home_id,
		"job": "Unassigned",
		"job_building_id": "",
		"skills": {
			"agriculture": 0.0,
			"mining": 0.0,
			"engineering": 0.0,
			"general": 0.0
		},
		"work_days": 0,
		"traits": [],
		"needs": initial_needs,
		"health": 100.0,
		"happiness": 90,
		"status": "healthy",
		"activity": "idle"
	}


func normalize_citizen(data: Dictionary, index: int, fallback_home_id: String = "") -> Dictionary:
	var fallback_id := "citizen-%02d" % (index + 1)
	var fallback_name := "Citizen %02d" % (index + 1)
	var normalized := create_citizen(
		String(data.get("id", fallback_id)),
		String(data.get("name", fallback_name)),
		int(data.get("age_years", 18 + ((index * 7) % 45))),
		String(data.get("home_id", fallback_home_id))
	)

	for key in ["job", "job_building_id", "work_days", "health", "happiness", "status", "activity"]:
		if data.has(key):
			normalized[key] = data[key]

	if data.get("traits", null) is Array:
		normalized["traits"] = data["traits"].duplicate(true)

	var skills_source: Variant = data.get("skills", {})
	if skills_source is Dictionary:
		var skills: Dictionary = normalized["skills"].duplicate(true)
		for skill_id in skills_source:
			if skills.has(skill_id):
				skills[skill_id] = clampf(float(skills_source[skill_id]), 0.0, 100.0)
		normalized["skills"] = skills

	var needs_source: Variant = data.get("needs", {})
	if needs_source is Dictionary:
		var needs: Dictionary = normalized["needs"].duplicate(true)
		for need_id in needs_source:
			if needs.has(need_id):
				needs[need_id] = clampf(float(needs_source[need_id]), 0.0, 100.0)
		normalized["needs"] = needs

	normalized["age_years"] = clampi(int(normalized.get("age_years", 18)), 1, 120)
	normalized["work_days"] = maxi(0, int(normalized.get("work_days", 0)))
	normalized["health"] = clampf(float(normalized.get("health", 100.0)), 0.0, 100.0)
	_update_summary(normalized)
	return normalized


func advance_daily_needs(
	citizen: Dictionary,
	food_supply_ratio: float,
	water_supply_ratio: float,
	housed: bool,
	service_levels: Dictionary = {}
) -> void:
	var needs: Dictionary = {}
	var needs_source: Variant = citizen.get("needs", {})
	if needs_source is Dictionary:
		needs = needs_source.duplicate(true)
	else:
		needs = _balance_data.get("initial_needs", {}).duplicate(true)

	var decay: Dictionary = _balance_data.get("daily_need_decay", {})
	var food_drain := float(decay.get("food", 18.0))
	var water_drain := float(decay.get("water", 20.0))
	needs["food"] = _apply_supply(float(needs.get("food", 100.0)), food_drain, food_supply_ratio)
	needs["water"] = _apply_supply(float(needs.get("water", 100.0)), water_drain, water_supply_ratio)

	var rest_recovery := float(_balance_data.get("rest_recovery_per_day", 12.0))
	needs["rest"] = clampf(float(needs.get("rest", 90.0)) + rest_recovery, 0.0, 100.0)

	var housing_target := 100.0 if housed else 0.0
	var housing_drain := float(decay.get("housing", 15.0))
	needs["housing"] = move_toward(float(needs.get("housing", 100.0)), housing_target, housing_drain)

	var social_supply := clampf(float(service_levels.get("social", 0.0)), 0.0, 1.0)
	var recreation_supply := clampf(float(service_levels.get("recreation", 0.0)), 0.0, 1.0)
	var safety_supply := clampf(float(service_levels.get("safety", 1.0)), 0.0, 1.0)
	needs["social"] = _apply_supply(float(needs.get("social", 100.0)), float(decay.get("social", 0.5)), social_supply)
	needs["recreation"] = _apply_supply(float(needs.get("recreation", 100.0)), float(decay.get("recreation", 0.5)), recreation_supply)
	needs["safety"] = _apply_supply(float(needs.get("safety", 100.0)), float(decay.get("safety", 0.0)), safety_supply)
	citizen["needs"] = needs

	var health_rules: Dictionary = _balance_data.get("health_rules", {})
	var health := float(citizen.get("health", 100.0))
	var critical_threshold := float(health_rules.get("critical_need_threshold", 20.0))
	var recovery_threshold := float(health_rules.get("recovery_need_threshold", 60.0))
	if float(needs["food"]) < critical_threshold or float(needs["water"]) < critical_threshold:
		health -= float(health_rules.get("damage_per_day", 2.0))
	elif float(needs["food"]) >= recovery_threshold and float(needs["water"]) >= recovery_threshold:
		health += float(health_rules.get("recovery_per_day", 1.0))
	citizen["health"] = clampf(health, 0.0, 100.0)
	_update_summary(citizen)


func record_work_day(citizen: Dictionary) -> void:
	var job := String(citizen.get("job", "Unassigned"))
	if job == "Unassigned":
		citizen["activity"] = "idle"
		return

	var skill_by_job := {
		"Farmer": "agriculture",
		"Miner": "mining",
		"Technician": "engineering",
		"Water Technician": "engineering"
	}
	var skill_id := String(skill_by_job.get(job, "general"))
	var skills: Dictionary = citizen.get("skills", {}).duplicate(true)
	var gain := float(_balance_data.get("skill_gain_per_work_day", 0.1))
	skills[skill_id] = clampf(float(skills.get(skill_id, 0.0)) + gain, 0.0, 100.0)
	citizen["skills"] = skills
	citizen["work_days"] = int(citizen.get("work_days", 0)) + 1
	citizen["activity"] = "working"


func get_happiness_breakdown(citizen: Dictionary) -> Dictionary:
	var needs: Dictionary = citizen.get("needs", {})
	var weights: Dictionary = _balance_data.get("happiness_weights", {})
	var total_weight := 0.0
	var weighted_score := 0.0
	var contributions: Dictionary = {}

	for factor_id in weights:
		var weight := maxf(0.0, float(weights[factor_id]))
		var value := float(citizen.get("health", 100.0)) if factor_id == "health" else float(needs.get(factor_id, 100.0))
		var contribution := clampf(value, 0.0, 100.0) * weight
		total_weight += weight
		weighted_score += contribution
		contributions[factor_id] = contribution

	var score := 100.0 if total_weight <= 0.0 else weighted_score / total_weight
	return {"score": int(round(score)), "contributions": contributions, "weights": weights.duplicate(true)}


func get_balance_data() -> Dictionary:
	return _balance_data.duplicate(true)


func _apply_supply(current: float, daily_drain: float, supply_ratio: float) -> float:
	var supplied := clampf(supply_ratio, 0.0, 1.0)
	return clampf(current - daily_drain + daily_drain * supplied, 0.0, 100.0)


func _update_summary(citizen: Dictionary) -> void:
	var breakdown := get_happiness_breakdown(citizen)
	citizen["happiness"] = int(breakdown.get("score", 100))
	var needs: Dictionary = citizen.get("needs", {})
	var health := float(citizen.get("health", 100.0))
	if health < 30.0:
		citizen["status"] = "critical"
	elif health < 60.0 or float(needs.get("food", 100.0)) < 25.0 or float(needs.get("water", 100.0)) < 25.0:
		citizen["status"] = "sick"
	else:
		citizen["status"] = "healthy"


func _merge_balance(base: Dictionary, incoming: Dictionary) -> void:
	for key in incoming:
		if not base.has(key):
			continue
		var base_value: Variant = base[key]
		var incoming_value: Variant = incoming[key]
		if base_value is Dictionary and incoming_value is Dictionary:
			var merged: Dictionary = base_value.duplicate(true)
			for nested_key in incoming_value:
				if merged.has(nested_key) and typeof(merged[nested_key]) == typeof(incoming_value[nested_key]):
					merged[nested_key] = incoming_value[nested_key]
			base[key] = merged
		elif typeof(base_value) == typeof(incoming_value):
			base[key] = incoming_value
