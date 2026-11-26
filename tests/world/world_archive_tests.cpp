#include <dream/world/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace dream::world;
using dream::serialization::ArchiveError;
using dream::serialization::BinaryWriter;

struct Health final { std::int32_t value{0}; };
struct DisplayName final { std::string value; };
struct Unregistered final { std::uint32_t value{0}; };

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

ComponentCodecRegistrationError register_test_codecs(ComponentCodecRegistry& registry) {
    const auto health = registry.register_component<Health>(
        0x1001U, 1U,
        [](const Health& value, BinaryWriter& writer) {
            return writer.write_i32(value.value);
        },
        [](dream::serialization::BinaryReader& reader, std::uint32_t version)
            -> std::optional<Health> {
            if (version != 1U) return std::nullopt;
            Health value;
            if (!reader.read_i32(value.value)) return std::nullopt;
            return value;
        });
    if (health != ComponentCodecRegistrationError::None) return health;

    return registry.register_component<DisplayName>(
        0x1002U, 1U,
        [](const DisplayName& value, BinaryWriter& writer) {
            return writer.write_string(value.value);
        },
        [](dream::serialization::BinaryReader& reader, std::uint32_t version)
            -> std::optional<DisplayName> {
            if (version != 1U) return std::nullopt;
            DisplayName value;
            if (!reader.read_string(value.value)) return std::nullopt;
            return value;
        });
}

