#pragma once

#include <dream/serialization/binary_archive.hpp>
#include <dream/world/scene.hpp>
#include <dream/world/transform.hpp>
#include <dream/world/world.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dream::world {

enum class ComponentCodecRegistrationError : std::uint8_t {
    None,
    InvalidTypeId,
    InvalidVersion,
    EmptyEncoder,
    EmptyDecoder,
    DuplicateTypeId,
    DuplicateComponentType
};

enum class WorldArchiveError : std::uint8_t {
    None,
    InvalidOptions,
    ArchiveFailure,
    EntityLimitExceeded,
    ComponentLimitExceeded,
    ComponentPayloadTooLarge,
    InvalidEntityId,
    InvalidParent,
    DuplicateComponent,
    UnregisteredComponent,
    UnknownComponent,
    UnsupportedComponentVersion,
    ComponentEncodeFailed,
    ComponentDecodeFailed,
    ComponentPayloadTrailingData,
    TrailingData,
    EntityCapacityExceeded,
    InvalidSceneName,
    InvalidSceneMarker,
    WorldArchiveFailure
};

struct WorldArchiveOptions final {
    serialization::ArchiveLimits archive{};
    std::uint32_t maximum_entities{100000U};
    std::uint32_t maximum_components_per_entity{256U};
    std::uint32_t maximum_component_bytes{16U * 1024U * 1024U};
    bool allow_unknown_components{false};
};

struct WorldArchiveWriteResult final {
    WorldArchiveError error{WorldArchiveError::None};
    serialization::ArchiveError archive_error{serialization::ArchiveError::None};
    std::uint32_t entity_count{0};
    std::vector<std::byte> bytes;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == WorldArchiveError::None;
    }
};

struct WorldArchiveReadResult final {
    WorldArchiveError error{WorldArchiveError::None};
    serialization::ArchiveError archive_error{serialization::ArchiveError::None};
    std::uint32_t entity_count{0};
    std::unique_ptr<World> world;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == WorldArchiveError::None && static_cast<bool>(world);
    }
};

// Maps stable, user-defined component IDs to versioned codecs. Register each
// component type exactly once. Codec functions must not mutate the source World
// while saving. The registry is safe to register/read concurrently; each save
// or load uses a stable sorted snapshot of its codecs.
class ComponentCodecRegistry final {
public:
    ComponentCodecRegistry() = default;
    ComponentCodecRegistry(const ComponentCodecRegistry&) = delete;
    ComponentCodecRegistry& operator=(const ComponentCodecRegistry&) = delete;

    template <typename Component, typename Encoder, typename Decoder>
    ComponentCodecRegistrationError register_component(
        std::uint64_t stable_type_id, std::uint32_t version,
        Encoder&& encoder, Decoder&& decoder) {
        static_assert(std::is_object_v<Component> &&
                      std::is_same_v<Component, std::remove_cv_t<Component>>,
                      "Serialized components must be unqualified object types");
        static_assert(std::is_move_constructible_v<Component>,
                      "Serialized components must be move constructible");

        if (stable_type_id == 0U)
            return ComponentCodecRegistrationError::InvalidTypeId;
        if (version == 0U)
            return ComponentCodecRegistrationError::InvalidVersion;

        using EncoderFunction = std::function<bool(
            const Component&, serialization::BinaryWriter&)>;
        using DecoderFunction = std::function<std::optional<Component>(
            serialization::BinaryReader&, std::uint32_t)>;

        EncoderFunction encode_function{std::forward<Encoder>(encoder)};
        DecoderFunction decode_function{std::forward<Decoder>(decoder)};
        if (!encode_function)
            return ComponentCodecRegistrationError::EmptyEncoder;
        if (!decode_function)
            return ComponentCodecRegistrationError::EmptyDecoder;

        std::unique_lock lock(mutex_);
        const std::type_index cpp_type{typeid(Component)};
        for (const auto& codec : codecs_) {
            if (codec.stable_type_id == stable_type_id)
                return ComponentCodecRegistrationError::DuplicateTypeId;
            if (codec.cpp_type == cpp_type)
                return ComponentCodecRegistrationError::DuplicateComponentType;
        }

        Codec codec;
        codec.stable_type_id = stable_type_id;
        codec.version = version;
        codec.cpp_type = cpp_type;
        codec.has_component = [](const World& world, Entity entity) {
            return world.has_component<Component>(entity);
        };
        codec.encode_component =
            [encode_function = std::move(encode_function)](
                const World& world, Entity entity,
                serialization::BinaryWriter& writer) mutable {
                bool called = false;
                bool encoded = false;
                const bool found = world.with_component<Component>(
                    entity, [&](const Component& value) {
                        called = true;
                        encoded = encode_function(value, writer);
                    });
                return found && called && encoded;
            };
        codec.decode_component =
            [decode_function = std::move(decode_function)](
                World& world, Entity entity,
                serialization::BinaryReader& reader,
                std::uint32_t stored_version) mutable {
                auto value = decode_function(reader, stored_version);
                if (!value) return false;
                return world.emplace_component<Component>(
                    entity, std::move(*value));
            };

        codecs_.push_back(std::move(codec));
        return ComponentCodecRegistrationError::None;
    }

    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return codecs_.size();
    }

