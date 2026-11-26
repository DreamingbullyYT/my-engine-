#include <dream/dmod/manifest_parser.hpp>

#include <toml++/toml.hpp>

#include <charconv>
#include <cstdint>
#include <string>

namespace dream::dmod {
namespace {

std::optional<core::Version> parse_version(const std::string& value) {
    core::Version version{};
    const auto first = value.find('.');
    if (first == std::string::npos) return std::nullopt;
    const auto second = value.find('.', first + 1);
    if (second == std::string::npos || value.find('.', second + 1) != std::string::npos) {
        return std::nullopt;
    }

    const auto parse_part = [](std::string_view part, std::uint32_t& output) {
        if (part.empty()) return false;
        const auto result = std::from_chars(part.data(), part.data() + part.size(), output);
        return result.ec == std::errc{} && result.ptr == part.data() + part.size();
    };

    if (!parse_part(std::string_view(value).substr(0, first), version.major) ||
        !parse_part(std::string_view(value).substr(first + 1, second - first - 1), version.minor) ||
        !parse_part(std::string_view(value).substr(second + 1), version.patch)) {
        return std::nullopt;
    }
    return version;
}

std::optional<std::string> required_string(
    const toml::table& table, std::string_view key, std::vector<std::string>& errors) {
    auto value = table[key].value<std::string>();
    if (!value) errors.emplace_back("Missing or invalid string field: " + std::string(key));
    return value;
}

std::optional<core::Version> required_version(
    const toml::table& table, std::string_view key, std::vector<std::string>& errors) {
    const auto value = required_string(table, key, errors);
    if (!value) return std::nullopt;
    auto parsed = parse_version(*value);
    if (!parsed) errors.emplace_back("Version must use MAJOR.MINOR.PATCH: " + std::string(key));
    return parsed;
}

} // namespace

ManifestParseResult parse_manifest(std::string_view source) {
    ManifestParseResult result;
    if (source.empty() || source.size() > 64 * 1024) {
        result.errors.emplace_back("Manifest must contain between 1 byte and 64 KiB.");
        return result;
    }

    toml::table root;
    try {
        root = toml::parse(source);
    } catch (const toml::parse_error& error) {
        result.errors.emplace_back("TOML parse error: " + std::string(error.description()));
        return result;
    }

    const auto format_version = root["format_version"].value<std::int64_t>();
    if (!format_version || *format_version != 1) {
        result.errors.emplace_back("format_version must be the integer 1.");
    }

    Manifest manifest;
    const auto id = required_string(root, "id", result.errors);
    const auto name = required_string(root, "name", result.errors);
    const auto version = required_version(root, "version", result.errors);
    const auto api_min = required_version(root, "engine_api_min", result.errors);

    if (id) manifest.id = *id;
    if (name) manifest.name = *name;
    if (version) manifest.version = *version;
    if (api_min) manifest.engine_api.minimum_inclusive = *api_min;

    const auto runtime_text = root["runtime"].value<std::string>();
    if (runtime_text) {
        if (*runtime_text == "native") manifest.runtime_kind = ModRuntimeKind::Native;
        else if (*runtime_text == "script") manifest.runtime_kind = ModRuntimeKind::Script;
        else result.errors.emplace_back("runtime must be either \"native\" or \"script\".");
    } else if (root.get("runtime")) {
        result.errors.emplace_back("runtime must be a string.");
    } else if (root.get("native_library")) {
        manifest.runtime_kind = ModRuntimeKind::Native;
    }

    if (manifest.runtime_kind == ModRuntimeKind::Native) {
        const auto library = required_string(root, "native_library", result.errors);
        if (library) manifest.native_library = *library;
        if (root.get("entry_script"))
            result.errors.emplace_back("Native manifests use native_library, not entry_script.");
    } else {
        const auto entry = required_string(root, "entry_script", result.errors);
        if (entry) manifest.entry_script = *entry;
        if (root.get("native_library"))
            result.errors.emplace_back("Script manifests must not declare native_library.");
    }

    if (const auto max_text = root["engine_api_max"].value<std::string>()) {
        const auto parsed = parse_version(*max_text);
        if (!parsed) result.errors.emplace_back("engine_api_max must use MAJOR.MINOR.PATCH.");
        else manifest.engine_api.maximum_exclusive = *parsed;
    } else if (root.get("engine_api_max")) {
        result.errors.emplace_back("engine_api_max must be a version string.");
    }

    if (const auto* node = root.get("capabilities")) {
        const auto* array = node->as_array();
        if (!array) {
            result.errors.emplace_back("capabilities must be an array of strings.");
        } else {
            for (const auto& item : *array) {
                const auto value = item.value<std::string>();
                if (!value) result.errors.emplace_back("Every capability must be a string.");
                else manifest.capabilities.push_back(*value);
            }
        }
    }

    if (const auto* node = root.get("dependencies")) {
        const auto* array = node->as_array();
        if (!array) {
            result.errors.emplace_back("dependencies must be an array of tables.");
        } else {
            for (const auto& item : *array) {
                const auto* table = item.as_table();
                if (!table) {
                    result.errors.emplace_back("Every dependency must be a table.");
                    continue;
                }

                Dependency dependency;
                const auto dep_id = required_string(*table, "id", result.errors);
                const auto dep_min = required_version(*table, "min_version", result.errors);
                if (dep_id) dependency.id = *dep_id;
                if (dep_min) dependency.versions.minimum_inclusive = *dep_min;

                if (const auto max_text = (*table)["max_version"].value<std::string>()) {
                    const auto parsed = parse_version(*max_text);
                    if (!parsed) result.errors.emplace_back("Dependency max_version must use MAJOR.MINOR.PATCH.");
                    else dependency.versions.maximum_exclusive = *parsed;
                } else if (table->get("max_version")) {
                    result.errors.emplace_back("Dependency max_version must be a version string.");
                }

                if (const auto optional = (*table)["optional"].value<bool>()) {
                    dependency.optional = *optional;
                } else if (table->get("optional")) {
                    result.errors.emplace_back("Dependency optional must be a boolean.");
                }
                manifest.dependencies.push_back(std::move(dependency));
            }
        }
    }

    const auto validation = validate_manifest(manifest);
    result.errors.insert(result.errors.end(), validation.errors.begin(), validation.errors.end());
    if (result.errors.empty()) result.manifest = std::move(manifest);
    return result;
}

} // namespace dream::dmod