std::vector<std::byte> make_unknown_component_archive() {
    BinaryWriter writer;
    (void)writer.write_header({1, 0});
    (void)writer.write_u32(1U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(UINT32_MAX);
    (void)writer.write_u32(1U);
    (void)writer.write_u64(0x9999U);
    (void)writer.write_u32(1U);
    (void)writer.write_u32(0U);
    return std::move(writer).take();
}

std::vector<std::byte> make_self_parent_archive() {
    BinaryWriter writer;
    (void)writer.write_header({1, 0});
    (void)writer.write_u32(1U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(0U);
    return std::move(writer).take();
}

std::vector<std::byte> make_cycle_archive() {
    BinaryWriter writer;
    (void)writer.write_header({1, 0});
    (void)writer.write_u32(2U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(1U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(1U);
    (void)writer.write_u32(0U);
    (void)writer.write_u32(0U);
    return std::move(writer).take();
}
}

int main() {
    ComponentCodecRegistry registry;
    require(register_transform_components(registry) ==
                ComponentCodecRegistrationError::None,
            "built-in Transform2D and Transform3D codecs register");
    require(register_test_codecs(registry) ==
                ComponentCodecRegistrationError::None,
            "application component codecs register");
    require(registry.size() == 4U, "registry contains four component codecs");

    require(registry.register_component<Health>(
                0x1001U, 1U,
                [](const Health&, BinaryWriter&) { return true; },
                [](dream::serialization::BinaryReader&, std::uint32_t)
                    -> std::optional<Health> { return Health{}; }) ==
                ComponentCodecRegistrationError::DuplicateTypeId,
            "duplicate persistent component IDs are rejected");
    require(registry.register_component<Health>(
                0x1003U, 1U,
                [](const Health&, BinaryWriter&) { return true; },
                [](dream::serialization::BinaryReader&, std::uint32_t)
                    -> std::optional<Health> { return Health{}; }) ==
                ComponentCodecRegistrationError::DuplicateComponentType,
            "a C++ component type cannot be registered twice");

    World source;
    const auto root = source.create_entity();
    const auto child = source.create_entity();
    const auto independent = source.create_entity();
    require(root && child && independent, "source entities are created");
    require(source.set_parent(child, root), "source hierarchy is connected");

    require(source.emplace_component<DisplayName>(root, DisplayName{"Root entity"}),
            "root name is added");
    require(source.emplace_component<Health>(child, Health{125}),
            "child health is added");
    require(source.emplace_component<Transform2D>(
                child, Transform2D{{10.5F, -2.0F}, 0.25F, {2.0F, 0.5F}}),
            "child 2D transform is added");
    require(source.emplace_component<Transform3D>(
                independent, Transform3D{{1, 2, 3}, {0, 0, 0, 1}, {1, 2, 3}}),
            "independent 3D transform is added");

    const auto saved = WorldSerializer::save(source, registry);
    require(saved, "world saves successfully");
    require(saved.entity_count == 3U, "archive reports entity count");
    require(!saved.bytes.empty(), "archive contains data");

    const auto saved_again = WorldSerializer::save(source, registry);
    require(saved_again && saved.bytes == saved_again.bytes,
            "world serialization is deterministic for an unchanged world");

    auto loaded = WorldSerializer::load(saved.bytes, registry);
    require(loaded, "world loads successfully");
    require(loaded.entity_count == 3U && loaded.world->entity_count() == 3U,
            "loaded world contains all entities");

    const auto loaded_entities = loaded.world->entities();
    require(loaded_entities.size() == 3U, "loaded entity snapshot has three entries");
    require(loaded.world->parent_of(loaded_entities[1]) == loaded_entities[0],
            "parent relationship is restored through archive-local IDs");
    require(loaded.world->is_ancestor(loaded_entities[0], loaded_entities[1]),
            "restored hierarchy supports ancestor queries");

    require(loaded.world->with_component<DisplayName>(
                loaded_entities[0], [](const DisplayName& value) {
                    require(value.value == "Root entity", "string component is restored");
                }),
            "restored root name is accessible");
    require(loaded.world->with_component<Health>(
                loaded_entities[1], [](const Health& value) {
                    require(value.value == 125, "integer component is restored");
                }),
            "restored child health is accessible");
    require(loaded.world->with_component<Transform2D>(
                loaded_entities[1], [](const Transform2D& value) {
                    require(value.position.x == 10.5F && value.position.y == -2.0F &&
                            value.rotation_radians == 0.25F &&
                            value.scale.x == 2.0F && value.scale.y == 0.5F,
                            "2D transform values are restored");
                }),
            "restored 2D transform is accessible");
    require(loaded.world->with_component<Transform3D>(
                loaded_entities[2], [](const Transform3D& value) {
                    require(value.position.x == 1.0F && value.position.y == 2.0F &&
                            value.position.z == 3.0F && value.scale.z == 3.0F,
                            "3D transform values are restored");
                }),
            "restored 3D transform is accessible");

    World unregistered_world;
    const auto unregistered_entity = unregistered_world.create_entity();
    require(unregistered_world.emplace_component<Unregistered>(
                unregistered_entity, Unregistered{7U}),
            "unregistered component is added");
    const auto unregistered_save = WorldSerializer::save(unregistered_world, registry);
    require(unregistered_save.error == WorldArchiveError::UnregisteredComponent,
            "saving refuses to silently discard unregistered component types");

    const auto unknown_archive = make_unknown_component_archive();
    const auto unknown_rejected = WorldSerializer::load(unknown_archive, registry);
    require(unknown_rejected.error == WorldArchiveError::UnknownComponent,
            "unknown component types are rejected by default");
    WorldArchiveOptions skip_unknown_options;
    skip_unknown_options.allow_unknown_components = true;
    const auto unknown_skipped = WorldSerializer::load(
        unknown_archive, registry, skip_unknown_options);
    require(unknown_skipped && unknown_skipped.world->entity_count() == 1U,
            "unknown components can be skipped explicitly");

    const auto self_parent = WorldSerializer::load(make_self_parent_archive(), registry);
    require(self_parent.error == WorldArchiveError::InvalidParent,
            "self-parent references are rejected");
    const auto cyclic_parent = WorldSerializer::load(make_cycle_archive(), registry);
    require(cyclic_parent.error == WorldArchiveError::InvalidParent,
            "cyclic entity hierarchies are rejected");

    auto trailing_bytes = saved.bytes;
    trailing_bytes.push_back(std::byte{0xAA});
    const auto trailing = WorldSerializer::load(trailing_bytes, registry);
    require(trailing.error == WorldArchiveError::TrailingData,
            "trailing archive data is rejected");

    auto unsupported_version = saved.bytes;
    unsupported_version[4] = std::byte{0x02};
    unsupported_version[5] = std::byte{0x00};
    const auto unsupported = WorldSerializer::load(unsupported_version, registry);
    require(unsupported.error == WorldArchiveError::ArchiveFailure &&
            unsupported.archive_error == ArchiveError::UnsupportedVersion,
            "unsupported world archive versions are rejected");

    WorldArchiveOptions entity_limit;
    entity_limit.maximum_entities = 2U;
    require(WorldSerializer::save(source, registry, entity_limit).error ==
                WorldArchiveError::EntityLimitExceeded,
            "save enforces entity-count limits");

    WorldArchiveOptions component_limit;
    component_limit.maximum_component_bytes = 4U;
    component_limit.archive.maximum_bytes = 64U * 1024U * 1024U;
    component_limit.archive.maximum_string_bytes = 4U;
    const auto oversized_component = WorldSerializer::save(
        source, registry, component_limit);
    require(oversized_component.error == WorldArchiveError::ComponentPayloadTooLarge,
            "save enforces per-component payload limits");

    WorldArchiveOptions invalid_options;
    invalid_options.maximum_component_bytes = 0U;
    require(WorldSerializer::load(saved.bytes, registry, invalid_options).error ==
                WorldArchiveError::InvalidOptions,
            "invalid world archive limits are rejected");

    World empty_world;
    const auto empty_archive = WorldSerializer::save(empty_world, registry);
    require(empty_archive && empty_archive.entity_count == 0U,
            "empty worlds can be saved");
    const auto empty_loaded = WorldSerializer::load(empty_archive.bytes, registry);
    require(empty_loaded && empty_loaded.world->entity_count() == 0U,
            "empty worlds can be restored");

    std::cout << "Dream Engine world archive tests passed.\n";
    return EXIT_SUCCESS;
}