private:
    struct Codec final {
        std::uint64_t stable_type_id{0};
        std::uint32_t version{0};
        std::type_index cpp_type{typeid(void)};
        std::function<bool(const World&, Entity)> has_component;
        std::function<bool(
            const World&, Entity, serialization::BinaryWriter&)> encode_component;
        std::function<bool(
            World&, Entity, serialization::BinaryReader&, std::uint32_t)> decode_component;
    };

    [[nodiscard]] std::vector<Codec> snapshot() const {
        std::shared_lock lock(mutex_);
        auto result = codecs_;
        std::sort(result.begin(), result.end(),
            [](const Codec& left, const Codec& right) {
                return left.stable_type_id < right.stable_type_id;
            });
        return result;
    }

    mutable std::shared_mutex mutex_;
    std::vector<Codec> codecs_;

    friend class WorldSerializer;
};

class WorldSerializer final {
public:
    WorldSerializer() = delete;

    [[nodiscard]] static WorldArchiveWriteResult save(
        const World& world, const ComponentCodecRegistry& registry,
        WorldArchiveOptions options = {}) {
        WorldArchiveWriteResult result;
        if (!valid_options(options)) {
            result.error = WorldArchiveError::InvalidOptions;
            return result;
        }

        const auto entities = world.entities();
        if (entities.size() > options.maximum_entities ||
            entities.size() > std::numeric_limits<std::uint32_t>::max()) {
            result.error = WorldArchiveError::EntityLimitExceeded;
            return result;
        }

        const auto codecs = registry.snapshot();
        std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> archive_ids;
        for (std::size_t index = 0; index < entities.size(); ++index) {
            const auto entity = entities[index];
            archive_ids.emplace(
                std::pair{entity.index(), entity.generation()},
                static_cast<std::uint32_t>(index));
        }

        serialization::BinaryWriter writer{options.archive};
        if (!writer.write_header({1, 0}) ||
            !writer.write_u32(static_cast<std::uint32_t>(entities.size()))) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = writer.error();
            return result;
        }

