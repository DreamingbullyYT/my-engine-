extends Node

signal package_finished(success: bool, message: String)
signal mod_state_changed(mod_id: String, enabled: bool)

const MODS_DIR := "user://mods"
const MAX_ARCHIVE_BYTES := 8 * 1024 * 1024
const MAX_ENTRIES := 128
const MAX_FILE_BYTES := 2 * 1024 * 1024
const MAX_TOTAL_BYTES := 8 * 1024 * 1024
const MOD_API_VERSION := 1

var _enabled: Dictionary = {}
var _manifests: Dictionary = {}
var _loading: Dictionary = {}


func create_and_run_sample() -> void:
	var package_path := MODS_DIR.path_join("_m4_sample.dmod")
	var manifest := {
		"id": "dreamingbully.m4_sample",
		"name": "M4 Sample Mod",
		"version": "1.0.0",
		"api_version": MOD_API_VERSION,
		"entry": "main.gd",
		"dependencies": {}
	}
	var script_source := """extends Node

func on_mod_load(host: Node) -> void:
	host.report_dmod_result("M4_DMOD_OK")
"""
	var packer := ZIPPacker.new()
	var error := packer.open(package_path)
	if error != OK:
		_finish(false, "Cannot create sample .dmod: %s" % error_string(error))
		return
	error = packer.start_file("mod.json")
	if error == OK:
		error = packer.write_file(JSON.stringify(manifest, "  ").to_utf8_buffer())
	if error == OK:
		error = packer.close_file()
	if error == OK:
		error = packer.start_file("main.gd")
	if error == OK:
		error = packer.write_file(script_source.to_utf8_buffer())
	if error == OK:
		error = packer.close_file()
	var close_error := packer.close()
	if error != OK or close_error != OK:
		_finish(false, "Cannot write sample .dmod archive.")
		return
	install_and_enable(package_path)


func install_and_enable(package_path: String) -> void:
	var error := DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(MODS_DIR))
	if error != OK and error != ERR_ALREADY_EXISTS:
		_finish(false, "Cannot create mods directory: %s" % error_string(error))
		return
	if not FileAccess.file_exists(package_path):
		_finish(false, "Package does not exist: %s" % package_path)
		return
	var archive := FileAccess.open(package_path, FileAccess.READ)
	if archive == null:
		_finish(false, "Cannot open package: %s" % error_string(FileAccess.get_open_error()))
		return
	var archive_size := archive.get_length()
	archive.close()
	if archive_size <= 0 or archive_size > MAX_ARCHIVE_BYTES:
		_finish(false, "Package size is outside the allowed limit.")
		return

	var reader := ZIPReader.new()
	error = reader.open(package_path)
	if error != OK:
		_finish(false, "Invalid ZIP archive: %s" % error_string(error))
		return
	var paths := reader.get_files()
	if paths.is_empty() or paths.size() > MAX_ENTRIES:
		reader.close()
		_finish(false, "Archive entry count is invalid.")
		return

	var seen: Dictionary = {}
	for path in paths:
		if not _is_safe_archive_path(path):
			reader.close()
			_finish(false, "Unsafe archive path rejected: %s" % path)
			return
		var normalized := path.to_lower()
		if seen.has(normalized):
			reader.close()
			_finish(false, "Duplicate archive path rejected: %s" % path)
			return
		seen[normalized] = true
	if not reader.file_exists("mod.json"):
		reader.close()
		_finish(false, "Package is missing root mod.json.")
		return

	var manifest_bytes := reader.read_file("mod.json")
	if manifest_bytes.is_empty() or manifest_bytes.size() > 32 * 1024:
		reader.close()
		_finish(false, "Manifest is empty or too large.")
		return
	var parsed: Variant = JSON.parse_string(manifest_bytes.get_string_from_utf8())
	if not parsed is Dictionary:
		reader.close()
		_finish(false, "Manifest must be a JSON object.")
		return
	var manifest: Dictionary = parsed
	var validation := _validate_manifest(manifest)
	if not validation.is_empty():
		reader.close()
		_finish(false, validation)
		return

	var mod_id := String(manifest["id"])
	var destination := MODS_DIR.path_join(mod_id)
	if DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(destination)):
		reader.close()
		_finish(false, "Mod is already installed: %s" % mod_id)
		return

	var staging := MODS_DIR.path_join(".staging_" + mod_id)
	_remove_tree(staging)
	error = DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(staging))
	if error != OK:
		reader.close()
		_finish(false, "Cannot create staging directory.")
		return

	var total_bytes := 0
	var sorted_paths := Array(paths)
	sorted_paths.sort()
	for path in sorted_paths:
		if path.ends_with("/"):
			continue
		var data := reader.read_file(path)
		if data.size() > MAX_FILE_BYTES:
			reader.close()
			_remove_tree(staging)
			_finish(false, "Archive entry exceeds per-file limit: %s" % path)
			return
		total_bytes += data.size()
		if total_bytes > MAX_TOTAL_BYTES:
			reader.close()
			_remove_tree(staging)
			_finish(false, "Archive exceeds total extracted-size limit.")
			return
		var target := staging.path_join(path)
		var parent_error := DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(target.get_base_dir()))
		if parent_error != OK and parent_error != ERR_ALREADY_EXISTS:
			reader.close()
			_remove_tree(staging)
			_finish(false, "Cannot create package subdirectory.")
			return
		var output := FileAccess.open(target, FileAccess.WRITE)
		if output == null:
			reader.close()
			_remove_tree(staging)
			_finish(false, "Cannot extract package entry: %s" % path)
			return
		output.store_buffer(data)
		output.close()
	reader.close()

	var entry_path := String(manifest["entry"])
	if not FileAccess.file_exists(staging.path_join(entry_path)):
		_remove_tree(staging)
		_finish(false, "Manifest entry script is missing.")
		return
	error = DirAccess.rename_absolute(ProjectSettings.globalize_path(staging), ProjectSettings.globalize_path(destination))
	if error != OK:
		_remove_tree(staging)
		_finish(false, "Cannot finalize package installation: %s" % error_string(error))
		return

	_manifests[mod_id] = manifest
	_finish(true, "Installed %s %s" % [mod_id, manifest["version"]])
	enable_mod(mod_id)


