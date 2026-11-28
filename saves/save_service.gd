extends Node

const SAVE_PATH := "user://saves/colony_slot_1.json"
const SAVE_SCHEMA := 1


func save_game(colony_snapshot: Dictionary, rocket_snapshot: Dictionary) -> bool:
	var directory_path := ProjectSettings.globalize_path("user://saves")
	var directory_error := DirAccess.make_dir_recursive_absolute(directory_path)
	if directory_error != OK and directory_error != ERR_ALREADY_EXISTS:
		push_error("[Save] Could not create save directory: %s" % error_string(directory_error))
		return false

	var file := FileAccess.open(SAVE_PATH, FileAccess.WRITE)
	if file == null:
		push_error("[Save] Could not open save file: %s" % error_string(FileAccess.get_open_error()))
		return false

	var payload := {
		"schema": SAVE_SCHEMA,
		"colony": colony_snapshot,
		"rocket": rocket_snapshot
	}
	file.store_string(JSON.stringify(payload, "\t"))
	var write_error := file.get_error()
	file.close()
	if write_error != OK:
		push_error("[Save] Save write failed: %s" % error_string(write_error))
		return false
	return true


func load_game() -> Dictionary:
	if not FileAccess.file_exists(SAVE_PATH):
		return {"ok": false, "error": "No save file exists yet."}

	var file := FileAccess.open(SAVE_PATH, FileAccess.READ)
	if file == null:
		return {"ok": false, "error": "Could not open save file."}
	var json := JSON.new()
	var parse_error := json.parse(file.get_as_text())
	file.close()
	if parse_error != OK:
		return {"ok": false, "error": "Save file is invalid JSON at line %d." % json.get_error_line()}
	if not (json.data is Dictionary):
		return {"ok": false, "error": "Save file root must be an object."}

	var payload: Dictionary = json.data
	if int(payload.get("schema", 0)) != SAVE_SCHEMA:
		return {"ok": false, "error": "Unsupported save schema."}
	if not (payload.get("colony", null) is Dictionary) or not (payload.get("rocket", null) is Dictionary):
		return {"ok": false, "error": "Save file is missing required game data."}
	return {"ok": true, "data": payload}