        for (std::size_t entity_index = 0; entity_index < entities.size(); ++entity_index) {
            const Entity entity = entities[entity_index];
            if (!world.is_alive(entity)) {
                result.error = WorldArchiveError::InvalidEntityId;
                return result;
            }

            std::uint32_t parent_archive_id = no_parent;
            const Entity parent = world.parent_of(entity);
            if (parent) {
                const auto found = archive_ids.find(
                    std::pair{parent.index(), parent.generation()});
                if (found == archive_ids.end()) {
                    result.error = WorldArchiveError::InvalidParent;
                    return result;
                }
                parent_archive_id = found->second;
            }

            const auto live_component_types = world.component_types(entity);
            for (const auto& component_type : live_component_types) {
                const auto found = std::find_if(
                    codecs.begin(), codecs.end(),
                    [&](const auto& codec) { return codec.cpp_type == component_type; });
                if (found == codecs.end()) {
                    result.error = WorldArchiveError::UnregisteredComponent;
                    return result;
                }
            }

            struct EncodedComponent final {
                std::uint64_t type_id{0};
                std::uint32_t version{0};
                std::vector<std::byte> payload;
            };
            std::vector<EncodedComponent> encoded_components;
            encoded_components.reserve(live_component_types.size());

            const auto component_string_limit = std::min<std::size_t>(
                options.archive.maximum_string_bytes, options.maximum_component_bytes);
            const serialization::ArchiveLimits component_limits{
                options.maximum_component_bytes, component_string_limit};

            for (const auto& codec : codecs) {
                if (!codec.has_component(world, entity)) continue;

                serialization::BinaryWriter component_writer{component_limits};
                bool encoded = false;
                try {
                    encoded = codec.encode_component(
                        world, entity, component_writer);
                } catch (...) {
                    result.error = WorldArchiveError::ComponentEncodeFailed;
                    return result;
                }
                if (!encoded) {
                    result.archive_error = component_writer.error();
                    result.error = result.archive_error == serialization::ArchiveError::TooLarge
                        ? WorldArchiveError::ComponentPayloadTooLarge
                        : WorldArchiveError::ComponentEncodeFailed;
                    return result;
                }
                if (!component_writer.good()) {
                    result.error = WorldArchiveError::ComponentPayloadTooLarge;
                    result.archive_error = component_writer.error();
                    return result;
                }
                if (component_writer.size() > options.maximum_component_bytes ||
                    component_writer.size() > std::numeric_limits<std::uint32_t>::max()) {
                    result.error = WorldArchiveError::ComponentPayloadTooLarge;
                    return result;
                }

                encoded_components.push_back({
                    codec.stable_type_id, codec.version,
                    std::move(component_writer).take()
                });
            }

            if (encoded_components.size() > options.maximum_components_per_entity ||
                encoded_components.size() > std::numeric_limits<std::uint32_t>::max()) {
                result.error = WorldArchiveError::ComponentLimitExceeded;
                return result;
            }

            const bool entity_written =
                writer.write_u32(static_cast<std::uint32_t>(entity_index)) &&
                writer.write_u32(parent_archive_id) &&
                writer.write_u32(static_cast<std::uint32_t>(encoded_components.size()));
            if (!entity_written) {
                result.error = WorldArchiveError::ArchiveFailure;
                result.archive_error = writer.error();
                return result;
            }

            for (const auto& component : encoded_components) {
                const bool component_written =
                    writer.write_u64(component.type_id) &&
                    writer.write_u32(component.version) &&
                    writer.write_u32(static_cast<std::uint32_t>(component.payload.size())) &&
                    writer.write_bytes(std::span<const std::byte>{component.payload});
                if (!component_written) {
                    result.error = WorldArchiveError::ArchiveFailure;
                    result.archive_error = writer.error();
                    return result;
                }
            }
        }

