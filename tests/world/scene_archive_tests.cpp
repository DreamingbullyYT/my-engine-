#include <dream/world/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
using namespace dream::world;
using dream::serialization::BinaryWriter;

struct Label final { std::string text; };

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    ComponentCodecRegistry registry;
    require(register_transform_components(registry) ==
                ComponentCodecRegistrationError::None,
            "transform codecs register");
    require(registry.register_component<Label>(
                0x2201U, 1U,
                [](const Label& value, BinaryWriter& writer) {
                    return writer.write_string(value.text);
                },
                [](dream::serialization::BinaryReader& reader, std::uint32_t version)
                    -> std::optional<Label> {
                    if (version != 1U) return std::nullopt;
                    Label value;
                    if (!reader.read_string(value.text)) return std::nullopt;
                    return value;
                }) == ComponentCodecRegistrationError::None,
            "scene label codec registers");

    Scene scene("Planetary Hub");
    const auto root = scene.world().create_entity();
    const auto child = scene.world().create_entity();
    require(root && child, "scene entities are created");
    require(scene.world().set_parent(child, root), "scene hierarchy is connected");
    require(scene.world().emplace_component<Label>(root, Label{"Main settlement"}),
            "scene label component is added");
    require(scene.world().emplace_component<Transform3D>(
                child, Transform3D{{12, 4, -7}, {0, 0, 0, 1}, {1, 1, 1}}),
            "scene 3D transform is added");

    auto system = scene.systems().add_system(
        "gameplay", SystemPhase::Update, 0,
        [](World&, const SystemContext&) {});
    require(static_cast<bool>(system), "scene system is registered");

    const auto saved = SceneSerializer::save(scene, registry);
    require(static_cast<bool>(saved), "scene saves successfully");
    require(!saved.bytes.empty(), "scene archive contains data");

    const auto loaded = SceneSerializer::load(saved.bytes, registry);
    require(static_cast<bool>(loaded), "scene restores successfully");
    require(loaded.scene->name() == "Planetary Hub",
            "scene name is restored");
    require(loaded.scene->world().entity_count() == 2U,
            "scene world is restored");
    const auto entities = loaded.scene->world().entities();
    require(loaded.scene->world().parent_of(entities[1]) == entities[0],
            "scene parent hierarchy is restored");
    require(loaded.scene->world().with_component<Label>(
                entities[0], [](const Label& value) {
                    require(value.text == "Main settlement",
                            "scene component content is restored");
                }),
            "scene label is accessible after restore");
    require(loaded.scene->world().with_component<Transform3D>(
                entities[1], [](const Transform3D& value) {
                    require(value.position.x == 12.0F &&
                            value.position.y == 4.0F &&
                            value.position.z == -7.0F,
                            "scene transform content is restored");
                }),
            "scene 3D transform is accessible after restore");

    require(loaded.scene->systems().system_count() == 0U,
            "runtime system callbacks are not serialized");
    const auto restored_system = loaded.scene->systems().add_system(
        "gameplay", SystemPhase::Update, 0,
        [](World&, const SystemContext&) {});
    require(static_cast<bool>(restored_system),
            "game code can register systems after scene restoration");

    auto wrong_marker = saved.bytes;
    wrong_marker[8] = std::byte{0};
    wrong_marker[9] = std::byte{0};
    wrong_marker[10] = std::byte{0};
    wrong_marker[11] = std::byte{0};
    const auto marker_result = SceneSerializer::load(wrong_marker, registry);
    require(marker_result.error == WorldArchiveError::InvalidSceneMarker,
            "wrong scene marker is rejected");

    auto trailing = saved.bytes;
    trailing.push_back(std::byte{0xAA});
    const auto trailing_result = SceneSerializer::load(trailing, registry);
    require(trailing_result.error == WorldArchiveError::TrailingData,
            "trailing scene bytes are rejected");

    Scene invalid_name(std::string{"\xC0\xAF", 2U});
    const auto invalid_name_result = SceneSerializer::save(invalid_name, registry);
    require(invalid_name_result.error == WorldArchiveError::InvalidSceneName,
            "invalid UTF-8 scene names are rejected");

    std::cout << "Dream Engine scene archive tests passed.\n";
    return EXIT_SUCCESS;
}