func enable_mod(mod_id: String) -> bool:
	if _enabled.has(mod_id):
		return true
	if not _manifests.has(mod_id):
		var manifest_path := MODS_DIR.path_join(mod_id).path_join("mod.json")
		if not FileAccess.file_exists(manifest_path):
			_finish(false, "Mod is not installed: %s" % mod_id)
			return false
		var file := FileAccess.open(manifest_path, FileAccess.READ)
		if file == null:
			return false
		var parsed: Variant = JSON.parse_string(file.get_as_text())
		file.close()
		if not parsed is Dictionary:
			_finish(false, "Installed mod manifest is invalid: %s" % mod_id)
			return false
		_manifests[mod_id] = parsed
	if _loading.has(mod_id):
		_finish(false, "Dependency cycle detected at mod: %s" % mod_id)
		return false
	_loading[mod_id] = true
	var manifest: Dictionary = _manifests[mod_id]
	var dependency_ids: Array = manifest.get("dependencies", {}).keys()
	dependency_ids.sort()
	for dependency_id in dependency_ids:
		if not _manifests.has(dependency_id):
			var dependency_manifest := MODS_DIR.path_join(dependency_id).path_join("mod.json")
			if not FileAccess.file_exists(dependency_manifest):
				_loading.erase(mod_id)
				_finish(false, "Missing dependency %s required by %s" % [dependency_id, mod_id])
				return false
			var dependency_file := FileAccess.open(dependency_manifest, FileAccess.READ)
			if dependency_file == null:
				_loading.erase(mod_id)
				return false
			var dependency_data: Variant = JSON.parse_string(dependency_file.get_as_text())
			dependency_file.close()
			if not dependency_data is Dictionary:
				_loading.erase(mod_id)
				return false
			_manifests[dependency_id] = dependency_data
		var required_version := String(manifest["dependencies"][dependency_id])
		if String(_manifests[dependency_id].get("version", "")) != required_version:
			_loading.erase(mod_id)
			_finish(false, "Dependency version mismatch: %s requires %s" % [dependency_id, required_version])
			return false
		if not enable_mod(String(dependency_id)):
			_loading.erase(mod_id)
			return false

	var script_path := MODS_DIR.path_join(mod_id).path_join(String(manifest["entry"]))
	var source_file := FileAccess.open(script_path, FileAccess.READ)
	if source_file == null:
		_loading.erase(mod_id)
		_finish(false, "Cannot read mod entry: %s" % mod_id)
		return false
	var source := source_file.get_as_text()
	source_file.close()
	var script := GDScript.new()
	script.source_code = source
	var parse_error := script.reload()
	if parse_error != OK:
		_loading.erase(mod_id)
		_finish(false, "Mod script parse failed (%s): %s" % [mod_id, error_string(parse_error)])
		return false
	var instance: Variant = script.new()
	if not instance is Node:
		_loading.erase(mod_id)
		_finish(false, "Mod entry must extend Node: %s" % mod_id)
		return false
	add_child(instance)
	_enabled[mod_id] = instance
	_loading.erase(mod_id)
	if instance.has_method("on_mod_load"):
		instance.call("on_mod_load", self)
	mod_state_changed.emit(mod_id, true)
	print("[M4] MOD_ENABLED id=%s version=%s" % [mod_id, manifest.get("version", "?")])
	return true