        result.entity_count = static_cast<std::uint32_t>(entities.size());
        result.bytes = std::move(writer).take();
        return result;
    }

    [[nodiscard]] static WorldArchiveReadResult load(
        std::span<const std::byte> bytes, const ComponentCodecRegistry& registry,
        WorldArchiveOptions options = {}) {
        WorldArchiveReadResult result;
        if (!valid_options(options)) {
            result.error = WorldArchiveError::InvalidOptions;
            return result;
        }

        serialization::BinaryReader reader{bytes, options.archive};
        if (!reader.good()) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (!reader.read_header({1, 0})) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }

        std::uint32_t entity_count = 0;
        if (!reader.read_u32(entity_count)) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (entity_count > options.maximum_entities) {
            result.error = WorldArchiveError::EntityLimitExceeded;
            return result;
        }

        const auto codecs = registry.snapshot();
        auto find_codec = [&](std::uint64_t type_id) -> const ComponentCodecRegistry::Codec* {
            const auto found = std::lower_bound(
                codecs.begin(), codecs.end(), type_id,
                [](const auto& codec, std::uint64_t id) {
                    return codec.stable_type_id < id;
                });
            return found != codecs.end() && found->stable_type_id == type_id
                ? &*found : nullptr;
        };

        auto staged_world = std::make_unique<World>();
        std::vector<Entity> entities;
        std::vector<std::uint32_t> parents;
        entities.reserve(entity_count);
        parents.reserve(entity_count);

        const auto component_string_limit = std::min<std::size_t>(
            options.archive.maximum_string_bytes, options.maximum_component_bytes);
        const serialization::ArchiveLimits component_limits{
            options.maximum_component_bytes, component_string_limit};

        for (std::uint32_t index = 0; index < entity_count; ++index) {
            std::uint32_t serialized_id = 0;
            std::uint32_t parent_id = no_parent;
            std::uint32_t component_count = 0;
            if (!reader.read_u32(serialized_id) ||
                !reader.read_u32(parent_id) ||
                !reader.read_u32(component_count)) {
                result.error = WorldArchiveError::ArchiveFailure;
                result.archive_error = reader.error();
                return result;
            }

            if (serialized_id != index) {
                result.error = WorldArchiveError::InvalidEntityId;
                return result;
            }
            if (parent_id != no_parent &&
                (parent_id >= entity_count || parent_id == serialized_id)) {
                result.error = WorldArchiveError::InvalidParent;
                return result;
            }
            if (component_count > options.maximum_components_per_entity) {
                result.error = WorldArchiveError::ComponentLimitExceeded;
                return result;
            }

            const Entity entity = staged_world->create_entity();
            if (!entity) {
                result.error = WorldArchiveError::EntityCapacityExceeded;
                return result;
            }
            entities.push_back(entity);
            parents.push_back(parent_id);

            std::unordered_set<std::uint64_t> seen_components;
            seen_components.reserve(component_count);
            for (std::uint32_t component_index = 0;
                 component_index < component_count; ++component_index) {
                std::uint64_t type_id = 0;
                std::uint32_t component_version = 0;
                std::uint32_t payload_size = 0;
                if (!reader.read_u64(type_id) ||
                    !reader.read_u32(component_version) ||
                    !reader.read_u32(payload_size)) {
                    result.error = WorldArchiveError::ArchiveFailure;
                    result.archive_error = reader.error();
                    return result;
                }
                if (type_id == 0U || component_version == 0U) {
                    result.error = WorldArchiveError::UnsupportedComponentVersion;
                    return result;
                }
                if (!seen_components.insert(type_id).second) {
                    result.error = WorldArchiveError::DuplicateComponent;
                    return result;
                }
                if (payload_size > options.maximum_component_bytes) {
                    result.error = WorldArchiveError::ComponentPayloadTooLarge;
                    return result;
                }

                std::vector<std::byte> payload(payload_size);
                if (!reader.read_bytes(std::span<std::byte>{payload})) {
                    result.error = WorldArchiveError::ArchiveFailure;
                    result.archive_error = reader.error();
                    return result;
                }

                const auto* codec = find_codec(type_id);
                if (codec == nullptr) {
                    if (options.allow_unknown_components) continue;
                    result.error = WorldArchiveError::UnknownComponent;
                    return result;
                }
                if (component_version > codec->version) {
                    result.error = WorldArchiveError::UnsupportedComponentVersion;
                    return result;
                }

                serialization::BinaryReader component_reader{
                    std::span<const std::byte>{payload}, component_limits};
                bool decoded = false;
                try {
                    decoded = codec->decode_component(
                        *staged_world, entity, component_reader, component_version);
                } catch (...) {
                    result.error = WorldArchiveError::ComponentDecodeFailed;
                    result.archive_error = component_reader.error();
                    return result;
                }
                if (!decoded) {
                    result.error = WorldArchiveError::ComponentDecodeFailed;
                    result.archive_error = component_reader.error();
                    return result;
                }
                if (!component_reader.good()) {
                    result.error = WorldArchiveError::ComponentDecodeFailed;
                    result.archive_error = component_reader.error();
                    return result;
                }
                if (component_reader.remaining() != 0U) {
                    result.error = WorldArchiveError::ComponentPayloadTrailingData;
                    return result;
                }
            }
        }

        if (reader.remaining() != 0U) {
            result.error = WorldArchiveError::TrailingData;
            return result;
        }

        for (std::size_t index = 0; index < parents.size(); ++index) {
            const auto parent_id = parents[index];
            if (parent_id == no_parent) continue;
            if (!staged_world->set_parent(entities[index], entities[parent_id])) {
                result.error = WorldArchiveError::InvalidParent;
                return result;
            }
        }

        result.entity_count = entity_count;
        result.world = std::move(staged_world);
        return result;
    }

private:
    static constexpr std::uint32_t no_parent =
        std::numeric_limits<std::uint32_t>::max();

    [[nodiscard]] static bool valid_options(
        const WorldArchiveOptions& options) noexcept {
        return options.archive.maximum_bytes > 0U &&
               options.archive.maximum_string_bytes <= options.archive.maximum_bytes &&
               options.archive.maximum_string_bytes <=
                   std::numeric_limits<std::uint32_t>::max() &&
               options.maximum_component_bytes > 0U &&
               options.maximum_component_bytes <= options.archive.maximum_bytes &&
               options.maximum_component_bytes <=
                   std::numeric_limits<std::uint32_t>::max();
    }
};

inline ComponentCodecRegistrationError register_transform_components(
    ComponentCodecRegistry& registry) {
    const auto transform2d = registry.register_component<Transform2D>(
        0x445245414D325452ULL, 1U,
        [](const Transform2D& value, serialization::BinaryWriter& writer) {
            return writer.write_f32(value.position.x) &&
                   writer.write_f32(value.position.y) &&
                   writer.write_f32(value.rotation_radians) &&
                   writer.write_f32(value.scale.x) &&
                   writer.write_f32(value.scale.y);
        },
        [](serialization::BinaryReader& reader, std::uint32_t version)
            -> std::optional<Transform2D> {
            if (version != 1U) return std::nullopt;
            Transform2D value;
            if (!reader.read_f32(value.position.x) ||
                !reader.read_f32(value.position.y) ||
                !reader.read_f32(value.rotation_radians) ||
                !reader.read_f32(value.scale.x) ||
                !reader.read_f32(value.scale.y)) return std::nullopt;
            return value;
        });
    if (transform2d != ComponentCodecRegistrationError::None) return transform2d;

    return registry.register_component<Transform3D>(
        0x445245414D335452ULL, 1U,
        [](const Transform3D& value, serialization::BinaryWriter& writer) {
            return writer.write_f32(value.position.x) &&
                   writer.write_f32(value.position.y) &&
                   writer.write_f32(value.position.z) &&
                   writer.write_f32(value.rotation.x) &&
                   writer.write_f32(value.rotation.y) &&
                   writer.write_f32(value.rotation.z) &&
                   writer.write_f32(value.rotation.w) &&
                   writer.write_f32(value.scale.x) &&
                   writer.write_f32(value.scale.y) &&
                   writer.write_f32(value.scale.z);
        },
        [](serialization::BinaryReader& reader, std::uint32_t version)
            -> std::optional<Transform3D> {
            if (version != 1U) return std::nullopt;
            Transform3D value;
            if (!reader.read_f32(value.position.x) ||
                !reader.read_f32(value.position.y) ||
                !reader.read_f32(value.position.z) ||
                !reader.read_f32(value.rotation.x) ||
                !reader.read_f32(value.rotation.y) ||
                !reader.read_f32(value.rotation.z) ||
                !reader.read_f32(value.rotation.w) ||
                !reader.read_f32(value.scale.x) ||
                !reader.read_f32(value.scale.y) ||
                !reader.read_f32(value.scale.z)) return std::nullopt;
            return value;
        });
}

} // namespace dream::world