func disable_mod(mod_id: String) -> bool:
	if not _enabled.has(mod_id):
		return true
	for candidate_id in _enabled.keys():
		var candidate: Dictionary = _manifests.get(candidate_id, {})
		if candidate.get("dependencies", {}).has(mod_id):
			_finish(false, "Disable dependent mod %s before %s." % [candidate_id, mod_id])
			return false
	var instance: Node = _enabled[mod_id]
	if instance.has_method("on_mod_unload"):
		instance.call("on_mod_unload")
	instance.queue_free()
	_enabled.erase(mod_id)
	mod_state_changed.emit(mod_id, false)
	print("[M4] MOD_DISABLED id=%s" % mod_id)
	return true


func report_dmod_result(result: String) -> void:
	if result == "M4_DMOD_OK":
		_finish(true, "M4 .dmod installed and executed successfully.")
		print("[M4] DMOD_ENTRY_EXECUTED")
	else:
		_finish(false, "Mod returned an unexpected result: %s" % result)


func _validate_manifest(manifest: Dictionary) -> String:
	for key in ["id", "name", "version", "api_version", "entry", "dependencies"]:
		if not manifest.has(key):
			return "Manifest is missing field: %s" % key
	if not manifest["id"] is String or not _valid_mod_id(String(manifest["id"])):
		return "Manifest mod id is invalid."
	if not manifest["name"] is String or String(manifest["name"]).strip_edges().is_empty():
		return "Manifest name is invalid."
	if not manifest["version"] is String or not _valid_version(String(manifest["version"])):
		return "Manifest version must use numeric major.minor.patch."
	if int(manifest["api_version"]) != MOD_API_VERSION:
		return "Unsupported mod API version."
	if not manifest["entry"] is String or not _is_safe_archive_path(String(manifest["entry"])) or String(manifest["entry"]).ends_with("/"):
		return "Manifest entry path is invalid."
	if not manifest["dependencies"] is Dictionary:
		return "Manifest dependencies must be an object."
	for dependency_id in manifest["dependencies"]:
		if not _valid_mod_id(String(dependency_id)) or not _valid_version(String(manifest["dependencies"][dependency_id])):
			return "Manifest contains an invalid dependency declaration."
	return ""


func _valid_mod_id(value: String) -> bool:
	var regex := RegEx.new()
	regex.compile("^[a-z0-9][a-z0-9_.-]{1,63}$")
	return regex.search(value) != null


func _valid_version(value: String) -> bool:
	var regex := RegEx.new()
	regex.compile("^[0-9]+\\.[0-9]+\\.[0-9]+$")
	return regex.search(value) != null


func _is_safe_archive_path(path: String) -> bool:
	if path.is_empty() or path.begins_with("/") or path.contains("\\") or path.contains(":"):
		return false
	for segment in path.trim_suffix("/").split("/"):
		if segment.is_empty() or segment == "." or segment == "..":
			return false
	return true


func _remove_tree(path: String) -> void:
	var absolute := ProjectSettings.globalize_path(path)
	if not DirAccess.dir_exists_absolute(absolute):
		return
	var directory := DirAccess.open(path)
	if directory == null:
		return
	for file_name in directory.get_files():
		DirAccess.remove_absolute(absolute.path_join(file_name))
	for directory_name in directory.get_directories():
		_remove_tree(path.path_join(directory_name))
	DirAccess.remove_absolute(absolute)


func _finish(success: bool, message: String) -> void:
	package_finished.emit(success, message)