namespace dream::world {

struct SceneArchiveWriteResult final {
    WorldArchiveError error{WorldArchiveError::None};
    serialization::ArchiveError archive_error{serialization::ArchiveError::None};
    WorldArchiveError world_error{WorldArchiveError::None};
    std::vector<std::byte> bytes;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == WorldArchiveError::None;
    }
};

struct SceneArchiveReadResult final {
    WorldArchiveError error{WorldArchiveError::None};
    serialization::ArchiveError archive_error{serialization::ArchiveError::None};
    WorldArchiveError world_error{WorldArchiveError::None};
    std::unique_ptr<Scene> scene;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == WorldArchiveError::None && static_cast<bool>(scene);
    }
};

// Scene archives store the scene name and its World. System callbacks are code,
// not persistent data: the game must register its systems again after loading.
class SceneSerializer final {
public:
    SceneSerializer() = delete;

    [[nodiscard]] static SceneArchiveWriteResult save(
        const Scene& scene, const ComponentCodecRegistry& registry,
        WorldArchiveOptions options = {}) {
        SceneArchiveWriteResult result;
        if (scene.name().empty() || scene.name().size() > 256U ||
            !serialization::is_valid_utf8(scene.name())) {
            result.error = WorldArchiveError::InvalidSceneName;
            return result;
        }

        auto world_result = WorldSerializer::save(scene.world(), registry, options);
        if (!world_result) {
            result.error = WorldArchiveError::WorldArchiveFailure;
            result.world_error = world_result.error;
            result.archive_error = world_result.archive_error;
            return result;
        }
        if (world_result.bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = serialization::ArchiveError::TooLarge;
            return result;
        }

        serialization::BinaryWriter writer{options.archive};
        constexpr std::uint32_t scene_marker = 0x454E4353U; // "SCNE" in little-endian
        if (!writer.write_header({1, 0}) ||
            !writer.write_u32(scene_marker) ||
            !writer.write_string(scene.name()) ||
            !writer.write_u32(static_cast<std::uint32_t>(world_result.bytes.size())) ||
            !writer.write_bytes(std::span<const std::byte>{world_result.bytes})) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = writer.error();
            return result;
        }

        result.bytes = std::move(writer).take();
        return result;
    }

    [[nodiscard]] static SceneArchiveReadResult load(
        std::span<const std::byte> bytes, const ComponentCodecRegistry& registry,
        WorldArchiveOptions options = {}) {
        SceneArchiveReadResult result;
        serialization::BinaryReader reader{bytes, options.archive};
        if (!reader.good()) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (!reader.read_header({1, 0})) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }

        std::uint32_t marker = 0;
        if (!reader.read_u32(marker)) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (marker != 0x454E4353U) {
            result.error = WorldArchiveError::InvalidSceneMarker;
            return result;
        }

        std::string scene_name;
        if (!reader.read_string(scene_name)) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (scene_name.empty() || scene_name.size() > 256U) {
            result.error = WorldArchiveError::InvalidSceneName;
            return result;
        }

        std::uint32_t world_size = 0;
        if (!reader.read_u32(world_size)) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (world_size > options.archive.maximum_bytes) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = serialization::ArchiveError::TooLarge;
            return result;
        }

        std::vector<std::byte> world_bytes(world_size);
        if (!reader.read_bytes(std::span<std::byte>{world_bytes})) {
            result.error = WorldArchiveError::ArchiveFailure;
            result.archive_error = reader.error();
            return result;
        }
        if (reader.remaining() != 0U) {
            result.error = WorldArchiveError::TrailingData;
            return result;
        }

        auto world_result = WorldSerializer::load(
            std::span<const std::byte>{world_bytes}, registry, options);
        if (!world_result) {
            result.error = WorldArchiveError::WorldArchiveFailure;
            result.world_error = world_result.error;
            result.archive_error = world_result.archive_error;
            return result;
        }

        try {
            result.scene = std::make_unique<Scene>(
                std::move(scene_name), std::move(world_result.world));
        } catch (...) {
            result.error = WorldArchiveError::InvalidSceneName;
            return result;
        }
        return result;
    }
};

} // namespace dream::world
